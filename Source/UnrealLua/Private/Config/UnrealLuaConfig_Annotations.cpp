#include "CoreMinimal.h"
#include "Config/UnrealLuaConfig.h"

namespace UnrealLua::Config
{
	TMap<FName, const char*> AnnotationsMap = 
	{
		{
			GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bLuaEnabled),
			"Determines whether Unreal Lua script loading runs.\n*UnrealLua Compiler will still run.\n*Requires application restart."
		},
		{"bCompilerEnabled",	
			"  --Whether the UnrealLua compiler should run at startup to create new Unreal types\n"
			"  --WARNING: If you have assets that derive from or use UnrealLua compiled types,\n"
			"  --setting this to false may invalidate them as Unreal would no longer be able to find\n"
			"  --the UnrealLua-compiled types\n"
		},
		{
			GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bOverrideTick),
			"If enabled, Lua scripts can override ReceiveTick/Tick functions.\n"
			"If disabled, no tick function will be overridden by Lua scripts.\n"
			"\n*Only applies on next Lua session start."
		},
		{
			GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, UnrealLuaToolsMenuKey),
			"Key to press for activating UnrealLua Tools Menu."
		},
		{
			GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bEnableLuaReplication),
			"If enabled, UnrealLua will check applied Lua scripts for replication properties\n"
			"and set up Replication components in Lua script-owning actors.\n"
			"Default is on."
		},
		{
			GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bMultithreadedReplication),
			"When checked, Lua replication components use multithreading to read and write Lua values.\n"
			"\n*Experimental, might not work yet."
		},
		{
			GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bAllowClientToServerRPCs),
			"If enabled, clients are allowed to remotely call Lua functions on the server.\n"
			"Standard Unreal Engine RPC behavior applies ( Client / NetMulticast / Server UFunctions).\n"
			"If disabled, clients will not be able to send RPCs to the server and if the server catches\n"
			"a client attmepting it, it will result in a kick of the player from the server. Default is on."
		},
		{
			GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bIgnoreInvalidServerRPC),
			"If enabled, servers will ignore client requests of executing nonexistant Lua functions on the server.\n"
			"If disabled, the server not finding a requested Lua function for a RPC will result in a failed RPC validation.\n"
			"In most games, this will cause the client to be kicked from the server. Default is on."
		},
		{
			GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bSelfTestOnStartup),
			"If enabled, UnrealLua performs a self-test on application startup\n"
			"to ensure that basic Lua script functionality is working.\n"
			"\n*Requires application restart."
		},
		{
			GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bCompilerEnabled),
			"Whether the UnrealLua compiler should run at startup to compile Lua-based Unreal types\n"
			"WARNING: If you have assets that derive from or use UnrealLua compiled types,\n"
			"setting this to off may invalidate these assets as Unreal would no longer be able to find\n"
			"the UnrealLua-compiled base types. Default is on.\n"
			"\n*Requires application restart."
		},
		{
			GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, GCMode),
			"When checked, use incremental UObject reference collecting, At the end of each frame,\n"
			"UnrealLua incrementally scans a list of Lua GC items (tables, upvalues, etc), attempting to nil\n"
			"any invalid UObject references. Incremental reference collecting may take a few frames to collect\n"
			"and nil all invalid UObject references.\n"
			"\n"
			"When unchecked, UnrealLua will instead perform a full Lua state scan after each Unreal Engine garbage collection.\n"
			"This nils all invalid UObject references immediately, but at the higher cost of having to scan the full Lua state\n"
			"immediately. Incremental scan has a small cost at the end of each frame, full scan has a large cost every once \n"
			"in a while after each Unreal Engine garbage collection.\n"
		},
		{
			GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, LuaIncrementalGCLimit),
			"Has only an effect if incremental mode is selected.\n"
			"Determines how many GC objects (tables, upvalues, etc) are scanned at the end of each frame attempting to\n"
			"nil any invalid UObject references\n"
			"A lower a value means less items will be scanned each frame and it might take more frames for all\n"
			"invalid UObject references in Lua to be set to nil, but with less performance cost per frame.\n"
			"A higher value allows for more GC items to be examined at the end of each frame, and thus a faster\n"
			"removal of invalid UObject references, but at the cost of lower performance each frame.\n"
			"The default value is 50, minimum value is 10, maximum value is 1000."
		},
		{
			GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, LuaGCStepPause),
			"Controls how long the Lua VM collector waits before starting a new cycle.\n"
			"A value of 200 means that the collector waits for the total memory in use to double before starting a new cycle.\n"
			"The default value is 120; the minimum value is 100, the maximum value is 1000."
		},
		{
			GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, LuaGCStepMultiplier),
			"Controls how many how many Lua elements the Lua GC marks or sweeps for each kilobyte of memory allocated.\n"
			"Larger values make the collector scan more items but also increase the time needed to complete each incremental step.\n"
			"Be careful with values less than 100, it might cause the collector to check too few items and can result in the\n"
			"collector never finishing a cycle.\n"
			"The default value is 100; the minimum value is 50, the maximum value is 1000."
		},
		{
			GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, LuaGCStepSize),
			"The garbage-collector step size controls after how many bytes of allocating new memory the collector runs a step.\n"
			"This parameter is logarithmic: A value of n means the interpreter will allocate 2n bytes between steps and perform\n"
			"equivalent work during the step. A too large value means the collector steps through the entire Lua state.\n"
			"The default value is 13, which means steps of approximately 8 Kbytes." 
		},
		{
			GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bAllowWriteOnReadOnlyProperties),
			"Allow Lua scripts to write and modify UProperties marked as ReadOnly." 
		},
		{
			GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bAllowWriteOnReadOnlyProperties),
			"Allow Lua scripts to write and modify Class Default Objects (CDOs)." 
		},
		{
			GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bOverrideInput),
			"On Lua script instantiation, attempt to find any input action and bind Lua script functions to them.\n"
			"//@TODO : explain better"
		},
		{
			GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bUsePackagePathForNativeDefaultScripts),
			"Normally, default scripts paths for native classes are just using their class names,\n"
			"i.e. Lua/DefaultScript/AActor.lua\n"
			"Setting this to true also makes the came include the package path when searching for default scripts:\n"
			"AActor's package path is /Script/Engine.Actor, so with this set to true, the loading path would be\n"
			"Lua/DefaultScript/Engine/AActor.lua\n"
			"\n"
			"This allows the user to configure whether to have all native scripts in the top Lua/DefaultScript/\n"
			"directory, or whether they should be nested in subdirectories per native module.\n"
			"Default is false."
		},
		{
			GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bUsePackagePathForBlueprintDefaultScripts),
			"If set to true, Blueprint classes will use their full package path to locate the default\n"
			"Lua script file. A Blueprint class located in /Game/Path/To/BlueprintClass will look for the"
			"default script path\n"
			"\"Lua/DefaultScript/Game/Path/To/BlueprintClass.lua\"\n"
			"If set to false, all slashes in the package path will be replaced with periods when searching\n"
			"for the default script path, looking in the DefaultScript directory for the file\n"
			"\"/DefaultScript/Game.Path.To.BlueprintClass.lua\" instead.\n"
			"Default is true."
		},
		{
			GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, ModsDirectoryLocation),
			"Determines where UnrealLua looks for mod directories. This path is relative\n"
			"to the project \"Content\" directory. If you want to have the mods directory in the\n"
			"project root directory, use \"../<modsDir>\" instead, i.e. \"../~Mods\"\n" 
			"Default is \"~Mods\", making it the /Content/~Mods directory"
		},
		{
			GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bMultithreadGC),
			"Determines whether to use multithreaded UObject reference collection in Lua states\n"
			"Default is true (on)."
		}
	};
}

FString UUnrealLuaConfig::GetConfigPropertyDescriptionForPropertyName(FName propName)
{
	const char* entry = UnrealLua::Config::AnnotationsMap.FindRef(propName);
	if (!entry)
	{
		return "";
	}
	return FString{entry};
}

FString UUnrealLuaConfig::GetConfigLuaAnnotationForPropertyName(FName propName)
{
	const char* entry = UnrealLua::Config::AnnotationsMap.FindRef(propName);
	if (entry)
	{
		FString str = entry;
		TArray<FString> lines;
		str.ParseIntoArrayLines(lines);
			
		FStringBuilderBase builder;
		for (const auto& line : lines)
		{
			builder << "  --" << line << "\n";
		}
		return builder.ToString();
	}
	return "";
}
