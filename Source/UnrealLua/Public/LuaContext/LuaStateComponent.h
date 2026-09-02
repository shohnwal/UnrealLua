// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaContextHelper.h"
#include "Components/ActorComponent.h"
#include "Interface/LuaContext.h"
#include "LuaStateComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UNREALLUA_API ULuaStateComponent : public UActorComponent, public ILuaContext
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	ULuaStateComponent();

protected:
	virtual void InitializeComponent() override;
	// Called when the game starts
	virtual void BeginPlay() override;

	void NotifyLoadEventFromGameInstanceLuaState(TScriptInterface<ILuaContext> luaContext, FName gameModeName, ELuaLoadEventType luaLoadEvent);
public:
	virtual FScopedLuaContext& GetScopedLuaContext() override;
	virtual void LoadGameMode(const FName& name) override;
	virtual FLoadedLuaGameModeSettings& GetLoadedLuaModeSettings() override;
	virtual void BroadcastLoadEvent(TScriptInterface<ILuaContext> ictx, FName gameModeName, ELuaLoadEventType loadState) override;
	
	virtual bool AllowMods() override { return true; }

	FOnLuaGameModeLoadEventDelegate OnLuaGameModeReloadEventNative;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bSynchronizeGameModeWithGameInstance;

	UPROPERTY(BlueprintAssignable)
	FOnLuaGameModeLoadEventDelegate OnLuaGameModeReloadEvent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FString LuaStateName;
	TSharedPtr<FScopedLuaContext> LuaContext = {};
};
