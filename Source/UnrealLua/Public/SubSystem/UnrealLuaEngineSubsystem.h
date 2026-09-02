// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "GarbageCollection/UnrealLuaGarbageCollector.h"
#include "LuaContext/LuaImportRegistry.h"
#include "LuaContext/ScopedLuaContext.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"
#include "UnrealLuaEngineSubsystem.generated.h"


class UUnrealLuaConfig;
struct FLuaGCObject;
class UUserDefinedEnum;
struct FUnrealLuaGameModInfo;
class UUnrealLuaDebug;
/**
 * 
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLuaGameSessionActiveChangedDelegate, UUnrealLuaEngineSubsystem*, luaSystem, bool, isSessionActive);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnLuaGameSessionActiveChangedNativeDelegate, UUnrealLuaEngineSubsystem*, bool);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLuaContextActiveChangedDelegate, UUnrealLuaEngineSubsystem*, luaSystem, TScriptInterface<ILuaContext>, luaContext, bool, isLuaContextActive);
UCLASS(Transient)
class UNREALLUA_API UUnrealLuaEngineSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
public:
	//Notification when a Lua game session starts or ends
	//Notifications are sent are sent after all systems have been set up (became active),
	//or before all systems get torn down (became inactive)
	UPROPERTY(BlueprintAssignable)
	FOnLuaGameSessionActiveChangedDelegate OnLuaGameSessionActiveChanged;
	FOnLuaGameSessionActiveChangedNativeDelegate OnLuaGameSessionActiveChangedNative = {};
	//Notification when a Lua state starts or stops participating in a session
	//Notifications are sent are sent after all systems have been set up (became active),
	//or before all systems get torn down (became inactive)
	UPROPERTY(BlueprintAssignable)
	FOnLuaContextActiveChangedDelegate OnLuaContextActiveChanged;
	
	static UUnrealLuaEngineSubsystem* Get();
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	void NotifyAllModulesLoaded();
	void InitCoreSystems();
	void NotifyEngineStartupComplete();
	virtual void Deinitialize() override;
	
	UUnrealLuaDebug* GetUnrealLuaDebug()
	{
		return this->LuaRuntimeInspector;
	}

	FLuaImportRegistry& GetLuaImportRegistry();

	
	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	void ReloadScript(FString scriptPath);

	void NotifyModuleChanged(FName moduleName, EModuleChangeReason ModuleChangeReason);

	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	void CreateUnrealLuaMetadata();

	void NotifyBeginGameSession(const TScriptInterface<ILuaContext>& ictx);
	void NotifyLuaContextInitialized(const TScriptInterface<ILuaContext>& ictx);
	void NotifyEndGameSession(const TScriptInterface<ILuaContext>& ictx);
	void CleanUpObjectsForLuaContext(const TScriptInterface<ILuaContext>& ictx, bool bShutDownLua);
	
	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	static bool IsGameSessionActive();
	
	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	TArray<TScriptInterface<ILuaContext>> GetActiveLuaContextList();
	const TArray<TScriptInterface<ILuaContext>>& GetActiveLuaContextListRef();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UUnrealLuaDebug> LuaRuntimeInspector = nullptr;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UUnrealLuaUObjectRegistry> UObjectRegistry = nullptr;
    
	UPROPERTY(VisibleAnywhere, BlueprintReadONly)
	TArray<TScriptInterface<ILuaContext>> ActiveLuaContexts = {};
		
	UPROPERTY(VisibleAnywhere)
	FUnrealLuaGarbageCollector LuaGarbageCollector = {};
	
	UPROPERTY(VisibleAnywhere)
	FLuaImportRegistry LuaImportRegistry = {};
	
	void NotifyEndFrame();
	
	FSimpleDelegate OnTriggerCompiler = {};
	FSimpleDelegate OnAllModulesLoaded = {};
	TDelegate<bool()> OnCompilerOKCheck = {};
	FSimpleDelegate OnEndFrame = {};
	
	static void AddReferencedObjects(UUnrealLuaEngineSubsystem* This, FReferenceCollector& collector);

	bool HasCompilerError() const;
	bool CanRunLua() const;
};
