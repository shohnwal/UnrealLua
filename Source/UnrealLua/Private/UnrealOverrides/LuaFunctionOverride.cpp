// Fill out your copyright notice in the Description page of Project Settings.

#if 0 //Legacy code
#include "UnrealLua/Public/FunctionOverride/LuaFunctionOverride.h"
#include "LuaLogMacros.h"
#include "Interface/LuaScriptable.h"
#include "LuaCallHelpers/LuaScriptOverrideCalls.h"
#include "Reflection/FunctionDescr.h"
#include "Replication/LuaScriptReplicationComponent.h"
#include "ScriptManagement/LuaUObjectItem.h"
#include "ScriptManagement/LuaUObjectRegistry.h"
//#include "Runtime/CoreUObject/Private/UObject/ScriptCore.cpp"

extern uint8 GRegisterNative(int32 NativeBytecodeIndex, const FNativeFuncPtr& Func);
#define Ex_LuaHook (EX_Max - 1)

class LuaScriptableObject;

TArray<FOverrideLuaCallStackElement> LuaFunctionOverride::OverrideCallStack = {};

bool LuaFunctionOverride::bDoSuperCall = false;

void LuaFunctionOverride::RegisterOverrideFunction()
{
	checkNoEntry();
	static bool bRegisteredLuaOverride = false;
	if(!bRegisteredLuaOverride)
	{
		LUA_OVERRIDE_LOG("Registering Ex_LuaHook function bytecode in Blueprint VM")
		GRegisterNative(Ex_LuaHook, &LuaFunctionOverride::CallLuaOverriddenFunction);
		bRegisteredLuaOverride = true;
	}
}

void LuaFunctionOverride::OverrideUFunction(UFunction * func)
{
	checkNoEntry();
	LUA_OVERRIDE_LOG("Overriding function %s", *func->GetName())
	int32 scriptsize = func->Script.Num(); 
	
	if (func->Script.Num() >= 8)
	{
		if(func->Script[5] == Ex_LuaHook)
		{
			//is an overridden function
			return;	
		}
	}
	// if script isn't empty
	/*
	if(func->Script.IsEmpty()) 
	{
		constexpr TArray<uint8, TInlineAllocator<4>> code{ Ex_LuaHook,EX_Return, EX_Nothing, EX_Nothing };
		func->Script.Insert(code, 0);
	}
	else
	*/
	{
		// EX_JumpIfNot : read next 4 bytes as integer to determine byte code array index to jump to
		// if instruction after that (Ex_LuaHook) returns true.
		// in this case, if Ex_LuaHook returns true, skip past the Ex_Nothing (Ex_Nothing here is at array index 7)
		// to the regular BP script
		// goto 8(a uint32 value) to skip return
		//
		// In short:
		// if Ex_LuaHook() == false
		//		goto script index 8, aka skip the return
		// else
		//		return (aka end called Ufunction)
		TArray<uint8, TInlineAllocator<8>> code{ EX_JumpIfNot,8,0,0,0,Ex_LuaHook,EX_Return,EX_Nothing };
		func->Script.Insert(code, 0);
	}
}

sol::variadic_results LuaFunctionOverride::__Super(sol::variadic_args args)
{
	verify(LuaFunctionOverride::OverrideCallStack.Num() > 0);
	LuaFunctionOverride::bDoSuperCall = true;
	FOverrideLuaCallStackElement& ele = LuaFunctionOverride::OverrideCallStack.Last();
	FFunctionDescr* fdescr = ele.FuncDescr;
	sol::variadic_results results = fdescr->PerformCall(ele.CallingObject, args);
	LuaFunctionOverride::bDoSuperCall = false;
	return results;
}

bool LuaFunctionOverride::IsOverridable(const UFunction* Function)
{
	constexpr EFunctionFlags disallowedFuncFlags = FUNC_Net | FUNC_Exec | FUNC_EditorOnly | FUNC_NetValidate | FUNC_Final | FUNC_UbergraphFunction;// | FUNC_Native;  
	if(Function->HasAnyFunctionFlags(disallowedFuncFlags))
	{
		return false;
	}

	static constexpr uint32 FlagMask = FUNC_Native | FUNC_Event | FUNC_Net;
	static constexpr uint32 FlagResult = FUNC_Native | FUNC_Event;
	return Function->HasAnyFunctionFlags(FUNC_BlueprintEvent) || (Function->FunctionFlags & FlagMask) == FlagResult;
}

void LuaFunctionOverride::OverrideClass(UClass* uclass)
{
	checkNoEntry();
	for (TFieldIterator<UFunction> it(uclass, EFieldIteratorFlags::ExcludeSuper); it; ++it)
	{
		UFunction* func = *it;
		if (IsOverridable(func))
		{
			OverrideUFunction(func);
		}
	}
}

void LuaFunctionOverride::RemoveOverrides(UClass* uclass)
{
	for (TFieldIterator<UFunction> it(uclass, EFieldIteratorFlags::ExcludeSuper); it; ++it)
	{
		UFunction* func = *it;
		if (IsOverridable(func))
		{
			RemoveUFunctionOverride(func);
		}
	}	
}

void LuaFunctionOverride::RemoveUFunctionOverride(UFunction* func)
{
	if (func->Script.Num() >= 8)
	{
		if(func->Script[5] == Ex_LuaHook)
		{
			//is an overridden function
			func->Script.RemoveAt(0, 8);	
		}
	}
	else
	{
		check(!func->Script.Contains(Ex_LuaHook));
	}
}

// if RESULT_PARAM is true, Blueprint VM won't execute code behind this hook
// otherwise if RESULT_PARAM is false, BP VM execute code continues
//*static_cast<bool*>(RESULT_PARAM) = false;
DEFINE_FUNCTION(LuaFunctionOverride::CallLuaOverriddenFunction)
{
	checkNoEntry();
	/*
	if(UnrealLua::LuaScriptCall::GetSuperCall() == false)
	{
		FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(Context);
		FLuaOverrideCallParams params{Context, CastChecked<ULuaFunction>(Stack.Node), Stack, RESULT_PARAM};
		if(item.ProcessEvent(params))
		{
			//Lua called sucessfully
			//If result == true, cancel UFunction script
			*static_cast<bool*>(RESULT_PARAM) = true;
		}		
	}
	*/
	UnrealLua::LuaScriptCall::SetSuperCall(false);
	//If result == false,let UFunction script continue
	*static_cast<bool*>(RESULT_PARAM) = false;
}
#endif
