// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/TextBlock.h"
#include "LuaValue/LuaScriptValue.h"
#include "LuaScriptValueEditor.generated.h"

/**
 * 
 */
UCLASS()
class UNREALLUA_API ULuaScriptValueEditor : public UUserWidget
{
	GENERATED_BODY()
public:
	void InitializeLuaScriptEditor(FLuaScriptValue* luaScriptValue);
	void UpdateCurrentScriptValue(const FString& currentValueString);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnrealLua")
	TWeakObjectPtr<UObject> LuaScriptValueOwner = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnrealLua")
	FString Key = "";
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnrealLua")
	FString OriginalValue = "";
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnrealLua")
	FString NewValueString = "";
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnrealLua")
	UTextBlock* KeyStringWidget = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnrealLua")
	UMultiLineEditableTextBox* EditValueTextBox = nullptr;
};
