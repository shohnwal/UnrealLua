// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../UnrealLuaTool.h"

#include "UnrealLuaErrorLogTool.generated.h"

/**
 * 
 */
UCLASS()
class UNREALLUATOOLS_API UUnrealLuaErrorLogTool : public UUnrealLuaTool
{
	GENERATED_BODY()
	public:
	virtual EVerticalAlignment GetMainMenuButtonAlignment() const override;
	virtual int32 GetToolMainMenuSortOrder() const override;
	virtual FString GetToolMainMenuButtonLabel() const override;
};
