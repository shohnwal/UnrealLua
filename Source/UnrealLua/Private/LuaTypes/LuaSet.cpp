// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaTypes/LuaSet.h"

#include "Reflection/PropertyHelper.h"
#include <algorithm>
#include <random>
#include "LuaCoreDelegates.h"
#include "Config/UnrealLuaConstants.h"
#include "Reflection/PropertyHelperTypes.h"

static const FDelegateHandle fLuaSetLuaTypeHandle = FLuaCoreDelegates::OnRegisterLuaUsertypes.AddStatic(&FLuaSet::RegisterUsertype);


FLuaScriptSet::FLuaScriptSet(FProperty* inner)
{
	this->Inner.Reset(inner);
	TArray<const FStructProperty*> encounteredStructProps;
	this->bContainsObjectReferences = UnrealLua::PropertyHelper::CanPropertyContainObjectReferences(this->Inner.Get()) && this->Inner->ContainsObjectReference(encounteredStructProps);
}

FLuaScriptSet::~FLuaScriptSet()
{
	FScriptSetHelper helper = FScriptSetHelper::CreateHelperFormElementProperty(this->Inner.Get(), static_cast<FScriptSet*>(this));
	helper.EmptyElements();
	this->RefCount = 0;
	this->Inner.Reset();
}

void FLuaScriptSet::AddReferencedObjects(FReferenceCollector& Collector)
{
	// if empty or owner object had been collected
	// AddReferencedObject will auto null propObj
	if (this->IsEmpty())
	{
		return;
	}
	if(!this->bContainsObjectReferences)
	{
		return;
	}
	//LUA_LOG("Collecting UObject references in TLuaSet")
	FProperty* inner = this->Inner.Get();
	FScriptSetHelper helper = FScriptSetHelper::CreateHelperFormElementProperty(inner, this);

	for(auto it = helper.CreateIterator(); it; ++it)
	{
		void* ptr = helper.GetElementPtr(it);
		if(ptr != nullptr)
		{
			if (UnrealLua::PropertyHelper::AddRefByProperty(Collector, inner, ptr))
			{
				//PropertyHelper will null out any invalid objects
				//helper.RemoveAt(index);
			};		
		}
	}
}

void FLuaScriptSet::AddRef()
{
	this->RefCount++;
	verifyf(this->RefCount >= 0, TEXT("RefCount of FLuaScriptSet %p is %d"), this, this->RefCount);
}

int32 FLuaScriptSet::RemoveRef()
{
	this->RefCount--;
	verifyf(this->RefCount >= 0, TEXT("RefCount of FLuaScriptSet %p is %d"), this, this->RefCount);
	return this->RefCount;
}

void FLuaSet::RegisterUsertype(sol::state_view& lua)
{
	sol::usertype<FLuaSet> ut = lua.new_usertype<FLuaSet>(
		"TSet",
		"new", sol::no_constructor,
		sol::call_constructor, [](sol::object innerType, sol::object initial, sol::this_state lua) ->sol::object
		{
			FProperty* innerprop = UnrealLua::PropertyHelper::CreateNewProperty(innerType, UnrealLua::PropertyNames::NAME_UnrealLuaSetInnerProperty);
			if(!innerprop)
			{
				return sol::nil;
			}
			return sol::object(lua, sol::in_place_type<FLuaSet>, innerprop, initial, lua);
		},
		"new", sol::no_constructor,
		"Add", &FLuaSet::Lua_Add,
		"Num", &FLuaSet::Lua_Num,
		"Find", &FLuaSet::Lua_Find,
		"Empty", &FLuaSet::Lua_Empty,
		"IsEmpty", &FLuaSet::Lua_IsEmpty,
		"Clear", &FLuaSet::Lua_Empty,
		"Remove",&FLuaSet::Lua_Remove,
		"Contains",&FLuaSet::Lua_Contains,
		"RemoveAt", &FLuaSet::Lua_RemoveAt,
		"RemoveAll", &FLuaSet::Lua_RemoveAll,
		"KeepAll", &FLuaSet::Lua_KeepAll,
		"Copy", &FLuaSet::Lua_Copy,
		"Any", &FLuaSet::Lua_Any,
		"ToTable", &FLuaSet::Lua_ToLuaTable,
		sol::meta_function::length, &FLuaSet::Lua_Num,
		sol::meta_function::index, &FLuaSet::__Index,
		sol::meta_function::pairs, &FLuaSet::__pairs,
		sol::meta_function::ipairs, &FLuaSet::__ipairs,
		sol::meta_function::new_index, &FLuaSet::__NewIndex
	);
}

FLuaSet::FLuaSet()
	: Inner(), Data()

{
	//there should never be any empty set!
	checkNoEntry();
}

FScriptSetHelper CreateHelperFormElementProperty(FProperty* prop, FScriptSet* scriptSet)
{
	return FScriptSetHelper::CreateHelperFormElementProperty(prop, scriptSet);
}

//From FSetPropertyDescr, the inner is the other properties inner,so here we need to duplicate it 
FLuaSet::FLuaSet(FSetProperty* setProp, FScriptSet* referencedScriptSet, bool bAsReference)
	: Inner(nullptr)
	, Data()
{
	if(bAsReference)
	{
		verify(setProp->ElementProp->Owner != nullptr)
		verify(setProp->ElementProp->GetFName() != UnrealLua::PropertyNames::NAME_UnrealLuaSetInnerProperty)
		this->Inner = setProp->ElementProp;
		this->Data.Emplace<FScriptSet*>(referencedScriptSet);
	}
	else
	{
		//copy
		this->Inner = UnrealLua::PropertyHelper::CreateNewProperty(setProp->ElementProp, UnrealLua::PropertyNames::NAME_UnrealLuaSetInnerProperty); 
		//Sets necessarily have to have hash value set, so lets trust that the Blueprint/UPROPERTY set has a hashable inner
		this->Inner->PropertyFlags |= CPF_HasGetValueTypeHash;
		this->Data.Emplace<FLuaScriptSet*>(new FLuaScriptSet(this->Inner));
		this->AddRef();
		this->Copy(referencedScriptSet, this->Inner, this->GetScriptSet());
	}
}

//used in sol_lua_push, the passed-in property is already the property we should use and has the flags set up
FLuaSet::FLuaSet(FProperty* inner, FScriptSet* otherContainer, bool bAsReference)
	: Inner(inner)
	, Data()
{
	verify(this->Inner->GetFName() == UnrealLua::PropertyNames::NAME_UnrealLuaSetInnerProperty)
	verify(this->Inner->Owner == nullptr);
	if(bAsReference)
	{
		verify(otherContainer != nullptr);
		this->Data.Emplace<FScriptSet*>(otherContainer);
	}
	else
	{
		//copy
		this->Data.Emplace<FLuaScriptSet*>(new FLuaScriptSet(this->Inner));
		this->AddRef();
		this->Copy(otherContainer, this->Inner, this->GetScriptSet());
	}
}

//Call constructor from Lua, the inner FProperty has already been constructed
FLuaSet::FLuaSet(FProperty* inner, sol::object initial, sol::this_state lua)
	: Inner(inner)
	, Data()
{
	verify(this->Inner->GetFName() == UnrealLua::PropertyNames::NAME_UnrealLuaSetInnerProperty)
	verify(this->Inner->Owner == nullptr);
	this->Data.Emplace<FLuaScriptSet*>(new FLuaScriptSet(this->Inner));
	this->AddRef();
	if(initial.get_type() == sol::type::table)
	{
		sol::table tbl = initial;
		for(int i = 1; i <= tbl.size(); ++i)
		{
			this->Lua_Add(tbl[i], lua);
		}
	}
}

FLuaSet::FLuaSet(const FLuaSet& other)
	: Inner(nullptr)
	, Data(other.Data)
{
	if(this->OwnsMemory())
	{
		this->AddRef();
		Inner = other.Inner;
	}
	else
	{
		if(other.Inner->GetFName() == UnrealLua::PropertyNames::NAME_UnrealLuaSetInnerProperty)
		{
			//This Inner is a manually created one, so we need to create a new one for the new array
			verify(other.Inner->Owner == nullptr)
			Inner = UnrealLua::PropertyHelper::CreateNewProperty(other.Inner, UnrealLua::PropertyNames::NAME_UnrealLuaSetInnerProperty);
		}
		else
		{
			//this is a UProperty reference
			verify(other.Inner->Owner != nullptr)
			Inner = other.Inner;
		}
	}
}

FLuaSet::FLuaSet(FLuaSet&& other) noexcept
	: Inner(other.Inner)
	, Data(other.Data)
{
	if(this->OwnsMemory())
	{
		//must add ref BEFORE decreasing ref from other, to keep data alive
		this->AddRef();
	}
	other.Inner = nullptr;
	other.Reset();
}

FLuaSet::~FLuaSet()
{
	this->Reset();
}

void FLuaSet::Reset()
{
	
	this->RemoveRef();
	if (this->Data.IsType<FScriptSet*>())
	{
		if (this->Inner && this->Inner->GetFName() == UnrealLua::PropertyNames::NAME_UnrealLuaSetInnerProperty)
		{
			delete this->Inner;
		}
	}
	else
	{
		//no need to manually delete the Inner ptr, the FLuaScriptSet destructor will delete it via the TUniquePtr
	}
	this->Data.Emplace<std::nullptr_t>();
	this->Inner = nullptr;
}

FScriptSet* FLuaSet::GetScriptSet() const
{
	return this->Data.IsType<FScriptSet*>() ? this->Data.Get<FScriptSet*>() : static_cast<FScriptSet*>(this->Data.Get<FLuaScriptSet*>());
}

int32 FLuaSet::Lua_Num() const
{
	FProperty* inner = this->Inner;
	FScriptSetHelper helper = CreateHelperFormElementProperty(inner, this->GetScriptSet());
	return helper.Num();
}

void FLuaSet::Lua_Add(sol::object luaValue, sol::this_state lua)
{
	FProperty* inner = this->Inner;
	if(!UnrealLua::PropertyHelper::IsCompatibleType(inner,luaValue))
	{
		return;
	}
	FScriptSetHelper helper = CreateHelperFormElementProperty(inner, this->GetScriptSet());
	
	FDefaultConstructedPropertyElement temp(inner);
	
	TSetPropertyValueParams params{inner, temp.GetObjAddress(), 0, luaValue};
	UnrealLua::PropertyHelper::SetPropertyValue_Direct(params);

	helper.AddElement(temp.GetObjAddress());
}

int32 FLuaSet::Lua_Find(sol::object luaValue, sol::this_state lua)
{
	FProperty* inner = this->Inner;
	if(!UnrealLua::PropertyHelper::IsCompatibleType(inner,luaValue))
	{
		return INDEX_NONE;
	}
	FScriptSetHelper helper = CreateHelperFormElementProperty(inner, this->GetScriptSet());

	FDefaultConstructedPropertyElement temp(inner);
	
	TSetPropertyValueParams params{inner, temp.GetObjAddress(), 0, luaValue};
	UnrealLua::PropertyHelper::SetPropertyValue_Direct(params);
	
	int32 foundIndex = helper.FindElementIndex(temp.GetObjAddress());
	
	if(foundIndex != INDEX_NONE)
	{
		foundIndex++;
	}
	return foundIndex;
}

bool FLuaSet::Lua_Remove(sol::object luaValue, sol::this_state lua)
{
	FProperty* inner = this->Inner;
	if(!UnrealLua::PropertyHelper::IsCompatibleType(inner,luaValue))
	{
		return false;
	}
	FScriptSetHelper helper = CreateHelperFormElementProperty(inner, this->GetScriptSet());
	
	FDefaultConstructedPropertyElement temp(inner);
	
	TSetPropertyValueParams params{inner, temp.GetObjAddress(), 0, luaValue};
	UnrealLua::PropertyHelper::SetPropertyValue_Direct(params);
	
	bool removed = helper.RemoveElement(temp.GetObjAddress());

	return removed;
}

bool FLuaSet::Lua_Contains(sol::object luaValue, sol::this_state lua)
{
	FProperty* inner = this->Inner;
	if(!UnrealLua::PropertyHelper::IsCompatibleType(inner,luaValue))
	{
		return false;
	}

	FScriptSetHelper helper = CreateHelperFormElementProperty(inner, this->GetScriptSet());

	FDefaultConstructedPropertyElement temp(inner);
	
	TSetPropertyValueParams params{inner, temp.GetObjAddress(), 0, luaValue};
	UnrealLua::PropertyHelper::SetPropertyValue_Direct(params);
	
	int32 foundIndex = helper.FindElementIndex(temp.GetObjAddress());

	if(foundIndex != INDEX_NONE)
	{
		foundIndex++;
	}
	return foundIndex != INDEX_NONE;
}

void FLuaSet::Lua_RemoveAt(sol::object luaValue, sol::this_state lua)
{
	if(luaValue.get_type() != sol::type::number)
	{
		return;
	}
	FProperty* inner = this->Inner;
	int32 index = luaValue.as<int32>();
	index--; //Lua index correction
	FScriptSetHelper helper = CreateHelperFormElementProperty(inner, this->GetScriptSet());

	helper.RemoveAt(index);
}

void FLuaSet::Lua_Empty()
{
	FProperty* inner = this->Inner;
	FScriptSetHelper helper = CreateHelperFormElementProperty(inner, this->GetScriptSet());
	helper.EmptyElements();
}

void FLuaSet::Append(const FLuaSet& toAppend)
{
	FProperty* inner = this->Inner;
	FScriptSetHelper helperSource = CreateHelperFormElementProperty(inner, toAppend.GetScriptSet());
	FScriptSetHelper helper = CreateHelperFormElementProperty(inner, this->GetScriptSet());
	for(FScriptSetHelper::FIterator it = helperSource.CreateIterator(); it; ++it)
	{
		helper.AddElement(helper.GetElementPtr(it));
	}
}

FProperty* FLuaSet::GetInner() const
{
	return this->Inner;
}

void FLuaSet::Copy(FScriptSet* src, FProperty* innerProp, FScriptSet* dest)
{
	FScriptSetHelper SrcSetHelper = FScriptSetHelper::CreateHelperFormElementProperty(innerProp, src);
	FScriptSetHelper DestSetHelper = FScriptSetHelper::CreateHelperFormElementProperty(innerProp, dest);

	int32 Num = SrcSetHelper.Num();
	DestSetHelper.EmptyElements(Num);

	if (Num == 0)
	{
		return;
	}

	for (int32 SrcIndex = 0; Num; ++SrcIndex)
	{
		if (SrcSetHelper.IsValidIndex(SrcIndex))
		{
			const int32 DestIndex = DestSetHelper.AddDefaultValue_Invalid_NeedsRehash();

			uint8* SrcData  = SrcSetHelper.GetElementPtr(SrcIndex);
			uint8* DestData = DestSetHelper.GetElementPtr(DestIndex);

			innerProp->CopyCompleteValue_InContainer(DestData, SrcData);

			--Num;
		}
	}

	DestSetHelper.Rehash();
}

void FLuaSet::Clone(FScriptSet* dest) const
{
	// blueprint stack will destroy the TArray
	// so deep-copy construct FScriptArray
	// it's very expensive
	if (!dest || !this->GetScriptSet() || this->GetScriptSet()->Num() == 0)
	{
		return;
	}
	FProperty* inner = this->Inner;
	
	FScriptSetHelper destHelper = CreateHelperFormElementProperty(inner, dest);

	FScriptSetHelper thisHelper = CreateHelperFormElementProperty(inner, this->GetScriptSet());
	destHelper.EmptyElements(this->GetScriptSet()->Num());
	for(FScriptSetHelper::FIterator it = thisHelper.CreateIterator(); it; ++it)
	{
		destHelper.AddElement(thisHelper.GetElementPtr(it));
	}
}
/*
void FLuaSet::Clone(FProperty* prop, FScriptSet* src, FScriptSet* dest) const
{
	// blueprint stack will destroy the TArray
	// so deep-copy construct FScriptArray
	// it's very expensive
	if (!dest || !src || !prop)
	{
		return;
	}

	FScriptSetHelper destHelper = CreateHelperFormElementProperty(prop, dest);

	FScriptSetHelper srcHelper = CreateHelperFormElementProperty(prop, src);
	destHelper.EmptyElements(src->Num());
	for(FScriptSetHelper::FIterator it = srcHelper.CreateIterator(); it; ++it)
	{
		destHelper.AddElement(srcHelper.GetElementPtr(it));
	}
}
*/

FLuaSet FLuaSet::Lua_Copy() const
{
	FProperty* inner = this->Inner;
	FProperty* prop = UnrealLua::PropertyHelper::CreateNewProperty(inner, UnrealLua::PropertyNames::NAME_UnrealLuaSetInnerProperty);
	verify(prop != nullptr);
	
	return FLuaSet{prop, this->GetScriptSet(), false};
}

sol::variadic_results FLuaSet::Lua_Any(sol::object num_o)
{
	const int32 maxIndex = this->GetScriptSet()->Num() -1;
	sol::variadic_results results{};
	if(maxIndex == -1)
	{
		return results;
	}

	FProperty* inner = this->Inner;
	int32 numToGet = !num_o.valid() ? 1 : num_o.as<int32>();
	FScriptSetHelper helper = CreateHelperFormElementProperty(inner, this->GetScriptSet());
	numToGet = FMath::Clamp(numToGet, 1, this->GetScriptSet()->Num());
	sol::this_state lua{num_o.lua_state()};
	if(numToGet == 1)
	{
		//get a single random item
		const int32 randIndex = FMath::RandRange(0, maxIndex);
		FGetPropertyValueParams getParams{inner, helper.GetElementPtr(randIndex), 0,  lua};
		results.emplace_back(UnrealLua::PropertyHelper::GetPropertyValue(getParams));
		return results;
	}

	//if the user wants exacttly as many entries as there are in the set
	if(numToGet == maxIndex + 1)
	{
		//just get all entries and shuffle
		results.reserve(numToGet);
		for(int32 i = 0; i <= maxIndex; i++)
		{
			FGetPropertyValueParams getParams{inner, helper.GetElementPtr(i), 0,  lua};
			results.emplace_back(UnrealLua::PropertyHelper::GetPropertyValue(getParams));
		}
		auto rng = std::default_random_engine {};
		std::shuffle(results.begin(), results.end(), rng);
		
		return results;
	}
	
	TSet<int32> alreadyChosen{};
	
	while(numToGet > 0)
	{
		const int32 randIndex = FMath::RandRange(0, maxIndex);
		if(alreadyChosen.Contains(randIndex))
		{
			continue;
		}
		alreadyChosen.Add(randIndex);
		FGetPropertyValueParams getParams{inner, helper.GetElementPtr(randIndex), 0,  lua};
		results.emplace_back(UnrealLua::PropertyHelper::GetPropertyValue(getParams));
		numToGet--;
	}
	return results;

}

FLuaSet& FLuaSet::Lua_KeepAll(sol::protected_function func, sol::variadic_args args, const sol::this_state lua)
{
	return this->Filter(func, args, true, lua);
}

FLuaSet& FLuaSet::Lua_RemoveAll(sol::protected_function func, sol::variadic_args args, const sol::this_state lua)
{
	return this->Filter(func, args, false, lua);
}

bool FLuaSet::Lua_IsEmpty() const
{
	FScriptSet* set = this->GetScriptSet();
	return set->IsEmpty();
}

bool FLuaSet::IsValid() const
{
	return this->Inner != nullptr && this->GetScriptSet() != nullptr;
}

//Filter for deciding which elements to keep
FLuaSet& FLuaSet::Filter(sol::protected_function func, sol::variadic_args args, bool bShouldKeep, sol::this_state lua)
{
	FProperty* inner = this->Inner;
	if(func.valid())
	{
		FScriptSetHelper helper = CreateHelperFormElementProperty(inner, this->GetScriptSet()); 
		
		for(int32 i = helper.Num() - 1; i >= 0; i--)
		{
			void* v = helper.GetElementPtr(i);
			FGetPropertyValueParams getParams{inner, v, 0,  lua};
			sol::object obj = UnrealLua::PropertyHelper::GetPropertyValue(getParams);
			sol::function_result res = func(obj, args);
			//If either
			//1. Result not valid or
			//2. We should keep but got false as result
			//3. We should not keep and get confirmation
			if(!res.valid() || (bShouldKeep && !res.get<bool>()) || (!bShouldKeep && res.get<bool>()))
			{
				helper.RemoveAt(i);
			}
		}	
	}
	return *this;
}

sol::table FLuaSet::Lua_ToLuaTable(sol::this_state lua_s)
{
	int32 num = this->GetScriptSet()->Num();
	sol::state_view lua = lua_s;
	sol::table results = lua.create_table();
	FProperty* inner = this->Inner;
	FScriptSetHelper helper = CreateHelperFormElementProperty(inner, this->GetScriptSet());
	for(int32 i = 0; i < num; i++)
	{
		FGetPropertyValueParams getParams{inner, helper.GetElementPtr(i), 0,  lua_s};
		results[i+1] = UnrealLua::PropertyHelper::GetPropertyValue(getParams);
	}
	return results;
}

sol::object FLuaSet::__Index(sol::object key, sol::this_state lua)
{
	int32 index = key.as<int32>();
	return this->__IndexInternal(index, lua);
}

sol::object FLuaSet::__IndexInternal(int32 index, sol::this_state lua)
{
	FProperty* inner = this->Inner;
	index--; //Lua Index correction
	FScriptSetHelper helper = CreateHelperFormElementProperty(inner, this->GetScriptSet());

	if(!helper.IsValidIndex(index))
	{
		return sol::nil;
	}
	void* valPtr = helper.GetElementPtr(index);
	FGetPropertyValueParams getParams{inner, valPtr,0, lua};
	sol::object val = UnrealLua::PropertyHelper::GetPropertyValue(getParams);
	return val;
}

bool FLuaSet::__NewIndex(sol::object value, sol::object op, sol::this_state lua)
{
	if(!op.valid() || op.as<bool>() == false)
	{
		return this->Lua_Remove(value, lua);
	}
	else
	{
		this->Lua_Add(value, lua);
		return true;
	}
}

bool FLuaSet::OwnsMemory() const
{
	return this->Data.IsType<FLuaScriptSet*>();
}

void FLuaSet::AddRef()
{
	this->Data.Get<FLuaScriptSet*>()->AddRef();;
}

int32 FLuaSet::RemoveRef()
{
	if(this->OwnsMemory())
	{
		if(this->Data.Get<FLuaScriptSet*>()->RemoveRef() == 0)
		{
			delete this->Data.Get<FLuaScriptSet*>();
			this->Data.Get<FLuaScriptSet*>() = nullptr;
		}
	}
	return -1;
}

bool FLuaSet::IsUPropertyReference() const
{
	return !this->OwnsMemory();
}

int FLuaSet::__next(lua_State* L)
{
	// this gets called
	// to start the first iteration, and every
	// iteration there after
	// the state you passed in pairs is argument 1
	// the key value is argument 2
	// we do not care about the key value here
	lua_iterator_state& it_state = sol::stack::get<sol::user<lua_iterator_state>>(L, 1);
	auto& it = it_state.it;
	if (it.GetLogicalIndex() == it_state.last.GetLogicalIndex()) {
		return sol::stack::push(L, sol::lua_nil);
	}
	int32 iteratorindex = it.GetLogicalIndex();
	// 2 values are returned (pushed onto the stack):
	// the key and the value
	// the state is left alone

	sol::object obj = it_state.owner.__IndexInternal(iteratorindex+1, L);
	
	int pushed = sol::stack::push(L, iteratorindex+1);
	pushed += sol::stack::push(L, obj);
	++it;
	return pushed;
}

int FLuaSet::__pairs(lua_State* L)
{
	FLuaSet& mt = sol::stack::get<FLuaSet&>(L, 1);
	lua_iterator_state it_state(mt);
	// pairs expects 3 returns:
	// the "next" function on how to advance,
	// the "table" itself or some state,
	// and an initial key value (can be nil)

	// next function controls iteration
	int pushed = sol::stack::push(L, &FLuaSet::__next);
	pushed += sol::stack::push<sol::user<lua_iterator_state>>(L, std::move(it_state));
	pushed += sol::stack::push(L, sol::lua_nil);
	return pushed;
}

int FLuaSet::__ipairs(lua_State* L)
{
	FLuaSet& mt = sol::stack::get<FLuaSet&>(L, 1);
	lua_iterator_state it_state(mt);
	// pairs expects 3 returns:
	// the "next" function on how to advance,
	// the "table" itself or some state,
	// and an initial key value (can be nil)

	// next function controls iteration
	int pushed = sol::stack::push(L, &FLuaSet::__next);
	pushed += sol::stack::push<sol::user<lua_iterator_state>>(L, std::move(it_state));
	pushed += sol::stack::push(L, sol::lua_nil);
	return pushed;
}

/*
void FLuaSet::SetThisPropertyReference(UObject* owner, const sol::object& value, sol::this_state lua)
{
	if(!value.is<FLuaSet>())
	{
		return;
	}
		
	FLuaSet& other = value.as<FLuaSet&>();
	if(!this->Inner || !other.Inner)
	{
		return;
	}
	if(!other.Inner->SameType(this->Inner))
	{
		return;
	}
	other.Clone(this->GetScriptSet());
}
*/

