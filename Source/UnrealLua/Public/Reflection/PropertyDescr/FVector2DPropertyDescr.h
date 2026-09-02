// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#pragma warning(disable : 4996) //Vector2D LessThan comparison operator is annoying
#include "CoreMinimal.h"
#include "LuaTypes/LuaScriptStruct.h"
#include "Reflection/PropertyHelper.h"
#include "Reflection/PropertyHelperTypes.h"
#include "LuaTypes/LuaUserdataTypeTraits.h"
#include "sol/sol.hpp"

struct FGetPropertyValueParams;

struct UNREALLUA_API FVector2DPropertyDescr
{
	static bool IsCompatibleType(FProperty* prop, const sol::object& luaValue);
	
	static sol::object GetPropertyValue(const FGetPropertyValueParams& params);
	static int GetPropertyValue(FPushPropertyValueParams& params);
	template<typename LUAOBJ>
	static void SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params);
	static FString GetPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params);

private:
	FVector2DPropertyDescr() { }
};


inline bool FVector2DPropertyDescr::IsCompatibleType(FProperty* prop, const sol::object& luaValue)
{
	return luaValue.is<FVector2D>() || luaValue.is<FVector>() || luaValue.is<FRotator>();
}

inline sol::object FVector2DPropertyDescr::GetPropertyValue(const FGetPropertyValueParams& params)
{
	FStructProperty* prop = CastField<FStructProperty>(params.Prop);
	bool getAsReference = true;
	if(prop->HasAnyPropertyFlags(CPF_ReturnParm /* can CPF_ReturnParm be by reference in the first place? */)) 
	{
		//out parms and return parms get copied out
		
		getAsReference = false;
	}
	else if(prop->HasAnyPropertyFlags(CPF_OutParm))
	{
		//Imitate out params:
		/*
		 * //out param
		 * local vec = FVector()
		 * modifyVector(vec)
		 *
		 * //out param and also as return param, both will refer to same struct
		 * local vec2 = FVector()
		 * local vec2_2 = modifyVector(vec2)
		 *
		 * //default-constructs vector as input
		 * //modifies it and returns as return param
		 * local vec3 = modifyVector(nil)
		 * local vec4 = modifyVector()
		 *
		 **/
		getAsReference = false;
		if(params.InputRecord)
		{
			FUFunctionCallInputLuaObjectRecordItem* found = params.InputRecord->FindByPredicate([prop](const FUFunctionCallInputLuaObjectRecordItem& item)
			{
				return prop == item.InputProp;
			});
			if(found)
			{
				//If there is a valid input value for that prop and it has the same UScriptStruct
				sol::object inputObj = found->LuaObj;
				if(inputObj.valid() && inputObj.is<FVector>())
				{
					//Copy output from BlueprintVM into input FLuaScriptStruct data, so it acts as an out param
					//@TODO : also needs to be done for instanced struct and shared struct
					FVector& vec = inputObj.as<FVector&>();
					prop->CopyCompleteValue(&vec, params.MemoryPtr);
					//and return it also as an output value
					//both luaValues will refer to the same FLuaScriptStruct
					return inputObj;
				}
			}
			//didnt find any input value record, will copy out value as return param	
		}
		//didnt find any input value record, will copy out value as return param
	}
	else
	{
		/* Non-out parm properties are gotten as reference:
		 * - UObject members
		 * - FStruct Members
		 * - Array members
		 * - etc
		 */
		//if the user wants to get a copy in Lua, they can do it with myStruct:Copy()
		getAsReference = true;
	}
	return sol::object(params.Lua.lua_state(), sol::in_place_type<FVector2D>, *static_cast<FVector*>(params.MemoryPtr));
}

inline int FVector2DPropertyDescr::GetPropertyValue(FPushPropertyValueParams& params)
{
	FStructProperty* prop = CastField<FStructProperty>(params.Prop);
	bool byRef = true;
	if(prop->HasAnyPropertyFlags(CPF_ReturnParm /* can CPF_ReturnParm be by reference in the first place? */)) 
	{
		//out parms and return parms get copied out
		
		byRef = false;
	}
	else if(prop->HasAnyPropertyFlags(CPF_OutParm))
	{
		//Imitate out params:
		/*
		 * //out param
		 * local vec = FVector()
		 * modifyVector(vec)
		 *
		 * //out param and also as return param, both will refer to same struct
		 * local vec2 = FVector()
		 * local vec2_2 = modifyVector(vec2)
		 *
		 * //default-constructs vector as input
		 * //modifies it and returns as return param
		 * local vec3 = modifyVector(nil)
		 * local vec4 = modifyVector()
		 *
		 **/
		byRef = false;
		if(params.InputRecord)
		{
			FUFunctionCallInputLuaObjectRecordItem* found = params.InputRecord->FindByPredicate([prop](const FUFunctionCallInputLuaObjectRecordItem& item)
			{
				return prop == item.InputProp;
			});
			if(found)
			{
				//If there is a valid input value for that prop and it has the same UScriptStruct
				sol::object inputObj = found->LuaObj;
				if(inputObj.valid() && inputObj.is<FVector2D>())
				{
					//Copy output from BlueprintVM into input FLuaScriptStruct data, so it acts as an out param
					//@TODO : also needs to be done for instanced struct and shared struct
					FVector2D& vec = inputObj.as<FVector2D&>();
					prop->CopyCompleteValue(&vec, params.MemoryPtr);
					//and return it also as an output value
					//both luaValues will refer to the same FLuaScriptStruct
					return sol::stack::push(params.Lua, inputObj);
				}
			}
			//didnt find any input value record, will copy out value as return param	
		}
		//didnt find any input value record, will copy out value as return param
	}
	else
	{
		/* Non-out parm properties are gotten as reference:
		 * - UObject members
		 * - FStruct Members
		 * - Array members
		 * - etc
		 */
		//if the user wants to get a copy in Lua, they can do it with myStruct:Copy()
		byRef = true;
	}
	FVector2D* vecPtr = static_cast<FVector2D*>(params.MemoryPtr);
	if(byRef)
	{
		return sol::stack::push<FVector2D*>(params.Lua.lua_state(), vecPtr);
	}
	else
	{
		return sol::stack::push<FVector2D>(params.Lua.lua_state(), *vecPtr);
	}
}

inline FString FVector2DPropertyDescr::GetPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params)
{
	if (params.MemoryPtr == nullptr)
	{
		return "FVector2D()";
	}
	else
	{
		FVector2D& vec = *static_cast<FVector2D*>(params.MemoryPtr);
		return "FVector2D{" + vec.ToString() + "}";
	}
}

template<typename LUAOBJ>
inline void FVector2DPropertyDescr::SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params)
{
	FStructProperty* prop = CastField<FStructProperty>(params.Prop);

	FVector2D vec = FVector2D::ZeroVector;
	if (params.LuaValue == sol::nil)
	{
		//Stays Zero vector
	}
	else if (params.LuaValue.template is<FVector2D>())
	{
		vec = params.LuaValue.template as<FVector2D>();
		if(params.InputRecord && prop->HasAllPropertyFlags(CPF_OutParm) && !prop->HasAllPropertyFlags(CPF_ReturnParm))
		{
			params.InputRecord->Emplace(prop, params.LuaValue);
		}
	}
	else if (params.LuaValue.template is<FRotator>())
	{
		vec = FVector2D(params.LuaValue.template as<FRotator>().Vector());
	}
	else if (params.LuaValue.template is <FVector>())
	{
		vec = FVector2D(params.LuaValue.template as<FVector>());;
	}
	else if(params.LuaValue.template is<FLuaScriptStruct>())
	{
		FLuaScriptStruct& ss = params.LuaValue.template as<FLuaScriptStruct&>();
		//We already know the ScriptStruct is Vector2D::ScripStruct from the LUT check
		verify(ss.Data != nullptr);
		vec = *reinterpret_cast<FVector2D*>(ss.Data);
	}
	else if(params.LuaValue.get_type() == sol::type::table)
	{
		sol::table argtbl = params.LuaValue.template as<sol::table>();
		UnrealLua::PropertyHelper::InitializeStructFromTable(CastField<FStructProperty>(params.Prop)->Struct, &vec, argtbl);
	}
	prop->CopyCompleteValue(params.MemoryPtr, &vec);
}

template void FVector2DPropertyDescr::SetPropertyValue(const TSetPropertyValueParams<sol::object>& params);
template void FVector2DPropertyDescr::SetPropertyValue(const TSetPropertyValueParams<sol::stack_object>& params);
