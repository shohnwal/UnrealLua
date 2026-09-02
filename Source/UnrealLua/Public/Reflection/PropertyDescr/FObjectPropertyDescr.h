// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Utility/LuaLogMacros.h"
#include "LuaTypes/LuaLightUserdata.h"
#include "LuaTypes/LuaSoftObjectWrapper.h"
#include "LuaTypes/LuaUClass.h"
#include "Reflection/PropertyHelperTypes.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"
#include "sol/sol.hpp"
class FReferenceCollector;
/**
 * 
 */
struct UNREALLUA_API FUObjectPropertyDescr
{
	template<typename TObjectProperty>
	static bool IsCompatibleType(FProperty* prop, const sol::object& obj);

	template<typename TObjectProperty>
	static sol::object GetPropertyValue(const FGetPropertyValueParams& params);

	template<typename TObjectProperty>
	static int GetPropertyValue(FPushPropertyValueParams& params);

	template<typename LUAOBJ>
	static void SetObjectPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params);
	template<typename LUAOBJ>
	static void SetWeakObjectPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params);
	template<typename LUAOBJ>
	static void SetSoftObjectPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params);
	template<typename LUAOBJ>
	static void SetClassPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params);
	template<typename LUAOBJ>
	static void SetInterfacePropertyValue(const TSetPropertyValueParams<LUAOBJ>& params);
	
	static uint32 AddRef(FObjectProperty* p, void* objectMemory, FReferenceCollector &collector, bool container = true);
	static uint32 AddRef(FClassProperty* p, void* objectMemory, FReferenceCollector &collector, bool container = true);
	static FString GetClassPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params);
	static FString GetInterfacePropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params);
	static FString GetObjectPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params);
	static FString GetWeakObjectPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params);
	static FString GetSoftObjectPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params);

private:
	FUObjectPropertyDescr() { }
};

template<typename T>
bool FUObjectPropertyDescr::IsCompatibleType(FProperty* p, const sol::object& obj_o)
{
	static_assert(!(std::is_same_v<T, FObjectProperty> || std::is_same_v<T, FInterfaceProperty> || std::is_same_v<T, FWeakObjectProperty> || std::is_same_v<T, FClassProperty>), "Can only use FObjectProperty or FWeakObjectProperty as template argument!");
	return false;
}

template<>
inline bool FUObjectPropertyDescr::IsCompatibleType<FObjectProperty>(FProperty* p, const sol::object& obj_o) 
{
	if(obj_o == sol::nil)
	{
		//nullptr is also a valid value
		return true;
	}
	FObjectProperty* prop = CastField<FObjectProperty>(p);
	UObject* obj = nullptr;

	if (obj_o.get_type() == sol::type::lightuserdata)
	{
		obj = UnrealLua::LightUserdata::GetUObject(obj_o);	
	}
	return obj && obj->IsA(prop->PropertyClass);
}

template<>
inline bool FUObjectPropertyDescr::IsCompatibleType<FInterfaceProperty>(FProperty* p, const sol::object& obj_o) 
{
	if(obj_o == sol::nil)
	{
		//nullptr is also a valid value, to set something to nullptr
		return true;
	}
	FInterfaceProperty* prop = CastField<FInterfaceProperty>(p);
	UObject* obj = nullptr;

	if (obj_o.get_type() == sol::type::lightuserdata)
	{
		obj = UnrealLua::LightUserdata::GetUObject(obj_o);	
	}
	if(!obj)
	{
		return false;
	}
	return obj->GetClass()->ImplementsInterface(prop->InterfaceClass);
}

template<>
inline bool FUObjectPropertyDescr::IsCompatibleType<FWeakObjectProperty>(FProperty* p, const sol::object& obj_o)
{
	if(obj_o == sol::nil)
	{
		//nullptr is also a valid value
		return true;
	}
	FWeakObjectProperty* prop = CastField<FWeakObjectProperty>(p);
	UObject* obj = nullptr;
	if (obj_o.get_type() == sol::type::lightuserdata)
	{
		obj = UnrealLua::LightUserdata::GetUObject(obj_o);	
	}
	return obj && obj->IsA(prop->PropertyClass);
}

template<>
inline bool FUObjectPropertyDescr::IsCompatibleType<FClassProperty>(FProperty* p, const sol::object& obj_o)
{
	if(obj_o == sol::nil)
	{
		//nullptr is also a valid value
		return true;
	}
	FClassProperty* prop = CastField<FClassProperty>(p);
	UClass* obj = nullptr;
	if (obj_o.is<FLuaUClass>())
	{
		obj = obj_o.as<FLuaUClass&>().TryLoadClass();
	}
	else if(obj_o.get_type() == sol::type::string)
	{
		sol::string_view v = obj_o.as<sol::string_view>();

		if (!v.empty())
		{
			//if beginning with U (UObjects) or A (AActor), try to find UClass in import-lua table first 
			if(v.starts_with("U") || v.starts_with("A"))
			{
				sol::state_view lua = obj_o.lua_state();
				sol::object clazzobj = sol::nil;
				clazzobj = lua["UE"][obj_o];
				if(clazzobj.is<FLuaUClass>())
				{
					obj = clazzobj.as<FLuaUClass&>().TryLoadClass();
				}
			}	
		}
	}
	return obj && obj->IsChildOf(prop->MetaClass);
}

template<typename T>
sol::object FUObjectPropertyDescr::GetPropertyValue(const FGetPropertyValueParams& params)
{
	static_assert(!(std::is_same_v<T, FObjectProperty> || std::is_same_v<T, FSoftObjectProperty> || std::is_same_v<T, FWeakObjectProperty> || std::is_same_v<T, FClassProperty>), "Can only use FObjectProperty or FWeakObjectProperty as template argument!");
	return sol::nil;
}

template<>
inline sol::object FUObjectPropertyDescr::GetPropertyValue<FObjectProperty>(const FGetPropertyValueParams& params)
{
	FObjectProperty* prop = CastField<FObjectProperty>(params.Prop);
	TObjectPtr<UObject> value = prop->GetObjectPtrPropertyValue(params.MemoryPtr);
	if(false && params.InputRecord)
	{
		/**
		 *@TODO : Not good, as this might overwrite/modify a cached FLuaUObjectWrapper in UObjectRegistry...
		 * Example:
		 * 
		 * -- Lua --
		 * function MyClass:Func()
		 *		--self might get assigned a new UObject -> break
		 *		self:CallFuncThatModifiesUObjectRef(self)
		 * end
		 *
		 * -- C++ --
		 * void CallFuncThatModifiesUObjectRef(UObject*& outRef) --outRef is a FLuaUObjectWrapper containing self
		 *		outRef = NewObject<UObject>()
		 */
		
		
		FUFunctionCallInputLuaObjectRecordItem* found = params.InputRecord->FindByPredicate([prop](const FUFunctionCallInputLuaObjectRecordItem& item)
		{
			return prop == item.InputProp;
		});
		if(found)
		{
			//If there is a valid input value for that prop and it has the same UScriptStruct
			sol::object inputObj = found->LuaObj;
			if(inputObj.valid())
			{
				if(value != nullptr && inputObj.is<UObject>())
				{
					//Copy output from BlueprintVM into input FLuaUObjectWrapper data, so it acts as an out param
					
					//FLuaUObjectWrapper& ss = inputObj.as<FLuaUObjectWrapper&>();
					//ss.Set(value);
					
					//and return it also as an output value
					//both luaValues will refer to the same FLuaUObjectWrapper
					//return inputObj;					
				}
				else
				{
					//can't reset the item itself, as reseting the inputObj wouldn't
					//modify the FLuaUObjectWrapper on the stack
					//inputObj.reset(); //nil the input value
				}
			}
		}
		//didnt find any input value record, will copy out value as return param	
	}
	if (value != nullptr)
	{
		return UnrealLua::UObjectRegistry::GetUObjectAsLightUserdata(params.Lua, value);
	}
	return sol::nil;
}

template<>
inline sol::object FUObjectPropertyDescr::GetPropertyValue<FWeakObjectProperty>(const FGetPropertyValueParams& params)
{
	FWeakObjectPtr ptr = static_cast<const FWeakObjectProperty*>(params.Prop)->GetPropertyValue(params.MemoryPtr);
	UObject* obj = ptr.Get();
	if (IsValid(obj))
	{
		return UnrealLua::UObjectRegistry::GetUObjectAsLightUserdata(params.Lua, obj);
	}
	return sol::nil;
}

template<>
inline sol::object FUObjectPropertyDescr::GetPropertyValue<FSoftObjectProperty>(const FGetPropertyValueParams& params)
{
	FSoftObjectProperty* prop = CastField<FSoftObjectProperty>(params.Prop);
	FSoftObjectPtr ptr = prop->GetPropertyValue(params.MemoryPtr);
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
			if(inputObj.valid())
			{
				if(inputObj.is<FLuaSoftObjectWrapper>())
				{
					//Copy output from BlueprintVM into input FLuaUObjectWrapper data, so it acts as an out param
					FLuaSoftObjectWrapper& ssw = inputObj.as<FLuaSoftObjectWrapper&>();
					ssw.Set(ptr);
					//and return it also as an output value
					//both luaValues will refer to the same FLuaUObjectWrapper
					return inputObj;					
				}
			}
		}
		//didnt find any input value record, will copy out value as return param	
	}

	return sol::object(params.Lua, sol::in_place_type<FLuaSoftObjectWrapper>, ptr);
}

template<>
inline sol::object FUObjectPropertyDescr::GetPropertyValue<FClassProperty>(const FGetPropertyValueParams& params)
{
	FClassProperty* prop = CastField<FClassProperty>(params.Prop); 
	UClass* value = Cast<UClass>(prop->GetObjectPropertyValue(params.MemoryPtr));
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
			if(inputObj.valid())
			{
				if(inputObj.is<FLuaUClass>())
				{
					//Copy output from BlueprintVM into input FLuaUObjectWrapper data, so it acts as an out param
					FLuaUClass& ssc = inputObj.as<FLuaUClass&>();
					ssc.Set(value);
					//and return it also as an output value
					//both luaValues will refer to the same FLuaUObjectWrapper
					return inputObj;					
				}
			}
		}
		//didnt find any input value record, will copy out value as return param	
	}


	if (IsValid(value))
	{
		return sol::object(params.Lua, sol::in_place_type<FLuaUClass>, value);//sol::make_object<UObject*>(lua, ret);
	}
	return sol::nil;
}

template<>
inline sol::object FUObjectPropertyDescr::GetPropertyValue<FInterfaceProperty>(const FGetPropertyValueParams& params)
{
	FInterfaceProperty* prop = CastField<FInterfaceProperty>(params.Prop);
	const FScriptInterface& i = prop->GetPropertyValue(params.MemoryPtr);

	UObject* value = i.GetObject();

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
			if(inputObj.valid())
			{
				//cant do out UObject-references for now
				/*
				if(value != nullptr && inputObj.is<FLuaUObjectWrapper>())
				{
					//Copy output from BlueprintVM into input FLuaUObjectWrapper data, so it acts as an out param
					FLuaUObjectWrapper& ssc = inputObj.as<FLuaUObjectWrapper&>();
					ssc.Set(value);
					//and return it also as an output value
					//both luaValues will refer to the same FLuaUObjectWrapper
					return inputObj;					
				}
				else
				{
					//inputObj.reset(); //nil the input value
				}
				*/
			}
		}
		//didnt find any input value record, will copy out value as return param	
	}
	
	if (IsValid(value))
	{
		return UnrealLua::UObjectRegistry::GetUObjectAsLightUserdata(params.Lua, value);
	}
	return sol::nil;
}


template<typename T>
int FUObjectPropertyDescr::GetPropertyValue(FPushPropertyValueParams& params)
{
	static_assert(!(std::is_same_v<T, FObjectProperty> || std::is_same_v<T, FSoftObjectProperty> || std::is_same_v<T, FWeakObjectProperty> || std::is_same_v<T, FClassProperty>), "Can only use FObjectProperty or FWeakObjectProperty as template argument!");
	return 0;
}

template<>
inline int FUObjectPropertyDescr::GetPropertyValue<FObjectProperty>(FPushPropertyValueParams& params)
{
	FObjectProperty* prop = CastField<FObjectProperty>(params.Prop);
	TObjectPtr<UObject> value = prop->GetObjectPtrPropertyValue(params.MemoryPtr);
	if(params.InputRecord)
	{
		FUFunctionCallInputLuaObjectRecordItem* found = params.InputRecord->FindByPredicate([prop](const FUFunctionCallInputLuaObjectRecordItem& item)
		{
			return prop == item.InputProp;
		});
		if(found)
		{
			//If there is a valid input value for that prop and it is a UObject...
			sol::object inputObj = found->LuaObj;
			if(inputObj.valid())
			{
				//@TODO : figure out how to do out-params for lightuserdata
			}
		}
		//didnt find any input value record, will copy out value as return param	
	}
	if (value != nullptr)
	{
		return UnrealLua::UObjectRegistry::PushUObjectAsLightUserdata(params.Lua, value);
	}
	return sol::stack::push(params.Lua, sol::nil);
}

template<>
inline int FUObjectPropertyDescr::GetPropertyValue<FWeakObjectProperty>(FPushPropertyValueParams& params)
{
	FWeakObjectPtr ptr = static_cast<const FWeakObjectProperty*>(params.Prop)->GetPropertyValue(params.MemoryPtr);
	UObject* obj = ptr.Get();
	if (IsValid(obj))
	{
		return UnrealLua::UObjectRegistry::PushUObjectAsLightUserdata(params.Lua, obj);
	}
	return sol::stack::push(params.Lua, sol::nil);
}

template<>
inline int FUObjectPropertyDescr::GetPropertyValue<FSoftObjectProperty>(FPushPropertyValueParams& params)
{
	FSoftObjectProperty* prop = CastField<FSoftObjectProperty>(params.Prop);
	FSoftObjectPtr ptr = prop->GetPropertyValue(params.MemoryPtr);
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
			if(inputObj.valid())
			{
				if(inputObj.is<FLuaSoftObjectWrapper>())
				{
					//Copy output from BlueprintVM into input FLuaUObjectWrapper data, so it acts as an out param
					FLuaSoftObjectWrapper& ssw = inputObj.as<FLuaSoftObjectWrapper&>();
					ssw.Set(ptr);
					//and return it also as an output value
					//both luaValues will refer to the same FLuaUObjectWrapper
					return sol::stack::push(params.Lua, inputObj);					
				}
			}
		}
		//didnt find any input value record, will copy out value as return param	
	}

	return sol::stack::push<FLuaSoftObjectWrapper>(params.Lua, ptr);
}

template<>
inline int FUObjectPropertyDescr::GetPropertyValue<FClassProperty>(FPushPropertyValueParams& params)
{
	FClassProperty* prop = CastField<FClassProperty>(params.Prop); 
	UClass* value = Cast<UClass>(prop->GetObjectPropertyValue(params.MemoryPtr));
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
			if(inputObj.valid())
			{
				if(inputObj.is<FLuaUClass>())
				{
					//Copy output from BlueprintVM into input FLuaUObjectWrapper data, so it acts as an out param
					FLuaUClass& ssc = inputObj.as<FLuaUClass&>();
					ssc.Set(value);
					//and return it also as an output value
					//both luaValues will refer to the same FLuaUObjectWrapper
					return sol::stack::push(params.Lua, inputObj);					
				}
			}
		}
		//didnt find any input value record, will copy out value as return param	
	}


	if (IsValid(value))
	{
		return sol::stack::push<FLuaUClass>(params.Lua, value);//sol::make_object<UObject*>(lua, ret);
	}
	return sol::stack::push(params.Lua, sol::nil);
}

template<>
inline int FUObjectPropertyDescr::GetPropertyValue<FInterfaceProperty>(FPushPropertyValueParams& params)
{
	FInterfaceProperty* prop = CastField<FInterfaceProperty>(params.Prop);
	const FScriptInterface& i = prop->GetPropertyValue(params.MemoryPtr);

	UObject* value = i.GetObject();

	if(params.InputRecord)
	{
		FUFunctionCallInputLuaObjectRecordItem* found = params.InputRecord->FindByPredicate([prop](const FUFunctionCallInputLuaObjectRecordItem& item)
		{
			return prop == item.InputProp;
		});
		if(found)
		{
			//If there is a valid input value for that prop and it is a UObject...
			sol::object inputObj = found->LuaObj;
			if(inputObj.valid())
			{
				//@TODO : figure out how to do out-params for lightuserdata
			}
		}
		//didnt find any input value record, will copy out value as return param	
	}
	
	if (IsValid(value))
	{
		return UnrealLua::UObjectRegistry::PushUObjectAsLightUserdata(params.Lua, value);
	}
	return sol::stack::push(params.Lua, sol::nil);
}
/*
template<typename T, typename LUAOBJ>
void FUObjectPropertyDescr::SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params)
{
	static_assert(!(std::is_same_v<T, sol::object> || std::is_same_v<T, sol::stack_object>), "Wroing order!");
	static_assert(!(std::is_same_v<T, FObjectProperty> || std::is_same_v<T, FSoftObjectProperty> || std::is_same_v<T, FWeakObjectProperty> || std::is_same_v<T, FClassProperty> || std::is_same_v<T, FInterfaceProperty>), "Can only use FObjectProperty or FWeakObjectProperty as template argument!");
}
*/

template<typename LUAOBJ>
void FUObjectPropertyDescr::SetObjectPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params)
{
	FObjectProperty* prop = CastField<FObjectProperty>(params.Prop);
	if(params.LuaValue == sol::nil || !params.LuaValue.valid())
	{
		//explicit nil -> nullptr
		prop->SetObjectPropertyValue(params.MemoryPtr, nullptr); 
	}	
	else
	{
		UObject* obj = nullptr;
		if (params.LuaValue.get_type() == sol::type::lightuserdata)
		{
			obj = UnrealLua::LightUserdata::GetUObject(params.LuaValue);
			if(prop->HasAllPropertyFlags(CPF_OutParm) && params.InputRecord)
			{
				params.InputRecord->Emplace(prop, params.LuaValue);
			}
		}
		else if (params.LuaValue.template is<FLuaSoftObjectWrapper>())
		{
			obj = params.LuaValue.template as<FLuaSoftObjectWrapper&>().Get();
		}
		
		if(obj && obj->IsA(prop->PropertyClass))
		{
			prop->SetObjectPropertyValue(params.MemoryPtr, obj);
		}
		else
		{
			//invalid value -> nullptr
			prop->SetObjectPropertyValue(params.MemoryPtr, nullptr);			
		}
	}
}

template
void FUObjectPropertyDescr::SetObjectPropertyValue(const TSetPropertyValueParams<sol::object>& params);
template
void FUObjectPropertyDescr::SetObjectPropertyValue(const TSetPropertyValueParams<sol::stack_object>& params);


template<typename LUAOBJ>
void FUObjectPropertyDescr::SetSoftObjectPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params)
{
	FSoftObjectProperty* prop = CastField<FSoftObjectProperty>(params.Prop);

	if(params.LuaValue.valid())
	{
		UObject* obj = nullptr;
		if (params.LuaValue.get_type() == sol::type::lightuserdata)
		{
			obj = UnrealLua::LightUserdata::GetUObject(params.LuaValue);	
		}
		else if (params.LuaValue.template is<FLuaSoftObjectWrapper>())
		{
			obj = params.LuaValue.template as<FLuaSoftObjectWrapper&>().Get();
		}
		if(obj && obj->IsA(prop->PropertyClass))
		{
			prop->SetObjectPropertyValue(params.MemoryPtr, obj);
		}
		else
		{
			//invalid value -> nullptr
			prop->SetObjectPropertyValue(params.MemoryPtr, nullptr);			
		}
	}
	else if(params.LuaValue == sol::nil)
	{
		//explicit nil -> nullptr
		prop->SetObjectPropertyValue(params.MemoryPtr, nullptr); 
	}	
}

template
void FUObjectPropertyDescr::SetSoftObjectPropertyValue(const TSetPropertyValueParams<sol::object>& params);

template
void FUObjectPropertyDescr::SetSoftObjectPropertyValue(const TSetPropertyValueParams<sol::stack_object>& params);


template<typename LUAOBJ>
void FUObjectPropertyDescr::SetClassPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params)
{
	FClassProperty* classProp = CastField<FClassProperty>(params.Prop);

	UClass* propClass = classProp->MetaClass;
	verify(propClass != nullptr);

	const LUAOBJ& val = params.LuaValue;

	if(val.valid())
	{
		if(val.template is<FLuaUClass>())
		{
			FLuaUClass& uclassval = val.template as<FLuaUClass&>(); 
			UClass* clazz = uclassval.TryLoadClass();
			if(clazz)
			{
				if(clazz->IsChildOf(propClass))
				{
					classProp->SetObjectPropertyValue(params.MemoryPtr, clazz);
				}
				else
				{
					LUA_LOG_ERROR("FLuaUClass::UClass %s is not a child class of %s for property %s", *clazz->GetName(), *classProp->MetaClass->GetName(), *classProp->GetName())
				}
			}
			else
			{
				LUA_LOG_ERROR("FLuaUClass::FSoftClassPath %s failed to load as TSubclass for class %s and property %s"  , *uclassval.TryLoadClass()->GetName(), *classProp->MetaClass->GetName(), *classProp->GetName())
			}
		
			return;
		}
		if (params.LuaValue.get_type() == sol::type::lightuserdata)
		{
			UObject* obj = UnrealLua::LightUserdata::GetUObject(params.LuaValue);
			if(obj)
			{
				UClass* clazz = obj ? obj->GetClass() : nullptr;
				if(clazz && clazz->IsChildOf(propClass))
				{
					classProp->SetObjectPropertyValue(params.MemoryPtr, obj);
				}
				return;
			}
		}
		if(val.get_type() == sol::type::string)
		{
			const FString name{val.template as<sol::string_view>().data()};

			if (name.IsEmpty())
			{
				//explicit zero string = nullptr
				classProp->SetObjectPropertyValue(params.MemoryPtr, nullptr);
				return;
			}

			UClass* clazz = nullptr;

			//if beginning with U (UObjects) or A (AActor), try to find UClass in import-lua table first 
			if(name.StartsWith("U", ESearchCase::CaseSensitive) || name.StartsWith("A", ESearchCase::CaseSensitive))
			{
				sol::state_view lua = val.lua_state();
				sol::object clazzobj = sol::nil;
				clazzobj = lua["UE"][val];
				if(clazzobj.is<FLuaUClass>())
				{
					clazz = clazzobj.as<FLuaUClass&>().TryLoadClass();
				}
			}
		
			if(!clazz)
			{
				//ConstructorHelpersInternal::FindOrLoadClass()
				clazz = FindObject<UClass>(nullptr, *name);
			}

			if(!clazz)
			{
				clazz = LoadObject<UClass>(nullptr, *name);
			}
		
			if(clazz)
			{
				if(clazz->IsChildOf(propClass))
				{
					classProp->SetObjectPropertyValue(params.MemoryPtr, clazz);
					return;
				}
				else
				{
					LUA_LOG_ERROR("UClass from string %s is not a child class of %s for property %s", *clazz->GetName(), *classProp->MetaClass->GetName(), *classProp->GetName())
				}
			}
			else
			{
				LUA_LOG_ERROR("FSoftClassPath from string %s is not a valid class as TSubclass for class %s and property %s"  , *name, *classProp->MetaClass->GetName(), *classProp->GetName())
			}
			return;
		}
	}
	//LUA_LOG_WARNING("Could not set UClass property value %s via provided properties", *classProp->GetFName().ToString())
	classProp->SetObjectPropertyValue(params.MemoryPtr, nullptr);
}
 
template
void FUObjectPropertyDescr::SetClassPropertyValue(const TSetPropertyValueParams<sol::object>& params);

template
void FUObjectPropertyDescr::SetClassPropertyValue(const TSetPropertyValueParams<sol::stack_object>& params);

template<typename LUAOBJ>
void FUObjectPropertyDescr::SetWeakObjectPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params)
{
	FWeakObjectProperty* prop = CastField<FWeakObjectProperty>(params.Prop);
	if (params.LuaValue.get_type() == sol::type::lightuserdata)
	{
		UObject* obj = UnrealLua::LightUserdata::GetUObject(params.LuaValue);
		if(obj && obj->IsA(prop->PropertyClass))
		{
			prop->SetObjectPropertyValue(params.MemoryPtr, obj);
			return;
		}
	}
	else if (params.LuaValue.template is<FLuaSoftObjectWrapper>())
	{
		UObject* obj = params.LuaValue.template as<FLuaSoftObjectWrapper&>().Get();
		if(obj && obj->IsA(prop->PropertyClass))
		{
			prop->SetObjectPropertyValue(params.MemoryPtr, obj);
			return;
		}
	}
	else if(params.LuaValue == sol::nil)
	{
		//explicit nil -> nullptr
		prop->SetObjectPropertyValue(params.MemoryPtr, nullptr); 
	}
	LUA_LOG_WARNING("Can't set UObject Property value : Passed in argument not compatible with Property class %s", *GetNameSafe(prop->PropertyClass))
	prop->SetObjectPropertyValue(params.MemoryPtr, nullptr);
}

template
void FUObjectPropertyDescr::SetWeakObjectPropertyValue(const TSetPropertyValueParams<sol::object>& params);
template
void FUObjectPropertyDescr::SetWeakObjectPropertyValue(const TSetPropertyValueParams<sol::stack_object>& params);

template<typename LUAOBJ>
void FUObjectPropertyDescr::SetInterfacePropertyValue(const TSetPropertyValueParams<LUAOBJ>& params)
{
	FInterfaceProperty* prop = CastField<FInterfaceProperty>(params.Prop);
	if(params.LuaValue.valid())
	{
		UObject* obj = nullptr;
		if (params.LuaValue.get_type() == sol::type::lightuserdata)
		{
			obj = UnrealLua::LightUserdata::GetUObject(params.LuaValue);	
		}
		if(obj && obj->GetClass()->ImplementsInterface(prop->InterfaceClass))
		{
			prop->SetPropertyValue(params.MemoryPtr, FScriptInterface{obj, obj->GetInterfaceAddress(prop->InterfaceClass)});		
		}
		else
		{
			//not valid argument -> nullptr
			prop->SetPropertyValue(params.MemoryPtr, FScriptInterface{nullptr, nullptr}); 
		}		
	}
	else if(params.LuaValue == sol::nil)
	{
		//explicit nil -> nullptr
		prop->SetPropertyValue(params.MemoryPtr, FScriptInterface{nullptr, nullptr}); 
	}
}

template
void FUObjectPropertyDescr::SetInterfacePropertyValue(const TSetPropertyValueParams<sol::object>& params);
template
void FUObjectPropertyDescr::SetInterfacePropertyValue(const TSetPropertyValueParams<sol::stack_object>& params);

inline uint32 FUObjectPropertyDescr::AddRef(FObjectProperty* p, void* objectMemory, FReferenceCollector& collector, bool container)
{
	uint32 numReferenced = 0;
	TObjectPtr<UObject>* obj = p->GetObjectPtrPropertyValuePtr(objectMemory);
	if(obj != nullptr)
	{
		collector.AddReferencedObject(*obj);
		if(IsValid(*obj))
		{
			//LUA_LOG("FUObjectPropertyDescr::AddRef %s", *GetNameSafe(*obj))
			numReferenced++;
		}
	}
	return numReferenced;
}


inline uint32 FUObjectPropertyDescr::AddRef(FClassProperty* p, void* objectMemory, FReferenceCollector& collector, bool container)
{
	uint32 numReferenced = 0;
	TObjectPtr<UObject>* obj = p->GetObjectPtrPropertyValuePtr(objectMemory);
	if(obj != nullptr)
	{		
		if(IsValid(*obj))
		{
			numReferenced++;;
		}
	}
	
	collector.AddReferencedObject(p->MetaClass);

	return numReferenced;
}

inline FString FUObjectPropertyDescr::GetClassPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params)
{
	return "nil";
}

inline FString FUObjectPropertyDescr::GetInterfacePropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params)
{
	return "nil";	
}

inline FString FUObjectPropertyDescr::GetObjectPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params)
{
	return "nil";
}

inline FString FUObjectPropertyDescr::GetWeakObjectPropertyValueAsLuaSyntaxValidString(
	const FGetPropertyValueAsLuaSyntaxStringParams& params)
{
	return "nil";
}

inline FString FUObjectPropertyDescr::GetSoftObjectPropertyValueAsLuaSyntaxValidString(
	const FGetPropertyValueAsLuaSyntaxStringParams& params)
{
	return "nil";
}

