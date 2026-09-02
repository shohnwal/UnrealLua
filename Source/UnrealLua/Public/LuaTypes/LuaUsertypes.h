// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "sol/sol.hpp"
/**
 * 
 */
class UNREALLUA_API FLuaUsertypes
{
public:
	static void RegisterLuaUserTypes(sol::state& lua);

	static TArray<TFunction<void(sol::state_view&)>> LuaUserTypesCallbacks;

private:
	FLuaUsertypes() {};
	~FLuaUsertypes() {};
public:
	static bool Is(sol::object obj, sol::object typeToCheck, sol::this_state lua);
};
