// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../UnrealLuaTool.h"
#include "UObject/Object.h"
#include "UnrealLuaStateInspectorTool.generated.h"

/**
 * 
 */
UCLASS()
class UNREALLUATOOLS_API UUnrealLuaStateInspectorTool : public UUnrealLuaTool
{
	GENERATED_BODY()
public:
	virtual int32 GetToolMainMenuSortOrder() const override;
	virtual FString GetToolMainMenuButtonLabel() const override;
};
