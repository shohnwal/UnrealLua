#pragma once

#include "CoreMinimal.h"
#include "LuaValue/LuaFunction.h"
#include "Reflection/PropertyHelperTypes.h"

#include "sol/sol.hpp"

struct FGetPropertyValueParams;

struct UNREALLUA_API FLuaFunctionPropertyDescr
{
	static bool IsCompatibleType(FProperty* prop, const sol::object& luaValue);

	static sol::object GetPropertyValue(const FGetPropertyValueParams& params);
	static int GetPropertyValue(FPushPropertyValueParams& params);

private:
	FLuaFunctionPropertyDescr() { }
};


inline bool FLuaFunctionPropertyDescr::IsCompatibleType(FProperty* prop, const sol::object& luaValue)
{
	verify(!luaValue.is<FLuaFunctionHandle>());
	return luaValue.is<sol::function>();
}

inline sol::object FLuaFunctionPropertyDescr::GetPropertyValue(const FGetPropertyValueParams& params)
{
	FLuaFunctionHandle* func = static_cast<FLuaFunctionHandle*>(params.MemoryPtr);
	if(func && func->IsValid())
	{
		return sol::object(func->GetFunction().lua_state(), func->GetFunction());	
	}
	return sol::nil;
}

inline int FLuaFunctionPropertyDescr::GetPropertyValue(FPushPropertyValueParams& params)
{
	FLuaFunctionHandle* func = static_cast<FLuaFunctionHandle*>(params.MemoryPtr);
	if(func && func->IsValid())
	{
		return sol::stack::push(params.Lua, func->GetFunction());	
	}
	return sol::stack::push(params.Lua, sol::nil);
}