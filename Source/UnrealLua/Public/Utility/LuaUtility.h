#pragma once

struct lua_State;
struct TValue;

namespace UnrealLua
{
	namespace Utility
	{
		#define ispseudo(i)		((i) <= LUA_REGISTRYINDEX)
		TValue *index2value (lua_State *L, int idx);
	}
}
