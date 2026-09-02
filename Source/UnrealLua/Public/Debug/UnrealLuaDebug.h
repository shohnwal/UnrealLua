// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputKeyEventArgs.h"
#include "BlueprintSupport/WeakStructView.h"
#include "DebugTools/UnrealLuaDebugTool.h"
#include "Engine/GameInstance.h"
#include "Interface/LuaContext.h"
#include "UObject/Object.h"
#include "UObjectRegistry/LuaUObjectItemView.h"
#include "Widgets/SCanvas.h"
#include "UObject/ScriptInterface.h"
#include "UnrealLuaDebug.generated.h"

class UUnrealLuaDebugMainMenuWidget;
class UUnrealLuaEngineSubsystem;
class UUnrealLuaDebug;
struct lua_State;
/**
 * 
 */
USTRUCT(BlueprintType)
struct UNREALLUA_API FLuaFileLinebreakContainer
{
	GENERATED_BODY()
	
	bool Contains(int32 lineIndex) const;
	TArray<int32> Linebreaks = {};
};

USTRUCT(BlueprintType)
struct UNREALLUA_API FUnrealLuaDebugUObjectWatcher
{
	FUnrealLuaDebugUObjectWatcher(UObject* watchedUObject = nullptr);
	
	void SetWatchedUObject(UObject* newWatchedUObject);
	bool IsValid() const;
	UObject* GetUObject() const;

	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnrealLua")
	TWeakObjectPtr<UObject> WatchedObject;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnrealLua")
	FLuaUObjectItemView LuaUObjectItemView;
};

USTRUCT(BlueprintType)
struct UNREALLUA_API FUnrealLuaDebugActorWatcher
{
	GENERATED_BODY()
	
	FUnrealLuaDebugActorWatcher(AActor* watchedActor = nullptr);
	
	void SetWatchedActor(AActor* newWatchedActor);
	void UpdateSubobjects();
	bool IsValid() const;
	UObject* GetUObject() const;
	bool operator==(const UObject& obj) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnrealLua")
	FUnrealLuaDebugUObjectWatcher WatchedActor;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnrealLua")
	TArray<FUnrealLuaDebugUObjectWatcher> SubObjects;
};

USTRUCT()
struct UNREALLUA_API FUnrealLuaGameInstanceSessionInfo
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, Category = "UnrealLua")
	TWeakObjectPtr<UGameInstance> GameInstance = nullptr;
	UPROPERTY(VisibleAnywhere, Category = "UnrealLua")
	TWeakObjectPtr<UGameViewportClient> GameViewportClient = nullptr;
	UPROPERTY(VisibleAnywhere, Category = "UnrealLua")
	UUnrealLuaDebugMainMenuWidget* DebugUICanvas = nullptr;
	UPROPERTY(VisibleAnywhere, Category = "UnrealLua")
	TMap<TObjectPtr<UClass>, TObjectPtr<UUnrealLuaDebugTool>> DebugTools = {};
	UPROPERTY(VisibleAnywhere, Category = "UnrealLua")
	TObjectPtr<UUnrealLuaDebugTool> CurrentDebugTool = {};
	
	void SetActiveTool(TSubclassOf<UUnrealLuaDebugTool> debugToolClass, FInstancedStruct& args);
	void DeactivateTool(TSubclassOf<UUnrealLuaDebugTool> debugToolClass);
	bool IsCurrentTool(TSubclassOf<UUnrealLuaDebugTool> debugToolClass);
	bool ToggleTool(TSubclassOf<UUnrealLuaDebugTool> debugToolClass, FInstancedStruct& args);
	void DeactivateCurrentToolInternal();
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActorSelectionChangedDelegate, bool, bHasWatchedActor, UUnrealLuaDebug*, luaDebugger);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnActorSelectionChangedNativeDelegate, bool, UUnrealLuaDebug*);

UCLASS(BlueprintType, Transient)
class UNREALLUA_API UUnrealLuaDebug : public UObject
{
	GENERATED_BODY()

	UUnrealLuaDebug();
	
public:
	static UUnrealLuaDebug* Get();
	
	UFUNCTION()
	void InitializeUI();
	void NotifyAllModulesLoaded();
public:
	void NotifyActorSelectionChanged(const TArray<UObject*>& Objects, bool bArg);
	void NotifyNewGameInstance(UGameInstance* GameInstance);
	void NotifyInputKeyEvent(const FInputKeyEventArgs& InputKeyEventArgs);
	
	void NotifyObjectDeleted(UObject* deletedObject);
	UFUNCTION()
	void NotifyActorDestroyed(AActor* destroyedActor);
	
	void SetActiveTool(UGameInstance* gameInstance, TSubclassOf<UUnrealLuaDebugTool> debugToolClass, FInstancedStruct& args);
	void DeactivateTool(UGameInstance* gameInstance, UClass* debugToolClass);
	bool IsCurrentTool(UGameInstance* gameInstance, UClass* debugToolClass);
	bool ToggleTool(UGameInstance* gameInstance,UClass* debugToolClass, FInstancedStruct& args);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UUnrealLuaEngineSubsystem* UnrealLuaEngineSubsystem = nullptr;
	
	UPROPERTY(BlueprintAssignable)
	FOnActorSelectionChangedDelegate OnWatchedActorChanged;
	FOnActorSelectionChangedNativeDelegate OnWatchedActorChangedNative = {};
	
	UPROPERTY(VisibleAnywhere)
	bool bisInActorPickingMode = false;

	UFUNCTION(BlueprintCallable)
	void EnableRealTimeTracing();

	UFUNCTION(BlueprintCallable)
	void EnableTracingForLuaContext(const TScriptInterface<ILuaContext>& ctx);

	UFUNCTION(BlueprintCallable)
	void DisableRealTimeTracing();
	
	UFUNCTION(BlueprintCallable)
	void DisableTracingForLuaContext(const TScriptInterface<ILuaContext>& ctx);
	
	UFUNCTION(BlueprintCallable)
	void SetLinebreakAt(const FString& fileName, int32 line);
	
	UFUNCTION(BlueprintCallable)
	void RemoveLinebreakAt(const FString& fileName, int32 line);
	UFUNCTION()
	void NotifyLuaContextActiveChanged(UUnrealLuaEngineSubsystem* LuaSystem, TScriptInterface<ILuaContext> LuaContext, bool bIsLuaContextActive);
	
	void HookFunctionCallInLuaState(lua_State* L);
	void HookLineBreakInLuaState(lua_State* L);
	FUnrealLuaGameInstanceSessionInfo* GetSessionDataForGameInstance(const UGameInstance* gi);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnrealLua")
	bool RealTimeTracingEnabled = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "UnrealLua")
	bool bReactToEditorActorPicking = false;
	
	//void NotifyActorDestroyed(AActor* destroyedActor);
	
	UFUNCTION()
	void NotifyGameSessionActiveChanged(UUnrealLuaEngineSubsystem* LuaSystem, bool bIsSessionActive);
	
	UFUNCTION(BlueprintCallable, Category = "UnrealLua")
	void SetWatchedActor(AActor* newWatchedActor);
	
	UFUNCTION(BlueprintCallable, Category = "UnrealLua")
	FUnrealLuaDebugActorWatcher GetWatchedActor() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess), Category = "UnrealLua")
	TMap<FString, FLuaFileLinebreakContainer> ActiveLinebreaks = {};
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnrealLua")
	FUnrealLuaDebugActorWatcher WatchedActor = {};
	
	UPROPERTY(VisibleAnywhere, Category = "UnrealLua")
	TArray<FUnrealLuaGameInstanceSessionInfo> ActiveGameSessionsInfo = {};
	
	UPROPERTY(VisibleAnywhere, Category = "UnrealLua")
	TSet<TObjectPtr<UClass>> DebugToolClasses = {};
	
	UPROPERTY(VisibleAnywhere, Category = "UnrealLua")
	TArray<TWeakObjectPtr<UUnrealLuaDebugMainMenuWidget>> CreatedDebugMainMenus = {};
};
