// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Utility/LuaLogMacros.h"
#include "LuaTypes/LuaEnum.h"
#include "LuaTypes/LuaLightUserdata.h"
#include "Reflection/PropertyHelperTypes.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"
#include "sol/sol.hpp"
#include "UObject/EnumProperty.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"

struct FGetPropertyValueParams;
/**
 * 
 */
struct UNREALLUA_API FEnumPropertyDescr
{
	static bool IsCompatibleType(FProperty* prop, const sol::object& obj);
	
	static sol::object GetPropertyValue(const FGetPropertyValueParams& params);
	static int GetPropertyValue(FPushPropertyValueParams& params);
	static sol::object GetBytePropertyValue(const FGetPropertyValueParams& params);
	static int GetBytePropertyValue(FPushPropertyValueParams& params);
	template<typename LUAOBJ>
	static void SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params);
	static FString GetPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params);

private:
	FEnumPropertyDescr() {}
};


inline bool FEnumPropertyDescr::IsCompatibleType(FProperty* p, const sol::object& obj)
{
	sol::type type = obj.get_type(); 
	return type == sol::type::number || type == sol::type::string || UnrealLua::LightUserdata::IsEnumEntry(obj);
}

inline sol::object FEnumPropertyDescr::GetPropertyValue(const FGetPropertyValueParams& params)
{
	FEnumProperty* prop = CastField<FEnumProperty>(params.Prop);
	uint8* memPtr = static_cast<uint8*>(params.MemoryPtr);
	int64 enumVal = prop->GetUnderlyingProperty()->GetSignedIntPropertyValue(memPtr);
	return UnrealLua::UObjectRegistry::GetEnumValueWrapper(prop->GetEnum(), enumVal, params.Lua);
}

inline int FEnumPropertyDescr::GetPropertyValue(FPushPropertyValueParams& params)
{
	FEnumProperty* prop = CastField<FEnumProperty>(params.Prop);
	uint8* memPtr = static_cast<uint8*>(params.MemoryPtr);
	int64 enumVal = prop->GetUnderlyingProperty()->GetSignedIntPropertyValue(memPtr);
	return UnrealLua::UObjectRegistry::PushEnumValueWrapper(prop->GetEnum(), enumVal, params.Lua);
}

inline sol::object FEnumPropertyDescr::GetBytePropertyValue(const FGetPropertyValueParams& params)
{
	FByteProperty* prop = CastField<FByteProperty>(params.Prop);
	uint64 val = prop->GetUnsignedIntPropertyValue(params.MemoryPtr);
	return UnrealLua::UObjectRegistry::GetEnumValueWrapper(prop->Enum, val, params.Lua);
}

inline int FEnumPropertyDescr::GetBytePropertyValue(FPushPropertyValueParams& params)
{
	FByteProperty* prop = CastField<FByteProperty>(params.Prop);
	uint64 val = prop->GetUnsignedIntPropertyValue(params.MemoryPtr);
	return UnrealLua::UObjectRegistry::PushEnumValueWrapper(prop->Enum, val, params.Lua);
}

inline FString FEnumPropertyDescr::GetPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params)
{
	FEnumProperty* prop = CastField<FEnumProperty>(params.Prop);
	int64 enumVal = prop->GetUnderlyingProperty()->GetSignedIntPropertyValue(params.MemoryPtr);
	UEnum* uenum = prop->GetEnum();
	if (params.MemoryPtr == nullptr)
	{
		return "\"" + uenum->GetNameByIndex(0).ToString() + "\"";
	}
	else
	{
		return "\"" + uenum->GetNameByValue(enumVal).ToString() + "\"";
	}
}

template<typename LUAOBJ>
void FEnumPropertyDescr::SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params)
{
	FEnumProperty* prop = CastField<FEnumProperty>(params.Prop);
	uint8* address = static_cast<uint8*>(params.MemoryPtr);
	if(params.LuaValue.valid())
	{
		const sol::type type = params.LuaValue.get_type(); 
		if(type == sol::type::number)
		{
			int64 value = params.LuaValue.template as<int64>();
			if(prop->GetEnum()->IsValidEnumValue(value))
			{
				prop->GetUnderlyingProperty()->SetIntPropertyValue(address, value);
				return;
			}		
		}
		else if(type == sol::type::string)
		{
			std::string_view strv = params.LuaValue.template as<std::string_view>();
			int64 value = prop->GetEnum()->GetValueByNameString(strv.data(), EGetByNameFlags::None);
			if(value != INDEX_NONE)
			{
				prop->GetUnderlyingProperty()->SetIntPropertyValue(address, value);
				return;
			}
			LUA_LOG_ERROR("Unknown enum value %hs for enum property %s of enum type %s", strv.data(), *prop->GetName(), *prop->GetEnum()->CppType)
			return;
		}
		/*
		else if(type == sol::type::userdata && params.LuaValue.template is<FLuaUEnumEntry>())
		{
			FLuaUEnumEntry* entry = params.LuaValue.template as<FLuaUEnumEntry*>();
			if(entry->uenum == prop->GetEnum())
			{
				int64 value = entry->Value;
				if(prop->GetEnum()->IsValidEnumValue(value))
				{
					prop->GetUnderlyingProperty()->SetIntPropertyValue(address, value);
					return;
				}	
			}
			LUA_LOG_ERROR("Incompatible LuaUEnumEntry enum type %s for enum property %s of enum type %s", *GetNameSafe(entry->uenum), *prop->GetName(), *prop->GetEnum()->CppType)
			return;
		}
		*/
		else if (type == sol::type::lightuserdata && UnrealLua::LightUserdata::IsEnumEntry(params.LuaValue))
		{
			FLuaUEnumEntry* entry = UnrealLua::LightUserdata::GetEnumEntry(params.LuaValue);
			verify(entry != nullptr);
			if(entry->uenum == prop->GetEnum())
			{
				int64 value = entry->Value;
				if(prop->GetEnum()->IsValidEnumValue(value))
				{
					prop->GetUnderlyingProperty()->SetIntPropertyValue(address, value);
					return;
				}	
			}
			LUA_LOG_ERROR("Incompatible LuaUEnumEntry enum type %s for enum property %s of enum type %s", *GetNameSafe(entry->uenum), *prop->GetName(), *prop->GetEnum()->CppType)
			return;
		}
	}
	else
	{
		int64 value = 0;
		prop->GetUnderlyingProperty()->SetIntPropertyValue(address, value);
	}		
}

template
void FEnumPropertyDescr::SetPropertyValue<sol::object>(const TSetPropertyValueParams<sol::object>& params);
template
void FEnumPropertyDescr::SetPropertyValue<sol::stack_object>(const TSetPropertyValueParams<sol::stack_object>& params);