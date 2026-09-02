// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UnrealLuaGameModInfo.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct UNREALLUA_API FUnrealLuaGameModInfo
{
	GENERATED_BODY()
	//Mod name, for now same as ModFolderName
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString ModName;
	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString Version;
	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString Author;
	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString Description;
	//Mod folder name inside the GameRoot/Mods/ directory
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) 
	FString ModFolderName;
	//Full path to the mod directory, does NOT end with a slash
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString Directory;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FString> ModDependencies;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bCanEverBeActive = false;
	
	//The absolute path to the mods Mod.lua file to attach as a script (if any) 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString LuaScriptFilePath;
};