// Fill out your copyright notice in the Description page of Project Settings.


#include "Interface/LuaContext.h"

#include "LuaContext/LuaContextHelper.h"
#include "LuaContext/ScopedLuaContext.h"
#include "Utility/LuaLogMacros.h"

UWorld* ILuaContext::GetWorldFromUObject() const
{
	return this->_getUObject()->GetWorld();
}

void ILuaContext::SetupLuaGameModeInternal(const FName& gameMode, const ELuaPathFlags luaPathFlags)
{
	FScopedLuaContext& ctx = this->GetScopedLuaContext();
	verify(ctx.IsInitialized());
	
	FLoadedLuaGameModeSettings& settings = this->GetLoadedLuaModeSettings();

	TScriptInterface<ILuaContext> ictx{this->_getUObject()};
	
	if(settings.CurrentGameMode != NAME_None)
	{
		this->BroadcastLoadEvent(ictx, gameMode, ELuaLoadEventType::PREUNLOAD);

		LUA_LOG("Unloading Lua game mode %s", *settings.CurrentGameMode.ToString());

		ctx.UnloadGameMode();

		this->BroadcastLoadEvent(ictx, gameMode, ELuaLoadEventType::UNLOADED);
	}

	if(gameMode == NAME_None)
	{
		LUA_LOG("No loading of Lua, gameMode is empty")
		return;
	}
	LUA_LOG("Loading Lua game mode %s", *gameMode.ToString())

	//Reset game mode settings
	settings = {};

	ctx.SetupLuaStateForGameMode(gameMode, luaPathFlags);

	verify(ctx.IsLuaLoaded());
	
	//For internal systems, such as Databases
	this->BroadcastLoadEvent(ictx, gameMode, ELuaLoadEventType::PROCESSING);

	this->BroadcastLoadEvent(ictx, gameMode, ELuaLoadEventType::LOADED);
}
