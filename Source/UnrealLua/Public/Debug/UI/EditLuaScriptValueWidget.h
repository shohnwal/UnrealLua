// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EditLuaScriptValueWidget.generated.h"

class UCanvasPanel;
/**
 * 
 */
UCLASS()
class UNREALLUA_API UEditLuaScriptValueWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
public:
	virtual APlayerController* GetOwningPlayer() const override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void OnWidgetRebuilt() override;

public:
	virtual void GetSlotNames(TArray<FName>& SlotNames) const override;
	virtual UWidget* GetContentForSlot(FName SlotName) const override;
	virtual void SetContentForSlot(FName SlotName, UWidget* Content) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnrealLua")
	UCanvasPanel* CanvasPanel = nullptr;
};