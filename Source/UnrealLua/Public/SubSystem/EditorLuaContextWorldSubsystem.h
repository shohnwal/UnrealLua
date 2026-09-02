// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaContext/LuaContextHelper.h"
#include "Interface/LuaContext.h"
#include "Interface/LuaScriptable.h"
#include "LuaContext/ScopedLuaContext.h"
#include "ScriptableUObject/LuaScriptSettings.h"
#include "Subsystems/WorldSubsystem.h"
#include "EditorLuaContextWorldSubsystem.generated.h"

UCLASS(Transient)
class UNREALLUA_API UEditorLuaContextWorldSubsystem : public UTickableWorldSubsystem, public ILuaContext, public ILuaScriptable
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void PostInitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	bool IsTickableInEditor() const;
	void NotifyEditorWorldBeginPlay(bool bHasBegunPlay);
private:
	void BeginPlayInternal();
	void EndPlayInternal();
public:
	void NotifyActorDestroyed(AActor* actor);

	virtual void Tick(float DeltaTime) override;
	virtual void Deinitialize() override;

	virtual TStatId GetStatId() const override;
	virtual FScopedLuaContext& GetScopedLuaContext() override { return *this->LuaContext.Get(); }
	virtual void LoadGameMode(const FName& name) override;
	virtual FLoadedLuaGameModeSettings& GetLoadedLuaModeSettings() override;
	virtual void BroadcastLoadEvent(TScriptInterface<ILuaContext> ictx, FName gameModeName, ELuaLoadEventType loadState) override;

	virtual bool AllowMods() override { return true; }
	
	virtual FLuaScriptSettings GetLuaScriptSettings_Implementation() override;

	virtual void SetLuaScriptSettings_Implementation(FLuaScriptSettings newSettings) override;

	TSharedPtr<FScopedLuaContext> LuaContext = {};

	FOnLuaGameModeLoadEventNative OnLuaGameModeReloadEventNative = {};

	bool bSubSystemHasBegunPlay = false;
	
	UPROPERTY()
	TObjectPtr<UWorld> World;

	UPROPERTY()
	FString PersistentLevelName;
	
	UPROPERTY(VisibleAnywhere)
	FLuaScriptSettings LuaScriptSettings;
};
