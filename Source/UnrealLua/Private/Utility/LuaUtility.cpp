#include "Utility/LuaUtility.h"

#include "lua.hpp"
extern "C"
{
	#include "lapi.h"
	#include "lobject.h"
	#include "lstate.h"
	#include "lua.h"
}
TValue* UnrealLua::Utility::index2value(lua_State* L, int idx)
{
	CallInfo *ci = L->ci;
	if (idx > 0) 
	{
		StkId o = ci->func.p + idx;
		api_check(L, idx <= ci->top.p - (ci->func.p + 1), "unacceptable index");
		if (o >= L->top.p)
		{
			return &G(L)->nilvalue;
		}
		else
		{
			return s2v(o);
		}
	}
	else if (!ispseudo(idx)) 
	{  /* negative index */
		api_check(L, idx != 0 && -idx <= L->top.p - (ci->func.p + 1), "invalid index");
		return s2v(L->top.p + idx);
	}
	else if (idx == LUA_REGISTRYINDEX)
	{
		return &G(L)->l_registry;
	}
	else 
	{  /* upvalues */
		idx = LUA_REGISTRYINDEX - idx;
		api_check(L, idx <= MAXUPVAL + 1, "upvalue index too large");
		if (ttisCclosure(s2v(ci->func.p))) 
		{  /* C closure? */
			CClosure *func = clCvalue(s2v(ci->func.p));
			return (idx <= func->nupvalues) ? &func->upvalue[idx-1] : &G(L)->nilvalue;
		}
		else 
		{  /* light C function or Lua function (through a hook)?) */
			api_check(L, ttislcf(s2v(ci->func.p)), "caller not a C function");
			return &G(L)->nilvalue;  /* no upvalues */
		}
	}
}
