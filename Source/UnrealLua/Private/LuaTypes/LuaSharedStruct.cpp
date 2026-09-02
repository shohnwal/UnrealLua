// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaTypes/LuaSharedStruct.h"

#include "LuaCoreDelegates.h"
#include "Utility/LuaLogMacros.h"
#include "LuaTypes/LuaScriptStruct.h"
#include "LuaTypes/LuaUStruct.h"
#include "Reflection/PropertyHelper.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"

static const FDelegateHandle fLuaSharedStructLuaTypeHandle = FLuaCoreDelegates::OnRegisterLuaUsertypes.AddStatic(&FLuaSharedStruct::RegisterUsertype);

void FLuaSharedStruct::RegisterUsertype(sol::state_view& lua)
{
	sol::usertype<FLuaSharedStruct> ut = lua.new_usertype<FLuaSharedStruct>(
		"TSharedStruct",
		sol::base_classes, sol::bases<FLuaScriptStructBase>(),
		"new", sol::no_constructor,
		sol::call_constructor, sol::factories
		(
			[](sol::this_state lua) { return sol::make_object<FLuaSharedStruct>(lua, FLuaSharedStruct{}); },
			[](const sol::string_view& path, sol::this_state lua) { return FLuaSharedStruct::MakeFromPath(path, lua);},
			[](const FLuaUStruct& metaStruct, sol::this_state lua){ return FLuaSharedStruct::MakeFromMetaStruct(metaStruct, lua);},
			[](const FLuaScriptStruct& dataStruct, sol::this_state lua){ return FLuaSharedStruct::MakeFromDataStruct(dataStruct, lua);}
		),
		sol::meta_function::index, &FLuaSharedStruct::__index,
		sol::meta_function::new_index, &FLuaSharedStruct::__newindex,
		sol::meta_function::equal_to, &FLuaSharedStruct::__equals,
		sol::meta_function::less_than_or_equal_to, &FLuaSharedStruct::__le,
		"InitializeAs", &FLuaSharedStruct::InitializeAs,
		"Copy", &FLuaSharedStruct::Lua_Copy,
		"IsValid", &FLuaSharedStruct::IsValid,
		"IsReference", &FLuaSharedStruct::IsReference
	);
}

FLuaSharedStruct::FLuaSharedStruct()
	: FLuaScriptStructBase(nullptr), SharedStruct()
{
	//RegisterGCObject();
}

FLuaSharedStruct::FLuaSharedStruct(UScriptStruct* meta)
	: FLuaScriptStructBase(meta), SharedStruct()
{
	verify(this->PropertyMapping != nullptr);
	SharedStruct.InitializeAs(meta, nullptr);
	//RegisterGCObject();
}

FLuaSharedStruct::FLuaSharedStruct(const FLuaScriptStruct& dataStruct)
	: FLuaScriptStructBase(dataStruct.GetScriptStruct()), SharedStruct()
{
	verify(this->PropertyMapping != nullptr);
	SharedStruct.InitializeAs(dataStruct.GetScriptStruct(), static_cast<uint8*>(dataStruct.GetMemory()));
	//RegisterGCObject();
}

FLuaSharedStruct::FLuaSharedStruct(const FSharedStruct& instance): FLuaScriptStructBase(instance.GetScriptStruct()), SharedStruct(instance)
{
	verify(this->PropertyMapping != nullptr);
	//RegisterGCObject();
}

FLuaSharedStruct::FLuaSharedStruct(const FSharedStruct* instance): FLuaScriptStructBase(instance->GetScriptStruct()), SharedStruct(*instance)
{
	verify(this->PropertyMapping != nullptr);
	//RegisterGCObject();
}

FLuaSharedStruct::FLuaSharedStruct(const FLuaSharedStruct* other): FLuaScriptStructBase(other->SharedStruct.GetScriptStruct()), SharedStruct(other->SharedStruct)
{
	verify(this->PropertyMapping != nullptr);
	//RegisterGCObject();
}

FLuaSharedStruct::FLuaSharedStruct(const FLuaSharedStruct& other): FLuaScriptStructBase(other.SharedStruct.GetScriptStruct()), SharedStruct(other.SharedStruct)
{
	verify(this->PropertyMapping != nullptr);
	//RegisterGCObject();
}

FLuaSharedStruct::FLuaSharedStruct(FLuaSharedStruct&& other) noexcept: FLuaScriptStructBase(other.SharedStruct.GetScriptStruct()), SharedStruct(other.SharedStruct)
{
	if(this->SharedStruct.IsValid())
	{
		verify(this->PropertyMapping != nullptr);
	}
	other.PropertyMapping = nullptr;
	other.SharedStruct.Reset();
	//RegisterGCObject();
}

FLuaSharedStruct::~FLuaSharedStruct()
{
	//UnregisterGCObject();
}

sol::object FLuaSharedStruct::MakeFromPath(sol::string_view path, sol::this_state lua)
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
	return FLuaSharedStruct::MakeFromMetaStruct(ustruct, lua);
}

sol::object FLuaSharedStruct::MakeFromMetaStruct(const FLuaUStruct& metaStruct, sol::this_state lua)
{
	UScriptStruct* ss = Cast<UScriptStruct>(metaStruct.TryLoad());
	if(!ss)
	{
		return sol::nil;
	}
	return sol::object(lua, sol::in_place_type<FLuaSharedStruct>, FLuaSharedStruct{ss});
}

sol::object FLuaSharedStruct::MakeFromDataStruct(const FLuaScriptStruct& dataStruct, sol::this_state lua)
{
	if(!dataStruct.GetScriptStruct())
	{
		return sol::nil;
	}
	return sol::object(lua, sol::in_place_type<FLuaSharedStruct>, FLuaSharedStruct{dataStruct});
}

sol::object FLuaSharedStruct::__index(FLuaSharedStruct* strct, sol::stack_object key, sol::this_state lua)
{
	if(!strct) [[unlikely]]
	{
		return sol::nil;
	}
	if(!strct->SharedStruct.IsValid()) [[unlikely]]
	{
		return sol::nil;
	}
	return UnrealLua::PropertyHelper::GetValueFromScriptStructProperty(key, *strct, lua);
}

bool FLuaSharedStruct::__newindex(FLuaSharedStruct* strct, sol::stack_object key, sol::stack_object value, sol::this_state lua)
{
	if(!strct || key.get_type() != sol::type::string)
	{
		return false;
	}
	if(!strct->SharedStruct.IsValid())
	{
		return false;
	}
	return UnrealLua::PropertyHelper::SetValueInScriptStructProperty(key, *strct, value);
}

bool FLuaSharedStruct::__equals(FLuaSharedStruct* me, FLuaSharedStruct* other)
{
	return me->SharedStruct == other->SharedStruct;
}

bool FLuaSharedStruct::InitializeAs(sol::object structType)
{
	if(structType.is<FLuaUStruct>())
	{
		FLuaUStruct& strct = structType.as<FLuaUStruct&>();
		UScriptStruct* ss = Cast<UScriptStruct>(strct.TryLoad());
		if(!::IsValid(ss))
		{
			return false;
		}
		this->SharedStruct.InitializeAs(ss, nullptr);
		this->UpdatePropertyMapping(ss);
		return true;
	}
	return false;
}

bool FLuaSharedStruct::IsReference() const
{
	//Shared structs are never a reference
	return false;
}

/*
void FLuaSharedStruct::SetThisPropertyReference(UObject* owner, const sol::object& value, sol::this_state lua)
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

void FLuaSharedStruct::CopyFrom(const UScriptStruct* ss, void* memory)
{
	this->SharedStruct.InitializeAs(ss, static_cast<const uint8*>(memory));
	this->UpdatePropertyMapping(ss);
}

const UScriptStruct* FLuaSharedStruct::GetScriptStruct() const
{
	return this->SharedStruct.IsValid() ? this->SharedStruct.GetScriptStruct() : nullptr;
}

sol::object FLuaSharedStruct::Lua_Copy(sol::this_state lua) const
{
	return sol::object(lua, sol::in_place_type<FLuaSharedStruct>, this);
}

FLuaSharedStruct FLuaSharedStruct::Copy() const
{
	FLuaSharedStruct copy;
	copy.CopyFrom(this->GetScriptStruct(), this->GetMemory());
	return copy;
}