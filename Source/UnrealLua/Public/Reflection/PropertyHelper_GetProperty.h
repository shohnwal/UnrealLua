#pragma once
#include "CoreMinimal.h"
#include "sol/sol.hpp"
#include "Reflection/PropertyHelperTypes.h"
struct FLuaScriptStructBase;
class FProperty;

namespace UnrealLua::PropertyHelper
{
	template<typename LUAOBJ>
	UNREALLUA_API sol::object GetValueFromScriptStructProperty(const LUAOBJ& key, FLuaScriptStructBase& strct, sol::this_state lua);

	UNREALLUA_API int GetValueFromScriptStructProperty(const sol::stack_object& key, FLuaScriptStructBase& strct);
	
	UNREALLUA_API bool SetValueInScriptStructProperty(const sol::stack_object& key, FLuaScriptStructBase& strct, const sol::stack_object& luaValue);
	
	UNREALLUA_API sol::object GetPropertyValue_InContainer(FGetPropertyValueParams& params);
	UNREALLUA_API sol::object GetPropertyValue(const FGetPropertyValueParams& params);
	UNREALLUA_API int GetPropertyValue_InContainer(FPushPropertyValueParams& params);
	UNREALLUA_API int GetPropertyValue(FPushPropertyValueParams& params);
}