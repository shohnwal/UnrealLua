// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Reflection/PropertyHelperTypes.h"

#include "sol/sol.hpp"

struct FGetPropertyValueParams;
class FStructOnScope;

struct UNREALLUA_API FRotatorPropertyDescr
{
	static bool IsCompatibleType(FProperty* prop, const sol::object& luaValue);
	
	static sol::object GetPropertyValue(const FGetPropertyValueParams& params);
	static int GetPropertyValue(FPushPropertyValueParams& params);
	template<typename LUAOBJ>
	static void SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params);
	static FString GetPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params);

private:
	FRotatorPropertyDescr () { }
};


inline bool FRotatorPropertyDescr::IsCompatibleType(FProperty* prop, const sol::object& luaValue)
{
	return luaValue.is<FRotator>() || luaValue.is<FVector>() || luaValue.is<FVector2D>();
}

inline sol::object FRotatorPropertyDescr::GetPropertyValue(const FGetPropertyValueParams& params)
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
    
	return sol::object(params.Lua.lua_state(), sol::in_place_type<FRotator>, *static_cast<FRotator*>(params.MemoryPtr));
}

inline int FRotatorPropertyDescr::GetPropertyValue(FPushPropertyValueParams& params)
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
				if(inputObj.valid() && inputObj.is<FRotator>())
				{
					//Copy output from BlueprintVM into input FLuaScriptStruct data, so it acts as an out param
					//@TODO : also needs to be done for instanced struct and shared struct
					FRotator& vec = inputObj.as<FRotator&>();
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
	FRotator* vecPtr = static_cast<FRotator*>(params.MemoryPtr);
	if(byRef)
	{
		return sol::stack::push<FRotator*>(params.Lua.lua_state(), vecPtr);
	}
	else
	{
		return sol::stack::push<FRotator>(params.Lua.lua_state(), *vecPtr);
	}
}

inline FString FRotatorPropertyDescr::GetPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params)
{
	if (params.MemoryPtr == nullptr)
	{
		return "FRotator()";
	}
	else
	{
		FRotator& rot = *static_cast<FRotator*>(params.MemoryPtr);
		return "FRotator{" + rot.ToString() + "}";
	}
}

template<typename LUAOBJ>
inline void FRotatorPropertyDescr::SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params)
{
	FStructProperty* prop = CastField<FStructProperty>(params.Prop);
	FRotator rot = FRotator::ZeroRotator;
	if (!params.LuaValue.valid())
	{

	}
	else if (params.LuaValue.template is<FRotator>())
	{
		rot = params.LuaValue.template as<FRotator>();
		if(params.InputRecord && prop->HasAllPropertyFlags(CPF_OutParm) && !prop->HasAllPropertyFlags(CPF_ReturnParm))
		{
			params.InputRecord->Emplace(prop, params.LuaValue);
		}
	}
	else if (params.LuaValue.template is<FVector>())
	{
		rot = params.LuaValue.template as<FVector>().ToOrientationRotator();
	}
	else if (params.LuaValue.template is<FVector2D>())
	{
		rot = FVector(params.LuaValue.template as<FVector2D>(),0).ToOrientationRotator();
	}
	else if(params.LuaValue.get_type() == sol::type::table)
	{
		sol::table argtbl = params.LuaValue.template as<sol::table>();
		UnrealLua::PropertyHelper::InitializeStructFromTable(CastField<FStructProperty>(params.Prop)->Struct, &rot, argtbl);
	}
	prop->CopyCompleteValue(params.MemoryPtr, &rot);
}

template void FRotatorPropertyDescr::SetPropertyValue(const TSetPropertyValueParams<sol::object>& params);
template void FRotatorPropertyDescr::SetPropertyValue(const TSetPropertyValueParams<sol::stack_object>& params);
