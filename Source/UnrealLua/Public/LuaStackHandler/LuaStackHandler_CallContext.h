#pragma once
#include "LuaCallHelpers/LuaCallContext.h"

template<UObjectPtrTypename U>
int sol_lua_push(lua_State* L, FLuaCallContext& callContext) //echecked : this gets called when pushing UObject* derived
{
	LOCAL_FUNC_LOG()

	int amount = UnrealLua::LightUserdata::PushUObject(L, callContext);
	
	return amount;
}

template<UObjectPtrTypename U>
FLuaCallContext* sol_lua_get(sol::types<FLuaCallContext>, lua_State* L, int index, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	static_assert(std::is_pointer_v<U>);
	int absolute_index = lua_absindex(L, index);
	
	//FLuaUObjectWrapper& a = sol::stack::get<FLuaUObjectWrapper&>(L, absolute_index);
	//UObject* ptr = a.Get();
	sol::stack_object obj{L, absolute_index};
	UObject* ptr = UnrealLua::LightUserdata::GetUObject(obj);

	tracking.use(1);
	U castedptr = Cast<std::remove_pointer_t<U>>(ptr); 
	return castedptr;
}

template<UObjectTypename U>
FLuaCallContext* sol_lua_get(sol::types<U>, lua_State* L, int index, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	static_assert(!std::is_pointer_v<U>);
	int absolute_index = lua_absindex(L, index);

	//FLuaUObjectWrapper& a = sol::stack::get<FLuaUObjectWrapper&>(L, absolute_index);
	//UObject* ptr = a.Get();
	sol::stack_object obj{L, absolute_index};
	UObject* ptr = UnrealLua::LightUserdata::GetUObject(obj);
	
	tracking.use(1);
	U* castedptr = Cast<U>(ptr); 
	return castedptr;
}

template <typename Handler>
bool sol_lua_check(sol::types<UObject>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{ 
	int absolute_index = lua_absindex(L, index);

	sol::stack_object obj{L, absolute_index};
	UObject* uobj = UnrealLua::LightUserdata::GetUObject(obj);
	bool success = uobj != nullptr;
	
	tracking.use(1);
	return success;
}

template <typename Handler>
bool sol_lua_check(sol::types<UObject*>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{ 
	int absolute_index = lua_absindex(L, index);

	sol::stack_object obj{L, absolute_index};
	UObject* uobj = UnrealLua::LightUserdata::GetUObject(obj);
	bool success = uobj != nullptr;

	tracking.use(1);
	return success;
}