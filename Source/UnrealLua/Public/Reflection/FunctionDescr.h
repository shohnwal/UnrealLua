// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PropertyHelperTypes.h"
#include "sol/sol.hpp"
#include "UObject/ObjectPtr.h"
struct FLuaValue;
/**
 * 
 */
struct FOutParmRec;
struct FFunctionDescr;

namespace UnrealLua::LuaScriptCall
{
	extern FFunctionDescr* CurrentFunc;
}

struct UNREALLUA_API FFunctionDescr
{
	static void RegisterUsertype(sol::state_view& lua);
	
	sol::variadic_results operator()(sol::stack_object self, sol::variadic_args args);
	//int operator()(lua_State* L);

	int PerformCall(UObject* obj, lua_State* L) const;
	sol::variadic_results PerformCall(UObject* obj, const sol::variadic_args& args) const;
	sol::variadic_results PerformCall(UObject* obj, const std::vector<sol::object>& args, sol::this_state lua) const;
	void PerformCall_NoReturnValues(UObject* obj, const TArray<FLuaValue>& args) const;

	bool IsValid() const
	{
		return this->Func != nullptr;
	}
private:
	
	void PrepareCallInternal(void* FuncMemory, const TArray<FLuaValue>& args, const FUFunctionCallInputLuaObjectRecord& inputRecord) const;
	void PrepareCallInternal(void* funcMemory, lua_State* L, FUFunctionCallInputLuaObjectRecord& inputRecord) const;
	void PrepareCallInternal(void* funcMemory, const sol::variadic_args& args, FUFunctionCallInputLuaObjectRecord& inputRecord) const;
	void PrepareCallInternal(void* funcMemory, const std::vector<sol::object>& args, FUFunctionCallInputLuaObjectRecord& inputRecord) const;

	void PerformCallInternal(UObject* context, UFunction* Function, void* FunctionArgsMemory) const;
	
	int EvaluateReturnValues(void* funcMemory, lua_State* lua, FUFunctionCallInputLuaObjectRecord& inputRecord) const;
	void EvaluateReturnValues(void* funcMemory, sol::variadic_results& results, sol::this_state lua, FUFunctionCallInputLuaObjectRecord& inputRecord) const;
public:
	FFunctionDescr(); 
	FFunctionDescr(FFunctionDescr&& other) noexcept;
	explicit FFunctionDescr(UFunction* function);

	FFunctionDescr(const FFunctionDescr& other) 
	{
		this->Func = other.Func;
		this->ReturnParm = other.ReturnParm;
		this->InputParms = other.InputParms;
		this->OutParms = other.OutParms;
		this->bIsUnrealLuaCompiledFunction = other.bIsUnrealLuaCompiledFunction;
	}
	bool operator== (const FFunctionDescr& right) const {
		return this->Func == right.Func;// && this->OwningObject == right.OwningObject;
	}
	void operator=(const FFunctionDescr &other) {
		this->Func = other.Func;
		this->ReturnParm = other.ReturnParm;
		this->InputParms = other.InputParms;
		this->OutParms = other.OutParms;
		this->bIsUnrealLuaCompiledFunction = other.bIsUnrealLuaCompiledFunction;
	}

	TArray<FProperty*> InputParms = {};
	TArray<FProperty*> OutParms = {};
	FProperty* ReturnParm = {};

	TObjectPtr<UFunction> Func = nullptr;
	bool bIsUnrealLuaCompiledFunction = false;
};
