// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UnrealLuaStringEntryKey.h"
#include "sol/sol.hpp"

namespace UnrealLua::StringCache
{
	extern UNREALLUA_API void Initialize();
	extern UNREALLUA_API void CleanUp();
	extern UNREALLUA_API FName GetFNameForStringView(const std::string_view& key);
	extern UNREALLUA_API FUnrealLuaNameEntryKey GetStringEntryKey(const std::string_view& key);
	extern UNREALLUA_API FUnrealLuaNameEntryKey GetStringEntryKey(FStringView key);
	extern UNREALLUA_API FName GetFNameForStringLuaObject(const sol::object& obj);
	extern UNREALLUA_API FName GetFNameForStringLuaObject(const sol::stack_object& obj);
}
