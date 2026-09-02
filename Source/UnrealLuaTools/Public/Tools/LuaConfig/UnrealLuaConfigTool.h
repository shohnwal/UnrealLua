// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Tools/UnrealLuaTool.h"
#include "UnrealLuaConfigTool.generated.h"

class SUnrealLuaConfigEditor;
/**
 * 
 */
UCLASS()
class UNREALLUATOOLS_API UUnrealLuaConfigTool : public UUnrealLuaTool
{
public:
	GENERATED_BODY()
	virtual void InitializeTool() override;
	virtual void Shutdown() override;
	virtual EVerticalAlignment GetMainMenuButtonAlignment() const override;
	virtual FString GetToolMainMenuButtonLabel() const override;
	virtual int32 GetToolMainMenuSortOrder() const override;
	virtual void NotifyAddedToMainMenu() override;
	virtual FReply NotifyInputKeyEvent(const FKey& key, EInputEvent eventType, UGameInstance* gameInstance) override;
	virtual void NotifyMainMenuButtonClicked_Implementation() override;
	
	TSharedPtr<SUnrealLuaConfigEditor> LuaConfigEditor = {};
};
