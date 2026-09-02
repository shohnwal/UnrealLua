#include "BlueprintSupport/UnrealLuaUtility.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "Interface/LuaContext.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"

static FAutoConsoleCommand unrealLuaCreateMetadataConsoleCommand(
	TEXT("lua.CreateMetaData"),
	TEXT("Create Unreal Engine Types Metadata for programming Lua scripts.")
	TEXT("Usage: \"lua.CreateMetaData\""),
	FConsoleCommandDelegate::CreateLambda([]()
	{

		UUnrealLuaEngineSubsystem* ess = GEngine->GetEngineSubsystem<UUnrealLuaEngineSubsystem>();
		if(!ess)
		{
			return;
		}
		ess->CreateUnrealLuaMetadata();
	})
);


static FAutoConsoleCommandWithWorldAndArgs unrealLuaMemoryCheckConsoleCommand(
	TEXT("lua.Memory"),
	TEXT("Get Information about Lua memory usage.")
	TEXT("Usage: \"lua.Memory\""),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		UUnrealLuaEngineSubsystem* ss = GEngine->GetEngineSubsystem<UUnrealLuaEngineSubsystem>();
		for(TScriptInterface<ILuaContext>& ctx : ss->ActiveLuaContexts)
		{
			uint64 mem = ctx->GetScopedLuaContext().GetLuaState().memory_used();
			LUA_LOG("LuaContext %s uses %llu bytes", *GetNameSafe(ctx->_getUObject()), mem)
		}
	})
);


static FAutoConsoleCommandWithWorldAndArgs unrealLuaScriptReloadConsoleCommand(
	TEXT("lua.Reload"),
	TEXT("Reaload a loaded Lua script.")
	TEXT("Usage: \"lua.Reload <ScriptPath>\""),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World)
		{
			UE_LOG(LogConsoleResponse, Display, TEXT("Error: invalid world"));
			return;
		}
		if (Args.Num() != 1)
		{
			UE_LOG(LogConsoleResponse, Display, TEXT("Error: Expecting 1 parameter"));
			return;
		}

		FString scriptName = Args[0];
		int32 ID = INDEX_NONE;

		TScriptInterface<ILuaContext> lctx = UUnrealLuaUtility::GetLuaContext(World);

		if(lctx)
		{
			if(scriptName.ToLower().Equals("all"))
			{
				lctx->GetScopedLuaContext().ReloadAllScripts();
			}
			else
			{
				lctx->GetScopedLuaContext().ReloadScript(*scriptName);	
			}
		}
	})
);
