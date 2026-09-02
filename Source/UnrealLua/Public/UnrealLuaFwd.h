#pragma once
#include "sol/forward.hpp"

class UObject;

namespace UnrealLua
{
	UNREALLUA_API bool IsUObject(const sol::object& obj);
	UNREALLUA_API bool IsUObject(const sol::stack_object& obj);
	
	UNREALLUA_API UObject* GetUObject(const sol::stack_object& obj);
	UNREALLUA_API UObject* GetUObject(const sol::object& obj);

	UNREALLUA_API bool IsEnum(const sol::stack_object& obj);
	UNREALLUA_API bool IsEnum(const sol::object& obj);
}