#include "LuaTypes/LuaInstancedStruct.h"

#include "LuaCoreDelegates.h"
#include "Utility/LuaLogMacros.h"
#include "LuaContext/ScopedLuaContext.h"
#include "LuaTypes/LuaSharedStruct.h"
#include "LuaTypes/LuaUStruct.h"
#include "Reflection/PropertyHelper.h"
#include "UnrealLua.h"

static const FDelegateHandle fLuaInstancedStructLuaTypeHandle = FLuaCoreDelegates::OnRegisterLuaUsertypes.AddStatic(&FLuaInstancedStruct::RegisterUsertype);

FLuaInstancedStructMemory::FLuaInstancedStructMemory()
	: RefCount(0)
{
	this->InstancedStruct.InitializeAs(nullptr, nullptr);
}

FLuaInstancedStructMemory::FLuaInstancedStructMemory(const UScriptStruct* ss, const void* memToCopyFrom)
	: RefCount(0)
{
	this->InstancedStruct.InitializeAs(ss, static_cast<const uint8*>(memToCopyFrom));
}

FLuaInstancedStructMemory::~FLuaInstancedStructMemory()
{
	this->InstancedStruct.Reset();
}

void FLuaInstancedStructMemory::AddReferencedObjects(FReferenceCollector& Collector)
{
	this->InstancedStruct.AddStructReferencedObjects(Collector);
}

void FLuaInstancedStructMemory::AddRef()
{
	this->RefCount++;
	verifyf(this->RefCount >= 0, TEXT("RefCount of FLuaInstancedStructMemory %p is %d"), this, this->RefCount);
}

int32 FLuaInstancedStructMemory::RemoveRef()
{
	this->RefCount--;
	verifyf(this->RefCount >= 0, TEXT("RefCount of FLuaScriptStructMemory %p is %d"), this, this->RefCount);
	return this->RefCount;
}

void* FLuaInstancedStructMemory::GetMutableMemory()
{
	return this->InstancedStruct.GetMutableMemory();
}

const UScriptStruct* FLuaInstancedStructMemory::GetScriptStruct()
{
	return this->InstancedStruct.GetScriptStruct();
}

void FLuaInstancedStruct::RegisterUsertype(sol::state_view& lua)
{
	sol::usertype<FLuaInstancedStruct> ut = lua.new_usertype<FLuaInstancedStruct>(
		"TInstancedStruct",
		sol::base_classes, sol::bases<FLuaScriptStructBase>(),
		"new", sol::no_constructor,
		sol::call_constructor, sol::factories
		(
			[](sol::this_state lua) { return sol::make_object<FLuaInstancedStruct>(lua, FLuaInstancedStruct{} ); },
			[](const sol::string_view& path, sol::this_state lua) { return FLuaInstancedStruct::MakeFromPath(path, lua);},
			[](const FLuaUStruct& metaStruct, sol::this_state lua){ return FLuaInstancedStruct::MakeFromMetaStruct(metaStruct, lua);},
			[](const FLuaScriptStruct& dataStruct, sol::this_state lua){ return FLuaInstancedStruct::MakeFromDataStruct(dataStruct, lua);}
		),
		//"IsReference", &FLuaInstancedStruct::IsReference,
		"InitializeAs", &FLuaInstancedStruct::Lua_InitializeAs,
		"Copy", &FLuaInstancedStruct::Lua_Copy,
		sol::meta_function::index, &FLuaInstancedStruct::__index,
		sol::meta_function::new_index, &FLuaInstancedStruct::__newindex,
		sol::meta_function::equal_to, &FLuaInstancedStruct::__equals,
		sol::meta_function::less_than_or_equal_to, &FLuaInstancedStruct::__le,
		"IsReference", &FLuaInstancedStruct::IsReference
		//sol::meta_function::garbage_collect, &FLuaInstancedStruct::__gc
	);
}

FLuaInstancedStruct::FLuaInstancedStruct(UScriptStruct* metaData)
	: FLuaScriptStructBase(metaData), Data()
{
	verify(this->PropertyMapping != nullptr);
	this->Data.Emplace<FLuaInstancedStructMemory*>(new FLuaInstancedStructMemory(metaData, nullptr));
	this->AddRef();
}

FLuaInstancedStruct::FLuaInstancedStruct(const FInstancedStruct* instance, bool asRef)
	: FLuaScriptStructBase(instance->GetScriptStruct()), Data()
{
	if(instance->GetScriptStruct())
	{
		verify(this->PropertyMapping != nullptr);
		if(asRef)
		{
			this->Data.Emplace<FInstancedStruct*>(const_cast<FInstancedStruct*>(instance));
		}
		else
		{
			this->Data.Emplace<FLuaInstancedStructMemory*>(new FLuaInstancedStructMemory(instance->GetScriptStruct(), instance->GetMemory()));
			this->AddRef();
		}	
	}
	else
	{
		//uninitialized instanced struct
		if(asRef)
		{
			this->Data.Emplace<FInstancedStruct*>(const_cast<FInstancedStruct*>(instance));
		}
		else
		{
			this->Data.Emplace<FLuaInstancedStructMemory*>(new FLuaInstancedStructMemory(nullptr, nullptr));
			this->AddRef();
		}	
	}
}

FLuaInstancedStruct::FLuaInstancedStruct(const FLuaScriptStruct& other)
	: FLuaScriptStructBase(other.GetScriptStruct()), Data()
{
	verify(this->PropertyMapping != nullptr);
	//From a normal FLuaScriptStruct we always do a full copy
	this->Data.Emplace<FLuaInstancedStructMemory*>(new FLuaInstancedStructMemory(other.GetScriptStruct(), other.GetMemory()));
	this->AddRef();
}

FLuaInstancedStruct::FLuaInstancedStruct(const FLuaInstancedStruct& other, bool asRef)
	: FLuaScriptStructBase(other), Data()
{
	if(asRef)
	{
		this->Data = other.Data;
		if(this->OwnsMemory())
		{
			this->AddRef();
		}
	}
	else
	{
		this->Data.Emplace<FLuaInstancedStructMemory*>(new FLuaInstancedStructMemory(other.GetScriptStruct(), other.GetMemory()));
		this->AddRef();
	}
}

// FLuaInstancedStruct::FLuaInstancedStruct(const FLuaInstancedStruct& other)
// 	: FLuaScriptStructBase(other), InstancedStruct(other.InstancedStruct), bOwnsMemory(other.bOwnsMemory)
// {
// 	this->AddRef();
// }


FLuaInstancedStruct::FLuaInstancedStruct(FLuaInstancedStruct&& other) noexcept
	: FLuaScriptStructBase(MoveTemp(other)), Data(MoveTemp(other.Data))
{
	if(this->OwnsMemory())
	{
		//must add ref BEFORE decreasing ref from other, to keep data alive
		this->AddRef();
	}
	verify(this->PropertyMapping == other.PropertyMapping);
	other.Reset();
	verify(other.GetMemory() == nullptr);
	verify(other.PropertyMapping == nullptr);
}

FLuaInstancedStruct::~FLuaInstancedStruct()
{
	this->Reset();
}

void FLuaInstancedStruct::AddRef()
{
	verify(this->Data.IsType<FLuaInstancedStructMemory*>());
	this->Data.Get<FLuaInstancedStructMemory*>()->AddRef();
}

int32 FLuaInstancedStruct::RemoveRef()
{
	if(this->OwnsMemory())
	{
		if(this->Data.Get<FLuaInstancedStructMemory*>()->RemoveRef() == 0)
		{
			//this->LuaInstancedStructMemory->~FLuaScriptStructMemory();
			delete this->Data.Get<FLuaInstancedStructMemory*>();
			this->Data.Emplace<std::nullptr_t>();
		}
	}
	return -1;
}

void FLuaInstancedStruct::Reset()
{
	this->RemoveRef();
	this->Data.Emplace<std::nullptr_t>();
	this->PropertyMapping = nullptr;
}

bool FLuaInstancedStruct::OwnsMemory() const
{
	return this->Data.IsType<FLuaInstancedStructMemory*>();
}

void FLuaInstancedStruct::CopyFrom(const UScriptStruct* ss, void* memory)
{
	if(!ss)
	{
		if(this->GetInstancedStruct())
		{
			this->GetInstancedStruct()->Reset();
			if(this->OwnsMemory())
			{
				this->Reset();
			}
		}
		return;
	}
	
	this->InitializeAs(ss, memory);
}

/*
void FLuaInstancedStruct::SetThisPropertyReference(UObject* owner, const sol::object& value, sol::this_state lua)
{
	if(!value.valid())
	{
		this->CopyFrom(nullptr, nullptr);
	}
	else if(value.is<FLuaScriptStruct>())
	{
		FLuaScriptStruct& otherStrct = value.as<FLuaScriptStruct>();
		this->CopyFrom(otherStrct.ScriptStruct, otherStrct.Data);
	}
	else if(value.is<FLuaInstancedStruct>())
	{
		FLuaInstancedStruct& otherInst = value.as<FLuaInstancedStruct&>();
		if(otherInst.InstancedStruct)
		{
			this->CopyFrom(otherInst.InstancedStruct->GetScriptStruct(), otherInst.GetMemory());	
		}
	}
	else if(value.is<FLuaSharedStruct>())
	{
		FLuaSharedStruct& otherStrct = value.as<FLuaSharedStruct>();
		this->CopyFrom(otherStrct.SharedStruct.GetScriptStruct(), otherStrct.GetMemory());
	}
	else if(value.is<FLuaUStruct>())
	{
		this->InitializeAs(value);
	}
}
*/

sol::object FLuaInstancedStruct::MakeFromDataStruct(const FLuaScriptStruct& dataStruct, sol::this_state lua)
{
	if(!dataStruct.GetScriptStruct())
	{
		return sol::nil;
	}
	return sol::object(lua, sol::in_place_type<FLuaInstancedStruct>, FLuaInstancedStruct{dataStruct});	
}

sol::object FLuaInstancedStruct::MakeFromMetaStruct(const FLuaUStruct& metaStruct, sol::this_state lua)
{
	UScriptStruct* ss = Cast<UScriptStruct>(metaStruct.TryLoad());
	if(!ss)
	{
		return sol::nil;
	}
	return sol::object(lua, sol::in_place_type<FLuaInstancedStruct>, FLuaInstancedStruct{ss});
}

sol::object FLuaInstancedStruct::MakeFromPath(const sol::string_view& path, sol::this_state lua)
{
	FString name{path.data()};
	
	if (name.IsEmpty())
	{
		return sol::nil;
	}
	
	LUA_LOG("Trying to import UStruct %s", *name)
	
	UScriptStruct* ustruct = FindObject<UScriptStruct>(nullptr, *name);
	
	if(!ustruct)
	{
		ustruct = LoadObject<UScriptStruct>(nullptr, *name);
	}

	if(!ustruct)
	{
		UE_LOG(LogTemp, Error, TEXT("Can't find UStruct named %s"), *name);
		return sol::nil;
	}
	return FLuaInstancedStruct::MakeFromMetaStruct(ustruct, lua);
}

FInstancedStruct* FLuaInstancedStruct::GetInstancedStruct() const
{
	if (!this->IsValid())
	{
		return nullptr;
	}
	if (this->OwnsMemory())
	{
		return &this->Data.Get<FLuaInstancedStructMemory*>()->InstancedStruct;
	}
	else
	{
		return this->Data.Get<FInstancedStruct*>();
	}
}

sol::object FLuaInstancedStruct::Lua_Copy(sol::this_state lua) const
{
	if(this->IsValid())
	{
		return sol::object(lua, sol::in_place_type<FLuaInstancedStruct>, this->GetInstancedStruct(), false);
	}
	return sol::nil;
}

FLuaInstancedStruct FLuaInstancedStruct::Copy() const
{
	FLuaInstancedStruct copy;
	copy.InitializeAs(this->GetScriptStruct(), this->GetMemory());
	return copy;
}

void FLuaInstancedStruct::Lua_InitializeAs(sol::object newStruct)
{
	const UScriptStruct* ss = nullptr;
	void* mem = nullptr;
	if(newStruct.get_type() == sol::type::string)
	{
		sol::string_view strv = newStruct.as<sol::string_view>();
		ss = LoadObject<UScriptStruct>(nullptr, StringCast<TCHAR>(strv.data()).Get());
	}
	else if(newStruct.is<FLuaUStruct>())
	{
		FLuaUStruct& strct = newStruct.as<FLuaUStruct&>();
		ss = Cast<UScriptStruct>(strct.TryLoad());
	}
	else if(newStruct.is<FLuaScriptStructBase>())
	{
		FLuaScriptStructBase& base = newStruct.as<FLuaScriptStructBase>();
		ss = base.GetScriptStruct();
		mem = base.GetMemory();
	}
	if(!ss)
	{
		return;
	}
	this->InitializeAs(ss, mem);
}

void FLuaInstancedStruct::InitializeAs(const UScriptStruct* meta, const void* copyFromMemory)
{
	if(!meta)
	{
		return;
	}
	this->Reset();
	this->UpdatePropertyMapping(meta);
	this->Data.Emplace<FLuaInstancedStructMemory*>(new FLuaInstancedStructMemory(meta, static_cast<const uint8*>(copyFromMemory)));
	this->AddRef();
}

/*
void FLuaInstancedStruct::__gc(FLuaInstancedStruct* me)
{
	if(me->InstancedStruct)
	{
		if(me->bOwnsMemory)
		{
			delete(me->InstancedStruct);
		}
		me->InstancedStruct = nullptr;
	}
}
*/

sol::object FLuaInstancedStruct::__index(FLuaInstancedStruct* strct, sol::object key, sol::this_state lua)
{
	if(!strct)
	{
		return sol::nil;
	}
	if(!strct->IsValid())
	{
		return sol::nil;
	}
	const UScriptStruct* meta = strct->GetScriptStruct();

	sol::object ret = UnrealLua::PropertyHelper::GetValueFromScriptStructProperty(key, *strct, lua); 
	return ret;
}

bool FLuaInstancedStruct::__newindex(FLuaInstancedStruct* strct, sol::stack_object key, sol::stack_object value, sol::this_state lua)
{
	if(!strct || key.get_type() != sol::type::string)
	{
		return false;
	}
	if(!strct->IsValid())
	{
		return false;
	}
	return UnrealLua::PropertyHelper::SetValueInScriptStructProperty(key, *strct, value);
}

bool FLuaInstancedStruct::__equals(FLuaInstancedStruct* me, FLuaInstancedStruct* other)
{
	return me->GetScriptStruct() == other->GetScriptStruct() && me->GetScriptStruct()->CompareScriptStruct(me->GetMemory(), other->GetMemory(), PPF_None);
}

void* FLuaInstancedStruct::GetMemory() const
{
	if(this->Data.IsType<std::nullptr_t>())
	{
		return nullptr;
	}
	if(this->OwnsMemory())
	{
		return this->Data.Get<FLuaInstancedStructMemory*>()->GetMutableMemory();
	}
	else
	{
		return this->Data.Get<FInstancedStruct*>()->GetMutableMemory();
	}
}

bool FLuaInstancedStruct::IsReference() const
{
	return this->Data.IsType<FInstancedStruct*>();
}

const UScriptStruct* FLuaInstancedStruct::GetScriptStruct() const
{
	if(this->Data.IsType<std::nullptr_t>())
	{
		return nullptr;
	}
	if(this->OwnsMemory())
	{
		return this->Data.Get<FLuaInstancedStructMemory*>()->GetScriptStruct();
	}
	else
	{
		return this->Data.Get<FInstancedStruct*>()->GetScriptStruct();
	}
}

bool FLuaInstancedStruct::IsUPropertyReference() const
{
	return !this->OwnsMemory();
}

/*
void FLuaInstancedStruct::AddReferencedObjects(FReferenceCollector& Collector)
{
	FScopeLock lock{&UnrealLua::PropertyHelper::GCLock};
	if(this->InstancedStruct == nullptr || !this->InstancedStruct->IsValid())
	{
		return;
	}
	void* structMem = this->InstancedStruct->GetMutableMemory();
	const UScriptStruct* meta = this->InstancedStruct->GetScriptStruct();
	Collector.AddReferencedObjects(meta, structMem);
}
*/
