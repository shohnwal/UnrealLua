// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaContext/StandaloneLuaContext.h"

#include "LuaContext/ScopedLuaContext.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"

UStandaloneLuaContext::UStandaloneLuaContext()
	: LuaContext(nullptr), LuaContextName()
{
	if (this->IsTemplate())
	{
		return;
	}
	UUnrealLuaEngineSubsystem::Get()->NotifyBeginGameSession(this);
}

bool UStandaloneLuaContext::IsReadyForFinishDestroy()
{
	if(this->HasAllFlags(EObjectFlags::RF_ClassDefaultObject))
	{
		return Super::IsReadyForFinishDestroy();
	}
	return Super::IsReadyForFinishDestroy() && (!this->LuaContext.IsValid() || this->GetScopedLuaContext().IsReadyForFinishDestroy());
}

void UStandaloneLuaContext::BeginDestroy()
{
	if(!this->HasAnyFlags(RF_DefaultSubObject | RF_ArchetypeObject))
	{
		UUnrealLuaEngineSubsystem::Get()->NotifyEndGameSession(this);

		this->LuaContext.Reset();
		
		this->OnLuaGameModeReloadEventNative.Clear();
	}	
	UObject::BeginDestroy();
}

FScopedLuaContext& UStandaloneLuaContext::GetScopedLuaContext()
{
	return *this->LuaContext.Get();
}

void UStandaloneLuaContext::LoadGameMode(const FName& name)
{
	verifyf(this->LuaContext.IsValid(), TEXT("No Luacontext found in standalone Lua Context. Please call UStandaloneLuaContext::InitializeLuaState first!"));
	this->SetupLuaGameModeInternal(name);
}

FLoadedLuaGameModeSettings& UStandaloneLuaContext::GetLoadedLuaModeSettings()
{
	return this->LuaContext.Get()->LoadedGameModeSettings;
}

void UStandaloneLuaContext::BroadcastLoadEvent(TScriptInterface<ILuaContext> ictx, FName gameModeName, ELuaLoadEventType loadState)
{
	this->OnLuaGameModeReloadEventNative.Broadcast(this, gameModeName, loadState);
}

void UStandaloneLuaContext::InitializeLuaStateAndLoadGameMode(ELuaContextType luaContextType, const FString& name, const FName& gameMode)
{
	if(!this->LuaContext.IsValid())
	{
		this->LuaContext = MakeShared<FScopedLuaContext>(this, luaContextType, name);
		UUnrealLuaEngineSubsystem::Get()->NotifyLuaContextInitialized(this);
	}
	//this->InitializeLuaState(ELuaContextType::None, name);
	this->LoadGameMode(gameMode);
}
