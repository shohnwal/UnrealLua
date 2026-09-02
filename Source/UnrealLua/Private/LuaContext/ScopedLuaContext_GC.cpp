
#include "LuaContext/ScopedLuaContext.h"
extern "C" {
	#include "lstate.h"
	#include "lobject.h"
	#include "ltable.h"
}
#include "lua.hpp"
#include "Utility/LuaLogMacros.h"
#include "utility/to_string.hpp"
#include "UnrealLua.h"
#include "Utility/CPUCycleTimer.h"
#include "sol/sol.hpp"
#include "Config/UnrealLuaConfig.h"
#include "Async/ParallelFor.h"

namespace UnrealLua::GC
{

	FCriticalSection GCLock = {};
	//static TArray<Table*, TInlineAllocator<50>> FoundTablesDuringGC = {};

	void clearkey (Node *n)
	{
		lua_assert(isempty(gval(n)));
		setdeadkey(n);  /* unused key; remove it */
	}

/*
	unsigned int luaH_realasize (const Table *t)
	{
#define limitequalsasize(t)	(isrealasize(t) || ispow2((t)->alimit))
		if (limitequalsasize(t))
			return t->alimit;  // this is the size
		else {
			unsigned int size = t->alimit;
			// compute the smallest power of 2 not smaller than 'size'
			size |= (size >> 1);
			size |= (size >> 2);
			size |= (size >> 4);
			size |= (size >> 8);
#if (UINT_MAX >> 14) > 3  // unsigned int has more than 16 bits
			size |= (size >> 16);
#if (UINT_MAX >> 30) > 3
			size |= (size >> 32);  // unsigned int has more than 32 bits
#endif
#endif
			size++;
			lua_assert(ispow2(size) && size/2 < t->alimit && t->alimit < size);
			return size;
		}
	}
*/	
	Table* GetRawUObjectWrapperMetaTable(lua_State* L)
	{
		return nullptr;
	}
	
	void HandleUObjectWrapperAddReference(const lu_byte& tag, const Value& value, Table* uobjMetaTable, FReferenceCollector& collector)
	{
		if (tag == LUA_VLIGHTUSERDATA)
		{
			UnrealLua::LightUserdata::AddReferencedUObject(tag, value, collector);	
		}
	}

	void CollectUObjectsInTable(lua_State* L, Table* table/*, Table* uobjMetaTable*/)
	{
		//check hash part
		int32 actualNodeSize = sizenode(table); 
		for(int32 nodeIndex = 0; nodeIndex < actualNodeSize; nodeIndex++)
		{
			Node* node = &table->node[nodeIndex];
			if(keyisnil(node) || isempty(&node->i_val) || ttisnil(&node->i_val))
			{
				continue;
			}
			
			TValue* valueSlot = gval(node);
			if(valueSlot == nullptr)
			{
				continue;
			}
			if(ttislightuserdata(valueSlot))
			{
				FUnrealLuaLightUserdataWrapper wrapper{valueSlot->value_.p};
				if (wrapper.IsInvalidUObjectReference())
				{
					setnilvalue(gval(node));
					setempty(gval(node));
					clearkey(node);

					//cleared both key and value, so no need to check key
					continue;							
				}
			}
			if(!keyisdead(node))
			{
				TValue key;
				getnodekey(L, &key, node);
				if(ttislightuserdata(valueSlot))
				{
					FUnrealLuaLightUserdataWrapper wrapper{valueSlot->value_.p};
					if (wrapper.IsInvalidUObjectReference())
					{
						setnilvalue(gval(node));
						setempty(gval(node));
						clearkey(node);
					}
				}
			}
		}

		//check array part
		int32 realsize = table->asize;//UnrealLua::GC::luaH_realasize(table);
		for(int32 arrIndex = 0; arrIndex < realsize; arrIndex++)
		{
			lu_byte* tag = getArrTag(table, arrIndex);
			if(*tag == LUA_VLIGHTUSERDATA/* ttislightuserdata(tag)*/)
			{
				Value* val = getArrVal(table, arrIndex);
				FUnrealLuaLightUserdataWrapper wrapper{val->p};
				if (wrapper.IsInvalidUObjectReference())
				{
					*tag = LUA_VNIL;
					//setnilvalue(value);
				}
			}
		}
	}

	void CollectUObjectsInUpvalue(lua_State* L, TValue* upval/*, Table* uobjMetaTable*/)
	{
		if(ttislightuserdata(upval))
		{
			FUnrealLuaLightUserdataWrapper wrapper{upval->value_.p};
			if (wrapper.IsInvalidUObjectReference())
			{
				setnilvalue(upval);
			}
		}
	}

	void CollectUObjectsInLuaThread(lua_State* th)
	{
		if (!th)
		{
			return;
		}

		for (StkId stackElement = th->stack.p; stackElement < th->top.p; stackElement++)
		{
			TValue* val = s2v(stackElement);
			if (ttislightuserdata(val))
			{
				FUnrealLuaLightUserdataWrapper wrapper{val->value_.p};
				if (wrapper.IsInvalidUObjectReference())
				{
					setnilvalue(val);
				}
			}
		}
	}

	void CollectUObjectsInTablesAndUpvalues(const TArray<Table*, TInlineAllocator<128>>& tables, const TArray<TValue*, TInlineAllocator<128>>& upvalues, lua_State* L)
	{
		//Table* uobjMetaTable = GetRawUObjectWrapperMetaTable(L);
		
		//const std::string_view uobjMT = &sol::usertype_traits<FLuaUObjectWrapper>::metatable()[0];

		ParallelForWithPreWork(tables.Num(), [L, /*uobjMetaTable,*/ &tables](int32 index)
		{
			Table* table = tables[index];
			CollectUObjectsInTable(L, table/*, uobjMetaTable*/);
		},
		[L, /*&uobjMetaTable,*/ &upvalues]()
		{
			for(TValue* upval : upvalues)
			{
				CollectUObjectsInUpvalue(L, upval/*, uobjMetaTable*/);
			}
		},!UUnrealLuaConfig::ShouldMultithreadGC());

		lua_gc(L, LUA_GCCOLLECT);
	}

	void GatherAllTablesAndUpvalues(lua_State* L, TArray<Table*, TInlineAllocator<128>>& tables, TArray<TValue*, TInlineAllocator<128>> upvalues)
	{
		global_State* G = L->l_G;
		GCObject* gc = G->allgc;
		while(gc)
		{
			switch (gc->tt)
			{
			case LUA_VTABLE:
				{
					Table* table = gco2t(gc);
					tables.Emplace(table);
					break;
				}
			case LUA_VUPVAL:
				{
					UpVal* uv = gco2upv(gc);
					TValue* val = uv->v.p;
					if(ttisfulluserdata(val))
					{
						upvalues.Emplace(val);
					}
					else if(ttislightuserdata(val))
					{
						upvalues.Emplace(val);
					}
				}
			default: break;;
			}
			gc = gc->next;
		}
	}
	void HandleInvalidUserDataInLuaState(lua_State* L)
	{
		//FCPUCycleTimer timer{"LuaGC: HandleInvalidUserDataInLuaState"};
		TArray<Table*, TInlineAllocator<128>> tables;
		TArray<TValue*, TInlineAllocator<128>> upvalues;

		GatherAllTablesAndUpvalues(L, tables, upvalues);

		UnrealLua::GC::CollectUObjectsInTablesAndUpvalues(tables, upvalues, L);
		
		lua_gc(L, LUA_GCCOLLECT);
	}
	

	bool IncrementalCollectUObjectsInGCObject(lua_State* L, GCObject* gco)
	{
		if(gco == nullptr)
		{
			return false;
		}
		switch (gco->tt)
		{
		case LUA_VTABLE:
			{
				Table* table = gco2t(gco);
				CollectUObjectsInTable(L, table);
				return true;
			}
		case LUA_VUPVAL:
			{
				UpVal* uv = gco2upv(gco);
				TValue* val = uv->v.p;
				if(val != nullptr)
				{
					if(ttisfulluserdata(val))
					{
						CollectUObjectsInUpvalue(L, val);
					}
					else if(ttislightuserdata(val))
					{
						CollectUObjectsInUpvalue(L, val);
					}
					return true;
				}
				break;
			}
		case LUA_VTHREAD:
			{
				lua_State* thread = gco2th(gco);
				CollectUObjectsInLuaThread(thread);
				return true;
			}
		}
		return false;
	}

	void AddUObjectReferencesInTable(lua_State* L, Table* table, Table* uobjMetaTable, FReferenceCollector& collector)
	{
		//check hash part
		int32 actualNodeSize = sizenode(table); 
		for(int32 nodeIndex = 0; nodeIndex < actualNodeSize; nodeIndex++)
		{
			Node* node = &table->node[nodeIndex];
			if(keyisnil(node) || isempty(&node->i_val) || ttisnil(&node->i_val))
			{
				continue;
			}
			
			TValue* valueSlot = gval(node);
			if(valueSlot == nullptr)
			{
				continue;
			}
			HandleUObjectWrapperAddReference(valueSlot->tt_, valueSlot->value_, uobjMetaTable, collector);
			if(!keyisdead(node))
			{
				TValue key;
				getnodekey(L, &key, node);
				HandleUObjectWrapperAddReference(key.tt_, key.value_, uobjMetaTable, collector);
			}
		}
	
		//check array part
		int32 realsize = table->asize;//UnrealLua::GC::luaH_realasize(table);
		for(int32 arrIndex = 0; arrIndex < realsize; arrIndex++)
		{
			//TValue* value = &table->array[arrIndex];
			lu_byte* tag = getArrTag(table, arrIndex);
			//if(ttislightuserdata(value))
			if(*tag == LUA_VLIGHTUSERDATA)
			{
				Value* val = getArrVal(table, arrIndex);
				HandleUObjectWrapperAddReference(*tag, *val, uobjMetaTable, collector);
			}
		}
	}

	void AddUObjectReferencesInUpvalue(lua_State* L, TValue* upvalue, Table* UobjMetaTable, const FReferenceCollector& collector)
	{
		
	}

	void AddReferenceUObjects(const TArray<Table*, TInlineAllocator<128>>& tables, const TArray<TValue*, TInlineAllocator<128>>& upvalues, lua_State* L, FReferenceCollector& collector)
	{
		Table* uobjMetaTable = GetRawUObjectWrapperMetaTable(L);
		
		//const std::string_view uobjMT = &sol::usertype_traits<FLuaUObjectWrapper>::metatable()[0];

		ParallelForWithPreWork(tables.Num(), [L, uobjMetaTable, &tables, &collector](int32 index)
		{
			Table* table = tables[index];

			AddUObjectReferencesInTable(L, table, uobjMetaTable, collector);
		},
		[L, &uobjMetaTable, &upvalues, &collector]()
		{
			for(TValue* upval : upvalues)
			{
				AddUObjectReferencesInUpvalue(L, upval, uobjMetaTable, collector);
			}
		},UUnrealLuaConfig::ShouldMultithreadGC());

	}

	void ProcessAddReferencedObjects(lua_State* L, FReferenceCollector& collector)
	{
		//FCPUCycleTimer timer{"LuaGC: ProcessAddReferencedObjects"};
		TArray<Table*, TInlineAllocator<128>> tables;
		TArray<TValue*, TInlineAllocator<128>> upvalues;

		GatherAllTablesAndUpvalues(L, tables, upvalues);

		UnrealLua::GC::AddReferenceUObjects(tables, upvalues, L, collector);
	}
}

void FScopedLuaContext::ProcessInvalidUObjectCollection(bool forcedCollection)
{
	if(this->ContextType == ELuaContextType::Minimal || this->ContextType == ELuaContextType::LuaScriptEditor || !this->IsLuaLoaded())
	{
		return;
	}
	if(UUnrealLuaConfig::GetGCMode() == EUnrealLuaGCMode::PostDestroy || forcedCollection)
	{
		LUA_LOG("Post-Destroy Lua GC")
		UnrealLua::GC::HandleInvalidUserDataInLuaState(this->LuaState);
	}
}

void FScopedLuaContext::IncrementalProcessInvalidLuaUObjects()
{
	if(UUnrealLuaConfig::GetGCMode() == EUnrealLuaGCMode::Incremental)
	{
		if(this->ContextType == ELuaContextType::Minimal || !this->IsLuaLoaded())
		{
			return;
		}
		lua_State* L = this->LuaState.lua_state();
		if(this->NextGCObjectToCheck == nullptr)
		{
			//LUA_LOG("Restarting incremental GC with %d objects to check", this->IncrementalNumObjectsToCheck);
			global_State* G = L->l_G;
			//this->NextGCObjectToCheck = obj2gco(G->mainthread);
			this->NextGCObjectToCheck = G->allgc;
		}
		int32 numToCheck = this->IncrementalNumObjectsToCheck;
		//Table* uobjMetaTable = UnrealLua::GC::GetRawUObjectWrapperMetaTable(L);
		int32 numChecked = 0;
		while(numToCheck > 0)
		{
			++numChecked;
			--numToCheck;
			GCObject* currentGCObject = this->NextGCObjectToCheck;
			if(currentGCObject == nullptr)
			{
				//LUA_LOG("Incremental GC ran out of objects, finished!")
				this->IncrementalNumObjectsToCheck = FMath::Max3(UUnrealLuaConfig::GetLuaIncrementalGCLimit(), numChecked, this->IncrementalNumObjectsToCheck - 1);
				break;
			}
			if(UnrealLua::GC::IncrementalCollectUObjectsInGCObject(L, currentGCObject))
			{
				//found a thread, table or upvalue, so count it as examined
				--numToCheck;
			}
			this->NextGCObjectToCheck = currentGCObject->next;
		}	
	}
}

void FScopedLuaContext::ResetIncrementalGC()
{
	this->NextGCObjectToCheck = nullptr;
	int32 limit = UUnrealLuaConfig::GetLuaIncrementalGCLimit();
	this->IncrementalNumObjectsToCheck = FMath::Clamp(this->IncrementalNumObjectsToCheck << 1, limit, limit * 10);
}

void FScopedLuaContext::AddReferencedObjects(FReferenceCollector& Collector)
{
	if (this->ContextType == ELuaContextType::Minimal || this->ContextType == ELuaContextType::LuaScriptEditor)
	{
		return;
	}
	Collector.AddReferencedObjects(this->LoadedScripts);
	Collector.AddReferencedObject(this->PlayerInputHandler);
	UnrealLua::GC::ProcessAddReferencedObjects(this->LuaState.lua_state(), Collector);
}