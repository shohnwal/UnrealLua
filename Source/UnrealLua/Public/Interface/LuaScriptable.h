// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Replication/LuaNetHandle.h"
#include "ScriptableUObject/LuaScriptSettings.h"
#include "UObject/Interface.h"
#include "LuaScriptable.generated.h"

struct FLuaUObjectItem;
struct FInputActionValue;
enum EInputEvent : int;
enum class ETriggerEvent : uint8;
struct FLuaScriptSettings;
// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable, BlueprintType)
class UNREALLUA_API ULuaScriptable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class UNREALLUA_API	ILuaScriptable
{
	GENERATED_BODY()
	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	FLuaScriptSettings GetLuaScriptSettings();
	virtual FLuaScriptSettings GetLuaScriptSettings_Implementation();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void SetLuaScriptSettings(FLuaScriptSettings newSettings);
	virtual void SetLuaScriptSettings_Implementation(FLuaScriptSettings newSettings);

	UFUNCTION()
	virtual bool LoadLuaScript();

	UFUNCTION(BlueprintNativeEvent)
	FLuaNetHandle GetUniqueLuaNetHandle(int32 input);
	virtual FLuaNetHandle GetUniqueLuaNetHandle_Implementation(int32 input);

	FLuaUObjectItem& GetUObjectItem();
};