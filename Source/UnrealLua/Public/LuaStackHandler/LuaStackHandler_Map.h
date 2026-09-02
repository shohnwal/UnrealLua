#pragma once
#include"CoreMinimal.h"
#include "LuaTypes/LuaMap.h"
#include "LuaStackHandlerMacros.h"
#include "Reflection/CPPPropertyDefs.h"
#include "Reflection/PropertyHelper.h"
#include "sol/sol.hpp"

template <typename Handler, typename Key, typename Value>
inline bool sol_lua_check(sol::types<TMap<Key, Value>>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	// indices can be negative to count backwards from the top of the stack,
	// rather than the bottom up
	// to deal with this, we adjust the index to
	// its absolute position using the lua_absindex function
	using KeyPropertyType = typename TypeToProp<std::remove_pointer_t<Key>>::PropertyType;
	using ValuePropertyType = typename TypeToProp<std::remove_pointer_t<Value>>::PropertyType;
	
	int absolute_index = lua_absindex(L, index);
	// Check first index for being the proper types
	bool success = sol::stack::check_usertype<FLuaMap>(L, absolute_index, handler);
	if(success)
	{
		FLuaMap& w = sol::stack::get<FLuaMap&>(L, absolute_index);
		FProperty* keyProp = w.GetKeyProperty(); 
		FProperty* valueProp = w.GetValueProperty(); 
		if(keyProp == nullptr || !keyProp->IsA<KeyPropertyType>() || valueProp == nullptr || !valueProp->IsA<ValuePropertyType>())
		{
			success = false;;
		}
	}
	tracking.use(1);
	return success;
}

template <typename Handler, typename Key, typename Value>
inline bool sol_lua_check(sol::types<TMap<Key, Value>*>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	// indices can be negative to count backwards from the top of the stack,
	// rather than the bottom up
	// to deal with this, we adjust the index to
	// its absolute position using the lua_absindex function
	using KeyPropertyType = typename TypeToProp<std::remove_pointer_t<Key>>::PropertyType;
	using ValuePropertyType = typename TypeToProp<std::remove_pointer_t<Value>>::PropertyType;
	
	int absolute_index = lua_absindex(L, index);
	// Check first index for being the proper types
	bool success = sol::stack::check_usertype<FLuaMap>(L, absolute_index, handler);
	if(success)
	{
		FLuaMap& w = sol::stack::get<FLuaMap&>(L, absolute_index);
		FProperty* keyProp = w.GetKeyProperty(); 
		FProperty* valueProp = w.GetValueProperty(); 
		if(keyProp == nullptr || !keyProp->IsA<KeyPropertyType>() || valueProp == nullptr || !valueProp->IsA<ValuePropertyType>())
		{
			success = false;;
		}
	}
	tracking.use(1);
	return success;
}

template <typename Key, typename Value>
inline TMap<Key, Value> sol_lua_get(sol::types<TMap<Key, Value>>, lua_State* L, int index, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	int absolute_index = lua_absindex(L, index);
	FLuaMap& a = sol::stack::get<FLuaMap&>(L, absolute_index);
	tracking.use(1);

	TMap<Key, Value> temp{};
	using KeyPropertyType = typename TypeToProp<Key>::PropertyType;
	using ValuePropertyType = typename TypeToProp<Value>::PropertyType;
	
	FProperty* keyProp = a.GetKeyProperty(); 
	FProperty* valueProp = a.GetValueProperty(); 

	if(keyProp->IsA<KeyPropertyType>() && valueProp->IsA<ValuePropertyType>())
	{
		temp.Reserve(a.Lua_Num());
		FScriptMap* tempDummy = reinterpret_cast<FScriptMap*>(&temp);
		a.Clone(a.GetScriptMap(), tempDummy);
		//FLuaMap::Copy(tempDummy, a.KeyProp, a.ValueProp, a.ScriptMap);
	}
	
	return temp;	
}

template <typename Key, typename Value>
inline TMap<Key, Value>* sol_lua_get(sol::types<TMap<Key, Value>*>, lua_State* L, int index, sol::stack::record& tracking)
{
	LOCAL_FUNC_LOG()
	int absolute_index = lua_absindex(L, index);
	FLuaMap& a = sol::stack::get<FLuaMap&>(L, absolute_index);
	tracking.use(1);

	TMap<Key, Value>* temp = nullptr;
	using KeyPropertyType = typename TypeToProp<Key>::PropertyType;
	using ValuePropertyType = typename TypeToProp<Value>::PropertyType;

	FProperty* keyProp = a.GetKeyProperty(); 
	FProperty* valueProp = a.GetValueProperty();
	//check if its the correct inner property
	if(keyProp->IsA<KeyPropertyType>() && valueProp->IsA<ValuePropertyType>())
	{
		temp = reinterpret_cast<TMap<Key, Value>*>(a.GetScriptMap());
	}
	return temp;	
}

template <typename Key, typename Value>
inline int sol_lua_push(lua_State* L, TMap<Key, Value> arg)
{
	LOCAL_FUNC_LOG()
	using KeyPropertyType = typename TypeToProp<std::remove_pointer_t<Key>>::PropertyType;
	using ValuePropertyType = typename TypeToProp<std::remove_pointer_t<Value>>::PropertyType;

	UClass* optionalKeyPropClass = nullptr;
	UClass* optionalValuePropClass = nullptr;
	
	if constexpr (std::is_same_v<KeyPropertyType,FObjectProperty>)
	{
		optionalKeyPropClass = std::remove_pointer_t<Key>::StaticClass();
	}
	else if constexpr(std::is_same_v<KeyPropertyType,FClassProperty>)
	{
		optionalKeyPropClass = std::remove_pointer_t<Key>::StaticClass();
	}
	else if constexpr(std::is_same_v<KeyPropertyType,FInterfaceProperty>)
    {
    	static_assert(TIsIInterface<Key, false>::Value);
    	optionalValuePropClass = std::remove_pointer_t<Key>::UClassType::StaticClass();
    }

	if constexpr (std::is_same_v<ValuePropertyType,FObjectProperty>)
	{
		optionalValuePropClass = std::remove_pointer_t<Value>::StaticClass();
	}
	else if constexpr(std::is_same_v<ValuePropertyType,FClassProperty>)
	{
		optionalValuePropClass = std::remove_pointer_t<Value>::StaticClass();
	}
	else if constexpr(std::is_same_v<ValuePropertyType,FInterfaceProperty>)
	{
		static_assert(TIsIInterface<Value, false>::Value);
		optionalValuePropClass = std::remove_pointer_t<Value>::UClassType::StaticClass();
	}
	
	FProperty* keyProp = UnrealLua::PropertyHelper::CreateNewProperty(KeyPropertyType::StaticClassCastFlags(), optionalKeyPropClass, UnrealLua::PropertyNames::NAME_UnrealLuaMapKeyProperty, RF_Public);
	FProperty* valueProp = UnrealLua::PropertyHelper::CreateNewProperty(ValuePropertyType::StaticClassCastFlags(), optionalValuePropClass, UnrealLua::PropertyNames::NAME_UnrealLuaMapValueProperty, RF_Public);
	verify(keyProp != nullptr);
	verify(valueProp != nullptr);
	
	int amount = sol::stack::push<FLuaMap>(L, keyProp, valueProp, reinterpret_cast<FScriptMap*>(&arg), false);
	// Return pushed amount
	return amount;
}

template <typename Key, typename Value>
inline int sol_lua_push(lua_State* L, TMap<Key, Value>* arg)
{
	LOCAL_FUNC_LOG()
	using KeyPropertyType = typename TypeToProp<std::remove_pointer_t<Key>>::PropertyType;
	using ValuePropertyType = typename TypeToProp<std::remove_pointer_t<Value>>::PropertyType;
	
	UClass* optionalKeyPropClass = nullptr;
	UClass* optionalValuePropClass = nullptr;
	
	if constexpr (std::is_same_v<KeyPropertyType,FObjectProperty>)
	{
		optionalKeyPropClass = std::remove_pointer_t<Key>::StaticClass();
	}
	else if constexpr(std::is_same_v<KeyPropertyType,FClassProperty>)
	{
		optionalKeyPropClass = std::remove_pointer_t<Key>::StaticClass();
	}

	if constexpr (std::is_same_v<ValuePropertyType,FObjectProperty>)
	{
		optionalValuePropClass = std::remove_pointer_t<Value>::StaticClass();
	}
	else if constexpr(std::is_same_v<ValuePropertyType,FClassProperty>)
	{
		optionalValuePropClass = std::remove_pointer_t<Value>::StaticClass();
	}
	
	FProperty* keyProp = UnrealLua::PropertyHelper::CreateNewProperty(KeyPropertyType::StaticClassCastFlags(), optionalKeyPropClass,  UnrealLua::PropertyNames::NAME_UnrealLuaMapKeyProperty, RF_Public);
	FProperty* valueProp = UnrealLua::PropertyHelper::CreateNewProperty(ValuePropertyType::StaticClassCastFlags(), optionalValuePropClass,  UnrealLua::PropertyNames::NAME_UnrealLuaMapValueProperty, RF_Public);
	verify(keyProp != nullptr);
	verify(valueProp != nullptr);
	
	int amount = sol::stack::push<FLuaMap>(L, keyProp, valueProp, reinterpret_cast<FScriptMap*>(arg), true);
	// Return pushed amount
	return amount;
}