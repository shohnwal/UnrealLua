#pragma once
#include"CoreMinimal.h"

#include "LuaStackHandlerMacros.h"
#include "LuaTypes/LuaSet.h"
#include "Reflection/CPPPropertyDefs.h"
#include "sol/sol.hpp"

template <typename Handler, typename Inner>
inline bool sol_lua_check(sol::types<TSet<Inner>>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	// indices can be negative to count backwards from the top of the stack,
	// rather than the bottom up
	// to deal with this, we adjust the index to
	// its absolute position using the lua_absindex function
	using InnerPropertyType = typename TypeToProp<std::remove_pointer_t<Inner>>::PropertyType;
	
	int absolute_index = lua_absindex(L, index);
	// Check first index for being the proper types
	bool success = sol::stack::check_usertype<FLuaSet>(L, absolute_index, handler);
	if(success)
	{
		FLuaSet& w = sol::stack::get<FLuaSet&>(L, absolute_index);
		if(w.GetInner() == nullptr || !w.GetInner()->IsA<InnerPropertyType>())
		{
			success = false;;
		}
	}
	tracking.use(1);
	return success;
}

template <typename Handler, typename Inner>
inline bool sol_lua_check(sol::types<TSet<Inner>*>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	// indices can be negative to count backwards from the top of the stack,
	// rather than the bottom up
	// to deal with this, we adjust the index to
	// its absolute position using the lua_absindex function
	using InnerPropertyType = typename TypeToProp<std::remove_pointer_t<Inner>>::PropertyType;
	
	int absolute_index = lua_absindex(L, index);
	// Check first index for being the proper types
	bool success = sol::stack::check_usertype<FLuaSet>(L, absolute_index, handler);
	if(success)
	{
		FLuaSet& w = sol::stack::get<FLuaSet&>(L, absolute_index);
		if(w.GetInner() == nullptr || !w.GetInner()->IsA<InnerPropertyType>())
		{
			success = false;;
		}
	}
	tracking.use(1);
	return success;
}

template <typename Inner>
inline TSet<Inner> sol_lua_get(sol::types<TSet<Inner>>, lua_State* L, int index, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	int absolute_index = lua_absindex(L, index);
	FLuaSet& a = sol::stack::get<FLuaSet&>(L, absolute_index);
	tracking.use(1);

	TSet<Inner> temp{};

	//check if its the correct inner property
	using InnerPropertyType = typename TypeToProp<Inner>::PropertyType;
	if(a.GetInner()->IsA<InnerPropertyType>())
	{
		FScriptSet* tempDummy = reinterpret_cast<FScriptSet*>(&temp);
		temp.Reserve(a.Lua_Num());
		a.Clone(tempDummy);
	}
	
	return temp;	
}

template <typename Inner>
inline TSet<Inner>* sol_lua_get(sol::types<TSet<Inner>*>, lua_State* L, int index, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	int absolute_index = lua_absindex(L, index);
	FLuaSet& a = sol::stack::get<FLuaSet&>(L, absolute_index);
	tracking.use(1);

	TSet<Inner>* temp = nullptr;

	//check if its the correct inner property
	using InnerPropertyType = typename TypeToProp<Inner>::PropertyType;
	if(a.GetInner()->IsA<InnerPropertyType>())
	{
		temp = reinterpret_cast<TSet<Inner>*>(a.GetScriptSet());
	}
	return temp;	
}

template <typename Inner>
inline int sol_lua_push(lua_State* L, TSet<Inner> arg)
{
	LOCAL_FUNC_LOG()
	using InnerPropertyType = typename TypeToProp<std::remove_pointer_t<Inner>>::PropertyType;

	UClass* innerClass = nullptr;
	
	if constexpr (std::is_same_v<InnerPropertyType,FObjectProperty>)
	{
		innerClass = std::remove_pointer_t<Inner>::StaticClass();
	}
	else if constexpr(std::is_same_v<InnerPropertyType,FClassProperty>)
	{
		innerClass = std::remove_pointer_t<Inner>::StaticClass();
	}
	FProperty* innerProp = UnrealLua::PropertyHelper::CreateNewProperty(InnerPropertyType::StaticClassCastFlags(), innerClass, UnrealLua::PropertyNames::NAME_UnrealLuaSetInnerProperty, RF_Public);
	verify(innerProp != nullptr);

	
	int amount = sol::stack::push<FLuaSet>(L, innerProp, reinterpret_cast<FScriptSet*>(&arg), false);
	// Return pushed amount
	return amount;
}

template <typename Inner>
inline int sol_lua_push(lua_State* L, TSet<Inner>* arg)
{
	LOCAL_FUNC_LOG()
	using InnerPropertyType = typename TypeToProp<std::remove_pointer_t<Inner>>::PropertyType;

	UClass* innerClass = nullptr;
	//@TODO : fix up flags and properties according to PropertyBag.cpp FProperty* CreatePropertyFromDesc
	if constexpr (std::is_same_v<InnerPropertyType,FObjectProperty>)
	{
		innerClass = std::remove_pointer_t<Inner>::StaticClass();
	}
	else if constexpr(std::is_same_v<InnerPropertyType,FClassProperty>)
	{
		innerClass = std::remove_pointer_t<Inner>::StaticClass();
	}
	FProperty* innerProp = UnrealLua::PropertyHelper::CreateNewProperty(InnerPropertyType::StaticClassCastFlags(), innerClass, UnrealLua::PropertyNames::NAME_UnrealLuaSetInnerProperty, RF_Public);
	verify(innerProp != nullptr);
	
	int amount = sol::stack::push<FLuaSet>(L, innerProp, reinterpret_cast<FScriptSet*>(arg), true);
	// Return pushed amount
	return amount;
}