// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "sol/sol.hpp"
#include "UObject/Object.h"
#include "SeamlessTravelLuaDataStorage.generated.h"

/**
 * 
 */

USTRUCT()
struct UNREALLUA_API FObjectLevelTransitionLuaDataStorage
{
	GENERATED_BODY()
	TWeakObjectPtr<UObject*> Object;
	sol::table SavedData;
};

USTRUCT()
struct UNREALLUA_API FGlobalDataLevelTransitionLuaDataStorage
{
	GENERATED_BODY()
	sol::table SavedData;
};


UCLASS()
class UNREALLUA_API USeamlessTravelLuaDataStorage : public UObject
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FObjectLevelTransitionLuaDataStorage> ObjectDataStorage;
	UPROPERTY()
	FGlobalDataLevelTransitionLuaDataStorage GlobalDataStorage;
};
