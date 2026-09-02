#pragma once
#include "CoreMinimal.h"
#include "LuaScriptSettings.generated.h"

struct FLuaStruct;
class ILuaScriptable;
class ILuaContext;
class ULuaContext;
class ULuaComponent;

struct UNREALLUA_API FSubobjectMapping
{
	FName SubobjectName;
	FName PropertyName;
};

/*
 Determines how a Lua script is loaded
 */
UENUM()
enum class ELuaScriptLoadingBehavior
{
	/*
		Actors load before UserConstructionScript
		ActorComponents load before BeginPlay
		UserWidgets load before the first time they are added to a WidgetTree
		All other UObjects need to have LoadluaScript called manually
	*/
	AUTO,
	/*
		Lua Script will be automatically loaded, but just as an empty table 
	*/
	EMPTY,
	/*
		Lua Script will be loaded manually by game code 
	*/
	MANUAL
};

/*
UENUM()
enum class ELuaScriptPathType
{
	ModdablePath,
	NonModdablePath,
	AbsolutePath
};
*/
USTRUCT(BlueprintType)
struct UNREALLUA_API FLuaScriptSettings
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FString ScriptPathOverride;
	
//	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
//	ELuaScriptPathType PathType = ELuaScriptPathType::ModdablePath;
};
