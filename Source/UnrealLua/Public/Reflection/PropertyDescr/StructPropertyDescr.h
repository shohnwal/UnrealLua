// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Config/UnrealLuaConstants.h"
#include "LuaTypes/LuaInstancedStruct.h"
#include "LuaTypes/LuaScriptStruct.h"
#include "LuaTypes/LuaSharedStruct.h"
#include "Reflection/PropertyHelper.h"
#include "Reflection/PropertyHelperTypes.h"
#include "sol/sol.hpp"
#include "Utility/LuaLogMacros.h"
#include "LuaContext/ScopedLuaContext.h"

struct FGetPropertyValueParams;
class FStructOnScope;
/**
 * 
 */
class UNREALLUA_API FStructPropertyDescr
{
public:
	static bool IsCompatibleType(FProperty* prop, const sol::object& luaValue);
	
	static sol::object GetPropertyValue(const FGetPropertyValueParams& params);
	static int GetPropertyValue(FPushPropertyValueParams& params);
	template<typename LUAOBJ>
	static void SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params);
	
	static sol::object GetInstancedStructPropertyValue(const FGetPropertyValueParams& params);
	static int GetInstancedStructPropertyValue(FPushPropertyValueParams& params);
	template<typename LUAOBJ>
	static void SetInstancedStructPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params);
	
	static sol::object GetSharedStructPropertyValue(const FGetPropertyValueParams& params);
	static int GetSharedStructPropertyValue(FPushPropertyValueParams& params);
	template<typename LUAOBJ>
	static void SetSharedStructPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params);
	
	static sol::object GetLuaValuePropertyValue(const FGetPropertyValueParams& params);
	static int GetLuaValuePropertyValue(const FPushPropertyValueParams& params);
	template<typename LUAOBJ>
	static void SetLuaValuePropertyValue(const TSetPropertyValueParams<LUAOBJ>& Params);
	
	static sol::object GetLuaFunctionPropertyValue(const FGetPropertyValueParams& Params);
	static int GetLuaFunctionPropertyValue(const FPushPropertyValueParams& params);
	template<typename LUAOBJ>
	static void SetLuaFunctionPropertyValue(const TSetPropertyValueParams<LUAOBJ>& Params);
	
	static sol::object GetLuaTablePropertyValue(const FGetPropertyValueParams& Params);
	static int GetLuaTablePropertyValue(const FPushPropertyValueParams& params);
	template<typename LUAOBJ>
	static void SetLuaTablePropertyValue(const TSetPropertyValueParams<LUAOBJ>& Params);

	static uint32 AddRef(FReferenceCollector& collector, FStructProperty* strProp, void* memory, bool container);
	static FString GetInstancedStructPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params);
	static FString GetSharedStructPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params);
	static FString GetLuaValuePropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params);
	static FString GetLuaFunctionPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params);
	static FString GetLuaTablePropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params);
	static FString GetPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params);

private:
	FStructPropertyDescr() {}
};


inline bool FStructPropertyDescr::IsCompatibleType(FProperty* p, const sol::object& luaValue)
{
	if(luaValue == sol::nil)
	{
		//we can always default construct
		return true;
	}
	FStructProperty* prop = CastField<FStructProperty>(p);

	if(luaValue.is<FLuaScriptStructBase>())
	{
		FLuaScriptStructBase& base = luaValue.as<FLuaScriptStructBase&>();
		if(base.GetScriptStruct()->IsChildOf(prop->Struct))
		{
			return true;	
		}
	}
	else if(luaValue.get_type() == sol::type::table)
	{
		//we can always construct from a table
		return true;
	}
	//if one luavalue given and struct has only one property, try to set it
	if(luaValue.valid() && prop->Struct->PropertyLink && prop->Struct->PropertyLink->Next == nullptr)
	{
		//only one property, can try to set it
		return true;
	}
	LUA_LOG("Can not set struct property %s : LuaValue is not a FLuaScriptStruct and not nil.", *p->GetName())
	return false;
}

inline sol::object FStructPropertyDescr::GetPropertyValue(const FGetPropertyValueParams& params)
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
		if(params.InputRecord)
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
			FUFunctionCallInputLuaObjectRecordItem* found = params.InputRecord->FindByPredicate([prop](const FUFunctionCallInputLuaObjectRecordItem& item)
			{
				return prop == item.InputProp;
			});
			if(found)
			{
				//If there is a valid input value for that prop and it has the same UScriptStruct
				sol::object inputObj = found->LuaObj;
				if(inputObj.valid() && inputObj.is<FLuaScriptStructBase>())
				{
					FLuaScriptStructBase& inputStruct = inputObj.as<FLuaScriptStructBase&>();
					if(inputStruct.GetScriptStruct()->IsChildOf(prop->Struct))
					{
						//Copy output from BlueprintVM into input Lua struct data, so it acts as an out param
						prop->CopyCompleteValue(inputStruct.GetMemory(), params.MemoryPtr);
						//and return it also as an output value
						//both luaValues will refer to the same FLuaScriptStruct
						return inputObj;						
					}
				}
			}
			//didnt find any input value record, will copy out value as return param	
		}
		byRef = false;
		//didnt find any input value record, will copy out value as return param
	}
	else
	{
		byRef = true;
		/* Non-out parm properties are gotten as reference:
		 * - UObject members
		 * - FStruct Members
		 * - Array members
		 * - etc
		 */
		//if the user wants to get a copy in Lua, they can do it with myStruct:Copy()
	}

	bool isConst = prop->HasAnyPropertyFlags(CPF_EditConst | CPF_BlueprintReadOnly | CPF_ConstParm);

	return sol::object(params.Lua.lua_state(), sol::in_place_type<FLuaScriptStruct>, prop->Struct, params.MemoryPtr, byRef, isConst);	
}

inline int FStructPropertyDescr::GetPropertyValue(FPushPropertyValueParams& params)
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
		if(params.InputRecord)
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
			FUFunctionCallInputLuaObjectRecordItem* found = params.InputRecord->FindByPredicate([prop](const FUFunctionCallInputLuaObjectRecordItem& item)
			{
				return prop == item.InputProp;
			});
			if(found)
			{
				//If there is a valid input value for that prop and it has the same UScriptStruct
				sol::object inputObj = found->LuaObj;
				if(inputObj.valid() && inputObj.is<FLuaScriptStructBase>())
				{
					FLuaScriptStructBase& inputStruct = inputObj.as<FLuaScriptStructBase&>();
					if(inputStruct.GetScriptStruct()->IsChildOf(prop->Struct))
					{
						//Copy output from BlueprintVM into input Lua struct data, so it acts as an out param
						prop->CopyCompleteValue(inputStruct.GetMemory(), params.MemoryPtr);
						//and return it also as an output value
						//both luaValues will refer to the same FLuaScriptStruct
						return sol::stack::push(params.Lua, inputObj);						
					}
				}
			}
			//didnt find any input value record, will copy out value as return param	
		}
		byRef = false;
		//didnt find any input value record, will copy out value as return param
	}
	else
	{
		byRef = true;
		/* Non-out parm properties are gotten as reference:
		 * - UObject members
		 * - FStruct Members
		 * - Array members
		 * - etc
		 */
		//if the user wants to get a copy in Lua, they can do it with myStruct:Copy()
	}

	bool isConst = prop->HasAnyPropertyFlags(CPF_EditConst | CPF_BlueprintReadOnly | CPF_ConstParm);

	return sol::stack::push<FLuaScriptStruct>(params.Lua.lua_state(), prop->Struct, params.MemoryPtr, byRef, isConst);
}

template<typename LUAOBJ>
inline void FStructPropertyDescr::SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params)
{
	FStructProperty* prop = CastField<FStructProperty>(params.Prop);
	
	//setting a member property value in a uobject or fstruct
	//this is always done via copy

	if(params.LuaValue.template is<FLuaScriptStructBase>())
	{
		//it's a compatible UScriptStruct
		FLuaScriptStructBase& ss = params.LuaValue.template as<FLuaScriptStructBase&>();
		if(ss.GetScriptStruct()->IsChildOf(prop->Struct))
		{
			prop->Struct->CopyScriptStruct(params.MemoryPtr, ss.GetMemory());

			if(params.InputRecord && prop->HasAllPropertyFlags(CPF_OutParm))
			{
				params.InputRecord->Emplace(prop, params.LuaValue);
			}
			return;
		}
	}
	else if(params.LuaValue.get_type() == sol::type::table)
	{
		sol::table argtbl = params.LuaValue.template as<sol::table>();
		UnrealLua::PropertyHelper::InitializeStructFromTable(prop->Struct, params.MemoryPtr, argtbl);
		return;
	}
	//if one luavalue given and struct has only one property, try to set it
	else if(prop->Struct->PropertyLink && prop->Struct->PropertyLink->Next == nullptr)
	{
		//only one property
		prop->InitializeValue(params.MemoryPtr);
		TSetPropertyValueParams setParms{prop->Struct->PropertyLink, params.MemoryPtr, 0, params.LuaValue};
		UnrealLua::PropertyHelper::SetPropertyValue_InContainer(setParms);
		return;
	}
	else
	{
		prop->InitializeValue(params.MemoryPtr);	
	}
}

template
void FStructPropertyDescr::SetPropertyValue(const TSetPropertyValueParams<sol::object>& params);
template
void FStructPropertyDescr::SetPropertyValue(const TSetPropertyValueParams<sol::stack_object>& params);

inline sol::object FStructPropertyDescr::GetInstancedStructPropertyValue(const FGetPropertyValueParams& params)
{
	FStructProperty* prop = CastField<FStructProperty>(params.Prop);
	FInstancedStruct* nativeMemoryPtr = static_cast<FInstancedStruct*>(params.MemoryPtr);
	bool asRef = true;
	if(prop->HasAnyPropertyFlags(CPF_ReturnParm /* can CPF_ReturnParm be by reference in the first place? */)) 
	{
		//out parms and return parms get copied out
		
		asRef = false;
	}
	else if(prop->HasAnyPropertyFlags(CPF_OutParm))
	{
		if(params.InputRecord)
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
			FUFunctionCallInputLuaObjectRecordItem* found = params.InputRecord->FindByPredicate([prop](const FUFunctionCallInputLuaObjectRecordItem& item)
			{
				return prop == item.InputProp;
			});
			if(found)
			{
				//If there is a valid input value for that prop and it has the same UScriptStruct
				sol::object inputObj = found->LuaObj;
				if(inputObj.valid() && inputObj.is<FLuaScriptStructBase>())
				{
					FLuaScriptStructBase& inputStruct = inputObj.as<FLuaScriptStructBase&>();
					if(prop->Struct->IsChildOf(inputStruct.GetScriptStruct()))
					{
						//Copy output from BlueprintVM into input Lua struct data, so it acts as an out param
						prop->CopyCompleteValue(inputStruct.GetMemory(), nativeMemoryPtr);
						//and return it also as an output value
						//both luaValues will refer to the same FLuaScriptStruct
						return inputObj;;						
					}
					else if(inputObj.is<FLuaInstancedStruct>())
					{
						FLuaInstancedStruct& is = inputObj.as<FLuaInstancedStruct&>();
						is.InitializeAs(prop->Struct);
						prop->CopyCompleteValue(inputStruct.GetMemory(), nativeMemoryPtr);
						return inputObj;
					}
				}
			}
			//didnt find any input value record, will copy out value as return param	
		}
		asRef = false;
		//didnt find any input value record, will copy out value as return param
	}
	else
	{
		asRef = true;
		/* Non-out parm properties are gotten as reference:
		 * - UObject members
		 * - FStruct Members
		 * - Array members
		 * - etc
		 */
		//if the user wants to get a copy in Lua, they can do it with myStruct:Copy()
	}
	return sol::object(params.Lua, sol::in_place_type<FLuaInstancedStruct>,nativeMemoryPtr, asRef);
}

inline int FStructPropertyDescr::GetInstancedStructPropertyValue(FPushPropertyValueParams& params)
{
	FStructProperty* prop = CastField<FStructProperty>(params.Prop);
	FInstancedStruct* nativeMemoryPtr = static_cast<FInstancedStruct*>(params.MemoryPtr);
	//bool asRef = prop->HasAnyPropertyFlags(CPF_Parm);
	//For now, always get instanced structs as ref, so it can be modified
	bool asRef = true;
	if(prop->HasAnyPropertyFlags(CPF_ReturnParm /* can CPF_ReturnParm be by reference in the first place? */)) 
	{
		//out parms and return parms get copied out
		
		asRef = false;
	}
	else if(prop->HasAnyPropertyFlags(CPF_OutParm))
	{
		if(params.InputRecord)
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
			FUFunctionCallInputLuaObjectRecordItem* found = params.InputRecord->FindByPredicate([prop](const FUFunctionCallInputLuaObjectRecordItem& item)
			{
				return prop == item.InputProp;
			});
			if(found)
			{
				//If there is a valid input value for that prop and it has the same UScriptStruct
				sol::object inputObj = found->LuaObj;
				if(inputObj.valid() && inputObj.is<FLuaScriptStructBase>())
				{
					FLuaScriptStructBase& inputStruct = inputObj.as<FLuaScriptStructBase&>();
					if(prop->Struct->IsChildOf(inputStruct.GetScriptStruct()))
					{
						//Copy output from BlueprintVM into input Lua struct data, so it acts as an out param
						prop->CopyCompleteValue(inputStruct.GetMemory(), nativeMemoryPtr);
						//and return it also as an output value
						//both luaValues will refer to the same FLuaScriptStruct
						return sol::stack::push(params.Lua, inputObj);						
					}
					else if(inputObj.is<FLuaInstancedStruct>())
					{
						FLuaInstancedStruct& is = inputObj.as<FLuaInstancedStruct&>();
						is.InitializeAs(prop->Struct);
						prop->CopyCompleteValue(inputStruct.GetMemory(), nativeMemoryPtr);
						return sol::stack::push(params.Lua, inputObj);
					}
				}
			}
			//didnt find any input value record, will copy out value as return param	
		}
		asRef = false;
		//didnt find any input value record, will copy out value as return param
	}
	else
	{
		asRef = true;
		/* Non-out parm properties are gotten as reference:
		 * - UObject members
		 * - FStruct Members
		 * - Array members
		 * - etc
		 */
		//if the user wants to get a copy in Lua, they can do it with myStruct:Copy()
	}
	return sol::stack::push<FLuaInstancedStruct>(params.Lua,nativeMemoryPtr, asRef);	
}

template<typename LUAOBJ>
void FStructPropertyDescr::SetInstancedStructPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params)
{
	FStructProperty* prop = CastField<FStructProperty>(params.Prop);
	//UProperty data
	FInstancedStruct* nativeMemoryPtr = static_cast<FInstancedStruct*>(params.MemoryPtr);
	
	if(params.LuaValue.template is<FLuaScriptStructBase>())
	{
		//Lua data
		FLuaScriptStructBase& ss = params.LuaValue.template as<FLuaScriptStructBase&>();
		
		//copy over values from Lua struct to UProperty struct
		nativeMemoryPtr->InitializeAs(ss.GetScriptStruct(), static_cast<uint8*>(ss.GetMemory()));
		
		if(params.InputRecord && prop->HasAllPropertyFlags(CPF_OutParm))
		{
			params.InputRecord->Emplace(prop, params.LuaValue);
		}
	}
	else if(params.LuaValue.get_type() == sol::type::table)
	{
		prop->InitializeValue(nativeMemoryPtr);
		if(const UScriptStruct* metaData = nativeMemoryPtr->GetScriptStruct(); metaData != nullptr)
		{
			sol::table argtbl = params.LuaValue.template as<sol::table>();
			UnrealLua::PropertyHelper::InitializeStructFromTable(metaData, nativeMemoryPtr->GetMutableMemory(), argtbl);			
		}
	}
}

template
void FStructPropertyDescr::SetInstancedStructPropertyValue(const TSetPropertyValueParams<sol::object>& params);
template
void FStructPropertyDescr::SetInstancedStructPropertyValue(const TSetPropertyValueParams<sol::stack_object>& params);

inline sol::object FStructPropertyDescr::GetSharedStructPropertyValue(const FGetPropertyValueParams& params)
{
	FStructProperty* prop = CastField<FStructProperty>(params.Prop);
	FSharedStruct* instance = static_cast<FSharedStruct*>(params.MemoryPtr);
	return sol::object(params.Lua, sol::in_place_type<FLuaSharedStruct>, *instance);
}

inline int FStructPropertyDescr::GetSharedStructPropertyValue(FPushPropertyValueParams& params)
{
	FStructProperty* prop = CastField<FStructProperty>(params.Prop);
	FSharedStruct* instance = static_cast<FSharedStruct*>(params.MemoryPtr);
	return sol::stack::push<FLuaSharedStruct>(params.Lua, *instance);
}


template<typename LUAOBJ>
void FStructPropertyDescr::SetSharedStructPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params)
{
	//@TODO: 
	unimplemented();
}

template void FStructPropertyDescr::SetSharedStructPropertyValue(const TSetPropertyValueParams<sol::object>& params);
template void FStructPropertyDescr::SetSharedStructPropertyValue(const TSetPropertyValueParams<sol::stack_object>& params);


inline sol::object FStructPropertyDescr::GetLuaFunctionPropertyValue(const FGetPropertyValueParams& params)
{
	FStructProperty* prop = CastField<FStructProperty>(params.Prop);
	FLuaFunctionHandle* luaValue = static_cast<FLuaFunctionHandle*>(params.MemoryPtr);
	return luaValue->GetFunctionAsObject();	
}

inline int FStructPropertyDescr::GetLuaFunctionPropertyValue(const FPushPropertyValueParams& params)
{
	FStructProperty* prop = CastField<FStructProperty>(params.Prop);
	FLuaFunctionHandle* luaValue = static_cast<FLuaFunctionHandle*>(params.MemoryPtr);
	return sol::stack::push(params.Lua, luaValue->GetFunction());
}


template <typename LUAOBJ>
void FStructPropertyDescr::SetLuaFunctionPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params)
{
	FStructProperty* prop = CastField<FStructProperty>(params.Prop);
	verify(prop->Struct == UnrealLua::StaticPackages::LuaFunction);
	
	FLuaFunctionHandle* luaValue = static_cast<FLuaFunctionHandle*>(params.MemoryPtr);
	if (params.LuaValue.valid() && params.LuaValue.get_type() == sol::type::function)
	{
		*luaValue = FLuaFunctionHandle::MakeHandle(params.LuaValue);
	}
	else
	{
		luaValue->Invalidate();
	}		
}

template void FStructPropertyDescr::SetLuaFunctionPropertyValue(const TSetPropertyValueParams<sol::object>& params);
template void FStructPropertyDescr::SetLuaFunctionPropertyValue(const TSetPropertyValueParams<sol::stack_object>& params);

inline sol::object FStructPropertyDescr::GetLuaTablePropertyValue(const FGetPropertyValueParams& params)
{
	FStructProperty* prop = CastField<FStructProperty>(params.Prop);
	FLuaTableHandle* luaValue = static_cast<FLuaTableHandle*>(params.MemoryPtr);
	return luaValue->GetTable();	
}

inline int FStructPropertyDescr::GetLuaTablePropertyValue(const FPushPropertyValueParams& params)
{
	FStructProperty* prop = CastField<FStructProperty>(params.Prop);
	FLuaTableHandle* luaValue = static_cast<FLuaTableHandle*>(params.MemoryPtr);
	return sol::stack::push(params.Lua, luaValue->GetTable());
}

template <typename LUAOBJ>
void FStructPropertyDescr::SetLuaTablePropertyValue(const TSetPropertyValueParams<LUAOBJ>& params)
{
	FStructProperty* prop = CastField<FStructProperty>(params.Prop);
	verify(prop->Struct == UnrealLua::StaticPackages::LuaFunction);
	
	FLuaTableHandle* luaValue = static_cast<FLuaTableHandle*>(params.MemoryPtr);
	if (params.LuaValue.valid() && params.LuaValue.get_type() == sol::type::table)
	{
		*luaValue = FLuaTableHandle::MakeHandle(params.LuaValue);
	}
	else
	{
		luaValue->Invalidate();
	}		
}

template void FStructPropertyDescr::SetLuaTablePropertyValue(const TSetPropertyValueParams<sol::object>& params);
template void FStructPropertyDescr::SetLuaTablePropertyValue(const TSetPropertyValueParams<sol::stack_object>& params);

inline uint32 FStructPropertyDescr::AddRef(FReferenceCollector& collector, FStructProperty* strProp, void* memory, bool container)
{
	uint32 numReferenced = 0;
	for (int n = 0; n < strProp->ArrayDim; ++n)
	{
		numReferenced += UnrealLua::PropertyHelper::AddRefByStruct(collector, strProp->Struct, memory);
	}
	collector.AddReferencedObject(strProp->Struct);
	
	return numReferenced;
}