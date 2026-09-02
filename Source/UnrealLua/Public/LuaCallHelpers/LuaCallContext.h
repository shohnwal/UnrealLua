// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "UnrealLua.h"
#include "UObject/ObjectPtr.h"


/**
 * 
 */

enum class ELuaScriptCallContextType : uint8
{
	LuaScriptOverride,
	LuaCompiledFunction,
};

struct UNREALLUA_API FLuaCallContext : ILuaLightUserdata
{
	virtual int __index(sol::stack_object key) override;
	virtual void __newindex(sol::stack_object key, sol::stack_object value) override;
	
	UObject* GetUObject() const { return this->Context;};
	UFunction* GetUFunction() const { return this->Function;};
	ELuaScriptCallContextType GetContextType() const { return this->ContextType; };
	
	FLuaCallContext(UObject* context, UFunction* function, ELuaScriptCallContextType contextType) 
	: Context(context), Function(function), ContextType(contextType)
	{
		
	}
private:
	TObjectPtr<UObject> Context;
	TObjectPtr<UFunction> Function;
	ELuaScriptCallContextType ContextType;
};

inline void execLuaScriptOverride()
{
	UFunction* func = nullptr;
	UObject* context = nullptr;
	
	bool shouldCallLua = false;
	if(shouldCallLua)
	{	
		//FLuaCallContext callContext{context, func, ELuaScriptCallContextType::LuaScriptOverride};
		//UnrealLua::LuaScriptCall::CallLuaFunctionSafeByName(&callContext, "SomeFunc", 1,2,3);
		

	}
}

inline void execLuaCompiledFunction()
{
	UFunction* func = nullptr;
	UObject* context = nullptr;
	
	//FLuaCallContext callContext{context, func, ELuaScriptCallContextType::LuaCompiledFunction};
	//UnrealLua::LuaScriptCall::CallLuaFunctionSafeByName(callContext, "SomeFunc", 1,2,3);
}

inline void Lua_SuperCall()
{
	UObject* obj = nullptr;
	UFunction* funcToCall = nullptr;
	
	FLuaCallContext* ctx = nullptr;
	if (ctx)
	{
		UFunction* contextFunc = nullptr;
		obj = ctx->GetUObject();
		contextFunc = ctx->GetUFunction();
		if (ctx->GetContextType() == ELuaScriptCallContextType::LuaScriptOverride)
		{
			//We are calling super from an UFunction-override LuaScript function
			funcToCall = contextFunc;
		}
		else if (ctx->GetContextType() == ELuaScriptCallContextType::LuaCompiledFunction)
		{
			//We are calling super from an UnrealLua-compiled function
			funcToCall = contextFunc->GetSuperFunction();
		}
	}
	
}
