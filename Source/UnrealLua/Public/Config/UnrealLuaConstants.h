#pragma once
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
enum class ELuaValueType : uint8;

struct FFrame;
typedef void (*FNativeFuncPtr)(UObject* Context, FFrame&, void*const);

//#include "UnrealLuaConstants.generated.h"
namespace UnrealLua
{
	namespace Flags
	{
		const EInternalObjectFlags AsyncObjectFlags = EInternalObjectFlags_AsyncLoading | /*EInternalObjectFlags::AsyncLoading |*/ EInternalObjectFlags::Async;
	}
	
	namespace LuaScriptKeys
	{
		inline const char* Super = "__super";
		inline const char* luaContext = "__LuaState";
		inline const char* NAME_ReceiveTick = "ReceiveTick";
		inline const char* NAME_Tick = "Tick";
		namespace ScriptAttributes
		{
			inline const char* ScriptAttributesKey = "__ScriptAttributes";
			inline const char* ReplicatedPropertiesKey = "__ReplicatedProperties";
			inline const char* PropertyReplicationFrequency = "PropertyReplicationFrequency";
			inline const char* ReplicationCondition = "ReplicationCondition";
			inline const char* OverrideInput = "OverrideInput";
			inline const char* StartWithTickEnabled = "StartWithTickEnabled";
			inline const char* SubObjectOverrides = "__SubobjectOverrides";
			inline const char* AutoRegisterForReplicationInOuter = "AutoRegisterForReplicationInOuter";
		}
	}

	namespace PropertyNames
	{
		const FName NAME_ReceiveBeginPlay = TEXT("ReceiveBeginPlay");
		const FName NAME_ReceiveEndPlay = TEXT("ReceiveEndPlay");
		const FName NAME_ReceiveTick(TEXT("ReceiveTick"));
		const FName NAME_Tick(TEXT("Tick"));
		const FName NAME_UserConstructionScript(TEXT("UserConstructionScript"));
		const FName NAME_UserWidgetConstruct(TEXT("Construct"));
		const FName NAME_UserWidgetDestruct(TEXT("Destruct"));
		const FName NAME_UserWidgetTick(TEXT("Tick"));
		const FName NAME_SetLuaScriptSettings{"SetLuaScriptSettings"};
		const FName NAME_GetLuaScriptSettings{"GetLuaScriptSettings"};
		const FName NAME_GetUniqueLuaNetHandle{"GetUniqueLuaNetHandle"};

		const FName NAME_UnrealLuaArrayInnerProperty{"UnrealLuaTArrayInnerProperty"};
		const FName NAME_UnrealLuaSetInnerProperty{"UnrealLuaTSetInnerProperty"};
		const FName NAME_UnrealLuaMapKeyProperty{"UnrealLuaTMapKeyProperty"};
		const FName NAME_UnrealLuaMapValueProperty{"UnrealLuaTMapValueProperty"};
		
		const FName NAME_UnrealLuaGetLuaScriptSettings{"GetLuaScriptSettings"};
	}

	namespace Paths
	{
		extern FString FullProjectDir; 
	}

	namespace scriptLoading
	{
		//.lua file has been loaded from disk, mixin-ed and monkeypatched with _mod files
		inline const char* ScriptLoadedFromDisk = "OnScriptLoaded";

		//The script has been duplicated and instanced and its __super owner and metatable (if any) has been set
		inline const char* ScriptInstanced = "OnScriptInstanced";
		inline const char* ScriptBeginPlay = "ScriptBeginPlay";
		inline const char* ScriptPreReload = "ScriptPreReload";
		inline const char* ScriptPostReload = "ScriptPostReload";
		inline const char* ScriptEndPlay = "ScriptEndPlay";

		inline const FString LuaGameModeFolder = "GameMode";
		inline const FString DefaultGameMode = "Default";
		inline const FString DefaultLuaScriptFolder = "DefaultScript";
		inline const FString DefaultLibsFolder = "Libs";
		inline const FString DefaultUnrealTypesFolder = "UnrealTypes";

		inline bool IsReservedLuaFolder(const FString& str)
		{
			return str.Equals(DefaultLuaScriptFolder) || str.Equals(DefaultLibsFolder) || str.Equals(DefaultUnrealTypesFolder);
		}
	}

	namespace StaticPackages
	{
		extern UScriptStruct* VectorStruct;
		extern UScriptStruct* Vector2DStruct;
		extern UScriptStruct* RotatorStruct;
		extern UScriptStruct* TransformStruct;
		extern UScriptStruct* InstancedStruct;
		extern UScriptStruct* SharedStruct;
		extern UScriptStruct* LuaValue;
		extern UScriptStruct* LuaTable;
		extern UScriptStruct* LuaFunction;
		extern UScriptStruct* LuaCoroutine;
		extern UScriptStruct* LuaDelegate;
	}
	
	namespace NativeFunctions
	{
		extern FNativeFuncPtr UObject_ProcessInternal;
	}
}