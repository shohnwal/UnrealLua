
#include "LuaContext/GameLuaContext.h"
#include "LuaContext/ScopedLuaContext.h"
#include "Utility/LuaLogMacros.h"
#include "Config/UnrealLuaConfig.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/CoreDelegates.h"
#include "Modules/ModuleManager.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"
#include "SubSystem/UnrealLuaGameWorldSubsystem.h"

UGameLuaContext::UGameLuaContext()
	: UGameInstanceSubsystem()
	, OnLuaGameModeReloadEventNative()
	, LuaContext(nullptr)
{
	//LuaFunctionOverride::RegisterOverrideFunction();
}

UGameLuaContext* UGameLuaContext::Get(UObject* worldContext)
{
	UWorld* world = worldContext->GetWorld();
	if(!world)
	{
		return nullptr;
	}
	UGameInstance* gi = world->GetGameInstance();
	if(!gi)
	{
		return nullptr;
	}
	return gi->GetSubsystem<UGameLuaContext>();
}

bool UGameLuaContext::ShouldCreateSubsystem(UObject* Outer) const
{
	return UUnrealLuaConfig::IsLuaEnabled();
}

sol::state_view UGameLuaContext::GetLuaState()
{
	return this->LuaContext->GetLuaState();
}

bool UGameLuaContext::IsReadyForFinishDestroy()
{
	if(this->HasAllFlags(EObjectFlags::RF_ClassDefaultObject))
	{
		return Super::IsReadyForFinishDestroy();
	}
	return Super::IsReadyForFinishDestroy() && (!this->LuaContext.IsValid() || this->GetScopedLuaContext().IsReadyForFinishDestroy());
}

void UGameLuaContext::LoadGameMode(const FName& gameMode)
{
	if (IsEngineExitRequested())
	{
		return;
	}
	if (!this->CanRunLua())
	{
		return;
	}
	LUA_LOG("UGameLuaContext: Loading Lua game mode %s in %s", *gameMode.ToString(), *GetFullNameSafe(this))
	verify(this->LuaContext.IsValid());
	
	if(!this->LuaContext->IsInitialized())
	{
		this->LuaContext->InitializeLuaState();
	}

	bool bWasLuaLoaded = this->LuaContext->IsLuaLoaded();
	
	this->SetupLuaGameModeInternal(gameMode);

	if(!bWasLuaLoaded)
	{
		//@TODO Load mods
	}
}

void UGameLuaContext::UnloadLuaGameMode()
{
	if(this->LuaContext->IsLuaLoaded())
	{
		//@TODO Unload mods

		this->SetupLuaGameModeInternal(NAME_None);
	}
}

void UGameLuaContext::Initialize(FSubsystemCollectionBase& Collection)
{
	//verify(UnrealLua::GLuaContext == nullptr);
	//UnrealLua::GLuaContext = this;
	Super::Initialize(Collection);
	
	LUA_LOG("Initializing GameLuaContext %s", *GetFullNameSafe(this))

	UUnrealLuaEngineSubsystem* ss = UUnrealLuaEngineSubsystem::Get();
	if (!GIsEditor)
	{
		LUA_LOG("Standalone GameLuaContext %s triggering UnrealLua compiler", *GetFullNameSafe(this))
		ss->InitCoreSystems();
		if (IsEngineExitRequested())
		{
			return;
		}
	}
	this->OnLuaGameModeReloadEventNative.Clear();
	
	//trigger FLuaUObjectRegistry loading 
	ss->NotifyBeginGameSession(this);
	
	this->LuaContext = MakeShared<FScopedLuaContext>(this, ELuaContextType::Game, FString::Printf(TEXT("%s"), *GetFullNameSafe(this)));
	
	ss->NotifyLuaContextInitialized(this);
	
	UGameInstance* gi = this->GetGameInstance(); 
	
	UWorld* world = gi->GetWorld();

	gi->OnNotifyPreClientTravel().AddUObject(this, &UGameLuaContext::NotifyPreClientTravel);
	
	UUnrealLuaGameWorldSubsystem* worldSubbsystem = world->GetSubsystem<UUnrealLuaGameWorldSubsystem>();
	if(worldSubbsystem)
	{
		worldSubbsystem->NotifyLuaContextInitialized(this);
	}
}

bool UGameLuaContext::CanRunLua() const
{
	return	UUnrealLuaEngineSubsystem::Get()->CanRunLua();
}

void UGameLuaContext::NotifyPreClientTravel(const FString& url, ETravelType tt, bool b)
{
	LUA_LOG("NotifyPreClientTravel : %s", *url)
}

void UGameLuaContext::Tick(float deltaTime)
{
	this->GetScopedLuaContext().Tick(deltaTime);
}

void UGameLuaContext::Deinitialize()
{
	LUA_LOG("Deinitializing UGameLuaContext %s", *GetFullNameSafe(this))
	
	checkSlow(IsInGameThread());

	UWorld* world = GetWorld();

	if(world)
	{
		UUnrealLuaGameWorldSubsystem* ctx = world->GetSubsystem<UUnrealLuaGameWorldSubsystem>();
		if(ctx)
		{
			ctx->NotifyPlaySessionEnded();
		}		
	}

	UUnrealLuaEngineSubsystem::Get()->NotifyEndGameSession(this);	

	this->LuaContext.Reset();
		
	this->OnLuaGameModeReloadEventNative.Clear();
	
	LUA_LOG("UGameLuaContext %s deinitialized", *GetFullNameSafe(this))
	
	Super::Deinitialize();
}

FScopedLuaContext& UGameLuaContext::GetScopedLuaContext()
{
	return *this->LuaContext.Get();	
}

TSharedPtr<FScopedLuaContext> UGameLuaContext::GetScopedLuaContextSharedPtr()
{
	return this->LuaContext;
}

/*
bool UGameLuaContext::HandleNetReceivedLuaGameModeString(UNetConnection* connection, uint8 MessageByte, const FString& MessageStr)
{
	checkNoEntry();
	FString luaGameMode;
	if(!FParse::Value(*MessageStr, TEXT("Lua="), luaGameMode))
	{
		LUA_LOG_WARNING("Could not find option 'Lua=' in welcome net message")
		//LUA_LOG_WARNING("Could not find option 'Lua=' in welcome net message")
		return false;
	}
	this->LoadGameMode(*luaGameMode);	
	if(!true)
	{
		LUA_LOG_WARNING("Could not load lua game type %s", *luaGameMode)
		return false;
	}
	return true;
}
*/

FLoadedLuaGameModeSettings& UGameLuaContext::GetLoadedLuaModeSettings()
{
	return this->GetScopedLuaContext().LoadedGameModeSettings;
}

void UGameLuaContext::BroadcastLoadEvent(TScriptInterface<ILuaContext> ictx, FName gameModeName, ELuaLoadEventType loadState)
{
	this->OnLuaGameModeReloadEventNative.Broadcast(this, gameModeName, loadState);
	this->OnLuaGameModeReloadEvent.Broadcast(this, gameModeName, loadState);
}

void UGameLuaContext::UnloadLua()
{
	this->BroadcastLoadEvent(this, NAME_None, ELuaLoadEventType::PREUNLOAD);
	this->LuaContext->Shutdown();
}

bool UGameLuaContext::IsLuaLoaded()
{
	return GetLoadedLuaModeSettings().bIsLuaLoaded;
}

TScriptInterface<ILuaContext> UGameLuaContext::GetBlueprintLuaContext(ELuaStateType luaStateType)
{
	if(luaStateType != ELuaStateType::DefaultState)
	{
		TScriptInterface<ILuaContext>* found = this->ChildLuaStates.Find(luaStateType);
		if(found)
		{
			return *found;	
		}
		return nullptr;
	}
	return this;
}

void UGameLuaContext::NotifyPreWorldChange(bool bSeamlessTravel)
{
	if (!this->LuaContext.IsValid())
	{
		return;
	}
	LUA_LOG("UGameLuaContext::NotifyPreWorldChange, seamless travel: %d", int32(bSeamlessTravel))
	this->OnLevelTravelUpdate.Broadcast(this, true, bSeamlessTravel);
	
	//If we don't seamless travel, perform a hard reset on lua state
	//bool bShutDownLua = !bSeamlessTravel;
	
	//@TODO : Check. Is this necessary? This would clear out any lua tables and such, even if we are just changing levels
	//This might be a leftover from when each world had a separate Lua state
	//UUnrealLuaEngineSubsystem::Get()->CleanUpObjectsForLuaContext(this, bShutDownLua);
}

void UGameLuaContext::NotifyWorldBeginPlay()
{
	this->OnWorldBeginPlayUpdate.Broadcast(true);
}

void UGameLuaContext::NotifyWorldEndPlay()
{
	this->OnWorldBeginPlayUpdate.Broadcast(false);
}
