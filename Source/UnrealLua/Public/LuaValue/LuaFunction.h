// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "sol/sol.hpp"
#include "LuaFunction.generated.h"

struct FLuaValue;

struct UNREALLUA_API FLuaFunction
{
	FLuaFunction(sol::function func);
	void Invalidate();
	bool IsValid() const;
	bool CallFunction(const TArray<FLuaValue>& args, TArray<FLuaValue>& outResults) const;
	bool CallFunction(const TArray<FLuaValue>& Args);
	sol::protected_function Func = sol::nil;
};

/**
 * A struct holding a reference to a Lua function
 * When the owning Lua context expires, this handle will automatically get invalidated
 */
USTRUCT(BlueprintType)
struct UNREALLUA_API FLuaFunctionHandle
{
	GENERATED_BODY()
	FLuaFunctionHandle()
	{
	}
	
	static FLuaFunctionHandle MakeHandle(sol::function func);
	~FLuaFunctionHandle()
	{
		this->Invalidate();
	}

	bool operator==(const sol::function& func) const;

	explicit FLuaFunctionHandle(const TSharedPtr<FLuaFunction>& coSharedPtr);

	explicit FLuaFunctionHandle(FLuaFunctionHandle&& other) noexcept;

	FLuaFunctionHandle(const FLuaFunctionHandle& other)
		: FunctionWrapper(other.FunctionWrapper)
	{}

	FLuaFunctionHandle& operator=(const FLuaFunctionHandle& other)
	{
		this->FunctionWrapper = other.FunctionWrapper;
		return *this;
	}

	bool CallFunction(const TArray<FLuaValue>& args, TArray<FLuaValue>& outResults) const;
	bool CallFunction(const TArray<FLuaValue>& args) const;
	bool IsValid() const;

	
	sol::protected_function GetFunction() const;
	sol::object GetFunctionAsObject() const;
	void Invalidate();
	TSharedPtr<FLuaFunction> FunctionWrapper = nullptr;
};


struct UNREALLUA_API FWeakLuaFunctionHandle
{
	FWeakLuaFunctionHandle(TSharedPtr<FLuaFunction>& coWrapper);
	void Invalidate();
	
	TWeakPtr<FLuaFunction> LuaFunctionWrapper{};
};
