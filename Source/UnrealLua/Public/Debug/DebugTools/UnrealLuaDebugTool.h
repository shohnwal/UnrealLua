// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Engine/EngineBaseTypes.h"
#include "Interface/LuaScriptable.h"
#include "StructUtils/InstancedStruct.h"

#include "UnrealLuaDebugTool.generated.h"

class UGameInstance;
class UGameViewportClient;
class UUnrealLuaDebug;

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable, Within=GameInstance)
class UNREALLUA_API UUnrealLuaDebugTool : public UObject, public ILuaScriptable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	UGameInstance* GetGameInstance() const;
	
	void Initialize(UUnrealLuaDebug* debug);
protected:
	UFUNCTION(BlueprintNativeEvent)
	void InitializeTool();
	virtual void InitializeTool_Implementation();
	
	UFUNCTION(BlueprintNativeEvent)
	void CreateToolWidgets();
	void CreateToolWidgets_Implementation();
	
public:
	UFUNCTION(BlueprintNativeEvent)
	void ActivateTool(FInstancedStruct data);
	virtual void ActivateTool_Implementation(FInstancedStruct data) {}
	UFUNCTION(BlueprintNativeEvent)
	void DeactivateTool();
	virtual void DeactivateTool_Implementation() {}

	UFUNCTION(BlueprintNativeEvent)
	void NotifyInputKeyEvent(const FKey& Key, EInputEvent EventType, UGameInstance* gameInstance);
	virtual void NotifyInputKeyEvent_Implementation(const FKey& Key, EInputEvent EventType, UGameInstance* gameInstance) {}
public:
	UFUNCTION(BlueprintNativeEvent)
	FName GetToolMainMenuButtonName() const;
	virtual FName GetToolMainMenuButtonName_Implementation() const;
	
	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	void SetActiveTool(TSubclassOf<UUnrealLuaDebugTool> debugToolClass, FInstancedStruct& args);
	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	void SetToolInactive();
	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	bool IsCurrentTool();
	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	bool ToggleTool(FInstancedStruct args);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnrealLua")
	TObjectPtr<UUnrealLuaDebug> UnrealLuaDebug = nullptr;
};
