// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Config/UnrealLuaConstants.h"
#include "sol/sol.hpp"
#include "LuaFunctionMapping.generated.h"


struct FEnhancedInputActionEventBinding;
struct FFunctionDescr;

UENUM()
enum class ELuaRPCFunctionType : uint8
{
	None,
	Server,
	Client,
	Multicast
};

struct UNREALLUA_API FLuaFunctionWrapper
{
	bool IsValid() const
	{
		return this->LuaScriptFunction.valid();
	}
	
	void SetFunction(sol::function func)
	{
		this->LuaScriptFunction = func;
	}
	sol::function GetFunction() const
	{
		return this->LuaScriptFunction;
	}

	sol::function* GetFunctionPtr()
	{
		return &LuaScriptFunction;
	}

private:
	sol::function LuaScriptFunction = sol::nil;
};

USTRUCT()
struct UNREALLUA_API FLuaFunctionMapping
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere)
	FName FuncName = NAME_None;
	sol::function LuaScriptFunction = sol::nil;

	bool IsValidMapping() const
	{
		return FuncName != NAME_None && LuaScriptFunction.valid();
	}

	bool operator==(const FLuaFunctionMapping& other) const
	{
		return this->FuncName == other.FuncName;
	}

	bool operator<(const FLuaFunctionMapping& other) const
	{
		return this->FuncName.LexicalLess(other.FuncName);
	}
	
	static uint32 GetKeyHash(const FLuaFunctionMapping& This)
	{
		return GetTypeHash(This.FuncName);
	}
};

inline uint32 GetTypeHash(const FLuaFunctionMapping& This)
{
	return GetTypeHash(This.FuncName);
}


//A function map holding function mappings for a single object (tick func + other funcs)
//these functions are those 
USTRUCT()
struct UNREALLUA_API FLuaScriptObjectFunctionMap
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, meta=(ShowOnlyInnerProperties))
	TArray<FLuaFunctionMapping> FuncMapping;
	UPROPERTY(VisibleAnywhere, meta=(ShowOnlyInnerProperties))
	FLuaFunctionMapping TickFuncMapping;
	
	void OverrideFuncMapping(UObject* owningObject, FName funcName, const sol::function& func);

	void SortFunctionsByCallAmount()
	{
		/*
		this->FuncMapping.Sort([](const FLuaFunctionMapping& left, const FLuaFunctionMapping& right)
		{
			return left.CallCounter > right.CallCounter;

		});
		*/
	}
	FLuaFunctionMapping* GetFuncMapping(FName funcName)
	{
		if(funcName == UnrealLua::PropertyNames::NAME_ReceiveTick)
		{
			if(this->TickFuncMapping.IsValidMapping())
			{
				return &this->TickFuncMapping;
			}
			return nullptr;
		}
		return this->FuncMapping.FindByPredicate([funcName](const FLuaFunctionMapping& item)
		{
			return item.FuncName == funcName;
		});
	}

	bool HasAnyMappings() const
	{
		return TickFuncMapping.IsValidMapping() || !FuncMapping.IsEmpty();
	}
};