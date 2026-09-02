// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaContext/LuaStateComponent.h"

#include "LuaContext/GameLuaContext.h"
#include "LuaContext/ScopedLuaContext.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"


// Sets default values for this component's properties
ULuaStateComponent::ULuaStateComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	this->PrimaryComponentTick.bCanEverTick = false;
	this->bWantsInitializeComponent = true;
	this->LuaStateName = "LuaStateComponent";
	this->bSynchronizeGameModeWithGameInstance = true;
	// ...
}

void ULuaStateComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if(this->HasAnyFlags(RF_ClassDefaultObject) || !this->GetWorld())
	{
		return;
	}
	this->LuaContext = MakeShared<FScopedLuaContext>(this, ELuaContextType::None, this->LuaStateName);

	if(bSynchronizeGameModeWithGameInstance)
	{
		UGameLuaContext* ctx = UGameLuaContext::Get(this);
		if(ctx)
		{
			ctx->OnLuaGameModeReloadEventNative.AddUObject(this, &ULuaStateComponent::NotifyLoadEventFromGameInstanceLuaState);
		}	
	}
}


// Called when the game starts
void ULuaStateComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void ULuaStateComponent::NotifyLoadEventFromGameInstanceLuaState(TScriptInterface<ILuaContext> luaContext, FName gameModeName, ELuaLoadEventType luaLoadEvent)
{
	if(luaLoadEvent == ELuaLoadEventType::PROCESSING)
	{
		this->LoadGameMode(gameModeName);
	}
}

FScopedLuaContext& ULuaStateComponent::GetScopedLuaContext()
{
	return *this->LuaContext.Get();
}

void ULuaStateComponent::LoadGameMode(const FName& name)
{
	this->SetupLuaGameModeInternal(name);
}

FLoadedLuaGameModeSettings& ULuaStateComponent::GetLoadedLuaModeSettings()
{
	return this->GetScopedLuaContext().LoadedGameModeSettings;
}

void ULuaStateComponent::BroadcastLoadEvent(TScriptInterface<ILuaContext> ictx, FName gameModeName, ELuaLoadEventType loadState)
{
	this->OnLuaGameModeReloadEventNative.Broadcast(this, gameModeName, loadState);
	this->OnLuaGameModeReloadEvent.Broadcast(this, gameModeName, loadState);
}