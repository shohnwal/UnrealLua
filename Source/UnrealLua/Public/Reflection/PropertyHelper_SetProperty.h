#pragma once
#include "CoreMinimal.h"
#include "PropertyHelperTypes.h"
#include "sol/forward.hpp"

struct FLuaUObjectItem;
struct FLuaValue;
class FProperty;

namespace UnrealLua::PropertyHelper
{
	template<typename LUAOBJ>
	UNREALLUA_API bool SetValueInUObjectProperty(const LUAOBJ& key, FLuaUObjectItem& objItem, const LUAOBJ& value);
	
	UNREALLUA_API bool SetValueInUObjectProperty(const sol::string_view& key, UObject* obj, const FProperty* sourceProperty, void* sourceValueAddress, bool bCallRepNotify = true);
	UNREALLUA_API bool SetValueInUObjectProperty(const sol::string_view& key, FLuaUObjectItem& objItem, const FProperty* sourceProperty, void* sourceValueAddress, bool bCallRepNotify = true);
	//used by replicator
	
	UNREALLUA_API bool SetValueInUObjectProperty(const sol::string_view& key, FLuaUObjectItem& objItem, const FLuaValue& value, bool bCallRepNotify = true);
	UNREALLUA_API bool SetValueInUObjectProperty(const sol::object& key, FLuaUObjectItem& objItem, const sol::object& value, bool bCallRepNotify = true);
	UNREALLUA_API bool SetValueInUObjectProperty(const sol::string_view& key, FLuaUObjectItem& item, const sol::object& value, bool bCallRepNotify = true);

	template<typename LUAOBJ>
	UNREALLUA_API bool SetValueInUObjectProperty(FProperty* prop, UObject* obj, const LUAOBJ& value);
	
	template<typename LUAOBJ>
	UNREALLUA_API void SetPropertyValue_InContainer(TSetPropertyValueParams<LUAOBJ>& params);
	template<typename LUAOBJ>
	UNREALLUA_API void SetPropertyValue_Direct(const TSetPropertyValueParams<LUAOBJ>& params);
	
	UNREALLUA_API void InitializeStructFromTable(const UScriptStruct* structMetaData, void* containerPtr, sol::table& table, bool resetStruct = true);
	template<typename S>
	UNREALLUA_API void InitializeStructFromTable(S& structData, sol::table& table, bool resetStruct = true)
	{
		UScriptStruct* ss = S::StaticStruct();
		InitializeStructFromTable(ss, &structData, table, resetStruct);
	}
	
	UNREALLUA_API void InitializeUObjectFromTable(UObject* obj, const sol::table& table, bool resetUObject = true);
	
	
}