// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaContextHelper.h"
#include "Interface/LuaContext.h"
#include "UObject/Object.h"
#include "LuaContextBlueprintBase.generated.h"

/**
 * 
 */

UCLASS(BlueprintType, Blueprintable, Abstract, Transient)
class UNREALLUA_API ULuaContextBlueprintBase : public UObject, public ILuaContext
{
	GENERATED_BODY()
public:
	virtual void LoadGameMode(const FName& name) override;

	virtual void BroadcastLoadEvent(TScriptInterface<ILuaContext> ictx, FName gameModeName, ELuaLoadEventType loadState) override;
	
	FOnLuaGameModeLoadEventDelegate OnLuaGameModeReloadEventNative;

	UPROPERTY(BlueprintAssignable, VisibleAnywhere)
	FOnLuaGameModeLoadEventDelegate OnLuaGameModeReloadEvent;

	virtual bool IsReadyForFinishDestroy() override;

	//UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	virtual FScopedLuaContext& GetScopedLuaContext() override;
public:
	virtual FLoadedLuaGameModeSettings& GetLoadedLuaModeSettings() override;

	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	void UnloadLua();
	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	bool IsLuaLoaded();
	
	virtual bool AllowMods() override { return true; }

private:
	TSharedPtr<FScopedLuaContext> LuaContext;
};
