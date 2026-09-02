// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#if 0 //Legacy code
#include "CoreMinimal.h"
#include "Runtime/CoreUObject/Public/UObject/ObjectMacros.h"

#include "sol/sol.hpp"

struct FFunctionDescr;
/**
 * 
 */

struct UNREALLUA_API FOverrideLuaCallStackElement
{
	FOverrideLuaCallStackElement()
		: FuncDescr(nullptr), CallingObject(nullptr), Script(sol::nil)
	{}
	FOverrideLuaCallStackElement(FFunctionDescr* funcDescr, UObject* obj, sol::table script)
		: FuncDescr(funcDescr), CallingObject(obj), Script(script)
	{}
	FFunctionDescr* FuncDescr;
	UObject* CallingObject;
	sol::table Script;
};
class UNREALLUA_API LuaFunctionOverride
{
public:
	static void RegisterOverrideFunction();
	static void OverrideUFunction(UFunction* func);
	DECLARE_FUNCTION(CallLuaOverriddenFunction);
	static bool bDoSuperCall;
	static TArray<FOverrideLuaCallStackElement> OverrideCallStack;

	sol::variadic_results __Super(sol::variadic_args args);
	static bool IsOverridable(const UFunction* Function);
	static void OverrideClass(UClass* class_);
	static void RemoveOverrides(UClass* class_);
	static void RemoveUFunctionOverride(UFunction* func);
};
#endif