// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Reflection/PropertyHelperTypes.h"
#include "UObject/UnrealType.h"
#include "sol/sol.hpp"
#include "UObject/Field.h"
/**
 * 
 */

struct FGetPropertyValueParams;

struct UNREALLUA_API FBoolPropertyDescr
{
	static bool IsCompatibleType(FProperty* p, const sol::object& obj);
	
	static sol::object GetPropertyValue(const FGetPropertyValueParams& params);
	static int GetPropertyValue(FPushPropertyValueParams& params);
	template<typename LUAOBJ>
	static void SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params);
	static FString GetPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params);

private:
	FBoolPropertyDescr() {}
};


inline bool FBoolPropertyDescr::IsCompatibleType(FProperty* p, const sol::object& obj) 
{ 
	//everything can be converted to bool
	return true; 
}

inline sol::object FBoolPropertyDescr::GetPropertyValue(const FGetPropertyValueParams& params)
{
	FBoolProperty* prop = CastField<FBoolProperty>(params.Prop);
	return sol::object(params.Lua, sol::in_place_type<bool>, prop->GetPropertyValue(params.MemoryPtr));
}

inline int FBoolPropertyDescr::GetPropertyValue(FPushPropertyValueParams& params)
{
	FBoolProperty* prop = CastField<FBoolProperty>(params.Prop);
	return sol::stack::push<bool>(params.Lua, prop->GetPropertyValue(params.MemoryPtr));
}

inline FString FBoolPropertyDescr::GetPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params)
{
	if (params.MemoryPtr == nullptr)
	{
		return "false";
	}
	else
	{
		FBoolProperty* prop = CastField<FBoolProperty>(params.Prop);
		bool val = prop->GetPropertyValue(params.MemoryPtr);
		return val ? "true" : "false";
	}
}

template<typename LUAOBJ>
void FBoolPropertyDescr::SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params)
{
	FBoolProperty* prop = CastField<FBoolProperty>(params.Prop);
	bool value = params.LuaValue.valid() ? params.LuaValue.template as<bool>() : false;
	prop->SetPropertyValue(params.MemoryPtr, value);
}

template
void FBoolPropertyDescr::SetPropertyValue<sol::object>(const TSetPropertyValueParams<sol::object>& params);
template
void FBoolPropertyDescr::SetPropertyValue<sol::stack_object>(const TSetPropertyValueParams<sol::stack_object>& params);