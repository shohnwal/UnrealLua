// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaTypes/LuaSet.h"
#include "Reflection/PropertyHelperTypes.h"
#include "sol/sol.hpp"
struct FGetPropertyValueParams;
/**
 * 
 */
struct UNREALLUA_API FSetPropertyDescr
{
	static bool IsCompatibleType(FProperty* prop, const sol::object& luaValue);
	
	static sol::object GetPropertyValue(const FGetPropertyValueParams& params);
	static int GetPropertyValue(FPushPropertyValueParams& params);
	template<typename LUAOBJ>
	static void SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params);
	static FString GetPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params);

private:
	FSetPropertyDescr() { }
};


inline bool FSetPropertyDescr::IsCompatibleType(FProperty* p, const sol::object& luaValue)
{
	if(luaValue.is<FLuaSet>())
	{
		FLuaSet& arr = luaValue.as<FLuaSet&>();
		return p->SameType(arr.GetInner()); 
	}
	return false;
}

inline sol::object FSetPropertyDescr::GetPropertyValue(const FGetPropertyValueParams& params)
{
	FSetProperty* prop = CastField<FSetProperty>(params.Prop);
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
				if(inputObj.valid() && inputObj.is<FLuaSet>())
				{
					FLuaSet& arr = inputObj.as<FLuaSet&>();
					if(prop->ElementProp->SameType(arr.GetInner()))
					{
						FScriptSet* nativeScriptSet = prop->GetPropertyValuePtr(params.MemoryPtr);
						FLuaSet::Copy(nativeScriptSet, arr.GetInner(), arr.GetScriptSet());
						return inputObj;
					}
				}
			}
			//didnt find any input value record, will copy out value as return param	
		}
		byRef = false;
		//didnt find any input value record, will copy out value as return 
	}
	FScriptSet* referencedSet = prop->GetPropertyValuePtr(params.MemoryPtr);
	return sol::object(params.Lua, sol::in_place_type<FLuaSet>, prop, referencedSet, byRef);
}

inline int FSetPropertyDescr::GetPropertyValue(FPushPropertyValueParams& params)
{
	FSetProperty* prop = CastField<FSetProperty>(params.Prop);
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
				if(inputObj.valid() && inputObj.is<FLuaSet>())
				{
					FLuaSet& arr = inputObj.as<FLuaSet&>();
					if(prop->ElementProp->SameType(arr.GetInner()))
					{
						FScriptSet* nativeScriptSet = prop->GetPropertyValuePtr(params.MemoryPtr);
						FLuaSet::Copy(nativeScriptSet, arr.GetInner(), arr.GetScriptSet());
						return sol::stack::push(params.Lua, inputObj);
					}
				}
			}
			//didnt find any input value record, will copy out value as return param	
		}
		byRef = false;
		//didnt find any input value record, will copy out value as return 
	}
	FScriptSet* referencedSet = prop->GetPropertyValuePtr(params.MemoryPtr);
	return sol::stack::push<FLuaSet>(params.Lua, prop, referencedSet, byRef);
}

inline FString FSetPropertyDescr::GetPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params)
{
	if (params.ContainerAsTable)
	{
		return "{}";
	}
	else
	{
		FSetProperty* prop = CastField<FSetProperty>(params.Prop);
		FProperty* inner = prop->ElementProp;
		FString typeStr = UnrealLua::PropertyHelper::GetPropertyTypeName(prop, true);
		
		
		return typeStr;
	}
}

template<typename LUAOBJ>
void FSetPropertyDescr::SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params)
{
	FSetProperty* prop = CastField<FSetProperty>(params.Prop);
	
	FScriptSet* targetScriptSet = prop->GetPropertyValuePtr(params.MemoryPtr);
	
	FScriptSetHelper helper = FScriptSetHelper::CreateHelperFormElementProperty(prop->ElementProp, targetScriptSet);
	helper.EmptyElements(0);
	
	//FString content;
	//prop->ExportText_Direct(content, targetScriptSet, targetScriptSet, nullptr, PPF_None, nullptr);
	//LUA_LOG("Set prop %s before setting : %s", *prop->GetName(), *content);
	
	if (params.LuaValue.get_type() == sol::type::userdata && params.LuaValue.template is<FLuaSet>())
	{
		FLuaSet& set = params.LuaValue.template as<FLuaSet&>();
		if(set.GetInner() && prop->ElementProp->SameType(set.GetInner()))
		{
			FScriptSet* sourceSet = set.GetScriptSet(); 
			//FString luaContent;
			//prop->ExportText_Direct(luaContent, sourceSet, sourceSet, nullptr, PPF_None, nullptr);
			//LUA_LOG("Set prop %s copying Lua set : %s", *prop->GetName(), *luaContent);
			
			FLuaSet::Copy(sourceSet, set.GetInner(), targetScriptSet);
			if(params.InputRecord && prop->HasAllPropertyFlags(CPF_OutParm))
			{
				params.InputRecord->Emplace(prop, params.LuaValue);
			}
		}
	}
	else if(params.LuaValue.get_type() == sol::type::table)
	{
		sol::table tbl = params.LuaValue.template as<sol::table>();
		
		//FString tblContent;
		//prop->ExportText_Direct(tblContent, targetScriptSet, targetScriptSet, nullptr, PPF_None, nullptr);
		//LUA_LOG("Set prop %s before merging table : %s", *prop->GetName(), *tblContent);
		
		for(int index = 0; index < tbl.size(); index++)
		{
			sol::object value = tbl[index+1];
			
			if(!UnrealLua::PropertyHelper::IsCompatibleType(prop->ElementProp, value))
			{
				continue;
			}
			FDefaultConstructedPropertyElement temp(prop->ElementProp);
	
			TSetPropertyValueParams newElementParams{prop->ElementProp, temp.GetObjAddress(), 0, value};
			UnrealLua::PropertyHelper::SetPropertyValue_Direct(newElementParams);

			helper.AddElement(temp.GetObjAddress());
		}
	}
	else if(UnrealLua::PropertyHelper::IsCompatibleType(prop->ElementProp, params.LuaValue))
	{
		//create an empty set and put this one element in
		FDefaultConstructedPropertyElement temp(prop->ElementProp);
	
		TSetPropertyValueParams newElementParams{prop->ElementProp, temp.GetObjAddress(), 0, params.LuaValue};
		UnrealLua::PropertyHelper::SetPropertyValue_Direct(newElementParams);

		helper.AddElement(temp.GetObjAddress());
	}
	//FString postcontent;
	//prop->ExportText_Direct(postcontent, targetScriptSet, targetScriptSet, nullptr, PPF_None, nullptr);
	//LUA_LOG("Set prop %s after setting : %s", *prop->GetName(), *postcontent)
}

template void FSetPropertyDescr::SetPropertyValue(const TSetPropertyValueParams<sol::object>& params);
template void FSetPropertyDescr::SetPropertyValue(const TSetPropertyValueParams<sol::stack_object>& params);