// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaContextHelper.h"
#include "ScopedLuaContext.h"
#include "Interface/LuaContext.h"
#include "UObject/Object.h"
#include "StandaloneLuaContext.generated.h"

struct FScopedLuaContext;
/**
 * 
 */
UCLASS()
class UNREALLUA_API UStandaloneLuaContext : public UObject, public ILuaContext
{
	GENERATED_BODY()
public:
	UStandaloneLuaContext();
	
	virtual bool IsReadyForFinishDestroy() override;

	virtual void BeginDestroy() override;
	
	virtual FScopedLuaContext& GetScopedLuaContext() override;
	virtual void LoadGameMode(const FName& name) override;
	virtual FLoadedLuaGameModeSettings& GetLoadedLuaModeSettings() override;
	virtual void BroadcastLoadEvent(TScriptInterface<ILuaContext> ictx, FName gameModeName, ELuaLoadEventType loadState) override;

	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	void InitializeLuaStateAndLoadGameMode(ELuaContextType luaContextType, const FString& name, const FName& gameMode);

	virtual bool AllowMods() override { return true; }

	TSharedPtr<FScopedLuaContext> LuaContext = nullptr;
	
	UPROPERTY()
	FName LuaContextName;
	
	FOnLuaGameModeLoadEventNative OnLuaGameModeReloadEventNative = {};
};

