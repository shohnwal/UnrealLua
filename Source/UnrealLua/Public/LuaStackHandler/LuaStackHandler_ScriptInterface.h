#pragma once
#include "CoreMinimal.h"
#include "LuaStackHandlerMacros.h"
#include "sol/sol.hpp"

////////////////////////////////////
//UClass
////////////////////////////////////
///

template <typename Handler, typename U>
inline bool sol_lua_check(sol::types<TScriptInterface<U>>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking) {
	LOCAL_FUNC_LOG()
	// indices can be negative to count backwards from the top of the stack,
	// rather than the bottom up
	// to deal with this, we adjust the index to
	// its absolute position using the lua_absindex function
	int absolute_index = lua_absindex(L, index);
	// Check first index for being the proper types
	bool success = sol::stack::check<UObject>(L, absolute_index, handler);
	tracking.use(1);
	return success;
}

template <typename U>
inline TScriptInterface<U> sol_lua_get(sol::types<TScriptInterface<U>>, lua_State* L, int index, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	int absolute_index = lua_absindex(L, index);
	UObject* a = sol::stack::get<UObject*>(L, absolute_index);
	tracking.use(1);
	
	return a;
}

template <typename U>
inline int sol_lua_push(lua_State* L, const TScriptInterface<U>* things)
{
	LOCAL_FUNC_LOG()
	int amount = 0;
	if(things->GetObject())
	{
		amount = UnrealLua::LightUserdata::PushUObject(L, things->GetObject());
	}
	else
	{
		amount = sol::stack::push(L, sol::nil);	
	}
	// Return pushed amount
	return amount;
}