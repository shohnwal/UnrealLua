#pragma once
#include "CoreMinimal.h"
#include "LuaDelegate.h"
//#include "LuaPrimitives.h"
//#include "LuaUClass.h"
//#include "LuaUStruct.h"
//#include "Misc/TVariant.h"

class UNREALLUA_API FLuaScriptMulticastDelegate
{
public:
	static void RegisterUsertype(sol::state_view& lua);
	/** Default constructor */
	FLuaScriptMulticastDelegate()
	{
	}
	
	FLuaScriptMulticastDelegate(sol::variadic_args args);

	static void Lua_Execute(sol::object self, sol::variadic_args args, sol::this_state lua);
	
	void Execute(const TArray<FLuaValue>& values ) const;
	
	void Clear();

	int64 Lua_Add(sol::stack_object target, sol::stack_object funcName, sol::variadic_args captureArgs, sol::this_state lua);
	//if funcname is nil, remove all bindings for target

	FLuaDelegateHandle AddDynamicListener(FLuaDelegate del);

	FLuaDelegateHandle AddLuaScriptListener(UObject* listener, const std::string funcName);
	
	void RemoveDynamicListener(const FLuaDelegate& del);
	void RemoveHandle(FLuaDelegateHandle Handle);
	void Lua_Remove(sol::stack_object target, sol::stack_object funcName, sol::this_state lua);

private:
	void Remove(int64 handle);
	void Remove(UObject* listener);
	void Remove(UObject* obj, const std::string_view& funcName);
	void Remove(UObject* obj, const FString& funcName);
	void Remove(const sol::table& table);
	void Remove(const sol::table& table, const std::string_view& funcName);
	void Remove(const sol::function& func);
	
	void ClearInvalidEntries();
	TArray<FLuaScriptDelegate> Callbacks = {};
	//TArray<TVariant<FLuaPrimitiveCPPType, FLuaUClass, FLuaUStruct>> Signature;
};
