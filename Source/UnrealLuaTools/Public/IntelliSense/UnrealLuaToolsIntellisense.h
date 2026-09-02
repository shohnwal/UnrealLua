// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "UObject/Object.h"
#include "UnrealLuaToolsIntellisense.generated.h"

struct FInputKeyEventArgs;
/**
 * 
 */
UCLASS()
class UNREALLUATOOLS_API UUnrealLuaToolsIntellisense : public UObject
{
	GENERATED_BODY()
	
	void NotifyKeyHit(const FInputKeyEventArgs& inputEvent);
};
