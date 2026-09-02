#pragma once
#include "CoreMinimal.h"
#include "ScriptableUObject/LuaScriptSettings.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LuaValue/LuaValue.h"
#include "sol/sol.hpp"
#include "LuaStruct.generated.h"

UCLASS(BlueprintType)
class UNREALLUA_API ULuaStructLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	static void ExecuteLuaFunction(FString luaFunction, TArray<FLuaValue>& args);
	static void ExecuteLuaFunctionNative(const char* const funcName, sol::variadic_args& args );
};

USTRUCT(BlueprintType)
struct UNREALLUA_API FLuaStruct// : public ILuaScriptable
{
	GENERATED_BODY()
	FLuaScriptSettings GetLuaScriptSettings();// override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FLuaScriptSettings LuaScriptSettings;
};
