#include "Config/UnrealLuaConfig.h"
#include "HAL/FileManager.h"
#include "LuaValue/LuaValue.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Reflection/PropertyHelper_ToString.h"

namespace UnrealLua::Config
{
	extern const TCHAR* defaultLuaConfig;
	

}




void UUnrealLuaConfig::WriteConfigToLuaFile()
{
	FUnrealLuaConfigData cachedSettings = this->GetCachedSettings();
	
	UUnrealLuaConfig* cfg = UUnrealLuaConfig::Get();
	
	FStringBuilderBase builder;
	
	builder << "local Config = {\n\n";
	
	for (TFieldIterator<FProperty> it (FUnrealLuaConfigData::StaticStruct()); it; ++it)
	{
		FProperty* prop = *it;
		FString annotation = UUnrealLuaConfig::GetConfigLuaAnnotationForPropertyName(prop->GetFName());
		if (!annotation.IsEmpty())
		{
			builder << annotation;	
		}
		
		builder << "  --Type: " << UnrealLua::PropertyHelper::GetPropertyTypeName(prop, true) << "\n";
		
		FProperty* cgfProp = cfg->GetClass()->FindPropertyByName(prop->GetFName());
		if (cgfProp)
		{
			FGetPropertyValueAsLuaSyntaxStringParams params{cgfProp, cfg, true, 0};
			FString defaultValue = UnrealLua::PropertyHelper::GetPropertyValueAsLuaSyntaxValidString_InContainer(params);
			builder << "  --Default: " << defaultValue << "\n";
		}
		
		FGetPropertyValueAsLuaSyntaxStringParams params{prop, &cachedSettings, true, 0};
		FString value = UnrealLua::PropertyHelper::GetPropertyValueAsLuaSyntaxValidString_InContainer(params);
			
		builder << "  " << prop->GetFName() << " = " << value << "," << "\n\n";
	}
	
	builder << "}\n\n";
	builder << "return Config";
	
	FString luaFile = FPaths::ProjectContentDir() + "Lua/LuaConfig.lua";
	
	FFileHelper::SaveStringToFile(builder.ToString(), *luaFile, FFileHelper::EEncodingOptions::ForceUTF8);
}

void UUnrealLuaConfig::WriteDefaultConfigToLuaFile()
{
	FString luaFile = FPaths::Combine(FPaths::ProjectContentDir(), "Lua/LuaConfig.lua");
	
	FFileHelper::SaveStringToFile(UnrealLua::Config::defaultLuaConfig, *luaFile, FFileHelper::EEncodingOptions::ForceUTF8);
}

FKey UUnrealLuaConfig::GetMainMenuKey()
{
	return UUnrealLuaConfig::GetCachedSettings().UnrealLuaToolsMenuKey;
}

namespace UnrealLua::Config
{
	const TCHAR* defaultLuaConfig = TEXT(R"###(local Config = {
  --Whether to load Lua at all for game runtime
  --NOTE: This does not affect UnrealLua-compiled classes being compiled
  --default: true
  --true|false: Activates or deactivates Lua scripts
  bLuaEnabled = true,

  --Whether the UnrealLua compiler should run at startup to create new Unreal types
  --WARNING: If you have assets that derive from or use UnrealLua compiled types,
  --setting this to false may invalidate them as Unreal would no longer be able to find
  --the UnrealLua-compiled types
  --default: true
  --true|false: Activates or deactivates UnrealLua compiler
  bCompilerEnabled = true,
  
  --Whether allow certain development features, such as creating a Metadata library of all Unreal types
  --true|false  (false) : 
  DevMode = false,
  
  --Mods directory name inside the content folder
  --Example : If "~Mods" is used, the used mods folder is MyGameProject/Content/~Mods/
  ModsDirectory = "../Mods",
  
  --UNIMPLEMENTED
  --Whether to override functions from a UInputComponent of actors
  OverrideInput = true,
  
  --UNIMPLEMENTED
  --The plugin automatically overrides all classes that inherit ILuaScriptable Interface
  --Here, you can specify additional classes that can be overridden, even if they are not ILuaScriptable
  --Note that without being ILuaScriptable, and thus not being able to provide the plugin will use  
  LuaEnabledClasses = {
    --["/UnrealLua/Compiled.UnrealLuaCompiledActor"] = {},

    --BUG! This breaks coroutines...
	--Maybe compiled types have some issues with their UFunction-overriding? 
    ["/Game/FirstPerson/Blueprints/Compiled/NewBlueprint1.NewBlueprint1"] = {},

    --Just the package name : Override all UFunctions of that class and all that inherit from it, use default script path
    ["/Game/FirstPerson/Blueprints/BP_FirstPersonProjectile.BP_FirstPersonProjectile"] = {},
    --Package name key and table{string} as value : 
    --ScriptPath string|nil : Custom script path
    --Overrude functions table{string}|nil : Override specific UFunctions only. If nil : Override all UFunctions
    ["/Script/UnrealLuaTest.UnrealLuaTestProjectile"] = { OverrideFunctions = {"ReceiveBeginPlay"}}

    --Example:
    --["/Script/UnrealLuaTest.MyItem"] = { ScriptPath = "/CutomScripts/Joe.lua", OverrideFunctions = {"ReceiveBeginPlay"}}
    --["/Game/FirstPerson/Blueprints/BP_FirstPersonProjectile.BP_FirstPersonProjectile"] = {},
  },
  
  --Determines how to look for default asset scripts. This can help if you do not want a deep folder structure for default scripts
  --Valid values : true|false|nil
  --Default : true
  --If true, take the full package path as a path of subfolders, with the asset name as the lua script file name
  --If false, this option replaces all slashes in the relative package path with dots, allowing a shallower folder structure
  --
  -- Setting    Type        Original Package Path             Lua file location
  --
  -- true       C++         /Script/MyGame.MyActor            Lua/AssetDefault/Script/MyGame/MyActor.lua
  --            Blueprint   /Game/Blueprints/Character/Enemy  Lua/AssetDefault/Game/Blueprints/Character/Enemy.lua
  --
  -- false      C++         /Script/MyGame.MyActor            Lua/AssetDefault/Script.MyGame.MyActor.lua
  --            Blueprint   /Game/Blueprints/Character/Enemy  Lua/AssetDefault/Game.Blueprints.Character.Enemy.lua
  PackagePathsAsFolders = true,
  
  --UNIMPLEMENTED
  --Determines lua game mode name to use in Blueprint editor simulation
  EditorGameMode = "Editor",
  
  --Sets whether tick UFunctions should be overridable by Lua
  --true|false|nil
  --Default: true
  OverrideTick = true,
  
  --Sets whether UnrealLua should perform a self-test on startup
  --Only usable in editor builds
  SelfTestOnStartup = true,
  
  --UNIMPLEMENTED
  UseLocalPlayerLuaSubSystem = false,
  
  --UNIMPLEMENTED
  --If true, clear the .Meta Lua metadata folder on startup
  ClearLuaMetaCache = true,
  
  --Disable Lua Subsystem during any of these listed maps
  --During any of these listed maps, mods can be activated or deactivated
  LuaDisabledMaps = {"LogoIntro", "MainMenu", "Credits"},
  
  --Disable Lua Subsystem during any of these listed game modes
  --During any of these listed game modes, mods can be activated or deactivated
  LuaDisabledGameModes = {"MainMenu"},
  
  --If false, server will kick clients that make an invalid server RPC request (Lua function not found, invalid UObject RPC targets)
  --true|false|nil
  --Default: true
  IgnoreInvalidServerRPC = true,
  
  --OverrideAllClasses = true
  --AdditionalLuaEnabledClasses = {"AMyActor", "UMyObject"},
  
  --HotReloadEnabled = true,
  
  --Determines when and how to scan and nil invalid UObject references in Lua
  --Incremental GC has better performance by spreading the Lua UObject reference processing over multiple frames, but it can
  --take a few frames to nil all invalid UObject references in Lua
  --If false, process the entire Lua state for invalid UObjects once after each Unreal garbage collection,
  --this clears invalid UObject references immediately, but might cause small hitches since the entire Lua state gets scanned
  --
  --Default : true
  --
  --false|nil : collect all invalid UObject references during UE PostGarbageCollection
  --true : Incrementally scan a few Lua GCObjects at the end of each frame
  IncrementalGC = true,
  
  --If IncrementalGC == true, set how many tables to scan per incremental Lua gc cycle
  --Automatic minimum is 10
  --default : 10
  IncrementalGCLimit = 10,
  
  --UNIMPLEMENTED
  --Whether to enable multithreaded UObject reference checking
  --default : true
  --true|false
  MultithreadGC = true,
  
  --Lua state settings
  --The collector starts a new cycle when the use of memory hits n% of the use after the previous collection.
  --If you experience noticeable pauses, try lowering setpause (e.g., to 150 or 110) 
  --to make the GC run more often, but with smaller memory steps. Don't use 100 or lower
  --Default nil : Lua default,  which is 200
  LuaGCStepPause = 120, --run more often
  
  --Lua state settings
  --If you need to quickly free up memory, you can increase setstepmul (e.g., to 300 or 400) to 
  --make the GC run more aggressively during its cycles
  --. Determines how many elements it marks or sweeps for each kilobyte of memory allocated. Larger values make
	--the collector more aggressive but also increase the size of each incremental step.
  --Default nil : Lua default,  which is 100
  LuaGCStepMultiplier = 1000, --let mem free steps free more memory
  
  --Lua state settings
  --The garbage-collector step size controls the size of each incremental step, specifically how many bytes the
	--interpreter allocates before performing a step. This parameter is logarithmic: A value of n means the interpreter
	--will allocate 2n bytes between steps and perform equivalent work during the step. A large value (e.g., 60) makes
	--the collector a stop-the-world (non-incremental) collector. The default value is 13, which means steps
	--of approximately 8 Kbytes. 
  LuaGCStepSize = 13
}

return Config
)###");
}
