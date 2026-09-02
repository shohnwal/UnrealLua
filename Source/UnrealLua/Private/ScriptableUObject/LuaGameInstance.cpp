#include "ScriptableUObject/LuaGameInstance.h"

#include "LuaContext/GameLuaContext.h"
#include "Engine/Engine.h"
#include "Engine/NetConnection.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"
#include "SubSystem/UnrealLuaGameWorldSubsystem.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"
//#include "Misc/ConfigCacheIni.h"

void ULuaGameInstance::Init()
{
	Super::Init();
	this->LuaContext = this->GetSubsystem<UGameLuaContext>();
	this->LuaContext->OnLuaGameModeReloadEventNative.AddUObject(this, &ULuaGameInstance::NotifyLuaLoadEvent);
	if(this->LuaContext->IsLuaLoaded())
	{
		UnrealLua::UObjectRegistry::LoadLuaScript(this, false);
	}
}

void ULuaGameInstance::NotifyLuaLoadEvent(TScriptInterface<ILuaContext> luaContext, FName name, ELuaLoadEventType loadEvent)
{
	if(loadEvent == ELuaLoadEventType::LOADED)
	{
		this->LoadLuaScriptInternal();
		this->PostLuaLoadFinish();
	}
}

void ULuaGameInstance::LoadLuaScriptInternal()
{
	if(this->HasAnyFlags(EObjectFlags::RF_ClassDefaultObject | EObjectFlags::RF_ArchetypeObject))
	{
		return;
	}
	UnrealLua::UObjectRegistry::LoadLuaScript(this, false);	
}


void ULuaGameInstance::ModifyClientTravelLevelURL(FString& LevelName)
{
	Super::ModifyClientTravelLevelURL(LevelName);
	LevelName += "?Lua=";
	LevelName += *this->GetSubsystem<UGameLuaContext>()->GetLoadedLuaModeSettings().CurrentGameMode.ToString();
}

FLuaScriptSettings ULuaGameInstance::GetLuaScriptSettings_Implementation()
{
	return this->LuaScriptSettings;
}

void ULuaGameInstance::SetLuaScriptSettings_Implementation(FLuaScriptSettings newSettings)
{
	this->LuaScriptSettings = newSettings;
}
