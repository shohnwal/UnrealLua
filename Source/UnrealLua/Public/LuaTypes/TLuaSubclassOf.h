// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaUClass.h"

/**
 * 
 */
struct UNREALLUA_API TLuaSubclassOf
{
	static void RegisterUsertype(sol::state_view& lua);
	
	UClass* GetUClass() const;
	FLuaUClass ClassPath = {};
	bool IsValid() const;
};
