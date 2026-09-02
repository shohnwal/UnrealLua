#pragma once
#include "CoreMinimal.h"

#include "LuaStackHandlerMacros.h"
#include "../Config/UnrealLuaConstants.h"
#include "LuaTypes/LuaArray.h"
#include "Reflection/CPPPropertyDefs.h"
#include "Reflection/PropertyHelper.h"
#include "sol/sol.hpp"

template <typename Handler, typename Inner>
inline bool sol_lua_check(sol::types<TArray<Inner>>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	// indices can be negative to count backwards from the top of the stack,
	// rather than the bottom up
	// to deal with this, we adjust the index to
	// its absolute position using the lua_absindex function
	using InnerPropertyType = typename TypeToProp<Inner>::PropertyType;
	
	int absolute_index = lua_absindex(L, index);
	// Check first index for being the proper types
	bool success = sol::stack::check_usertype<FLuaArray>(L, absolute_index, handler);
	if(success)
	{
		FLuaArray& w = sol::stack::get<FLuaArray&>(L, absolute_index);
		if(w.GetInner() == nullptr || !w.GetInner()->IsA<InnerPropertyType>())
		{
			success = false;;
		}
	}
	tracking.use(1);
	return success;
}

template <typename Handler, typename Inner>
inline bool sol_lua_check(sol::types<TArray<Inner>*>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	// indices can be negative to count backwards from the top of the stack,
	// rather than the bottom up
	// to deal with this, we adjust the index to
	// its absolute position using the lua_absindex function
	using InnerPropertyType = typename TypeToProp<Inner>::PropertyType;
	
	int absolute_index = lua_absindex(L, index);
	// Check first index for being the proper types
	bool success = sol::stack::check_usertype<FLuaArray>(L, absolute_index, handler);
	if(success)
	{
		FLuaArray& w = sol::stack::get<FLuaArray&>(L, absolute_index);
		if(w.GetInner() == nullptr || !w.GetInner()->IsA<InnerPropertyType>())
		{
			success = false;;
		}
	}
	tracking.use(1);
	return success;
}

template <typename Inner>
inline TArray<Inner> sol_lua_get(sol::types<TArray<Inner>>, lua_State* L, int index, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	int absolute_index = lua_absindex(L, index);
	FLuaArray& a = sol::stack::get<FLuaArray&>(L, absolute_index);
	tracking.use(1);

	TArray<Inner> temp{};

	//check if its the correct inner property
	using InnerPropertyType = typename TypeToProp<Inner>::PropertyType;
	if(a.GetInner()->IsA<InnerPropertyType>())
	{
		FScriptArray* tempDummy = reinterpret_cast<FScriptArray*>(&temp);
		temp.Reserve(a.Num());
		//a.CloneArray(tempDummy, a.Inner, a.ScriptArray);
		FLuaArray::Copy(tempDummy, a.GetInner(), a.GetScriptArray(), a.GetInner());
	}
	
	return temp;	
}

template <typename Inner>
inline TArray<Inner>* sol_lua_get(sol::types<TArray<Inner>*>, lua_State* L, int index, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	int absolute_index = lua_absindex(L, index);
	FLuaArray& a = sol::stack::get<FLuaArray&>(L, absolute_index);
	tracking.use(1);

	TArray<Inner>* temp = nullptr;

	//check if its the correct inner property
	using InnerPropertyType = typename TypeToProp<Inner>::PropertyType;
	if(a.GetInner()->IsA<InnerPropertyType>())
	{
		temp = reinterpret_cast<TArray<Inner>*>(a.GetScriptArray());
	}
	return temp;	
}

template <typename Inner>
inline int sol_lua_push(lua_State* L, TArray<Inner> arg)
{
	LOCAL_FUNC_LOG()
	using InnerPropertyType = typename TypeToProp<Inner>::PropertyType;

	UClass* optionalPropClass = nullptr;
	
	if constexpr (std::is_same_v<InnerPropertyType,FObjectProperty>)
	{
		static_assert(UObjectPtrTypename<Inner>);
		optionalPropClass = std::remove_pointer_t<Inner>::StaticClass();
	}
	else if constexpr(std::is_same_v<InnerPropertyType,FClassProperty>)
	{
		static_assert(IsUClassPtr<Inner>);
		optionalPropClass = std::remove_pointer_t<Inner>::StaticClass();
	}
	else if constexpr(std::is_same_v<InnerPropertyType,FInterfaceProperty>)
	{
		static_assert(TIsIInterface<Inner, false>::Value);
		optionalPropClass = std::remove_pointer_t<Inner>::UClassType::StaticClass();
	}
	
	FProperty* innerProp = UnrealLua::PropertyHelper::CreateNewProperty(InnerPropertyType::StaticClassCastFlags(), optionalPropClass, UnrealLua::PropertyNames::NAME_UnrealLuaArrayInnerProperty, RF_Public);
	verify(innerProp != nullptr);
	verify(innerProp->Owner == nullptr);
	verify(innerProp->GetFName() == UnrealLua::PropertyNames::NAME_UnrealLuaArrayInnerProperty);
	//@TODO : fix up flags and properties according to PropertyBag.cpp FProperty* CreatePropertyFromDesc

	int amount = sol::stack::push<FLuaArray>(L, *innerProp, reinterpret_cast<FScriptArray*>(&arg), false);
	// Return pushed amount
	return amount;
}

template <typename Inner>
inline int sol_lua_push(lua_State* L, TArray<Inner>* arg)
{
	LOCAL_FUNC_LOG()
	using InnerPropertyType = typename TypeToProp<Inner>::PropertyType;
	
	UClass* optionalPropClass = nullptr;
	
	if constexpr (std::is_same_v<InnerPropertyType,FObjectProperty>)
	{
		static_assert(UObjectPtrTypename<Inner>);
		optionalPropClass = std::remove_pointer_t<Inner>::StaticClass();
	}
	else if constexpr(std::is_same_v<InnerPropertyType,FClassProperty>)
	{
		static_assert(IsUClassPtr<Inner>);
		optionalPropClass = std::remove_pointer_t<Inner>::StaticClass();
	}
	else if constexpr(std::is_same_v<InnerPropertyType,FInterfaceProperty>)
	{
		static_assert(TIsIInterface<Inner, false>::Value);
		optionalPropClass = std::remove_pointer_t<Inner>::UClassType::StaticClass();
	}
	
	FProperty* innerProp = UnrealLua::PropertyHelper::CreateNewProperty(InnerPropertyType::StaticClassCastFlags(), optionalPropClass, UnrealLua::PropertyNames::NAME_UnrealLuaArrayInnerProperty, RF_Public);
	verify(innerProp != nullptr);
	verify(innerProp->Owner == nullptr);
	verify(innerProp->GetFName() == UnrealLua::PropertyNames::NAME_UnrealLuaArrayInnerProperty);
	int amount = sol::stack::push<FLuaArray>(L, *innerProp, reinterpret_cast<FScriptArray*>(arg), true);
	// Return pushed amount
	return amount;
}