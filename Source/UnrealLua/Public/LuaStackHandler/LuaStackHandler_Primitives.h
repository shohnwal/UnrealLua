#pragma once
#include "LuaTypes/LuaPrimitives.h"
#include "sol/sol.hpp"
/*
template <typename Handler>
bool sol_lua_check(sol::types<FLuaPrimitiveCPPType>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking) {
	// indices can be negative to count backwards from the top of the stack,
	// rather than the bottom up
	// to deal with this, we adjust the index to
	// its absolute position using the lua_absindex function
	int absolute_index = lua_absindex(L, index);
	// Check first index for being the proper types
	bool success = sol::stack::check<FLuaPrimitiveCPPType>(L, absolute_index, handler);
	tracking.use(1);
	return success;
}

inline FLuaPrimitiveCPPType sol_lua_get(sol::types<FLuaPrimitiveCPPType>, lua_State* L, int index, sol::stack::record& tracking)
{
	int absolute_index = lua_absindex(L, index);
	FLuaPrimitiveCPPType a = sol::stack::get<FLuaPrimitiveCPPType>(L, absolute_index);
	tracking.use(1);
	return a;
}
*/