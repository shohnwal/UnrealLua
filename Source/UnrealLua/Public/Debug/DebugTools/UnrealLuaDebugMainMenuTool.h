// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UnrealLuaDebugTool.h"
#include "UnrealLuaDebugMainMenuTool.generated.h"

/**
 * 
 */
UCLASS()
class UNREALLUA_API UUnrealLuaDebugMainMenuTool : public UUnrealLuaDebugTool
{
	GENERATED_BODY()
protected:
	virtual void InitializeTool_Implementation() override;
public:
	virtual void ActivateTool_Implementation(FInstancedStruct data) override;
	virtual void DeactivateTool_Implementation() override;
	virtual void NotifyInputKeyEvent_Implementation(const FKey& Key, EInputEvent EventType, UGameInstance* gameInstance) override;
	virtual FName GetToolMainMenuButtonName_Implementation() const override;
};
