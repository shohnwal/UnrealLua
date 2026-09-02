// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaContext/LuaContextBlueprintBase.h"

#include "Utility/LuaLogMacros.h"
#include "LuaContext/ScopedLuaContext.h"

void ULuaContextBlueprintBase::LoadGameMode(const FName& gameMode)
{
	LUA_LOG("Loading Lua game mode %s in LuaContext %s", *gameMode.ToString(), *this->GetName());
	this->SetupLuaGameModeInternal(gameMode);	
}

void ULuaContextBlueprintBase::BroadcastLoadEvent(TScriptInterface<ILuaContext> ictx, FName gameModeName, ELuaLoadEventType loadState)
{
	this->OnLuaGameModeReloadEventNative.Broadcast(this, gameModeName, loadState);
	this->OnLuaGameModeReloadEvent.Broadcast(this, gameModeName, loadState);
}

bool ULuaContextBlueprintBase::IsReadyForFinishDestroy()
{
	return !this->LuaContext.IsValid() || this->LuaContext->IsReadyForFinishDestroy();
}

FScopedLuaContext& ULuaContextBlueprintBase::GetScopedLuaContext()
{
	verify(this->LuaContext.IsValid());
	return *this->LuaContext.Get();		
}

FLoadedLuaGameModeSettings& ULuaContextBlueprintBase::GetLoadedLuaModeSettings()
{
	return this->GetScopedLuaContext().LoadedGameModeSettings;
}

void ULuaContextBlueprintBase::UnloadLua()
{
	this->BroadcastLoadEvent(this, NAME_None, ELuaLoadEventType::PREUNLOAD);
	this->LuaContext = nullptr;
}

bool ULuaContextBlueprintBase::IsLuaLoaded()
{
	return this->LuaContext && this->LuaContext->IsLuaLoaded();
}
