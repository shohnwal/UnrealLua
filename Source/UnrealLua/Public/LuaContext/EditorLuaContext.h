// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
/*
#include "CoreMinimal.h"
#if WITH_EDITOR
#include "EditorSubsystem.h"
#endif
#include "LoadedLuaGameModeSettings.h"
#include "LuaContextBase.h"
#include "UObject/NoExportTypes.h"
#include "LuaContext/ScopedLuaContext.h"
#include "Interface/LuaContext.h"
#include "EditorLuaContext.generated.h"

class LuaScriptObjectBase;
class ALuaActor;
class ILuaScriptable;
class ULuaContext;
class ULuaScriptInstance;
struct FLuaTable;
class ULuaContext;

#if WITH_EDITOR
UCLASS(BlueprintType)
class UNREALLUA_API UEditorLuaContext : public UEditorSubsystem, public ILuaContext
{
public:
	GENERATED_BODY()
	
	FOnLuaGameModeLoadEventNative OnLuaGameModeReloadEventNative = {};
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual FScopedLuaContext& GetScopedLuaContext() override;
protected:
	virtual FLoadedLuaGameModeSettings& GetLoadedLuaModeSettings() override {return this->LuaContext.Get()->LoadedGameModeSettings;}
public:
	virtual void LoadGameMode(const FName& name) override;
	virtual void BroadcastLoadEvent(TScriptInterface<ILuaContext> ictx, FName gameModeName, ELuaLoadEventType loadState) override;
private:
	TSharedPtr<FScopedLuaContext> LuaContext = nullptr;
};
#endif
*/