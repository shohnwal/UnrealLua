// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ScriptableUObject/LuaScriptSettings.h"
#include "Blueprint/UserWidget.h"
#include "Interface/LuaScriptable.h"
#include "LuaUserWidget.generated.h"

/**
 * 
 */

UCLASS()
class UNREALLUA_API ULuaUserWidget : public UUserWidget, public ILuaScriptable
{
	GENERATED_BODY()
public:
	
	virtual void BeginPlayNative();
	
	UFUNCTION(BlueprintImplementableEvent)
	void BeginPlay();

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	virtual FLuaScriptSettings GetLuaScriptSettings_Implementation() override;

	virtual void SetLuaScriptSettings_Implementation(FLuaScriptSettings newSettings) override;

	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	UWidget* CreateRootWidget(TSubclassOf<UWidget> widgetClass, FName widgetName = NAME_None);
	
	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	 UWidget* CreateWidget(TSubclassOf<UWidget> widgetClass, FName widgetName = NAME_None);

	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	UWidget* SetRootWidget(UWidget* newRoot);

	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	void ClearWidgets();

	UPROPERTY(EditDefaultsonly, meta=(ShowOnlyInnerProperties))
	FLuaScriptSettings LuaScriptSettings;
	
	UPROPERTY(Transient, BlueprintReadOnly)
	bool bNeedsExplicitLuaTicking;
	UPROPERTY(Transient, BlueprintReadOnly)
	bool bHasBegunPlay;

	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	void SetLuaTickEnabled(bool enabled = false);
	
	UPROPERTY(Transient, BlueprintReadOnly)
	bool bLuaTickEnabled = false;

	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	bool IsOverlappingWidget(const FPointerEvent& pointerEvent, UWidget* widget);
};
