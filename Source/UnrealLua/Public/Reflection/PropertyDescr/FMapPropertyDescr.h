#pragma once

#include "CoreMinimal.h"
#include "LuaTypes/LuaMap.h"
#include "Reflection/PropertyHelper.h"
#include "Reflection/PropertyHelperTypes.h"
#include "sol/sol.hpp"

struct FGetPropertyValueParams;

namespace UnrealLua
{
	struct UNREALLUA_API FMapPropertyDescr
	{
		static bool IsCompatibleType(FProperty* prop, const sol::object& luaValue);
		static bool IsCompatibleKey(FProperty* keyProp, const sol::object& luaValue);
		static bool IsCompatibleValue(FProperty* valueProp, const sol::object& luaValue);
		static bool IsCompatibleTypePair(FProperty* keyProp, FProperty* valueProp, sol::object luaKey, const sol::object& luaValue);
		static sol::object GetPropertyValue(const FGetPropertyValueParams& params);
		static int GetPropertyValue(FPushPropertyValueParams& params);
		template<typename LUAOBJ>
		static void SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>&);
		static FString GetPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params);
	};
}


inline bool UnrealLua::FMapPropertyDescr::IsCompatibleType(FProperty* prop, const sol::object& luaValue)
{
	return false;
}

inline bool UnrealLua::FMapPropertyDescr::IsCompatibleKey(FProperty* keyProp, const sol::object& luaValue)
{
	return UnrealLua::PropertyHelper::IsCompatibleType(keyProp, luaValue);
}

inline bool UnrealLua::FMapPropertyDescr::IsCompatibleValue(FProperty* valueProp, const sol::object& luaValue)
{
	return UnrealLua::PropertyHelper::IsCompatibleType(valueProp, luaValue);
}

inline bool UnrealLua::FMapPropertyDescr::IsCompatibleTypePair(FProperty* keyProp, FProperty* valueProp, sol::object luaKey, const sol::object& luaValue)
{
	return IsCompatibleKey(keyProp, luaKey) && IsCompatibleValue(valueProp, luaValue);
}

inline sol::object UnrealLua::FMapPropertyDescr::GetPropertyValue(const FGetPropertyValueParams& params)
{
	FMapProperty* prop = CastField<FMapProperty>(params.Prop);

	bool byRef = true;
	if(prop->HasAnyPropertyFlags(CPF_ReturnParm /* can CPF_ReturnParm be by reference in the first place? */))
	{
		byRef = false;
	}
	else if(prop->HasAnyPropertyFlags(CPF_OutParm))
	{
		if(params.InputRecord)
		{
			FUFunctionCallInputLuaObjectRecordItem* found = params.InputRecord->FindByPredicate([prop](const FUFunctionCallInputLuaObjectRecordItem& item)
			{
				return prop == item.InputProp;
			});
			if(found)
			{
				//If there is a valid input value for that prop and it has the same Array properties
				sol::object inputObj = found->LuaObj;
				if(inputObj.valid() && inputObj.is<FLuaMap>())
				{
					FLuaMap& luaMap = inputObj.as<FLuaMap&>();
					if(prop->KeyProp->SameType(luaMap.GetKeyProperty()) && prop->ValueProp->SameType(luaMap.GetValueProperty()))
					{
						const FScriptMap* nativeMap = static_cast<FScriptMap*>(params.MemoryPtr);
						luaMap.Clone(nativeMap, luaMap.GetScriptMap());
						return inputObj;	
					}
				}
			}
			//didnt find any input value record, will copy out value as return param	
		}
		byRef = false;
		//didnt find any input value record, will copy out value as return 
	}

	FScriptMap* map = prop->GetPropertyValuePtr(params.MemoryPtr);
	return sol::object(params.Lua.lua_state(), sol::in_place_type<FLuaMap>, prop, map, byRef);
}

inline int UnrealLua::FMapPropertyDescr::GetPropertyValue(FPushPropertyValueParams& params)
{
	FMapProperty* prop = CastField<FMapProperty>(params.Prop);

	bool byRef = true;
	if(prop->HasAnyPropertyFlags(CPF_ReturnParm /* can CPF_ReturnParm be by reference in the first place? */))
	{
		byRef = false;
	}
	else if(prop->HasAnyPropertyFlags(CPF_OutParm))
	{
		byRef = false;
		if(params.InputRecord)
		{
			FUFunctionCallInputLuaObjectRecordItem* found = params.InputRecord->FindByPredicate([prop](const FUFunctionCallInputLuaObjectRecordItem& item)
			{
				return prop == item.InputProp;
			});
			if(found)
			{
				//If there is a valid input value for that prop and it has the same Array properties
				sol::object inputObj = found->LuaObj;
				if(inputObj.valid() && inputObj.is<FLuaMap>())
				{
					FLuaMap& map = inputObj.as<FLuaMap&>();
					if(prop->KeyProp->SameType(map.GetKeyProperty()) && prop->ValueProp->SameType(map.GetValueProperty()))
					{
						FScriptMap* nativeMap = prop->GetPropertyValuePtr(params.MemoryPtr);
						map.Clone(nativeMap, map.GetScriptMap());
						return sol::stack::push(params.Lua, inputObj);
					}
				}
			}
			//didnt find any input value record, will copy out value as return param	
		}
		//didnt find any input value record, will copy out value as return 
	}
	FScriptMap* map = prop->GetPropertyValuePtr(params.MemoryPtr);
	return sol::stack::push<FLuaMap>(params.Lua.lua_state(), prop, map, byRef);
}

inline FString UnrealLua::FMapPropertyDescr::GetPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params)
{
	if (params.MemoryPtr == nullptr)
	{
		return UnrealLua::PropertyHelper::GetPropertyTypeName(params.Prop, true);
	}
	else
	{
		if (params.ContainerAsTable)
		{
			return "{}";
		}
		else
		{
			return UnrealLua::PropertyHelper::GetPropertyTypeName(params.Prop, true);
		}
	}
}

template<typename LUAOBJ>
void UnrealLua::FMapPropertyDescr::SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params)
{

	FMapProperty* prop = CastField<FMapProperty>(params.Prop);
	FScriptMap* targetScriptMap = prop->GetPropertyValuePtr(params.MemoryPtr);
	
	FScriptMapHelper helper = FScriptMapHelper::CreateHelperFormInnerProperties(prop->KeyProp, prop->ValueProp, targetScriptMap);
	helper.EmptyValues();
	
	//FString content;
	//prop->ExportText_Direct(content, targetScriptMap, targetScriptMap, nullptr, PPF_None, nullptr);
	//LUA_LOG("Map prop %s before setting : %s", *prop->GetName(), *content);
	
	if (params.LuaValue.get_type() == sol::type::userdata && params.LuaValue.template is<FLuaMap>())
	{
		FLuaMap& luaMap = params.LuaValue.template as<FLuaMap&>();	
		if(prop->KeyProp->SameType(luaMap.GetKeyProperty()) && prop->ValueProp->SameType(luaMap.GetValueProperty()))
		{
			FScriptMap* sourceMap = luaMap.GetScriptMap();
			//FString luaContent;
			//prop->ExportText_Direct(luaContent, sourceMap, sourceMap, nullptr, PPF_None, nullptr);
			//LUA_LOG("Map prop %s copying Lua map : %s", *prop->GetName(), *luaContent);
			
			FLuaMap::Copy(targetScriptMap, prop->KeyProp, prop->ValueProp, sourceMap);
			
			if (params.InputRecord && prop->HasAllPropertyFlags(CPF_OutParm))
			{
				params.InputRecord->Emplace(prop, params.LuaValue);
			}
		}	
	}
	else if (params.LuaValue.get_type() == sol::type::table)
	{
		sol::table tbl = params.LuaValue.template as<sol::table>();
		
		//FString tblContent;
		//prop->ExportText_Direct(tblContent, targetScriptMap, targetScriptMap, nullptr, PPF_None, nullptr);
		//LUA_LOG("Map prop %s before merging table : %s", *prop->GetName(), *tblContent);
		
		tbl.for_each([&helper, keyProp = prop->KeyProp, valueProp = prop->ValueProp](const sol::object& key, const sol::object& value)
		{
			if(key == sol::nil)
			{
				return;
			}
			if(value == sol::nil)
			{
				return;
			}
			if(!UnrealLua::FMapPropertyDescr::IsCompatibleTypePair(keyProp, valueProp, key, value))
			{
				return;
			}

			FDefaultConstructedPropertyElement tempKey(keyProp);
			FDefaultConstructedPropertyElement tempValue(valueProp);

			TSetPropertyValueParams keyParams{keyProp, tempKey.GetObjAddress(),0, key};
			UnrealLua::PropertyHelper::SetPropertyValue_Direct(keyParams);
			TSetPropertyValueParams valParams{valueProp, tempValue.GetObjAddress(),0, value};
			UnrealLua::PropertyHelper::SetPropertyValue_Direct(valParams);

			helper.AddPair(tempKey.GetObjAddress(), tempValue.GetObjAddress());
		});
	}
	//FString postcontent;
	//prop->ExportText_Direct(postcontent, targetScriptMap, targetScriptMap, nullptr, PPF_None, nullptr);
	//LUA_LOG("Map prop %s after setting : %s", *prop->GetName(), *postcontent);

}

template
void UnrealLua::FMapPropertyDescr::SetPropertyValue<sol::object>(const TSetPropertyValueParams<sol::object>& params);
template
void UnrealLua::FMapPropertyDescr::SetPropertyValue<sol::stack_object>(const TSetPropertyValueParams<sol::stack_object>& params);