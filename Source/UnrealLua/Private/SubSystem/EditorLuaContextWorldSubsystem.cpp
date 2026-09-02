// Fill out your copyright notice in the Description page of Project Settings.


#include "SubSystem/EditorLuaContextWorldSubsystem.h"

#include "LuaContext/LuaContextHelper.h"
#include "Utility/LuaLogMacros.h"
#include "Engine/World.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h" 
#include "UObjectRegistry/LuaUObjectRegistry.h"

bool UEditorLuaContextWorldSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return false;
	/*
	UWorld* world = CastChecked<UWorld>(Outer);

	return world->IsPreviewWorld();
	*/
}

void UEditorLuaContextWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	LUA_LOG("Editor Lua Game Context Initialize")
	Super::Initialize(Collection);
	this->LuaContext = MakeShared<FScopedLuaContext>(this, ELuaContextType::Editor);
	this->World = this->GetWorld();
	FOnActorDestroyed::FDelegate del;
	del.BindUObject(this, &UEditorLuaContextWorldSubsystem::NotifyActorDestroyed);
	(void)this->World->AddOnActorDestroyedHandler(del);
}

void UEditorLuaContextWorldSubsystem::PostInitialize()
{
	LUA_LOG("Editor Lua Game Context PostInitialize")
	Super::PostInitialize();

	this->GetWorld()->GetOnBeginPlayEvent().AddUObject(this, &UEditorLuaContextWorldSubsystem::NotifyEditorWorldBeginPlay);
	
	this->PersistentLevelName = this->GetWorld()->GetName();

	this->LuaScriptSettings.ScriptPathOverride = *FString::Printf(TEXT("/Level/%s"), *this->PersistentLevelName);
}

void UEditorLuaContextWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	this->BeginPlayInternal();
}

bool UEditorLuaContextWorldSubsystem::IsTickableInEditor() const
{
	return Super::IsTickableInEditor();
}

void UEditorLuaContextWorldSubsystem::NotifyEditorWorldBeginPlay(bool bHasBegunPlay)
{
	if(bHasBegunPlay && !this->bSubSystemHasBegunPlay)
	{
		this->BeginPlayInternal();
	}
	else if(!bHasBegunPlay && this->bSubSystemHasBegunPlay)
	{
		this->EndPlayInternal();
	}
}

void UEditorLuaContextWorldSubsystem::BeginPlayInternal()
{
	LUA_LOG("Editor Lua Game Context Beginplay")
	
	if(!this->bSubSystemHasBegunPlay)
	{
		//@TODO : Fix this, this happens after this Lua state already initialized its stuff
		//@TODO : think: what tthings are needed from config before gamemode is started?
		//@TODO : Maybe make game mode apply the lua config settings?
		checkNoEntry()
		UUnrealLuaEngineSubsystem::Get()->NotifyBeginGameSession(this);
		this->bSubSystemHasBegunPlay = true;
		this->LoadGameMode("dev");

		if(this->LuaContext.Get()->IsLuaLoaded())
		{
			UnrealLua::UObjectRegistry::LoadLuaScript(this, false);
			sol::state_view lua = this->LuaContext.Get()->GetLuaState(); 
			lua["World"] = this->GetWorld();
		}
	}
}

void UEditorLuaContextWorldSubsystem::EndPlayInternal()
{
	if(!this->bSubSystemHasBegunPlay)
	{
		return;
	}
	LUA_LOG("Editor Lua Game Context EndPlay")
	//FModuleManager::Get().GetModuleChecked<FUnrealLuaModule>("UnrealLua").NotifyGameSessionEnd();
	UUnrealLuaEngineSubsystem::Get()->NotifyEndGameSession(this);
	
	this->LuaContext.Reset();
	
	this->bSubSystemHasBegunPlay = false;
}

void UEditorLuaContextWorldSubsystem::NotifyActorDestroyed(AActor* actor)
{
	UnrealLua::UObjectRegistry::NotifyActorDestroyed(actor);
}


void UEditorLuaContextWorldSubsystem::Tick(float DeltaTime)
{
	if(this->bSubSystemHasBegunPlay && !this->GetWorld()->HasBegunPlay())
	{
		this->EndPlayInternal();
	}
}

void UEditorLuaContextWorldSubsystem::Deinitialize()
{
	LUA_LOG("Editor Lua Game Context Deinitialize")
	this->EndPlayInternal();
	Super::Deinitialize();
}

TStatId UEditorLuaContextWorldSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UEditorLuaContextWorldSubsystem, STATGROUP_Tickables);
}

void UEditorLuaContextWorldSubsystem::LoadGameMode(const FName& name)
{
	this->SetupLuaGameModeInternal(name);
}

FLoadedLuaGameModeSettings& UEditorLuaContextWorldSubsystem::GetLoadedLuaModeSettings()
{
	return this->GetScopedLuaContext().LoadedGameModeSettings;
}

void UEditorLuaContextWorldSubsystem::BroadcastLoadEvent(TScriptInterface<ILuaContext> ictx, FName gameModeName, ELuaLoadEventType loadState)
{
	this->OnLuaGameModeReloadEventNative.Broadcast(ictx, gameModeName, loadState);
}

FLuaScriptSettings UEditorLuaContextWorldSubsystem::GetLuaScriptSettings_Implementation()
{
	return this->LuaScriptSettings;
}

void UEditorLuaContextWorldSubsystem::SetLuaScriptSettings_Implementation(FLuaScriptSettings newSettings)
{
	this->LuaScriptSettings = newSettings;
}
