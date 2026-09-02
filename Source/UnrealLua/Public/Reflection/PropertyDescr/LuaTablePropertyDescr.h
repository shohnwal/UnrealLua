// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaValue/LuaTable.h"
#include "Reflection/PropertyHelperTypes.h"
#include "sol/sol.hpp"

/**
 * 
 */
struct FGetPropertyValueParams;

namespace UnrealLua
{
	namespace PropertyHandlers
	{
		namespace LuaTableProperty
		{
			UNREALLUA_API bool IsCompatibleType(FProperty* prop, const sol::object& luaValue);

			UNREALLUA_API sol::object GetPropertyValue(const FGetPropertyValueParams& params);
			UNREALLUA_API int GetPropertyValue(FPushPropertyValueParams& params);
			template<typename LUAOBJ>
			UNREALLUA_API void SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params);
			
		}

	}
}


inline bool UnrealLua::PropertyHandlers::LuaTableProperty::IsCompatibleType(FProperty* prop, const sol::object& luaValue)
{
	verify(!luaValue.is<FLuaTable>());
	return luaValue.is<sol::table>();
}

inline sol::object UnrealLua::PropertyHandlers::LuaTableProperty::GetPropertyValue(const FGetPropertyValueParams& params)
{
	FLuaTableHandle* tbl = static_cast<FLuaTableHandle*>(params.MemoryPtr);
	if(tbl && tbl->IsValid())
	{
		return tbl->GetTable();	
	}
	return sol::nil;
}

inline int UnrealLua::PropertyHandlers::LuaTableProperty::GetPropertyValue(FPushPropertyValueParams& params)
{
	FLuaTableHandle* tbl = static_cast<FLuaTableHandle*>(params.MemoryPtr);
	if(tbl && tbl->IsValid())
	{
		return sol::stack::push(params.Lua, tbl->GetTable());	
	}
	return sol::stack::push(params.Lua, sol::nil);
}

template<typename LUAOBJ>
void UnrealLua::PropertyHandlers::LuaTableProperty::SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params)
{
	verify(!params.LuaValue.template is<FLuaTable>());

	FStructProperty* prop = CastFieldChecked<FStructProperty>(params.Prop);
	if(params.LuaValue == sol::nil)
	{
		params.Prop->InitializeValue(params.MemoryPtr);
		return;
	}
	else if (params.LuaValue.template is<sol::table>())
	{
		FLuaTable* targetMemory = static_cast<FLuaTable*>(params.MemoryPtr);
		FLuaTable table;
		table.Table = params.LuaValue.template as<sol::table>();
		*targetMemory = table;
	}
}

template void UnrealLua::PropertyHandlers::LuaTableProperty::SetPropertyValue(const TSetPropertyValueParams<sol::object>& params);
template void UnrealLua::PropertyHandlers::LuaTableProperty::SetPropertyValue(const TSetPropertyValueParams<sol::stack_object>& params);