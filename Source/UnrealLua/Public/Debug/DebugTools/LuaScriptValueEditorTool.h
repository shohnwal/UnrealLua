// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UnrealLuaDebugTool.h"
#include "Debug/UI/LuaScriptValueEditor.h"
#include "LuaValue/LuaValue.h"
#include "UObject/Object.h"
#include "LuaScriptValueEditorTool.generated.h"

USTRUCT()
struct UNREALLUA_API FUnrealLuaDebugEditScriptValueToolData
{
	GENERATED_BODY()
	
	UPROPERTY()
	TObjectPtr<UObject> Context = nullptr;
	FLuaScriptValue* LuaScriptValuePtr = nullptr;
};

struct FLuaScriptValue;
/**
 * 
 */
UCLASS()
class UNREALLUA_API ULuaScriptValueEditorTool : public UUnrealLuaDebugTool
{
	GENERATED_BODY()
protected:
	virtual void InitializeTool_Implementation() override;
public:
	virtual void ActivateTool_Implementation(FInstancedStruct data) override;
	virtual void DeactivateTool_Implementation() override;
	virtual void NotifyInputKeyEvent_Implementation(const FKey& Key, EInputEvent EventType, UGameInstance* gameInstance) override;
	
	UFUNCTION()
	void NotifyEditedLuaScriptValueChanged(FLuaValue LuaValue);
	
	UFUNCTION()
	bool ProcessAndSetValue(const FString& valueString);
private:
	void RemoveEditorWidget();
public:
	UPROPERTY(VisibleAnywhere, Category = "UnrealLua")
	TObjectPtr<UObject> Context = nullptr;
	UPROPERTY(VisibleAnywhere, Category = "UnrealLua")
	FInstancedStruct Data = {};
	FLuaScriptValue* LuaScriptValuePtr = nullptr;
	
	UPROPERTY(VisibleAnywhere, Category = "UnrealLua")
	ULuaScriptValueEditor* EditorWidget = nullptr;
	UPROPERTY(VisibleAnywhere, Category = "UnrealLua")
	TSubclassOf<ULuaScriptValueEditor> LuaScriptValueEditorClass = nullptr;
};
