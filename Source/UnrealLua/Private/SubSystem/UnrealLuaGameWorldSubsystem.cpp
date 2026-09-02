#include "SubSystem/UnrealLuaGameWorldSubsystem.h"

#include "EngineUtils.h"
#include "LuaContext/GameLuaContext.h"
#include "GameMapsSettings.h"
#include "Utility/LuaLogMacros.h"
#include "TimerManager.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/WorldSettings.h"

#include "LuaTypes/LuaSoftObjectWrapper.h"
#include "Reflection/FunctionDescr.h"
#include "Reflection/PropertyMapping.h"
#include "UObjectRegistry/LuaUObjectItem.h"

#include "UnrealLua.h"
#include "sol/sol.hpp"
#include "Config/UnrealLuaConfig.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"

FLuaTimerData::FLuaTimerData(): LuaTimerHandle(GenerateNewHandle())
{
}

FLuaTimerData FLuaTimerData::GenerateNewData()
{
	FLuaTimerData newData;
	newData.LuaTimerHandle = GenerateNewHandle();
	return newData;
}

int64 FLuaTimerData::GenerateNewHandle()
{
	static int64 newLuaTimerHandle = 0;
	newLuaTimerHandle++;
	return newLuaTimerHandle;	
}

bool UUnrealLuaGameWorldSubsystem::IsTickableInEditor() const
{
	return Super::IsTickableInEditor();
}

void UUnrealLuaGameWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LUA_LOG("UUnrealLuaGameWorldSubsystem::Initialize %s", *GetFullNameSafe(this))
	this->World = this->GetWorld();
	this->QueuedTimers = {};
	FOnActorDestroyed::FDelegate del;
	del.BindUObject(this, &UUnrealLuaGameWorldSubsystem::NotifyActorDestroyed);
	(void)this->World->AddOnActorDestroyedHandler(del);

	this->World->GetOnBeginPlayEvent().AddUObject(this, &UUnrealLuaGameWorldSubsystem::NotifyWorldBeginPlayUpdate);
	
	//cant load lua yet, URL may not have been set yet
}

FString UUnrealLuaGameWorldSubsystem::ParseGameMode()
{
	UGameLuaContext* ctx = UGameLuaContext::Get(this);

	this->SetLuaContext(ctx);
	
	FURL& InURL = this->GetWorld()->URL;
	
	//1. Check for hard override in game options...
	FString luaGameMode;

	//Server can manually set Lua= when starting a map
	//Clients will receive it when connecting
	FString Options;
	for (int32 i = 0; i < InURL.Op.Num(); i++)
	{
		Options += TEXT("?");
		Options += InURL.Op[i];
		FParse::Value(*InURL.Op[i], TEXT("Lua="), luaGameMode);
	}
	if(luaGameMode.IsEmpty())
	{
		//No Lua= option supplied
		//On server this can happen if no Lua= parameter override is used
		//check on server if user used a Game= override to supply a gamemode override
		FString gameModeOverride = InURL.GetOption(TEXT("Game="), TEXT(""));

		if (!gameModeOverride.IsEmpty())
		{
			//If there was a GameMode override via Game= option, load the gamemode class and grab its name 
			UClass* GameModeFromURL = StaticLoadClass(AGameModeBase::StaticClass(), nullptr, *gameModeOverride);
			if(GameModeFromURL)
			{
				luaGameMode = GameModeFromURL->GetName();
			}
		}
	}
	if(luaGameMode.IsEmpty())
	{
		//No lua supplied
		//This happens if neither Lua= nor Game= override was found
		//->if server, read default game mode
		AWorldSettings* ws = this->GetWorld()->GetWorldSettings();
		UClass* gmclass = ws->DefaultGameMode;
		if(gmclass == nullptr)
		{
			//no game mode override in worldsettings
			//use defualt game mode from project settings
			gmclass = FindObject<UClass>(nullptr, *UGameMapsSettings::GetGlobalDefaultGameMode());
		}
		if(gmclass != nullptr)
		{
			luaGameMode = gmclass->GetName();
		
			if(gmclass->HasAnyClassFlags(EClassFlags::CLASS_CompiledFromBlueprint))
			{
				luaGameMode.RemoveFromStart(TEXT("BP"), ESearchCase::IgnoreCase);
				luaGameMode.RemoveFromStart(TEXT("_"));
				luaGameMode.RemoveFromEnd(TEXT("_C"));
				luaGameMode.RemoveFromEnd(TEXT("BP"), ESearchCase::IgnoreCase);
				luaGameMode.RemoveFromEnd(TEXT("_"));
			}
			luaGameMode.RemoveFromEnd(TEXT("GameMode"), ESearchCase::IgnoreCase);
		}
		if(luaGameMode.IsEmpty())
		{
			LUA_LOG_WARNING("lua game mode is empty. During Lua= option setting 'BP_' and 'GameMode_BP_C' get stripped from the front/end. Make sure your game mode has a unique name! Will use default Game Mode name %s instead", *UnrealLua::scriptLoading::DefaultGameMode)
			luaGameMode = UnrealLua::scriptLoading::DefaultGameMode;
		}
		else if(UnrealLua::scriptLoading::IsReservedLuaFolder(luaGameMode))
		{
			LUA_LOG_WARNING("Lua game mode string %s is a reserved Lua directory name. During Lua= option setting 'BP_' and 'GameMode_BP_C' get stripped from the front/end. Make sure your game mode has a unique name that does not equal a reserved directory! Will use default Game Mode name %s instead", *luaGameMode, *UnrealLua::scriptLoading::DefaultGameMode)
			luaGameMode = UnrealLua::scriptLoading::DefaultGameMode;
		}
		FString newOp = TEXT("Lua=") + luaGameMode;
		InURL.AddOption(*newOp);
	}
	else if(UnrealLua::scriptLoading::IsReservedLuaFolder(luaGameMode))
	{
		LUA_LOG_WARNING("Lua game mode string %s is a reserved Lua directory name. During Lua= option setting 'BP_' and 'GameMode_BP_C' get stripped from the front/end. Make sure your game mode has a unique name that does not equal a reserved directory! Will use Game Mode name %s instead", *luaGameMode, *UnrealLua::scriptLoading::DefaultGameMode)
		luaGameMode = UnrealLua::scriptLoading::DefaultGameMode;
	}
	verify(!luaGameMode.IsEmpty());
	//Persistent level is already there

	return luaGameMode;
}

void UUnrealLuaGameWorldSubsystem::PostInitialize()
{
	Super::PostInitialize();
	LUA_LOG("UUnrealLuaGameWorldSubsystem::Postinitialize %s", *GetFullNameSafe(this))
	//URL is set by now on server

	//this subsystem should only be created when there is already a game instance running and set up
	
	this->UsedLuaGameModeName = this->ParseGameMode();
	
	this->PersistentLevelName = *this->GetWorld()->GetName();
	this->LuaScriptSettings.ScriptPathOverride = *FString::Printf(TEXT("/Level/%s"), *this->PersistentLevelName.ToString());
	
	this->TryLoadGameMode();
}

void UUnrealLuaGameWorldSubsystem::TryLoadGameMode()
{
	LUA_LOG("UUnrealLuaGameWorldSubsystem::TryLoadGameMode %s", *GetFullNameSafe(this))
	verify(this->LuaContext);
	if (!this->LuaContext->CanRunLua())
	{
		LUA_LOG("UUnrealLuaGameWorldSubsystem::TryLoadGameMode aborted, can not run Lua")
		return;
	}
	
	if(this->UsedLuaGameModeName.IsEmpty())
	{
		this->LuaContext->UnloadLuaGameMode();
		LUA_LOG("ULuaGameWorldSubsystem: UsedLuaGameModeName is empty, loaded scripts are reset for map %s", *this->GetWorld()->GetMapName())
		return;
	}

	UWorld* world = this->GetWorld();

	if(UUnrealLuaConfig::IsGameModeDisabledForLua(UsedLuaGameModeName))
	{
		this->LuaContext->UnloadLuaGameMode();
		LUA_LOG("ULuaGameWorldSubsystem for map %s will not load : Game Mode %s disabled for Lua via LuaConfig)", *world->GetMapName(), *UsedLuaGameModeName)
		return;
	}

	UGameMapsSettings const* MapSettings = GetDefault<UGameMapsSettings>();
	if (world->GetMapName().Equals(MapSettings->GetGameDefaultMap()))
	{
		this->LuaContext->UnloadLuaGameMode();
		LUA_LOG("ULuaGameWorldSubsystem for map %s will not load : Map is default map)", *world->GetMapName())
		return;
	}
	
	if(UUnrealLuaConfig::IsMapDisabledForLua(world->GetMapName()))
	{
		this->LuaContext->UnloadLuaGameMode();
		LUA_LOG("ULuaGameWorldSubsystem for map %s will not load : map disabled for Lua via LuaConfig)", *world->GetMapName())
		return;
	}
	
	LUA_LOG("ULuaGameWorldSubsystem %s now loading game mode %s", *this->World->GetMapName(), *this->UsedLuaGameModeName)
	this->LuaContext->LoadGameMode(*this->UsedLuaGameModeName);

	if(this->LuaContext->IsLuaLoaded())
	{
		sol::state_view lua = this->LuaContext->GetLuaState(); 
		lua["World"] = this;

		FTimerDelegate del;
		del.BindUObject(this, &UUnrealLuaGameWorldSubsystem::TickLuaContexts);
		this->GetWorld()->GetTimerManager().SetTimer(this->tickContextsHandle, del, 0.1f, true);
	}
}

TStatId UUnrealLuaGameWorldSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(ULuaWorldSubsystem, STATGROUP_Tickables);
}

void UUnrealLuaGameWorldSubsystem::NotifyWorldBeginPlayUpdate(bool worldHasBegunPlay)
{
	LUA_LOG("UUnrealLuaGameWorldSubsystem::NotifyWorldBeginPlayUpdate %s %d", *GetFullNameSafe(this), (int)worldHasBegunPlay)
	if(worldHasBegunPlay)
	{
		//Already handled via virtual function below
		return;
	}
	if(UGameInstance* gi = this->GetWorld()->GetGameInstance())
	{
		if(UGameLuaContext* ctx = gi->GetSubsystem<UGameLuaContext>())
		{
			ctx->NotifyWorldEndPlay();
		}
	}
}

void UUnrealLuaGameWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	LUA_LOG("UUnrealLuaGameWorldSubsystem::OnWorldBeginPlay %s", *GetFullNameSafe(this))
	verify(UUnrealLuaEngineSubsystem::IsGameSessionActive())
	
	Super::OnWorldBeginPlay(InWorld);

	if(!this->LuaContext->IsLuaLoaded())
	{
		LUA_LOG_WARNING("ULuaGameWorldSubsystem::OnWorldBeginPlay :  Unable to begin play in level %s : LuaContext %s not loaded", *this->GetWorld()->GetMapName(), *this->UsedLuaGameModeName)
		return;
	}
	
	verify(this->LuaContext);

	this->bHasBegunPlay = true;
	
	UnrealLua::UObjectRegistry::LoadLuaScript(this, false);

	if(UGameInstance* gi = this->GetWorld()->GetGameInstance())
	{
		if(UGameLuaContext* ctx = gi->GetSubsystem<UGameLuaContext>())
		{
			ctx->NotifyWorldBeginPlay();
		}
	}

	this->ReceiveBeginPlay();

	//Actors already placed in the map won't have their ConstructScript called
	//->Need to manually load Lua scripts for those
	UnrealLua::UObjectRegistry::LoadLuaScriptsForNetLoadActors(this->GetWorld(), false);
	
	this->OnLuaReady.Broadcast();
	this->OnLuaReady.Clear();
	this->OnGameSessionBegin.Broadcast();
}

bool UUnrealLuaGameWorldSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool UUnrealLuaGameWorldSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* world = CastChecked<UWorld>(Outer);
	if(!UUnrealLuaEngineSubsystem::Get()->CanRunLua())
	{
		//LUA_LOG("ULuaGameWorldSubsystem %s will not load : Lua disabled via LuaConfig!", *world->GetMapName())
		return false;
	}
	
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		//LUA_LOG("ULuaGameWorldSubsystem %s will not load : Super returned false (probably not a game world)", *world->GetMapName())
		return false;
	}

	EWorldType::Type worldType = world->WorldType;
	if(worldType != EWorldType::Game && worldType != EWorldType::PIE)
	{
		return false;
	}

	FWorldContext* worldContext = GEngine->GetWorldContextFromWorld(world);
	if(!worldContext)
	{
		return false;
	}
	if(worldContext->SeamlessTravelHandler.IsInTransition() && worldContext->SeamlessTravelHandler.HasSwitchedToDefaultMap())
	{
		LUA_LOG("ULuaGameWorldSubsystem %s will not load : Is default map during seamless travel", *world->GetMapName())
		return false;		
	}

	UGameInstance* gi = world->GetGameInstance();
	if(!gi)
	{
		LUA_LOG("ULuaGameWorldSubsystem %s will not load : No game instance running", *world->GetMapName())
		return false;
	}
	
	UGameLuaContext* ctx = gi->GetSubsystem<UGameLuaContext>();
	if(!ctx)
	{
		LUA_LOG("ULuaGameWorldSubsystem %s will not load : No GameLuaContext running in GameInstance!", *world->GetMapName())
		return false;
	}
	
	if (world->GetMapName() == TEXT("Untitled"))
	{
		LUA_LOG("ULuaGameWorldSubsystem %s will not load : Untitled game world, probably just an empty transition map)", *world->GetMapName())
		return false;
	}
	
	if(UUnrealLuaConfig::IsMapDisabledForLua(world->GetMapName()))
	{
		LUA_LOG("ULuaGameWorldSubsystem %s : Disabled Lua Map via LuaConfig. Unloading Lua)", *world->GetMapName())
		ctx->UnloadLuaGameMode();
		return false;	
	}
	
	return true;
}

ETickableTickType UUnrealLuaGameWorldSubsystem::GetTickableTickType() const
{
	return Super::GetTickableTickType();
}

FLuaScriptSettings UUnrealLuaGameWorldSubsystem::GetLuaScriptSettings_Implementation()
{
	return const_cast<UUnrealLuaGameWorldSubsystem*>(this)->LuaScriptSettings;
}

void UUnrealLuaGameWorldSubsystem::SetLuaScriptSettings_Implementation(FLuaScriptSettings newSettings)
{
	this->LuaScriptSettings = newSettings;
}

void UUnrealLuaGameWorldSubsystem::Tick(float DeltaTime)
{
	this->ReceiveTick(DeltaTime);
	
	this->OnTick.Broadcast(DeltaTime);
}

void UUnrealLuaGameWorldSubsystem::TickLuaContexts()
{
	this->LuaContext->Tick(0.1f);
}

void UUnrealLuaGameWorldSubsystem::NotifyActorDestroyed(AActor* actor)
{
	UnrealLua::UObjectRegistry::NotifyActorDestroyed(actor);
}

void UUnrealLuaGameWorldSubsystem::Deinitialize()
{
	LUA_LOG("UUnrealLuaGameWorldSubsystem::Deinitialize %s", *GetFullNameSafe(this))
	if(this->World->IsPlayInEditor())
	{
		//FModuleManager::Get().GetModuleChecked<FUnrealLuaModule>("UnrealLua").NotfiyPIEEnd();
	}
	
	this->ClearTimers();

	bool bSeamlessTravel = this->World->IsInSeamlessTravel();
	
	if(bSeamlessTravel)
	{
		auto& travelInfo = GEngine->SeamlessTravelHandlerForWorld(this->World);
		bool isDefaultMap = travelInfo.HasSwitchedToDefaultMap();
	}
	
	
	this->LuaContext->NotifyPreWorldChange(bSeamlessTravel);

	Super::Deinitialize();
}

void UUnrealLuaGameWorldSubsystem::NotifyPlaySessionEnded()
{
	LUA_LOG("UUnrealLuaGameWorldSubsystem::NotifyPlaySessionEnded %s", *GetFullNameSafe(this))
	this->OnGameSessionEnd.Broadcast();
	if(this->LuaContext && this->LuaContext->IsLuaLoaded())
	{
		sol::state_view lua = this->LuaContext->GetLuaState(); 
		lua["World"] = sol::nil;
	}
	this->ClearTimers();
}

void UUnrealLuaGameWorldSubsystem::NotifyLuaContextInitialized(UGameLuaContext* gameLuaContext)
{
	//this gets called if UGameLuaContext gets created AFTER this one gets created
	//We already know the URL from postinitialize, so assign context and immediately load lua
	this->SetLuaContext(gameLuaContext);
	this->TryLoadGameMode();	
}

void UUnrealLuaGameWorldSubsystem::RegisterManualTick(UObject* obj, bool bSetTickEnabled)
{
	if (!obj)
	{
		return;
	}

	
	verify(!obj->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) && obj->GetClass()->HasAnyClassFlags(CLASS_Native))
	
	if (AActor* actor = Cast<AActor>(obj))
	{
		if (bSetTickEnabled)
		{
			if (this->OnTick.IsBoundToObject(actor))
			{
				return;
			}
			this->OnTick.AddUObject(actor, &AActor::ReceiveTick);
		}
		else
		{
			this->OnTick.RemoveAll(actor);
		}
	}
	else if (UActorComponent* cmp = Cast<UActorComponent>(obj))
	{
		if (bSetTickEnabled)
		{
			if (this->OnTick.IsBoundToObject(cmp))
			{
				return;
			}
			this->OnTick.AddUObject(cmp, &UActorComponent::ReceiveTick);
		}
		else
		{
			this->OnTick.RemoveAll(cmp);
		}
	}
}

void UUnrealLuaGameWorldSubsystem::ClearTimers()
{
	for(auto& timer : this->QueuedTimers)
	{
		this->GetWorld()->GetTimerManager().ClearTimer(timer.TimerManagerHandle);
	}
	this->QueuedTimers.Empty();
	
	if(this->tickContextsHandle.IsValid())
	{
		this->GetWorld()->GetTimerManager().ClearTimer(this->tickContextsHandle);
	}	
}

void UUnrealLuaGameWorldSubsystem::SetLuaContext(UGameLuaContext* ctx)
{
	if(!this->LuaContext && IsValid(ctx))
	{
		this->LuaContext = ctx;
	}
}

int64 UUnrealLuaGameWorldSubsystem::SetTimer(UObject* target, FLuaValue callbackv, float interval, bool loop, float initialDelay, TArray<FLuaValue> additionalArgs)
{
	if (!IsValid(target))
	{
		return 0;
	}
	
	sol::state_view lua = this->LuaContext->GetLuaState();
	if (!lua)
	{
		return 0;
	}
	
	sol::object callback = callbackv.GetValue(lua.lua_state());
	sol::type callbackType = callback.get_type();
	if(callbackType != sol::type::string && callbackType != sol::type::function)
	{
		return 0;
	}

	FLuaTimerData& timerData = this->QueuedTimers.Emplace_GetRef();

	std::vector<sol::object> addargs;
	for (FLuaValue& val : additionalArgs)
	{
		addargs.emplace_back(val.GetValue(lua.lua_state()));
	}

	timerData.loop = loop;
	timerData.Target = UnrealLua::LightUserdata::GetUObjectAsTaggedLightUserdata(target, lua);
	timerData.Callback = callback;
	timerData.Args = addargs;

	verify(timerData.LuaTimerHandle != 0);
	
	FTimerDelegate del{};
	del.BindUObject(this, &UUnrealLuaGameWorldSubsystem::NotifyTimerTriggered, timerData.LuaTimerHandle);
	this->World->GetTimerManager().SetTimer(timerData.TimerManagerHandle, del, interval, loop, initialDelay);
	
	return timerData.LuaTimerHandle;
}
/*
int64 UUnrealLuaGameWorldSubsystem::SetTimerWithStringCallback(UObject* target, FString callback, float interval, bool loop, float initialDelay, TArray<FLuaValue> additionalArgs)
{
	if (callback.IsEmpty())
	{
		return 0;
	}
	sol::state_view lua = this->LuaContext->GetLuaState();
	if (!lua)
	{
		return 0;
	}
	sol::object callback_o = sol::make_object(lua, callback);
	return this->SetTimer(target, callback_o, interval, loop, initialDelay, additionalArgs);
}
*/

int64 UUnrealLuaGameWorldSubsystem::Delay(UObject* target, FLuaValue callback, float delay, TArray<FLuaValue> additionalArgs)
{
	return this->SetTimer(target, callback, delay, false, 0.0f, additionalArgs);
}
/*
int64 UUnrealLuaGameWorldSubsystem::DelayWithStringCallback(UObject* target, FString callback, float delay, TArray<FLuaValue> additionalArgs)
{
	return this->SetTimerWithStringCallback(target, callback, delay, false, 0, additionalArgs);
}
*/
sol::object UUnrealLuaGameWorldSubsystem::Delay(sol::variadic_args args, sol::this_state lua)
{
	return this->SetTimer(args, lua);
}

sol::object UUnrealLuaGameWorldSubsystem::SetTimer(sol::variadic_args args, sol::this_state lua)
{
	UWorld* world = this->GetWorld();
	if(!world)
	{
		return sol::nil;
	}

	int argssize = args.size();
	
	// 1. World:SetTimer(obj, "MyFunc", 3.0)
	if(argssize < 3)
	{
		//must have at least a target, a callback and an interval
		return sol::nil;
	}

	if(!UnrealLua::LightUserdata::IsUObject(args[0].get<sol::stack_object>()) && !args[0].is<sol::table>())
	{
		//not a valid self arg
		return sol::nil;
	}

	sol::object callback = args[1];
	sol::type callbackType = args[1].get_type();
	if(callbackType != sol::type::string && callbackType != sol::type::function)
	{
		return sol::nil;
	}
	float interval_o = args[2];
	bool shouldLoop_o = argssize > 3 ? args[3] : false;
	float initialDelay_o = argssize > 4 ? args[4] : interval_o;
	std::vector<sol::object> additionalArgs;
	if (argssize > 5)
	{
		additionalArgs.reserve(argssize - 5);
		for(int i = 5; i < argssize; i++)
		{
			additionalArgs.emplace_back(args[i]);
		}		
	}
	
	// 1. World:SetTimer()
	return this->SetTimer(args[0], callback, interval_o, shouldLoop_o, initialDelay_o, additionalArgs, lua);
}

sol::object UUnrealLuaGameWorldSubsystem::SetTimer(sol::object target_o, sol::object callback, float interval, bool loop, float initialDelay, std::vector<sol::object>& callbackArgs, sol::this_state lua)
{
	sol::type callbackType = callback.get_type();
	if(callbackType != sol::type::string && callbackType != sol::type::function)
	{
		return sol::nil;
	}

	FLuaTimerData& timerData = this->QueuedTimers.Emplace_GetRef();

	timerData.loop = loop;
	timerData.Target = target_o;
	timerData.Callback = callback;
	timerData.Args = callbackArgs;

	verify(timerData.LuaTimerHandle != 0);
	
	FTimerDelegate del{};
	del.BindUObject(this, &UUnrealLuaGameWorldSubsystem::NotifyTimerTriggered, timerData.LuaTimerHandle);
	this->World->GetTimerManager().SetTimer(timerData.TimerManagerHandle, del, interval, loop, initialDelay);
	
	return sol::make_object<int64>(lua, timerData.LuaTimerHandle);
}

void UUnrealLuaGameWorldSubsystem::ClearTimersForObject(const UObject* objectToClear)
{
	UWorld* world = this->GetWorld();
	if(!world)
	{
		return;
	}
	for(TArray<FLuaTimerData>::TIterator timerIt = this->QueuedTimers.CreateIterator(); timerIt; ++timerIt)
	{
		if(!timerIt->Target.valid())
		{
			this->World->GetTimerManager().ClearTimer(timerIt->TimerManagerHandle);
			timerIt.RemoveCurrent();		
		}
		if(UObject* target =  UnrealLua::GetUObject(timerIt->Target))
		{
			if(target == objectToClear)
			{
				this->World->GetTimerManager().ClearTimer(timerIt->TimerManagerHandle);
				timerIt.RemoveCurrent();
			}
		}
	}
}

void UUnrealLuaGameWorldSubsystem::NotifyTimerTriggered(int64 luaTimerHandle)
{
	int32 index = this->QueuedTimers.IndexOfByPredicate([luaTimerHandle](const FLuaTimerData& item)
	{
		return item.LuaTimerHandle == luaTimerHandle;
	});
	if(index == INDEX_NONE)
	{
		return;
	}
	FLuaTimerData timerData = this->QueuedTimers[index];

	if(!timerData.Target.valid())
	{
		this->World->GetTimerManager().ClearTimer(timerData.TimerManagerHandle);
		this->QueuedTimers.RemoveAt(index);
		return;
	}

	if(!timerData.loop)
	{
		this->World->GetTimerManager().ClearTimer(timerData.TimerManagerHandle);
		this->QueuedTimers.RemoveAt(index);
	}
	
	bool bHasUObjectTarget = false;
	UObject* target = nullptr;
	if(UObject* ref = UnrealLua::LightUserdata::GetUObject(timerData.Target))
	{
		bHasUObjectTarget = true;
		target = ref;
	}
	else if(timerData.Target.is<FLuaSoftObjectWrapper>())
	{
		bHasUObjectTarget = true;
		target = timerData.Target.as<FLuaSoftObjectWrapper>().Get();
	}

	if(!IsValid(target) || target->HasAnyFlags(RF_MirroredGarbage | RF_BeginDestroyed))
	{
		this->QueuedTimers.RemoveAt(index);
		return;		
	}

	//@TODO : Dont call if has UObject target but is no longer valid
	//Also, remove it from the timer queue

	//@TODO : param may also be a function

	sol::type callbackType = timerData.Callback.get_type();
	if(callbackType != sol::type::string && callbackType != sol::type::function)
	{
		this->QueuedTimers.RemoveAt(index);
		return;
	}

	if(callbackType == sol::type::string)
	{
		if(bHasUObjectTarget)
		{
			if(target == nullptr)
			{
				//object got destroyed
				this->QueuedTimers.RemoveAt(index);
				return;		
			}
			
			//
			// if(timerData.Callback.is<FLuaRPCFunction>())
			// {
			// 	sol::string_view strv = timerData.Callback.as<sol::string_view>();
			// 	FLuaRPCFunction& rpc = timerData.Callback.as<FLuaRPCFunction>();
			// 	rpc.RPCCallOnObject(target, strv.data(), timerData.Args);
			//
			// 	return;
			// }
			//

			FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(target);
			const FHashedFieldMapping* mapping = item.GetPropertyMapping(timerData.Callback);
			
			if(mapping && mapping->IsFunction())
			{
				mapping->GetFunction()->PerformCall(target, timerData.Args, timerData.Callback.lua_state());
			}
			else if(target->Implements<ULuaScriptable>()) //@TODO : What about actors that are not ULuaScriptable but are designated Lua-compatible via LuaConfig?
			{
				UnrealLua::LuaScriptCall::CallLuaFunctionSafeByKey(target, timerData.Callback, target, sol::as_args(timerData.Args));
			}
		}
		else if(timerData.Target.is<sol::table>())
		{
			sol::table tbl = timerData.Target;
			UnrealLua::LuaScriptCall::CallLuaFunctionSafeByKey(tbl, timerData.Callback, sol::as_args(timerData.Args));		
		}	
	}
	else if(callbackType == sol::type::function)
	{
		sol::function func = timerData.Callback.as<sol::function>();

		timerData.Args.emplace(timerData.Args.begin(), timerData.Target);
		
		UnrealLua::LuaScriptCall::CallLuaFunctionSafe(func, sol::as_args(timerData.Args));
	}
}

bool UUnrealLuaGameWorldSubsystem::IsPaused() const
{
	return this->GetWorld()->IsPaused();
}

AActor* UUnrealLuaGameWorldSubsystem::SpawnActor(UClass* InClass, FVector const Location, FRotator const Rotation)
{
	FActorSpawnParameters params{};
	return this->GetWorld()->SpawnActor(InClass, &Location, &Rotation, params);
}

AActor* UUnrealLuaGameWorldSubsystem::SpawnActorAbsolute(UClass* Class, FTransform const& AbsoluteTransform)
{
	return this->GetWorld()->SpawnActorAbsolute(Class, AbsoluteTransform);
}

AActor* UUnrealLuaGameWorldSubsystem::SpawnActorDeferred(UClass* Class, FTransform const& Transform, AActor* Owner, APawn* Instigator, ESpawnActorCollisionHandlingMethod CollisionHandlingOverride, ESpawnActorScaleMethod TransformScaleMethod)
{
	return this->GetWorld()->SpawnActorDeferred<AActor>(Class, Transform, Owner, Instigator, CollisionHandlingOverride, TransformScaleMethod);
}

AGameModeBase* UUnrealLuaGameWorldSubsystem::GetAuthGameMode() const
{
	return this->GetWorld()->GetAuthGameMode();
}

AGameStateBase* UUnrealLuaGameWorldSubsystem::GetGameState() const
{
	return this->GetWorld()->GetGameState();
}

ENetMode UUnrealLuaGameWorldSubsystem::GetNetMode() const
{
	return this->GetWorld()->GetNetMode();
}

bool UUnrealLuaGameWorldSubsystem::DestroyActor(AActor* Actor, bool bNetForce, bool bShouldModifyLevel)
{
	return this->GetWorld()->DestroyActor(Actor, bNetForce, bShouldModifyLevel);
}
