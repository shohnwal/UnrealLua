// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaValue/LuaValue.h"

/**
 *  Holds Lua type information of globally accessible tyes such as UClasses, UStructs, UEnums and Lua-specific types (TArray, etc)
 */
struct UNREALLUATOOLS_API FLuaTypesRepository
{
	void PopulateLuaTypesRepository(const FScopedLuaContext& ctx);
	
	TMap<FString, FLuaValue> TypeNameToTypeInfoMap = {};
};
