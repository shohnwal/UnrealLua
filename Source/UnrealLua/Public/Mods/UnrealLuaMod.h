// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UnrealLuaGameModInfo.h"
#include "Interface/LuaScriptable.h"
#include "UObject/Object.h"
#include "UnrealLuaMod.generated.h"

/**
 * 
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnModActiveChangedDelegate, UUnrealLuaMod*, mod, bool, bIsActive);
UCLASS(BlueprintType)
class UNREALLUA_API UUnrealLuaMod : public UObject, public ILuaScriptable
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintAssignable, Category = "UnrealLua")
	FOnModActiveChangedDelegate OnModActiveChanged;
	virtual void ModActivated();
	UFUNCTION(BlueprintImplementableEvent, Category = "UnrealLua")
	void ReceiveModActivated();
	
	virtual void ModDeactivated();
	UFUNCTION(BlueprintImplementableEvent, Category = "UnrealLua")
	void ReceiveModDeactivated();

	UFUNCTION()
	bool IsActive() const;

	void SetModEnabled(bool newIsEnabled);

	UFUNCTION()
	FString GetDirectory() const { return this->ModInfo.Directory; }

	UFUNCTION(BlueprintImplementableEvent, Category = "UnrealLua")
	void ReceiveModLoaded();

	virtual void NotifyWorldBeginPlay();
	UFUNCTION(BlueprintImplementableEvent, Category = "UnrealLua")
	void ReceiveWorldBeginPlay();

	virtual void NotifyWorldEndPlay();
	UFUNCTION(BlueprintImplementableEvent, Category = "UnrealLua")
	
	void ReceiveWorldEndPlay();
	

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnrealLua")
	bool bQueuedIsEnabled;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnrealLua")
	bool bIsEnabled;

	UFUNCTION(BlueprintCallable, Category = "UnrealLua")
	FString GetModName() const { return this->ModInfo.ModName; };
	UFUNCTION(BlueprintCallable, Category = "UnrealLua")
	FString GetModDirectory() const  { return this->ModInfo.Directory;}
	UFUNCTION(BlueprintCallable, Category = "UnrealLua")
	bool IsModEnabled() const { return this->ModInfo.bCanEverBeActive; }

	virtual FLuaScriptSettings GetLuaScriptSettings_Implementation() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnrealLua")
	FUnrealLuaGameModInfo ModInfo;
};
