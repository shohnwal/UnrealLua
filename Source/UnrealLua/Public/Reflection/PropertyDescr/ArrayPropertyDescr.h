// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "LuaTypes/LuaArray.h"
#include "Reflection/PropertyHelper.h"
#include "sol/sol.hpp"
/**
 * 
 */
struct UNREALLUA_API FArrayPropertyDescr
{
	static bool IsCompatibleType(FProperty* prop, const sol::object& luaValue);
	
	static sol::object GetPropertyValue(const FGetPropertyValueParams& params);
	static int GetPropertyValue(FPushPropertyValueParams& params);
	template<typename LUAOBJ>
	static void SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params);
	static FString GetPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params);

private:
	FArrayPropertyDescr() { }
};


inline bool FArrayPropertyDescr::IsCompatibleType(FProperty* p, const sol::object& luaValue)
{
	FArrayProperty* prop = CastField<FArrayProperty>(p);
	if(luaValue.is<FLuaArray>())
	{
		FLuaArray& arr = luaValue.as<FLuaArray&>();
		return arr.GetInner()->StaticClassCastFlagsPrivate() == prop->Inner->StaticClassCastFlagsPrivate(); 
	}
	return false;
}

inline sol::object FArrayPropertyDescr::GetPropertyValue(const FGetPropertyValueParams& params)
{
	FArrayProperty* prop = CastField<FArrayProperty>(params.Prop);
	bool byRef = true;
	if(prop->HasAnyPropertyFlags(CPF_ReturnParm /* can CPF_ReturnParm be by reference in the first place? */))
	{
		byRef = false;
	}
	else if(prop->HasAnyPropertyFlags(CPF_OutParm))
	{
		//arrays are always function-returned via copy
		if(params.InputRecord)
		{
			int32 index = params.InputRecord->IndexOfByPredicate([prop](const FUFunctionCallInputLuaObjectRecordItem& item)
			{
				return prop == item.InputProp;
			});
			if(index != INDEX_NONE)
			{
				const FUFunctionCallInputLuaObjectRecordItem& found = (*params.InputRecord)[index];

				//If there is a valid input value for that prop and it has the same Array properties
				sol::object inputObj = found.LuaObj;
				params.InputRecord->RemoveAtSwap(index);
				
				if(inputObj.valid() && inputObj.is<FLuaArray>())
				{
					FLuaArray& arr = inputObj.as<FLuaArray&>();
					if(prop->Inner->SameType(arr.GetInner()))
					{
						FScriptArray* arrToCopyFrom = prop->GetPropertyValuePtr(params.MemoryPtr);
						FLuaArray::Copy(arr.GetScriptArray(), arr.GetInner(), arrToCopyFrom, prop->Inner);
						return inputObj;	
					}
				}
			}
			//didnt find any input value record, will copy out value as return param	
		}
		byRef = false;
		//didnt find any input value record, will copy out value as return 
	}
	FScriptArray* propArray = static_cast<FScriptArray*>(params.MemoryPtr);
	return sol::object(params.Lua, sol::in_place_type<FLuaArray>,*prop, propArray, byRef);
}



inline int FArrayPropertyDescr::GetPropertyValue(FPushPropertyValueParams& params)
{
	FArrayProperty* prop = CastField<FArrayProperty>(params.Prop);
	bool byRef = true;
	if(prop->HasAnyPropertyFlags(CPF_ReturnParm /* can CPF_ReturnParm be by reference in the first place? */))
	{
		byRef = false;
	}
	else if(prop->HasAnyPropertyFlags(CPF_OutParm))
	{
		//arrays are always function-returned via copy
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
				if(inputObj.valid() && inputObj.is<FLuaArray>())
				{
					FLuaArray& arr = inputObj.as<FLuaArray&>();
					if(prop->Inner->SameType(arr.GetInner()))
					{
						FScriptArray* nativeArray = prop->GetPropertyValuePtr(params.MemoryPtr);
						FLuaArray::Copy(arr.GetScriptArray(), arr.GetInner(), nativeArray, prop->Inner);
						return sol::stack::push(params.Lua, inputObj);
					}
				}
			}
			//didnt find any input value record, will copy out value as return param	
		}
		//didnt find any input value record, will copy out value as return 
	}
	FScriptArray* nativeArray = static_cast<FScriptArray*>(params.MemoryPtr);
	return sol::stack::push<FLuaArray>(params.Lua,*prop, nativeArray, byRef);
}

inline FString FArrayPropertyDescr::GetPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params)
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
			FArrayProperty* prop = CastField<FArrayProperty>(params.Prop);
			FProperty* inner = prop->Inner;
			FString innerStr = UnrealLua::PropertyHelper::GetPropertyTypeName(inner, true);
			FString arrStr = "TArray( " + innerStr +" )";
		
			return arrStr;
		}
	}
}

template<typename LUAOBJ>
void FArrayPropertyDescr::SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params)
{
	FArrayProperty* prop = CastField<FArrayProperty>(params.Prop);
	
	FScriptArray* targetScriptArray = static_cast<FScriptArray*>(params.MemoryPtr);

	FScriptArrayHelper helper = FScriptArrayHelper::CreateHelperFormInnerProperty(prop->Inner, targetScriptArray);
	FProperty* inner = prop->Inner;
	helper.EmptyValues();

	/*
	FString content;
	prop->ExportText_Direct(content, targetScriptArray, targetScriptArray, nullptr, PPF_None, nullptr);
	LUA_LOG("Array prop %s before setting : %s", *prop->GetName(), *content);
	*/
	if (params.LuaValue.get_type() == sol::type::userdata && params.LuaValue.template is<FLuaArray>())
	{
		FLuaArray& arr = params.LuaValue.template as<FLuaArray&>();
		if(arr.GetInner() && inner->SameType(arr.GetInner()))
		{
			FScriptArray* sourceArray = arr.GetScriptArray();
			/*		
			FString luaContent;
			prop->ExportText_Direct(luaContent, sourceArray, sourceArray, nullptr, PPF_None, nullptr);
			LUA_LOG("Array prop %s copying Lua array : %s", *prop->GetName(), *luaContent);
			*/
			//prop->CopyValuesInternal(targetScriptArray, sourceArray, 1);
			FLuaArray::Copy(targetScriptArray, prop->Inner, sourceArray, arr.GetInner());
			if(params.InputRecord && prop->HasAllPropertyFlags(CPF_OutParm))
			{
				params.InputRecord->Emplace(prop, params.LuaValue);
			}
		}
	}
	else if(params.LuaValue.get_type() == sol::type::table)
	{
		//case where a table is submitted as a value
		
		sol::table tbl = params.LuaValue.template as<sol::table>();
		
		int32 numInTbl = tbl.size();
		
		/*
		FString tblContent;
		prop->ExportText_Direct(tblContent, targetScriptArray, targetScriptArray, nullptr, PPF_None, nullptr);
		LUA_LOG("Array prop %s before merging table : %s", *prop->GetName(), *tblContent);
		*/
		for(int32 index = 0; index < numInTbl; ++index)
		{
			sol::object value = tbl[index+1];
			
			if(!UnrealLua::PropertyHelper::IsCompatibleType(inner, value))
			{
				continue;
			}
			
			uint8* indexptr = helper.GetElementPtr(helper.AddValue());
			const TSetPropertyValueParams newElementParams{inner, indexptr, 0, value};
			UnrealLua::PropertyHelper::SetPropertyValue_Direct(newElementParams);
		}
	}
	//case where a single value is provided
	else if(UnrealLua::PropertyHelper::IsCompatibleType(prop->Inner, params.LuaValue))
	{
		//create an empty array and put this one element in

		helper.AddValues(1);

		uint8* indexptr = helper.GetElementPtr(0);
		const TSetPropertyValueParams newElementParams{prop->Inner, indexptr, 0, params.LuaValue};
        UnrealLua::PropertyHelper::SetPropertyValue_Direct(newElementParams);
	}
	
	/*
	FString postcontent;
	prop->ExportText_Direct(postcontent, targetScriptArray, targetScriptArray, nullptr, PPF_None, nullptr);
	LUA_LOG("Array prop %s after setting : %s", *prop->GetName(), *postcontent);
	*/
}

template void FArrayPropertyDescr::SetPropertyValue(const TSetPropertyValueParams<sol::object>& params);
template void FArrayPropertyDescr::SetPropertyValue(const TSetPropertyValueParams<sol::stack_object>& params);
