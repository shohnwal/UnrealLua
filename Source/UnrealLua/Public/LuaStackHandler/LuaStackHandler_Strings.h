#pragma once
#include "CoreMinimal.h"
#include "LuaStackHandlerMacros.h"
#include "sol/sol.hpp"

template <typename Handler>
inline bool sol_lua_check(sol::types<FString>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	// indices can be negative to count backwards from the top of the stack,
	// rather than the bottom up
	// to deal with this, we adjust the index to
	// its absolute position using the lua_absindex function
	int absolute_index = lua_absindex(L, index);
	// Check first index for being the proper types

	bool success = sol::stack::check<std::string>(L, absolute_index, handler);
	tracking.use(1);
	return success;
}

inline FString sol_lua_get(sol::types<FString>, lua_State* L, int index, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	int absolute_index = lua_absindex(L, index);
	sol::string_view a = sol::stack::get<sol::string_view>(L, absolute_index);
	tracking.use(1);
	
	FString temp{};
	if(!a.empty())
	{
		temp.Append(a.data());
	}		
	return temp;	
}

inline int sol_lua_push(lua_State* L, FString arg)
{
	LOCAL_FUNC_LOG()
	int amount = 0;

	//std::string str{StringCast<char>(*arg).Get()};
	//Do a full copy, in which case const doesn't matter
	amount = sol::stack::push<std::string>(L, StringCast<char>(*arg).Get());

	// Return pushed amount
	return amount;
}

template <typename Handler>
inline bool sol_lua_check(sol::types<FName>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	// indices can be negative to count backwards from the top of the stack,
	// rather than the bottom up
	// to deal with this, we adjust the index to
	// its absolute position using the lua_absindex function
	int absolute_index = lua_absindex(L, index);
	// Check first index for being the proper types

	bool success = sol::stack::check<std::string>(L, absolute_index, handler);
	tracking.use(1);
	return success;
}

inline FName sol_lua_get(sol::types<FName>, lua_State* L, int index, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	int absolute_index = lua_absindex(L, index);
	sol::string_view a = sol::stack::get<sol::string_view>(L, absolute_index);
	tracking.use(1);
	
	FString temp{};
	if(!a.empty())
	{
		temp.Append(a.data());
	}		
	return *temp;	

}

inline int sol_lua_push(lua_State* L, FName arg)
{
	LOCAL_FUNC_LOG()
	int amount = 0;

	std::string str{StringCast<char>(*arg.ToString()).Get()};
	//Do a full copy, in which case const doesn't matter
	amount = sol::stack::push<std::string>(L, str);

	// Return pushed amount
	return amount;
}

template <typename Handler>
inline bool sol_lua_check(sol::types<FText>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	// indices can be negative to count backwards from the top of the stack,
	// rather than the bottom up
	// to deal with this, we adjust the index to
	// its absolute position using the lua_absindex function
	int absolute_index = lua_absindex(L, index);
	// Check first index for being the proper types

	bool success = sol::stack::check<std::string>(L, absolute_index, handler);
	tracking.use(1);
	return success;
}

inline FText sol_lua_get(sol::types<FText>, lua_State* L, int index, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	int absolute_index = lua_absindex(L, index);
	sol::string_view a = sol::stack::get<sol::string_view>(L, absolute_index);
	tracking.use(1);
	
	FString temp{};
	if(!a.empty())
	{
		temp.Append(a.data());
	}
	return FText::FromString(temp);	
}

inline int sol_lua_push(lua_State* L, FText* arg)
{
	LOCAL_FUNC_LOG()
	int amount = 0;

	std::string str{StringCast<char>(*(*arg).ToString()).Get()};
	//Do a full copy, in which case const doesn't matter
	amount = sol::stack::push<std::string>(L, str);

	// Return pushed amount
	return amount;
}

inline int sol_lua_push(lua_State* L, FText arg)
{
	LOCAL_FUNC_LOG()
	int amount = 0;

	std::string str{StringCast<char>(*arg.ToString()).Get()};
	//Do a full copy, in which case const doesn't matter
	amount = sol::stack::push<std::string>(L, str);

	// Return pushed amount
	return amount;
}