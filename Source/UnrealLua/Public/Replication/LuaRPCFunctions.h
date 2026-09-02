#pragma once
#include "CoreMinimal.h"
#include "sol/sol.hpp"


//A lua Function starting with SERVER_, CLIENT_ or MULTICAST_ as key
struct UNREALLUA_API FLuaRPCFunction
{
	static void RegisterUsertype(sol::state_view& pairs);
	
	FLuaRPCFunction();
	FLuaRPCFunction(sol::function, const sol::string_view& funcName);
	FLuaRPCFunction(FLuaRPCFunction&& other);
	FLuaRPCFunction(const FLuaRPCFunction& other);
	sol::function LuaFunc;
	TUniquePtr<FString> FuncName;
	void operator()(sol::stack_object self, sol::variadic_args args);
	sol::object GetValue(sol::this_state lua) const;
};