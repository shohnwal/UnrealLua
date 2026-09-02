#pragma once
#include "CoreMinimal.h"
#include "sol/sol.hpp"
#include "LuaStackHandler/LuaStackHandler.h"
#include "Utility/LuaLogMacros.h"

namespace UnrealLua
{
	UNREALLUA_API bool IsUObject(const sol::stack_object& obj);
	UNREALLUA_API bool IsUObject(const sol::object& obj);
	UNREALLUA_API UObject* GetUObject(const sol::stack_object& obj);
	UNREALLUA_API UObject* GetUObject(const sol::object& obj);

	UNREALLUA_API bool IsEnum(const sol::stack_object& obj);
	UNREALLUA_API bool IsEnum(const sol::object& obj);

	template<UObjectTypename U, typename LUAOBJ>
	bool IsUObject(const LUAOBJ& obj) requires std::is_same_v<LUAOBJ, sol::object> || std::is_same_v<LUAOBJ, sol::stack_object> 
	{
		return Cast<U>(GetUObject(obj)) != nullptr;
	}
	
	UNREALLUA_API bool IsGameSessionActive();
}
namespace UnrealLua::LuaScriptCall
{
	UNREALLUA_API FLuaUObjectItem* GetHandleForObject(UObject* obj);
	UNREALLUA_API sol::function GetLuaFunction(const FLuaUObjectItem* item, const char* funcName);
	UNREALLUA_API sol::function GetLuaFunction(const FLuaUObjectItem* item, const TCHAR* funcName);
	UNREALLUA_API sol::function GetLuaFunction(const FLuaUObjectItem* item, const sol::object& key);
	
	template<typename... Args>
	inline sol::protected_function_result CallLuaFunctionSafe(sol::protected_function func, Args&&... args)
	{
		static_assert((!std::is_base_of_v<std::vector<sol::object>, std::remove_reference_t<std::remove_pointer_t<Args>>> && ...), "No direct std::vector<sol::object> allowed in CallLuaFunctionSafe, please wrap it with sol::as_args()!");

		if(!func.valid())
		{
			return {};
		}
		//std::vector<sol::object> args_v = UnrealLua::ConvertValues::MakeArgs(func.lua_state(), args...);
		sol::protected_function_result result = func(std::forward<Args>(args)...);
		if(!result.valid())
		{
			const std::string errStr = result.get<sol::error>().what();
			UE_LOG(LogTemp, Error, TEXT("Error during function call : \n%hs"), errStr.c_str());
			return {};
		}
		return result;
	}
	template<typename... Args>
	inline sol::protected_function_result CallLuaFunctionSafeByName(sol::table table, const char* const funcname, Args&&... args)
	{
		if(!table.valid())
		{
			return {};
		}
		sol::function func = table.raw_get_or<sol::function>(funcname, sol::nil);
		if(!func.valid() || func.get_type() != sol::type::function)
		{
			return {};
		}
		return UnrealLua::LuaScriptCall::CallLuaFunctionSafe(func.as<sol::function>(), std::forward<Args>(args)...);
	}
	template<typename... Args>
	inline sol::protected_function_result CallLuaFunctionSafeByKey(sol::table table, sol::object& key, Args&&... args)
	{
		if(!table.valid())
		{
			return {};
		}
		sol::function func = table.raw_get_or<sol::function>(key, sol::nil);
		if(!func.valid() || func.get_type() != sol::type::function)
		{
			return {};
		}
		return UnrealLua::LuaScriptCall::CallLuaFunctionSafe(func, std::forward<Args>(args)...);
	}
	template<typename... Args>
	inline sol::protected_function_result CallLuaFunctionSafeByKey(const FLuaUObjectItem* handle, sol::object& key, Args&&... args)
	{
		return UnrealLua::LuaScriptCall::CallLuaFunctionSafe(GetLuaFunction(handle, key), std::forward<Args>(args)...);
	}

	template<typename... Args>
	inline sol::protected_function_result CallLuaFunctionSafeByKey(UObject* obj, sol::object& key, Args&&... args)
	{
		return UnrealLua::LuaScriptCall::CallLuaFunctionSafeByKey(GetHandleForObject(obj), key, std::forward<Args>(args)...);
	}
	
	template<typename... Args>
	inline sol::protected_function_result CallLuaFunctionSafeByName(const FLuaUObjectItem* handle, const FName& funcname, Args&&... args)
	{
		return UnrealLua::LuaScriptCall::CallLuaFunctionSafe(GetLuaFunction(handle, *funcname.ToString()), std::forward<Args>(args)...);
	}
	template<typename... Args>
	inline sol::protected_function_result CallLuaFunctionSafeByName(const FLuaUObjectItem* handle, const FString& funcname, Args&&... args)
	{
		return UnrealLua::LuaScriptCall::CallLuaFunctionSafe(GetLuaFunction(handle, *funcname), std::forward<Args>(args)...);
	}
	template<typename... Args>
	inline sol::protected_function_result CallLuaFunctionSafeByName(const FLuaUObjectItem* handle, const char* const funcname, Args&&... args)
	{
		return UnrealLua::LuaScriptCall::CallLuaFunctionSafe(GetLuaFunction(handle, funcname), std::forward<Args>(args)...);
	}
	template<typename... Args>
	inline sol::protected_function_result CallLuaFunctionSafeByName(UObject* obj, const char* const funcname, Args&&... args)
	{
		return UnrealLua::LuaScriptCall::CallLuaFunctionSafeByName(GetHandleForObject(obj), funcname, std::forward<Args>(args)...);
	}
	template<typename... Args>
	inline sol::protected_function_result CallLuaFunctionSafeByName(UObject* obj, const FName& funcname, Args&&... args)
	{
		return UnrealLua::LuaScriptCall::CallLuaFunctionSafeByName(GetHandleForObject(obj), funcname, std::forward<Args>(args)...);
	}
	template<typename... Args>
	inline sol::protected_function_result CallLuaFunctionSafeByName(UObject* obj, const FString& funcname, Args&&... args)
	{
		return UnrealLua::LuaScriptCall::CallLuaFunctionSafeByName(GetHandleForObject(obj), funcname, std::forward<Args>(args)...);
	}
}