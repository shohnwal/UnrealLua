// Fill out your copyright notice in the Description page of Project Settings.


#include "Debug/UnrealLuaDebug.h"
#if WITH_EDITOR
//#include "Editor.h"
//#include "LevelEditor.h"
#endif
#include "EngineUtils.h"
#include "Config/UnrealLuaConfig.h"
#include "Engine/GameViewportClient.h"
#include "Modules/ModuleManager.h"
#include "sol/sol.hpp"
#include "InputCoreTypes.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Viewport.h"
#include "Debug/DebugTools/UnrealLuaDebugActorPickerTool.h"
#include "Debug/DebugTools/UnrealLuaDebugMainMenuTool.h"
#include "Debug/UI/UnrealLuaDebugMainMenuWidget.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"
#include "Utility/LuaLogMacros.h"

namespace UnrealLua::LuaHooks
{
	UUnrealLuaDebug* LuaDebugger = nullptr;
	
	void LuaRealTimeHook(lua_State *L, lua_Debug *ar)
	{
		
	}
	void LuaHook (lua_State *L, lua_Debug *ar)
	{
		//lua function got called
		if (ar->event == LUA_HOOKCALL)
		{
			//check function one level above this hook to see if we should activate linebreak
			lua_Debug callInfo{};
			if (lua_getstack(L, 1, &callInfo))
			{
				lua_getinfo(L, "Snl", &callInfo);
				FString str = callInfo.source;
				str.Append("::");
				str.AppendInt(callInfo.currentline);
				LUA_LOG("Lua func called: %s", *str)
				if (FLuaFileLinebreakContainer* foundLinebreaks = LuaDebugger->ActiveLinebreaks.Find(str))
				{
					//We found a file with line breaks
					//->Add hooks for lines and return
					lua_sethook(L, UnrealLua::LuaHooks::LuaHook, LUA_MASKCALL | LUA_MASKLINE | LUA_MASKRET,1);
				}	
			}
		}
		//A line got executed
		else if (ar->event == LUA_HOOKLINE)
		{
			lua_Debug callInfo{};
			lua_getstack(L, 1, &callInfo);
			lua_getinfo(L, "Sn", &callInfo);
			FString str = callInfo.source;
			int32 line = callInfo.currentline;
			str.Append("::");
			str.AppendInt(callInfo.currentline);
			LUA_LOG("Lua line executed called: %s", *str)
			
			FLuaFileLinebreakContainer& foundLinebreaks = LuaDebugger->ActiveLinebreaks.FindChecked(str);
			if (foundLinebreaks.Contains(line))
			{
				
			}
		}
		//A function is returning
		else if (ar->event == LUA_HOOKRET)
		{
			//check function one level above this hook to see if we should linebreak
			lua_Debug callInfo{};
			lua_getstack(L, 1, &callInfo);
			lua_getinfo(L, "Sn", &callInfo);
			FString str = callInfo.source;
			int32 line = callInfo.currentline;
			str.Append("::");
			str.AppendInt(callInfo.currentline);
			LUA_LOG("Lua call return: %s", *str)
			FLuaFileLinebreakContainer& foundLinebreaks = LuaDebugger->ActiveLinebreaks.FindChecked(str);
			if (foundLinebreaks.Contains(line))
			{
				//We found a file with line breaks
				//->Add hooks for lines and return
				lua_sethook(L, UnrealLua::LuaHooks::LuaHook, LUA_MASKCALL | LUA_MASKLINE | LUA_MASKRET,1);
			}
			else
			{
				//We don't have any line breaks for that file
				//-> Remove line break hook to save performance
				lua_sethook(L, UnrealLua::LuaHooks::LuaHook, LUA_MASKCALL | LUA_MASKRET,1);
			}
		}
	}
}

bool FLuaFileLinebreakContainer::Contains(int32 lineIndex) const
{
	return this->Linebreaks.Contains(lineIndex);
}

FUnrealLuaDebugUObjectWatcher::FUnrealLuaDebugUObjectWatcher(UObject* watchedUObject)
{
	this->SetWatchedUObject(watchedUObject);
}

void FUnrealLuaDebugUObjectWatcher::SetWatchedUObject(UObject* watchedUObject)
{
	this->WatchedObject = watchedUObject;
	if (this->WatchedObject.IsValid())
	{
		this->LuaUObjectItemView = &UnrealLua::UObjectRegistry::GetUObjectItem(watchedUObject);
	}
	else
	{
		this->LuaUObjectItemView = {};
	}
}

bool FUnrealLuaDebugUObjectWatcher::IsValid() const
{
	return this->WatchedObject.IsValid();
}

UObject* FUnrealLuaDebugUObjectWatcher::GetUObject() const
{
	return this->WatchedObject.Get();
}

FUnrealLuaDebugActorWatcher::FUnrealLuaDebugActorWatcher(AActor* watchedActor)
{
	this->SetWatchedActor(watchedActor);
}

void FUnrealLuaDebugActorWatcher::SetWatchedActor(AActor* newWatchedActor)
{
	this->WatchedActor.SetWatchedUObject(newWatchedActor);
	if (::IsValid(newWatchedActor))
	{
		this->UpdateSubobjects();
	}
	else
	{
		this->SubObjects.Empty();
	}
}

void FUnrealLuaDebugActorWatcher::UpdateSubobjects()
{
	this->SubObjects.Empty();
	
	TArray<UObject*> subobjects{};
	GetObjectsWithOuter(this->WatchedActor.GetUObject(), subobjects, true);
	for (UObject* subobj : subobjects)
	{
		this->SubObjects.Emplace(subobj);
	}
}

bool FUnrealLuaDebugActorWatcher::IsValid() const
{
	return this->WatchedActor.IsValid();
}

UObject* FUnrealLuaDebugActorWatcher::GetUObject() const
{
	return this->WatchedActor.GetUObject();
}

bool FUnrealLuaDebugActorWatcher::operator==(const UObject& obj) const
{
	return this->WatchedActor.WatchedObject == &obj;
}

void FUnrealLuaGameInstanceSessionInfo::SetActiveTool(TSubclassOf<UUnrealLuaDebugTool> debugToolClass, FInstancedStruct& args)
{
	if (!debugToolClass)
	{
		this->DeactivateCurrentToolInternal();
		return;
	}
	
	if (this->CurrentDebugTool)
	{
		this->DeactivateCurrentToolInternal();
	}
	
	TObjectPtr<UUnrealLuaDebugTool> debugTool = this->DebugTools.FindChecked(debugToolClass);
	this->CurrentDebugTool = debugTool;
	
	this->CurrentDebugTool->ActivateTool(args); 
}


void FUnrealLuaGameInstanceSessionInfo::DeactivateTool(TSubclassOf<UUnrealLuaDebugTool> debugToolClass)
{
	if (!debugToolClass)
	{
		this->DeactivateCurrentToolInternal();
		return;
	}
	if (this->CurrentDebugTool)
	{
		if (this->CurrentDebugTool->GetClass() == debugToolClass)
		{
			this->DeactivateCurrentToolInternal();
		}
	}
}

bool FUnrealLuaGameInstanceSessionInfo::IsCurrentTool(TSubclassOf<UUnrealLuaDebugTool> debugToolClass)
{
	return (!debugToolClass && !this->CurrentDebugTool) || (this->CurrentDebugTool && this->CurrentDebugTool->GetClass() == debugToolClass);
}

bool FUnrealLuaGameInstanceSessionInfo::ToggleTool(TSubclassOf<UUnrealLuaDebugTool> debugToolClass, FInstancedStruct& args)
{
	if (this->IsCurrentTool(debugToolClass))
	{
		this->DeactivateTool(debugToolClass);
		return false;
	}
	else
	{
		this->SetActiveTool(debugToolClass, args);
		return true;
	}
}

void FUnrealLuaGameInstanceSessionInfo::DeactivateCurrentToolInternal()
{
	if (this->CurrentDebugTool)
	{
		this->CurrentDebugTool->DeactivateTool();
	}
	this->CurrentDebugTool = nullptr;	
}

UUnrealLuaDebug::UUnrealLuaDebug()
{
	if (this->IsTemplate())
	{
		return;
	}
	UnrealLua::LuaHooks::LuaDebugger = this;
	this->UnrealLuaEngineSubsystem = CastChecked<UUnrealLuaEngineSubsystem>(this->GetOuter());
	this->UnrealLuaEngineSubsystem->OnLuaContextActiveChanged.AddDynamic(this, &UUnrealLuaDebug::NotifyLuaContextActiveChanged);
	this->UnrealLuaEngineSubsystem->UObjectRegistry->OnActorDestroyed.AddDynamic(this, &UUnrealLuaDebug::NotifyActorDestroyed);
	this->UnrealLuaEngineSubsystem->UObjectRegistry->OnRemovedObjectEventDelegate.AddUObject(this, &UUnrealLuaDebug::NotifyObjectDeleted);
	this->UnrealLuaEngineSubsystem->OnLuaGameSessionActiveChanged.AddDynamic(this, &UUnrealLuaDebug::NotifyGameSessionActiveChanged);
	FCoreDelegates::OnAllModuleLoadingPhasesComplete.AddUObject(this, &UUnrealLuaDebug::NotifyAllModulesLoaded);
/*
#if WITH_EDITOR
	if (GEditor)
	{
		FLevelEditorModule& levelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
		levelEditor.OnActorSelectionChanged().AddUObject(this, &UUnrealLuaDebug::NotifyActorSelectionChanged);
	}
#endif
*/
}

UUnrealLuaDebug* UUnrealLuaDebug::Get()
{
	return UUnrealLuaEngineSubsystem::Get()->GetUnrealLuaDebug();
}

void UUnrealLuaDebug::InitializeUI()
{
	
}

void UUnrealLuaDebug::NotifyAllModulesLoaded()
{
	//All modules are loaded, now go look for any debug tools
	this->DebugToolClasses.Empty();
	for (TObjectIterator<UClass> classIt; classIt; ++classIt)
	{
		if (classIt->ImplementsInterface(UUnrealLuaDebugTool::StaticClass()))
		{
			this->DebugToolClasses.Emplace(*classIt);
		}
	}
	
	//In editor builds, all modules are loaded before it's possible for any game instance to exist
	//However, GameInstances can be created later for PIE
	FWorldDelegates::OnStartGameInstance.AddUObject(this, &UUnrealLuaDebug::NotifyNewGameInstance);
	
	//If this is a standalone GameEngine, then the GameInstance should already exist post-GameEngine::Init
	{
		for (TObjectIterator<UGameInstance> gameInstanceIt; gameInstanceIt; ++gameInstanceIt)
		{
			this->NotifyNewGameInstance(*gameInstanceIt);
		}	
	}
}

void UUnrealLuaDebug::NotifyActorSelectionChanged(const TArray<UObject*>& Objects, bool bArg)
{
	if (!bReactToEditorActorPicking)
	{
		return;
	}
	if (Objects.Num() != 1)
	{
		this->SetWatchedActor(nullptr);
		return;
	}
	AActor* actor = CastChecked<AActor>(Objects[0]);
	this->SetWatchedActor(actor);
}

void UUnrealLuaDebug::NotifyNewGameInstance(UGameInstance* gameInstance)
{
	UGameViewportClient* viewport = gameInstance->GetGameViewportClient();
	viewport->OnInputKey().AddUObject(this, &UUnrealLuaDebug::NotifyInputKeyEvent);

	UUnrealLuaDebugMainMenuWidget* mainWidget = CreateWidget<UUnrealLuaDebugMainMenuWidget>(gameInstance, UUnrealLuaDebugMainMenuWidget::StaticClass(), "UnrealLuaDebugWidget");
	mainWidget->AddToViewport();
	mainWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	verify(mainWidget->IsInViewport())
	FUnrealLuaGameInstanceSessionInfo& sessionInfo = this->ActiveGameSessionsInfo.Emplace_GetRef(FUnrealLuaGameInstanceSessionInfo{gameInstance, viewport, mainWidget, {}});
	
	//Create a new set of tools for each game instance
	for (UClass* toolClass : this->DebugToolClasses)
	{
		UUnrealLuaDebugTool* newTool = NewObject<UUnrealLuaDebugTool>(gameInstance, toolClass, toolClass->GetFName(), RF_Transient);
		sessionInfo.DebugTools.Emplace(toolClass, newTool);
	}
	
	for (TTuple<TObjectPtr<UClass>, TObjectPtr<UUnrealLuaDebugTool>>& newTool : sessionInfo.DebugTools)
	{
		newTool.Value->Initialize(this);
	}
}

void UUnrealLuaDebug::NotifyInputKeyEvent(const FInputKeyEventArgs& inputKeyEventArgs)
{
	FViewport* viewport = inputKeyEventArgs.Viewport;
	
	UGameInstance* gameInstance = nullptr;
	
	if (viewport)
	{
		if (FViewportClient* viewportclient = viewport->GetClient())
		{
			if (UWorld* world = viewportclient->GetWorld())
			{
				gameInstance = world->GetGameInstance();
			}
		}
	}
	
	if (gameInstance)
	{
		return;
	}
	
	FUnrealLuaGameInstanceSessionInfo* sessionInfo = this->GetSessionDataForGameInstance(gameInstance);
	if (!sessionInfo)
	{
		return;
	}
	
	FKey key = inputKeyEventArgs.Key;
	EInputEvent eventType = inputKeyEventArgs.Event;
	if (sessionInfo->CurrentDebugTool)
	{
		sessionInfo->CurrentDebugTool->NotifyInputKeyEvent(key, eventType, gameInstance);
	}
	//@TODO : Use ULuaConfig::DebugKey
	if (key == EKeys::F6)
	{
		FInstancedStruct params{};
		sessionInfo->SetActiveTool(UUnrealLuaDebugMainMenuTool::StaticClass(), params);
	}
}

void UUnrealLuaDebug::NotifyObjectDeleted(UObject* deletedObject)
{
	AActor* outerActor = deletedObject->GetTypedOuter<AActor>();
	if (outerActor && *outerActor == this->WatchedActor)
	{
		this->SetWatchedActor(nullptr);
	}
}

void UUnrealLuaDebug::NotifyActorDestroyed(AActor* destroyedActor)
{
	if (!this->WatchedActor.IsValid() || this->WatchedActor == *destroyedActor)
	{
		this->SetWatchedActor(nullptr);
	}
}

void UUnrealLuaDebug::SetActiveTool(UGameInstance* gameInstance, TSubclassOf<UUnrealLuaDebugTool> debugToolClass, FInstancedStruct& args)
{
	FUnrealLuaGameInstanceSessionInfo* sessionInfo = this->GetSessionDataForGameInstance(gameInstance);
	if (sessionInfo)
	{
		sessionInfo->SetActiveTool(debugToolClass, args);
	}
}

void UUnrealLuaDebug::DeactivateTool(UGameInstance* gameInstance, UClass* debugToolClass)
{
	FUnrealLuaGameInstanceSessionInfo* sessionInfo = this->GetSessionDataForGameInstance(gameInstance);
	if (sessionInfo)
	{
		sessionInfo->DeactivateTool(debugToolClass);
	}
}

bool UUnrealLuaDebug::IsCurrentTool(UGameInstance* gameInstance, UClass* debugToolClass)
{
	FUnrealLuaGameInstanceSessionInfo* sessionInfo = this->GetSessionDataForGameInstance(gameInstance);
	if (sessionInfo)
	{
		return sessionInfo->IsCurrentTool(debugToolClass);
	}
	return false;
}

bool UUnrealLuaDebug::ToggleTool(UGameInstance* gameInstance, UClass* debugToolClass, FInstancedStruct& args)
{
	FUnrealLuaGameInstanceSessionInfo* sessionInfo = this->GetSessionDataForGameInstance(gameInstance);
	if (sessionInfo)
	{
		return sessionInfo->ToggleTool(debugToolClass, args);
	}
	return false;
}


void UUnrealLuaDebug::EnableRealTimeTracing()
{
	this->RealTimeTracingEnabled = true;
	
	TArray<TScriptInterface<ILuaContext>> luaContextList = this->UnrealLuaEngineSubsystem->GetActiveLuaContextList();
	
	for (TScriptInterface<ILuaContext>& ctx : luaContextList)
	{
		this->EnableTracingForLuaContext(ctx);
	}
}

void UUnrealLuaDebug::EnableTracingForLuaContext(const TScriptInterface<ILuaContext>& ctx)
{
	this->RealTimeTracingEnabled = true;
	
	lua_State* L = ctx->GetScopedLuaContext().GetLuaThisState().lua_state();
	lua_sethook(L, UnrealLua::LuaHooks::LuaHook, LUA_MASKCALL,1);
}

void UUnrealLuaDebug::DisableRealTimeTracing()
{
	this->RealTimeTracingEnabled = false;
	
	TArray<TScriptInterface<ILuaContext>> luaContextList = this->UnrealLuaEngineSubsystem->GetActiveLuaContextList();
	
	if (luaContextList.IsEmpty())
	{
		return;
	}
	
	for (TScriptInterface<ILuaContext>& ctx : luaContextList)
	{
		this->DisableTracingForLuaContext(ctx);
	}
}

void UUnrealLuaDebug::DisableTracingForLuaContext(const TScriptInterface<ILuaContext>& ctx)
{
	lua_State* L = ctx->GetScopedLuaContext().GetLuaThisState().lua_state();
	lua_sethook(L, nullptr, 0 ,0);
}

void UUnrealLuaDebug::SetLinebreakAt(const FString& fileName, int32 line)
{
}

void UUnrealLuaDebug::RemoveLinebreakAt(const FString& fileName, int32 line)
{
}

void UUnrealLuaDebug::NotifyLuaContextActiveChanged(UUnrealLuaEngineSubsystem* luaSystem, TScriptInterface<ILuaContext> luaContext, bool bIsLuaContextActive)
{
	if (this->RealTimeTracingEnabled && bIsLuaContextActive)
	{
		this->EnableTracingForLuaContext(luaContext);
	}
	else if (!bIsLuaContextActive)
	{
		this->DisableTracingForLuaContext(luaContext);
	}
}

void UUnrealLuaDebug::HookFunctionCallInLuaState(lua_State* L)
{
	sol::state_view lua{L};
	lua_sethook(L, UnrealLua::LuaHooks::LuaHook, 'c',0);
}

FUnrealLuaGameInstanceSessionInfo* UUnrealLuaDebug::GetSessionDataForGameInstance(const UGameInstance* gi)
{
	return this->ActiveGameSessionsInfo.FindByPredicate([gi](FUnrealLuaGameInstanceSessionInfo& sessionInfo)
	{
		return gi == sessionInfo.GameInstance;
	});
}

void UUnrealLuaDebug::NotifyGameSessionActiveChanged(UUnrealLuaEngineSubsystem* LuaSystem, bool bIsSessionActive)
{
	if (!bIsSessionActive)
	{
		this->SetWatchedActor(nullptr);
	}
	for (TArray<FUnrealLuaGameInstanceSessionInfo>::TIterator sessionInfoIt = this->ActiveGameSessionsInfo.CreateIterator(); sessionInfoIt; ++sessionInfoIt)
	{
		if (!sessionInfoIt->GameInstance.IsValid())
		{
			sessionInfoIt.RemoveCurrent();
		}
	}
}

void UUnrealLuaDebug::SetWatchedActor(AActor* newWatchedActor)
{
	if (IsValid(newWatchedActor))
	{
		this->WatchedActor.SetWatchedActor(newWatchedActor);
		this->OnWatchedActorChanged.Broadcast(true, this);
		this->OnWatchedActorChangedNative.Broadcast(true, this);
	}
	else
	{
		this->WatchedActor.SetWatchedActor(nullptr);
		this->OnWatchedActorChanged.Broadcast(false, this);
		this->OnWatchedActorChangedNative.Broadcast(false, this);
	}
}

FUnrealLuaDebugActorWatcher UUnrealLuaDebug::GetWatchedActor() const
{
	return this->WatchedActor;
}
