// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
namespace UnrealLuaTools::ScriptTemplates
{
	UNREALLUATOOLS_API FString MakeLuaScriptAttributesTemplate(const FString& scriptName = "Script", bool withAnnotations = true);
	UNREALLUATOOLS_API FString MakeLuaScriptReplicationTemplate(const FString& scriptName = "Script", bool withAnnotations = true);
}