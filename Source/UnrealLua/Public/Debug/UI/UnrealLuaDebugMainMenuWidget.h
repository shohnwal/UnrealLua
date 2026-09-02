// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Debug/DebugTools/UnrealLuaDebugTool.h"
#include "Interface/LuaScriptable.h"
#include "UnrealLuaDebugMainMenuWidget.generated.h"

class UVerticalBox;
class UButton;
class UUnrealLuaDebugTool;
class UCanvasPanelSlot;
class UCanvasPanel;
/**
 * 
 */
UCLASS()
class UNREALLUA_API UUnrealLuaDebugMainMenuWidget : public UUserWidget, public ILuaScriptable
{
	GENERATED_BODY()
public:
	
	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	UCanvasPanelSlot* AddWidgetToMainCanvas(UWidget* widget);
	
	virtual void BeginDestroy() override;

	UFUNCTION()
	void NotifyToolButtonClicked();
	void AddTool(UUnrealLuaDebugTool* tool);
	
protected:
	virtual void NativeOnInitialized() override;
	
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UCanvasPanel* Canvas = nullptr;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UPanelWidget* MainMenuBUttonContainer = nullptr;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<TObjectPtr<UClass>, TObjectPtr<UUnrealLuaDebugTool>> RegisteredTools = {};
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<FName, TObjectPtr<UButton>> RegisteredToolButtons = {};
};
