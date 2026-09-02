// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Reflection/FunctionDescr.h"
#include "sol/sol.hpp"
#include "UObject/Class.h"
#include "UObject/ScriptInterface.h"
#include "UnrealLuaCompiledUFunction.generated.h"

/**
 * 
 */

class ILuaContext;

struct UNREALLUA_API FLuaFunctionForWorld
{
	UWorld* World = nullptr;
	sol::function Func = {};
};

UCLASS()
class UNREALLUA_API UUnrealLuaCompiledUFunction : public UFunction
{
public:
	GENERATED_BODY()
	virtual void Bind() override;
	void Initialize();
	DECLARE_FUNCTION(execLuaCompiledUFunction);
	DECLARE_FUNCTION(execLuaDummyInterfaceFunction);
	sol::variadic_results PerformDirectLuaCall(sol::stack_object Self, const sol::variadic_args& Args);
	
	void SetLuaBytecode(const sol::bytecode& byteCode);
	void RemoveLuaContext(const TScriptInterface<ILuaContext>& ctx);
	void NotifyWorldTearDown(UWorld* World);
	
	virtual void SetSuperStruct(UStruct* NewSuperStruct) override;
	const FFunctionDescr* GetParentDescr() const;
private:
	sol::function GetFuncForWorld(UWorld* World);
	sol::function GetFuncForLuaState(lua_State* L);
	FLuaFunctionForWorld& GetFuncEntryForLuaState(lua_State* L);
	TArray<FLuaFunctionForWorld, TInlineAllocator<1>> LuaFunctionPairs = {};
	
	FFunctionDescr SuperFunctionDescr = {};
	
	sol::bytecode CompiledByteCode = {};
};
