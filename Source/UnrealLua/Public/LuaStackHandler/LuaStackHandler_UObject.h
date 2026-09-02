#pragma once
#include "CoreMinimal.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"
#include "LuaStackHandlerMacros.h"
#include "LuaTypes/LuaLightUserdata.h"
#include "LuaTypes/LuaUClass.h"
//#include "LuaTypes/LuaUObjectWrapper.h"
#include "LuaTypes/LuaUStruct.h"
#include "sol/sol.hpp"
#include "Reflection/CPPPropertyDefs.h"

////////////////////////////////////
//UClass
////////////////////////////////////
///

template <typename Handler, IsUClassPtr U>
bool sol_lua_check(sol::types<U>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking) {
	LOCAL_FUNC_LOG()
	// indices can be negative to count backwards from the top of the stack,
	// rather than the bottom up
	// to deal with this, we adjust the index to
	// its absolute position using the lua_absindex function
	int absolute_index = lua_absindex(L, index);
	// Check first index for being the proper types
	bool success = sol::stack::check<FLuaUClass>(L, absolute_index, handler);
	tracking.use(1);
	return success;
}

template <IsUClassPtr U>
UClass* sol_lua_get(sol::types<U>, lua_State* L, int index, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	int absolute_index = lua_absindex(L, index);
	FLuaUClass a = sol::stack::get<FLuaUClass>(L, absolute_index);
	tracking.use(1);
	return a.TryLoadClass();
}

template <IsUClassPtr U>
int sol_lua_push(lua_State* L, U things)
{
	LOCAL_FUNC_LOG()
	int amount = sol::stack::push<FLuaUClass>(L, FLuaUClass{things});
	// Return pushed amount
	return amount;
}

////////////////////////////////////
//UObject
////////////////////////////////////
///

template<UObjectPtrTypename U>
int sol_lua_push(lua_State* L, U things) //echecked : this gets called when pushing UObject* derived
{
	LOCAL_FUNC_LOG()

	int amount = 0;
	if (things == nullptr)
	{
		lua_pushnil(L);
		amount = 1;
	}
	else
	{
		amount = UnrealLua::LightUserdata::PushUObject(L, things);
	//  amount = UnrealLua::UObjectRegistry::PushUObjectWrapper(L, things);
	}
	return amount;
}

template<UObjectPtrTypename U>
U sol_lua_get(sol::types<U>, lua_State* L, int index, sol::stack::record& tracking)
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
U* sol_lua_get(sol::types<U>, lua_State* L, int index, sol::stack::record& tracking)
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

template <typename Handler, UObjectPtrTypename U>
bool sol_lua_check(sol::types<U>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{ 
	LOCAL_FUNC_LOG()
	static_assert(std::is_pointer_v<U>);
	// indices can be negative to count backwards from the top of the stack,
	// rather than the bottom up
	// to deal with this, we adjust the index to
	// its absolute position using the lua_absindex function
	int absolute_index = lua_absindex(L, index);
	sol::stack_object obj{L, absolute_index};

	bool success = UnrealLua::LightUserdata::IsUObjectType<std::remove_pointer_t<U>>(obj);

	tracking.use(1);
	return success;
}

template <typename Handler, UObjectTypename U>
bool sol_lua_check(sol::types<U>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{ 
	LOCAL_FUNC_LOG()
	static_assert(!std::is_pointer_v<U>);
	// indices can be negative to count backwards from the top of the stack,
	// rather than the bottom up
	// to deal with this, we adjust the index to
	// its absolute position using the lua_absindex function
	int absolute_index = lua_absindex(L, index);

	sol::stack_object obj{L, absolute_index};
	bool success = UnrealLua::LightUserdata::IsUObjectType<U>(obj);

	tracking.use(1);
	return success;
}

////////////////////////////////////
//UEnum
////////////////////////////////////
template <typename Handler>
bool sol_lua_check(sol::types<UEnum*>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	// indices can be negative to count backwards from the top of the stack,
	// rather than the bottom up
	// to deal with this, we adjust the index to
	// its absolute position using the lua_absindex function
	int absolute_index = lua_absindex(L, index);
	// Check first index for being the proper types
	sol::stack_object obj{L, absolute_index};
	bool success = UnrealLua::LightUserdata::IsEnum(obj);
	tracking.use(1);
	return success;
}

inline UEnum* sol_lua_get(sol::types<UEnum*>, lua_State* L, int index, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	int absolute_index = lua_absindex(L, index);
	sol::stack_object obj{L, absolute_index};
	bool bIsEnum = UnrealLua::LightUserdata::IsEnum(obj);
	tracking.use(1);
	if (!bIsEnum)
	{
		return nullptr;
	}
	return UnrealLua::LightUserdata::GetUEnum(obj);
}

inline int sol_lua_push(lua_State* L, const UEnum* things)
{
	LOCAL_FUNC_LOG()
	return UnrealLua::LightUserdata::PushUEnum(things, L);
}
////////////////////////////////////
//ScriptStruct
////////////////////////////////////
template <typename Handler>
bool sol_lua_check(sol::types<UScriptStruct*>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	
	// indices can be negative to count backwards from the top of the stack,
	// rather than the bottom up
	// to deal with this, we adjust the index to
	// its absolute position using the lua_absindex function
	int absolute_index = lua_absindex(L, index);
	// Check first index for being the proper types
	bool success = sol::stack::check<FLuaUStruct>(L, absolute_index, handler);
	tracking.use(1);
	return success;
}

inline UScriptStruct* sol_lua_get(sol::types<UScriptStruct*>, lua_State* L, int index, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	int absolute_index = lua_absindex(L, index);
	FLuaUStruct a = sol::stack::get<FLuaUStruct>(L, absolute_index);
	tracking.use(1);
	return Cast<UScriptStruct>(a.TryLoad());
}

inline int sol_lua_push(lua_State* L, const UScriptStruct* things)
{
	LOCAL_FUNC_LOG()
	int amount = sol::stack::push<FLuaUStruct>(L, FLuaUStruct{const_cast<UScriptStruct*>(things)});
	// Return pushed amount
	return amount;
}