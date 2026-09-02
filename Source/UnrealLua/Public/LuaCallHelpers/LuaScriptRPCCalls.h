#pragma once
#include "CoreMinimal.h"
#include "sol/forward.hpp"

namespace UnrealLua::LuaScriptCall
{
	void UNREALLUA_API RPCCall(sol::object self, sol::object funcName, sol::variadic_args args);
	void UNREALLUA_API RPCCallOnObject(UObject* self, const FString& funcName, sol::variadic_args& args);
}