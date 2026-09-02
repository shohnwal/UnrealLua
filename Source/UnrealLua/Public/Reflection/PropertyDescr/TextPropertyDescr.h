// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Reflection/PropertyHelperTypes.h"
#include "UObject/UnrealType.h"
#include "sol/sol.hpp"
#include "sol/utility/is_integer.hpp"
#include "UObject/TextProperty.h"
/**
 * 
 */

struct FGetPropertyValueParams;
class FStructOnScope;

struct UNREALLUA_API FTextPropertyDescr
{
	static bool IsCompatibleType(FProperty* prop, const sol::object& luaValue);
	static sol::object GetPropertyValue(const FGetPropertyValueParams& params);
	static int GetPropertyValue(FPushPropertyValueParams& params);
	template<typename LUAOBJ>
	static void SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params);
	static FString GetPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params);

private:
	FTextPropertyDescr() { }
};


inline bool FTextPropertyDescr::IsCompatibleType(FProperty* prop, const sol::object& luaValue)
{
	return luaValue.get_type() == sol::type::string;
}

inline sol::object FTextPropertyDescr::GetPropertyValue(const FGetPropertyValueParams& params)
{
	FTextProperty* prop = CastField<FTextProperty>(params.Prop);  
	FText* txt = prop->GetPropertyValuePtr(params.MemoryPtr);
	return sol::make_object(params.Lua, txt);
}

inline int FTextPropertyDescr::GetPropertyValue(FPushPropertyValueParams& params)
{
	FTextProperty* prop = CastField<FTextProperty>(params.Prop);  
	FText* txt = prop->GetPropertyValuePtr(params.MemoryPtr);
	return sol::stack::push(params.Lua, txt);
}

inline FString FTextPropertyDescr::GetPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params)
{
	if (params.MemoryPtr == nullptr)
	{
		return "";
	}
	else
	{
		FTextProperty* prop = CastField<FTextProperty>(params.Prop);
		FText* txt = prop->GetPropertyValuePtr(params.MemoryPtr);
		return "\"" + txt->ToString() + "\"";	
	}
}

template<typename LUAOBJ>
inline void FTextPropertyDescr::SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params)
{
	FTextProperty* prop = CastField<FTextProperty>(params.Prop);
	prop->InitializeValue(params.MemoryPtr);
	
	if(!params.LuaValue.valid())
	{
		prop->SetPropertyValue(params.MemoryPtr, FText::GetEmpty());
		return;
	}
	FString str{};
	if(params.LuaValue.get_type() == sol::type::string)
	{
		str = params.LuaValue.template as<sol::string_view>().data();
	}
	else if(params.LuaValue.get_type() == sol::type::number)
	{
		if(sol::utility::is_integer(params.LuaValue))
		{
			int64 number = params.LuaValue.template as<int>();
			str = FString::Printf(TEXT("%lld"), number);
		}
		else
		{
			double number = params.LuaValue.template as<double>();
			str = FString::Printf(TEXT("%g"), number);
		}
	}
	
	FText txt = FText::FromString(str);
	prop->SetPropertyValue(params.MemoryPtr, txt);
}

template void FTextPropertyDescr::SetPropertyValue(const TSetPropertyValueParams<sol::object>& params);
template void FTextPropertyDescr::SetPropertyValue(const TSetPropertyValueParams<sol::stack_object>& params);
