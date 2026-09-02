#pragma once
#include "CoreMinimal.h"
#include "UObject/ScriptInterface.h"
#include "LuaContextHelper.generated.h"

class UUnrealLuaMod;
struct FLoadedLuaGameModeSettings;
struct FModInfo;
struct FScopedLuaContext;
struct FLuaContextBase;
class ILuaContext;

UENUM(BlueprintType)
enum class ELuaLoadEventType : uint8
{
	UNLOADED 	UMETA(DisplayName="Unloaded"),
	//Lua game mode is about to be unloaded/reloaded
	PREUNLOAD 	UMETA(DisplayName="PreUnload"),
	//Lua game mode was loaded, but still setting up some internal objects (Database, Combat, etc)
	PROCESSING	UMETA(DisplayName="Internal processing"),
	//Lua game mode was loaded / reloaded
	LOADED		UMETA(DisplayName="Loaded"),
	//Lua game mode loading failed
	FAILED		UMETA(DisplayName="Failed"),
};

struct UNREALLUA_API FLuaContextHelper
{
	static void SetupLuaGameMode(const TScriptInterface<ILuaContext>& ictx, const FName& name);
};

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnLuaGameModeLoadEventNative, TScriptInterface<ILuaContext>, const FName, ELuaLoadEventType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLuaGameModeLoadEventDelegate, TScriptInterface<ILuaContext>, luaContext, const FName, gameModeName,  ELuaLoadEventType, loadEvent);
