// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Tools/UnrealLuaTool.h"
#include "DefaultLuaScriptCreatorTool.generated.h"

/**
 * 
 */
UCLASS()
class UNREALLUATOOLS_API UDefaultLuaScriptCreatorTool : public UUnrealLuaTool
{
public:
	GENERATED_BODY()
	void InitializeTool() override;
};
