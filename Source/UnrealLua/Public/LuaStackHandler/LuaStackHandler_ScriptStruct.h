#pragma once
#include "CoreMinimal.h"
#include "LuaValue/LuaValue.h"
#include "Reflection/StructTemplateConcepts.h"
#include "LuaStackHandlerMacros.h"
#include "LuaTypes/LuaScriptStruct.h"
#include "LuaTypes/LuaInstancedStruct.h"
#include "LuaTypes/LuaSharedStruct.h"
#include "sol/sol.hpp"

template <typename Handler>
inline bool sol_lua_check(sol::types<FLuaValue>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{
	//anything can be convertedto FLuaValue
	return true;
}

template <typename Handler>
inline bool sol_lua_check(sol::types<FLuaValue*>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{
	//anything can be convertedto FLuaValue
	return true;
}

template <typename Handler, IsUStruct Arg>
inline bool sol_lua_check(sol::types<Arg>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{
	// indices can be negative to count backwards from the top of the stack,
	// rather than the bottom up
	// to deal with this, we adjust the index to
	// its absolute position using the lua_absindex function
	int absolute_index = lua_absindex(L, index);
	// Check first index for being the proper types

	bool success = sol::stack::check_usertype<FLuaScriptStruct>(L, absolute_index, handler);
	if(success)
	{
		const FLuaScriptStruct& w = sol::stack::get<FLuaScriptStruct&>(L, absolute_index);
		if(w.GetScriptStruct() == nullptr || w.GetScriptStruct() != Arg::StaticStruct() || w.Data == nullptr)
		{
			success = false;;
		}
	}
	tracking.use(1);
	return success;
}

template <typename Handler, IsUStruct Arg>
inline bool sol_lua_check(sol::types<Arg&>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{
	// indices can be negative to count backwards from the top of the stack,
	// rather than the bottom up
	// to deal with this, we adjust the index to
	// its absolute position using the lua_absindex function
	int absolute_index = lua_absindex(L, index);
	// Check first index for being the proper types

	bool success = sol::stack::check_usertype<FLuaScriptStruct>(L, absolute_index, handler);
	if(success)
	{
		const FLuaScriptStruct& w = sol::stack::get<FLuaScriptStruct&>(L, absolute_index);
		if(w.GetScriptStruct() == nullptr || w.GetScriptStruct() != Arg::StaticStruct() || w.Data == nullptr)
		{
			success = false;
		}
	}
	tracking.use(1);
	return success;
}

template <typename Handler, IsUStructPtr Arg>
inline bool sol_lua_check(sol::types<Arg>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	// indices can be negative to count backwards from the top of the stack,
	// rather than the bottom up
	// to deal with this, we adjust the index to
	// its absolute position using the lua_absindex function
	int absolute_index = lua_absindex(L, index);
	// Check first index for being the proper types

	bool success = sol::stack::check_usertype<FLuaScriptStruct>(L, absolute_index, handler);
	if(success)
	{
		const FLuaScriptStruct& w = sol::stack::get<FLuaScriptStruct&>(L, absolute_index);
		if(w.GetScriptStruct() == nullptr || w.GetScriptStruct() != std::remove_pointer_t<Arg>::StaticStruct() || w.Data == nullptr)
		{
			success = false;;
		}
	}
	tracking.use(1);
	return success;
}

template <typename Handler>
inline bool sol_lua_check(sol::types<FSharedStruct>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{
	// indices can be negative to count backwards from the top of the stack,
	// rather than the bottom up
	// to deal with this, we adjust the index to
	// its absolute position using the lua_absindex function
	int absolute_index = lua_absindex(L, index);
	// Check first index for being the proper types

	bool success = sol::stack::check_usertype<FLuaSharedStruct>(L, absolute_index, handler);
	tracking.use(1);
	return success;
}

template <typename Handler>
inline bool sol_lua_check(sol::types<FSharedStruct*>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{
	// indices can be negative to count backwards from the top of the stack,
	// rather than the bottom up
	// to deal with this, we adjust the index to
	// its absolute position using the lua_absindex function
	int absolute_index = lua_absindex(L, index);
	// Check first index for being the proper types

	bool success = sol::stack::check_usertype<FLuaSharedStruct>(L, absolute_index, handler);
	tracking.use(1);
	return success;
}

template <typename Handler>
inline bool sol_lua_check(sol::types<FInstancedStruct>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{
	// indices can be negative to count backwards from the top of the stack,
	// rather than the bottom up
	// to deal with this, we adjust the index to
	// its absolute position using the lua_absindex function
	int absolute_index = lua_absindex(L, index);
	// Check first index for being the proper types

	bool success = sol::stack::check_usertype<FLuaInstancedStruct>(L, absolute_index, handler);
	tracking.use(1);
	return success;
}

template <typename Handler>
inline bool sol_lua_check(sol::types<FInstancedStruct*>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{
	// indices can be negative to count backwards from the top of the stack,
	// rather than the bottom up
	// to deal with this, we adjust the index to
	// its absolute position using the lua_absindex function
	int absolute_index = lua_absindex(L, index);
	// Check first index for being the proper types

	bool success = sol::stack::check_usertype<FLuaInstancedStruct>(L, absolute_index, handler);
	tracking.use(1);
	return success;
}

inline FLuaValue sol_lua_get(sol::types<FLuaValue>, lua_State* L, int index, sol::stack::record& tracking)
{
	int absolute_index = lua_absindex(L, index);
	sol::object obj = sol::stack::get<sol::object>(L, absolute_index);
	tracking.use(1);

	return FLuaValue{obj};		
}

template <IsUStruct Arg>
inline std::remove_const_t<Arg> sol_lua_get(sol::types<Arg>, lua_State* L, int index, sol::stack::record& tracking)
{
	int absolute_index = lua_absindex(L, index);
	const FLuaScriptStruct& luaValue = sol::stack::get<FLuaScriptStruct&>(L, absolute_index);
	tracking.use(1);

	UScriptStruct* argSS = Arg::StaticStruct();
	Arg copy{};
	argSS->InitializeStruct(&copy);
	
	if(argSS->IsChildOf(luaValue.GetScriptStruct()))
	{
		argSS->CopyScriptStruct(&copy, luaValue.GetMemory());
	}
	return copy;		
}

template <IsUStruct Arg>
inline Arg* sol_lua_get(sol::types<Arg*>, lua_State* L, int index, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	int absolute_index = lua_absindex(L, index);
	const FLuaScriptStruct& a = sol::stack::get<FLuaScriptStruct&>(L, absolute_index);
	tracking.use(1);
	
	if(a.GetScriptStruct() == Arg::StaticStruct() && a.Data != nullptr)
	{
		return reinterpret_cast<Arg*>(a.Data);
	}
	return nullptr;	
}

inline FSharedStruct sol_lua_get(sol::types<FSharedStruct>, lua_State* L, int index, sol::stack::record& tracking)
{
	int absolute_index = lua_absindex(L, index);
	const FLuaSharedStruct& a = sol::stack::get<FLuaSharedStruct&>(L, absolute_index);
	tracking.use(1);
	return a.SharedStruct;	
}

inline FSharedStruct* sol_lua_get(sol::types<FSharedStruct*>, lua_State* L, int index, sol::stack::record& tracking)
{
	int absolute_index = lua_absindex(L, index);
	FLuaSharedStruct* a = sol::stack::get<FLuaSharedStruct*>(L, absolute_index);
	tracking.use(1);
	return &a->SharedStruct;
}

inline FInstancedStruct sol_lua_get(sol::types<FInstancedStruct>, lua_State* L, int index, sol::stack::record& tracking)
{
	int absolute_index = lua_absindex(L, index);
	const FLuaInstancedStruct& a = sol::stack::get<FLuaInstancedStruct&>(L, absolute_index);
	tracking.use(1);

	FInstancedStruct temp{};
	if(a.GetInstancedStruct() != nullptr)
	{
		temp.InitializeAs(a.GetScriptStruct(), static_cast<const uint8*>(a.GetMemory()));
	}
	return temp;	
}

inline FInstancedStruct* sol_lua_get(sol::types<FInstancedStruct*>, lua_State* L, int index, sol::stack::record& tracking)
{
	int absolute_index = lua_absindex(L, index);
	const FLuaInstancedStruct& a = sol::stack::get<FLuaInstancedStruct&>(L, absolute_index);
	tracking.use(1);
	return a.GetInstancedStruct();
}

inline int sol_lua_push(sol::types<FLuaValue>, lua_State* L, const FLuaValue& arg)
{
	return arg.PushValue(L);;
}

inline int sol_lua_push(lua_State* L, const FLuaValue* const arg)
{
	return arg->PushValue(L);;
}

template <IsLightUserdataStruct Arg>
inline int sol_lua_push(lua_State* L, Arg& arg)
{
	return sol::stack::push(L, sol::light(&arg));;;
}

template <IsLightUserdataStructPtr Arg>
inline int sol_lua_push(lua_State* L, Arg arg)
{
	return sol::stack::push(L, sol::light(arg));;
}

template <IsUStruct Arg>
inline int sol_lua_push(lua_State* L, Arg arg)
{
	UScriptStruct* strct = std::remove_const_t<std::remove_pointer_t<std::remove_reference_t<Arg>>>::StaticStruct();
	//Do a full copy, in which case const doesn't matter
	return sol::stack::push<FLuaScriptStruct>(L, {strct, static_cast<void*>(&arg), false, false});
}

template <IsUStructPtr Arg>
inline int sol_lua_push(lua_State* L, Arg arg)
{
	constexpr bool bIsConst = std::is_const_v<std::remove_reference_t<std::remove_pointer_t<Arg>>>;

	UScriptStruct* strct = std::remove_const_t<std::remove_pointer_t<std::remove_reference_t<Arg>>>::StaticStruct();
	
	//Pointer passing assumes that the data will stay alive during the Lua call
	if constexpr (bIsConst)
	{
		return sol::stack::push<FLuaScriptStruct>(L, {strct, static_cast<const void*>(arg), true});
	}
	else
	{
		return sol::stack::push<FLuaScriptStruct>(L, {strct, static_cast<void*>(arg), true, false});
	}
}

inline int sol_lua_push(lua_State* L, const FSharedStruct& arg)
{
	return sol::stack::push<FLuaSharedStruct>(L, FLuaSharedStruct{arg});
}

inline int sol_lua_push(lua_State* L, const FSharedStruct* const arg)
{
	return sol::stack::push<FLuaSharedStruct>(L, FLuaSharedStruct{arg});;
}

inline int sol_lua_push(lua_State* L, const FInstancedStruct& arg)
{
	return sol::stack::push<FLuaInstancedStruct>(L, FLuaInstancedStruct{&arg, false});
}

inline int sol_lua_push(lua_State* L, const FInstancedStruct* const arg)
{
	return sol::stack::push<FLuaInstancedStruct>(L, FLuaInstancedStruct{arg, true});;
}


