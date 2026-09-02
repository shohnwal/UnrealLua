// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Reflection/PropertyHelperTypes.h"
#include "sol/sol.hpp"
#include "UObject/UnrealType.h"
#include "StringHandling/UnrealLuaStringCache.h"
#include "UObject/Field.h"

class FNameProperty;
struct FGetPropertyValueParams;
/**
 * 
 */

struct UNREALLUA_API FNamePropertyDescr
{
	static bool IsCompatibleType(FProperty* prop, const sol::object& luaValue);
	
	static sol::object GetPropertyValue(const FGetPropertyValueParams& params);
	static int GetPropertyValue(FPushPropertyValueParams& params);
	template<typename LUAOBJ>
	static void SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params);
	static FString GetPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params);

private:
	FNamePropertyDescr() { }
};


inline bool FNamePropertyDescr::IsCompatibleType(FProperty* prop, const sol::object& luaValue)
{
	return luaValue.get_type() == sol::type::string;
}

inline sol::object FNamePropertyDescr::GetPropertyValue(const FGetPropertyValueParams& params)
{
	FNameProperty* prop = CastField<FNameProperty>(params.Prop);
	FName name = prop->GetPropertyValue(params.MemoryPtr);
	auto chars = StringCast<char>(*name.ToString());
	return sol::object(params.Lua, sol::in_place_type<std::string>, chars.Get());
}

inline int FNamePropertyDescr::GetPropertyValue(FPushPropertyValueParams& params)
{
	FNameProperty* prop = CastField<FNameProperty>(params.Prop);
	FName name = prop->GetPropertyValue(params.MemoryPtr);
	auto chars = StringCast<char>(*name.ToString());
	return sol::stack::push(params.Lua, chars.Get());
}

inline FString FNamePropertyDescr::GetPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params)
{
	if (params.MemoryPtr == nullptr)
	{
		return "\"\"";
	}
	else
	{
		FNameProperty* prop = CastField<FNameProperty>(params.Prop);
		FName name = prop->GetPropertyValue(params.MemoryPtr);
		return "\"" + name.ToString() + "\"";
	}
}

template<typename LUAOBJ>
void FNamePropertyDescr::SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params)
{
	FNameProperty* prop = CastField<FNameProperty>(params.Prop);
	if(params.LuaValue.valid())
	{
		FName name = UnrealLua::StringCache::GetFNameForStringLuaObject(params.LuaValue);
		prop->SetPropertyValue(params.MemoryPtr, name);
	}
	else
	{
		FName none{NAME_None};
		prop->SetPropertyValue(params.MemoryPtr, none);
	}
}

template void FNamePropertyDescr::SetPropertyValue<sol::object>(const TSetPropertyValueParams<sol::object>& params);
template void FNamePropertyDescr::SetPropertyValue<sol::stack_object>(const TSetPropertyValueParams<sol::stack_object>& params);