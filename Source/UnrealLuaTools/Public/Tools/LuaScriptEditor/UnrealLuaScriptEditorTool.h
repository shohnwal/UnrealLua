// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../UnrealLuaTool.h"
#include "UnrealLuaScriptEditorTool.generated.h"

class SLuaScriptEditor;
/**
 * 
 */
UCLASS()
class UNREALLUATOOLS_API UUnrealLuaScriptEditorTool : public UUnrealLuaTool
{
	GENERATED_BODY()
public:
	virtual void InitializeTool() override;
	void CreateLuaScriptEditor();
	TSharedRef<SLuaScriptEditor> GetLuaScriptEditor();
	virtual void NotifyMainMenuButtonClicked_Implementation() override;
	virtual void ActivateTool(const FUnrealLuaTooleActivateCallback& callback) override;
	virtual void DeactivateTool() override;
	virtual void Shutdown() override;
	virtual void BeginDestroy() override;
	
	void NotifyCloseWindowButtonClicked(const TSharedRef<SWindow>& window);
	virtual int32 GetToolMainMenuSortOrder() const override;
	virtual FString GetToolMainMenuButtonLabel() const override;
	
	TSharedPtr<SLuaScriptEditor> UnrealEditorLuaScriptEditor = {};
};
