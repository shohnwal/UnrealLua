// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputKeyEventArgs.h"
#include "LuaStackHandler/LuaStackHandler.h"
#include "LuaValue/LuaFunction.h"
#include "UObject/Object.h"
#include "LuaStateInputHandler.generated.h"

class UGameViewportClient;
class AActor;
struct FLuaUEnumMapping;
enum class ETriggerEvent : uint8;
enum EInputEvent : int;
class UDynamicBlueprintBinding;



USTRUCT()
struct UNREALLUA_API FUnrealLuaInputKeyBinding
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere)
	uint64 BindingHandle = 0;
	
	UPROPERTY(VisibleAnywhere)
	FString BindingID = TEXT("");
	
	UPROPERTY(VisibleAnywhere)
	FKey Key = {};
	
	FModifierKeysState Modifiers = {};
	
	UPROPERTY(VisibleAnywhere)
	FLuaFunctionHandle Callback = {};
};

USTRUCT()
struct UNREALLUA_API FUnrealLuaInputKeyBindingCollection
{
	GENERATED_BODY()
	
	FUnrealLuaInputKeyBinding& FindOrAddBinding(const FKey& Key, const FString& id);
	FUnrealLuaInputKeyBinding* FindBinding(const FString& bindingID);
	uint64 UnbindBinding(const FString& String);
	bool UnbindBinding(uint64 handle);
	bool IsEmpty();

	UPROPERTY(VisibleAnywhere)
	TArray<FUnrealLuaInputKeyBinding> Bindings = {};
};

/**
 * 
 */
DECLARE_DELEGATE_ThreeParams(FOnActionInputDelegate, AActor*, FName, EInputEvent);

UCLASS(Transient)
class UNREALLUA_API ULuaStateInputHandler : public UObject
{
	GENERATED_BODY()
public:
	ULuaStateInputHandler();
	
	virtual void BeginDestroy() override;

	UFUNCTION()
	uint64 BindKeyEvent(const FKey key, const FLuaFunctionHandle& Function, const FString& bindingID);
	
	UFUNCTION()
	bool UnbindKeyEventByHandle(uint64 handle);
	
	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	bool UnbindKeyEvent(const FKey key, const FString& bindingID);
	
	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	void PressKey(FKey key, bool holdKey = false);
	
	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	void ReleaseKey(FKey key);
	
	void NotifyInputKeyEvent(const FInputKeyEventArgs& InputKeyEventArgs, const FString& bindingID );

	UPROPERTY()
	TObjectPtr<UGameViewportClient> Viewport = nullptr;
	
	UPROPERTY()
	TObjectPtr<UEnum> TriggerEventEnum = nullptr;
	UPROPERTY()
	TObjectPtr<UEnum> InputEventEnum = nullptr;
	
	UPROPERTY(VisibleAnywhere)
	TMap<FKey, FUnrealLuaInputKeyBindingCollection> KeyToBindingsMap;
	
	UPROPERTY(VisibleAnywhere)
	TMap<int64, FKey> HandleToKeyMap;
};


