// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LuaContextTypes.generated.h"

/**
 * 
 */

UENUM()
enum class ELuaContextID : int32
{
	Default
};

namespace UnrealLua::LuaContext
{
	void GenerateLuaContextUTypeFiles();
}
