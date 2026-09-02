#pragma once
#include "CoreMinimal.h"
#include "sol/sol.hpp"
#include "UObject/ObjectPtr.h"

struct FLuaUObjectItem;
struct FLuaFunctionMapping;
struct FLuaScriptSettings;
class ULuaComponent;

struct UNREALLUA_API FLuaOverrideCallParams
{
	bool ValidDataForExecution();
	
	TObjectPtr<UFunction> Function;
	FFrame& Stack;
	void* ResultParam = nullptr;
	sol::function* FuncMapping = nullptr;
	sol::object CallingObjectReference = sol::nil;
};

namespace UnrealLua::LuaScriptCall
{
	extern bool bSuperCall;
	
	inline UNREALLUA_API void SetSuperCall(bool superCall)
	{
		bSuperCall = superCall;
	}
	
	inline UNREALLUA_API bool GetSuperCall()
	{
		return bSuperCall;
	}
	
	bool UNREALLUA_API CallUFunctionOverride(FLuaOverrideCallParams& params);
	bool UNREALLUA_API CallLuaImplementedUFunction(FLuaOverrideCallParams& params);
	bool UNREALLUA_API CallTickUFunctionOverride(FLuaOverrideCallParams& params);
	bool UNREALLUA_API CallWidgetTickUFunctionOverride(FLuaOverrideCallParams& params);

	int UNREALLUA_API SuperCall(lua_State* L);

	void CallMulticastDelegateBoundFunction(sol::table script, const std::string& funcName, UFunction* delegateFunction, void* parms);
	void CallMulticastDelegateBoundFunction(UObject* self, sol::function luaFunc, UFunction* func, void* parms);
	
	//@TODO fix ScriptCore.cpp UObject::SkipFunction crash when Function has no parameters
	
}