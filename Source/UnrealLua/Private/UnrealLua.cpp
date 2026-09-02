#include "UnrealLua.h"

#include "SubSystem/UnrealLuaEngineSubsystem.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"

namespace UnrealLua
{
	bool IsUObject(const sol::stack_object& obj)
	{
		return UnrealLua::LightUserdata::IsUObject(obj);
	}

	bool IsUObject(const sol::object& obj)
	{
		return UnrealLua::LightUserdata::IsUObject(obj);
	}

	UObject* GetUObject(const sol::stack_object& obj)
	{
		return UnrealLua::LightUserdata::GetUObject(obj);
	}

	UObject* GetUObject(const sol::object& obj)
	{
		return UnrealLua::LightUserdata::GetUObject(obj);
	}

	bool IsEnum(const sol::stack_object& obj)
	{
		return UnrealLua::LightUserdata::IsEnum(obj);
	}

	bool IsEnum(const sol::object& obj)
	{
		return UnrealLua::LightUserdata::IsEnum(obj);
	}

	bool IsGameSessionActive()
	{
		return UUnrealLuaEngineSubsystem::IsGameSessionActive();
	}
}
namespace UnrealLua::LuaScriptCall
{
	FLuaUObjectItem* GetHandleForObject(UObject* obj)
	{
		return &UnrealLua::UObjectRegistry::GetUObjectItem(obj);
	}

	sol::function GetLuaFunction(const FLuaUObjectItem* handle, const char* funcName)
	{
		return handle->GetLuaScriptFunction(funcName);
	}

	sol::function GetLuaFunction(const FLuaUObjectItem* handle, const TCHAR* funcName)
	{
		return handle->GetLuaScriptFunction(funcName);
	}

	sol::function GetLuaFunction(const FLuaUObjectItem* handle, const sol::object& key)
	{
		return handle->GetLuaScriptFunction(key);
	}
}
