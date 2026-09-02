#include "LuaContext/LuaContextHelper.h"

#include "LuaContext/LoadedLuaGameModeSettings.h"
#include "Utility/LuaLogMacros.h"
#include "Interface/LuaContext.h"
#include "LuaContext/ScopedLuaContext.h"

class UUnrealLuaMod;

void FLuaContextHelper::SetupLuaGameMode(const TScriptInterface<ILuaContext>& ictx, const FName& gameMode)
{
	FScopedLuaContext& ctx = ictx->GetScopedLuaContext();
	verify(ctx.IsInitialized());
	
	FLoadedLuaGameModeSettings& settings = ictx->GetLoadedLuaModeSettings();

	if(settings.CurrentGameMode != NAME_None)
	{
		ictx->BroadcastLoadEvent(ictx, gameMode, ELuaLoadEventType::PREUNLOAD);

		LUA_LOG("Unloading Lua game mode %s", *settings.CurrentGameMode.ToString());

		ctx.UnloadGameMode();

		ictx->BroadcastLoadEvent(ictx, gameMode, ELuaLoadEventType::UNLOADED);
	}

	if(gameMode == NAME_None)
	{
		LUA_LOG("No loading of Lua, gameMode is empty")
		return;
	}
	LUA_LOG("Loading Lua game mode %s", *gameMode.ToString())

	//Reset game mode settings
	settings = {};

	ctx.SetupLuaStateForGameMode(gameMode);

	verify(ctx.IsLuaLoaded());
	
	//For internal systems, such as Databases
	ictx->BroadcastLoadEvent(ictx, gameMode, ELuaLoadEventType::PROCESSING);

	ictx->BroadcastLoadEvent(ictx, gameMode, ELuaLoadEventType::LOADED);	
}
