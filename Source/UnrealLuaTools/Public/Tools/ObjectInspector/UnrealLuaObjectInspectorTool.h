// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Tools/UnrealLuaTool.h"
#include "UnrealLuaObjectInspectorTool.generated.h"

class SLuaScriptValueEditor;
class SUnrealLuaObjectInspector;
class UUnrealLuaUserWidget;
class UUserWidget;
class UImage;
/**
 * 
 */
UCLASS()
class UNREALLUATOOLS_API UUnrealLuaObjectInSpectorTool : public UUnrealLuaTool
{
	GENERATED_BODY()

public:
	virtual void InitializeTool() override;
	virtual void ActivateTool(const FUnrealLuaTooleActivateCallback& preActivateCallback) override;
	virtual int32 GetToolMainMenuSortOrder() const override;
	virtual FString GetToolMainMenuButtonLabel() const override;
	virtual void NotifyAddedToMainMenu() override;
	virtual FReply NotifyInputKeyEvent(const FKey& key, EInputEvent eventType, UGameInstance* gameInstance) override;
	virtual void DeactivateTool() override;
	virtual void Shutdown() override;
	virtual void NotifyMainMenuButtonClicked_Implementation() override;

	virtual void Tick(float dt) override;
	virtual bool RequiresTick_Implementation() override;
	
	void SetWatchedObject(UObject* watchedObject);
	
	UPROPERTY()
	TObjectPtr<UUnrealLuaUserWidget> CursorWidget = nullptr;
	UPROPERTY()
	TObjectPtr<UImage> CursorImage = nullptr;
	
	TSharedPtr<SUnrealLuaObjectInspector> ObjectWatcherWidget = {};
};
