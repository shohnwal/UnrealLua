// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/UnrealType.h"
#include "Reflection/PropertyHelperTypes.h"
#include "UObject/StrProperty.h"

#include "sol/sol.hpp"
#include "utility/is_integer.hpp"
struct FGetPropertyValueParams;
/**
 * 
 */
class FStructOnScope;

struct UNREALLUA_API FStrPropertyDescr
{
	static bool IsCompatibleType(FProperty* prop, const sol::object& luaValue);
	
	static sol::object GetPropertyValue(const FGetPropertyValueParams& params);
	static int GetPropertyValue(FPushPropertyValueParams& params);
	template<typename LUAOBJ>
	static void SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params);
	static FString GetPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params);

private:
	FStrPropertyDescr() {}
};


inline bool FStrPropertyDescr::IsCompatibleType(FProperty* prop, const sol::object& luaValue)
{
	return luaValue.get_type() == sol::type::string;
}

inline sol::object FStrPropertyDescr::GetPropertyValue(const FGetPropertyValueParams& params)
{
	const FStrProperty* prop = CastField<FStrProperty>(params.Prop);
	FString* txt = prop->GetPropertyValuePtr(params.MemoryPtr);
	return sol::make_object(params.Lua, StringCast<char>(**txt).Get());
}

inline int FStrPropertyDescr::GetPropertyValue(FPushPropertyValueParams& params)
{
	const FStrProperty* prop = CastField<FStrProperty>(params.Prop);
	FString* txt = prop->GetPropertyValuePtr(params.MemoryPtr);
	return sol::stack::push(params.Lua, StringCast<char>(**txt).Get());
}

inline FString FStrPropertyDescr::GetPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params)
{
	if (params.MemoryPtr == nullptr)
	{
		return "\"\"";
	}
	else
	{
		FStrProperty* prop = CastField<FStrProperty>(params.Prop);
		FString* txt = prop->GetPropertyValuePtr(params.MemoryPtr);
		return "\"" + *txt + "\"";
	}
}

template<typename LUAOBJ>
inline void FStrPropertyDescr::SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params)
{
	//@TODO : parse numbers into string? 
	const FStrProperty* prop = CastField<FStrProperty>(params.Prop);
	sol::type luaType = params.LuaValue.get_type(); 
	if(luaType == sol::type::string)
	{
		std::string_view strv = params.LuaValue.template as<sol::string_view>();
		FString str = strv.data();
		FString* targetPtr = prop->GetPropertyValuePtr(params.MemoryPtr);
		*targetPtr = str;
		//prop->SetPropertyValue(targetPtr, str);	
	}
	else if (luaType == sol::type::number)
	{
		FString str{};
		if (sol::utility::is_integer(params.LuaValue))
		{
			int64 value = params.LuaValue.template as<int64>();
			str = FString::FromInt(value);		
		}
		else
		{
			double value = params.LuaValue.template as<double>();
			str = FString::SanitizeFloat(value);		
		}
		FString* targetPtr = prop->GetPropertyValuePtr(params.MemoryPtr);
		*targetPtr = str;
	}
	else
	{
		prop->InitializeValue(params.MemoryPtr);
	}
}

template void FStrPropertyDescr::SetPropertyValue(const TSetPropertyValueParams<sol::object>& params);
template void FStrPropertyDescr::SetPropertyValue(const TSetPropertyValueParams<sol::stack_object>& params);