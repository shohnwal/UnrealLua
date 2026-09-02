#include "Utility/UnrealVersion.h"
#if UNREALLUA_UE_VERSION_OLDER_THAN(5,8,0)
#include "InstancedStruct.h"
#endif
#include "Net/Core/PushModel/PushModel.h"
#include "Reflection/PropertyHelperTypes.h"
#include "Reflection/PropertyDescr/ArrayPropertyDescr.h"
#include "Reflection/PropertyDescr/EnumPropertyDescr.h"
#include "Reflection/PropertyDescr/FBoolPropertyDescr.h"
#include "Reflection/PropertyDescr/FMapPropertyDescr.h"
#include "Reflection/PropertyDescr/FNamePropertyDescr.h"
#include "Reflection/PropertyDescr/FNumericPropertyDescr.h"
#include "Reflection/PropertyDescr/FObjectPropertyDescr.h"
#include "Reflection/PropertyDescr/FVector2DPropertyDescr.h"
#include "Reflection/PropertyDescr/FVectorPropertyDescr.h"
#include "Reflection/PropertyDescr/MulticastDelegatePropertyDescr.h"
#include "Reflection/PropertyDescr/RotatorPropertyDescr.h"
#include "Reflection/PropertyDescr/SetPropertyDescr.h"
#include "Reflection/PropertyDescr/StrPropertyDescr.h"
#include "Reflection/PropertyDescr/StructPropertyDescr.h"
#include "Reflection/PropertyDescr/TextPropertyDescr.h"
#include "StringHandling/UnrealLuaStringCache.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"
//#include "Reflection/PropertyHelper_Utility.h"
#include "Config/UnrealLuaConfig.h"
#include "sol/sol.hpp"
#include "UObjectRegistry/LuaUObjectItem.h"


namespace UnrealLua::PropertyHelper
{
	template<typename LUAOBJ>
	void SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params)
	{
		//Setting read-only properties not allowed, unlss they are explicitly editable or a function parameter
		FProperty* prop = params.Prop; 
		if(prop->HasAnyPropertyFlags(CPF_BlueprintReadOnly) && !prop->HasAnyPropertyFlags(CPF_Parm))
		{
			LUA_LOG_WARNING("Attempting to write to read-only property %s", *prop->GetFullName())
			if(!UUnrealLuaConfig::ShouldAllowWriteOnReadOnlyProperties())
			{
				LUA_LOG_ERROR("Can't write to read-only property %s", *prop->GetFullName())
				return;			
			}
		}

		uint64 interestingFlags = prop->GetCastFlags() & supportedPropTypeFlags;

		if(!interestingFlags) [[unlikely]]
		{
			//no overlapping match with any supported properties
			LUA_LOG_ERROR("Can't set value for property %s, property type %s is not supported by UnrealLua", *prop->GetFullName(), *prop->GetCPPType())
			return;
		}

		ELuaSupportedClassCastFlags supportedProperty = static_cast<ELuaSupportedClassCastFlags>(interestingFlags & ~(interestingFlags - 1));

		switch(supportedProperty)
		{
		case LUA_CASTCLASS_FInt8Property:
			FNumericPropertyDescr<int8, FInt8Property>::SetPropertyValue(params);
			break;
		case LUA_CASTCLASS_FByteProperty:
			{
				const FByteProperty* bprop = CastField<FByteProperty>(params.Prop);
				if(bprop->Enum)
				{
					sol::type valueType = params.LuaValue.valid() ? params.LuaValue.get_type() : sol::type::nil; 
					if(valueType == sol::type::number)
					{
						int64 value = params.LuaValue.template as<int64>();
						if(bprop->Enum->IsValidEnumValue(value))
						{
							bprop->SetIntPropertyValue(params.MemoryPtr, value);
							return;
						}		
					}
					else if(valueType == sol::type::string)
					{
						FName name = UnrealLua::StringCache::GetFNameForStringLuaObject(params.LuaValue);
						int64 value = bprop->Enum->GetValueByName(name);
						if(value != INDEX_NONE)
						{
							bprop->SetIntPropertyValue(params.MemoryPtr, value);
							return;
						}
					}
				}
				else
				{
					FNumericPropertyDescr<uint8, FByteProperty>::SetPropertyValue(params);
				}
			}
			break;
		case LUA_CASTCLASS_FIntProperty:
			FNumericPropertyDescr<int32, FIntProperty>::SetPropertyValue(params);
			break;
		case LUA_CASTCLASS_FFloatProperty:
			FNumericPropertyDescr<float, FFloatProperty>::SetPropertyValue(params);
			break;
		case LUA_CASTCLASS_FUInt64Property:
			FNumericPropertyDescr<uint64, FUInt64Property>::SetPropertyValue(params);
			break;
		case LUA_CASTCLASS_FClassProperty:
			FUObjectPropertyDescr::SetClassPropertyValue(params);
			break;
		case LUA_CASTCLASS_FUInt32Property:
			FNumericPropertyDescr<uint32, FUInt32Property>::SetPropertyValue(params);
			break;
		case LUA_CASTCLASS_FInterfaceProperty:
			FUObjectPropertyDescr::SetInterfacePropertyValue(params);
			break;
		case LUA_CASTCLASS_FNameProperty:
			FNamePropertyDescr::SetPropertyValue(params);
			break;
		case LUA_CASTCLASS_FStrProperty:
			FStrPropertyDescr::SetPropertyValue(params);
			break;
		case LUA_CASTCLASS_FObjectProperty:
			FUObjectPropertyDescr::SetObjectPropertyValue(params);
			break;
		case LUA_CASTCLASS_FBoolProperty:
			FBoolPropertyDescr::SetPropertyValue(params);
			break;
		case LUA_CASTCLASS_FUInt16Property:
			FNumericPropertyDescr<uint16, FUInt16Property>::SetPropertyValue(params);
			break;
		case LUA_CASTCLASS_FStructProperty:
			{
				FStructProperty* sprop = CastField<FStructProperty>(params.Prop);
				const UScriptStruct* spropStruct = sprop->Struct;

				if (spropStruct->IsChildOf(UnrealLua::StaticPackages::VectorStruct))              
				{
					FVectorPropertyDescr::SetPropertyValue(params);
				}
				else if (sprop->Struct == UnrealLua::StaticPackages::Vector2DStruct)
				{
					FVector2DPropertyDescr::SetPropertyValue(params);
				}
				else if (sprop->Struct == UnrealLua::StaticPackages::RotatorStruct)
				{
					FRotatorPropertyDescr::SetPropertyValue(params);
				}
				else if(spropStruct == UnrealLua::StaticPackages::InstancedStruct)
				{
					FStructPropertyDescr::SetInstancedStructPropertyValue(params);
				}
				else if (spropStruct == UnrealLua::StaticPackages::SharedStruct)
				{
					FStructPropertyDescr::SetSharedStructPropertyValue(params);
				}
				else if (spropStruct == UnrealLua::StaticPackages::LuaValue)
				{
					FStructPropertyDescr::SetLuaValuePropertyValue(params);
				}
				else if (spropStruct == UnrealLua::StaticPackages::LuaFunction)
				{
					FStructPropertyDescr::SetLuaFunctionPropertyValue(params);
				}
				else if (spropStruct == UnrealLua::StaticPackages::LuaTable)
				{
					FStructPropertyDescr::SetLuaTablePropertyValue(params);
				}
				else
				{
					FStructPropertyDescr::SetPropertyValue(params);
				}
			}
			break;
		case LUA_CASTCLASS_FArrayProperty:
			FArrayPropertyDescr::SetPropertyValue(params);
			break;
		case LUA_CASTCLASS_FInt64Property:
			FNumericPropertyDescr<int64, FInt64Property>::SetPropertyValue(params);
			break;
		case LUA_CASTCLASS_FMulticastDelegateProperty:
			FMulticastDelegatePropertyDescr::SetPropertyValue(params);
			break;
		case LUA_CASTCLASS_FWeakObjectProperty:
			FUObjectPropertyDescr::SetWeakObjectPropertyValue(params);
			break;
		case LUA_CASTCLASS_FSoftObjectProperty:
			FUObjectPropertyDescr::SetSoftObjectPropertyValue(params);
			break;
		case LUA_CASTCLASS_FTextProperty:
			FTextPropertyDescr::SetPropertyValue(params);
			break;
		case LUA_CASTCLASS_FInt16Property:
			FNumericPropertyDescr<int16, FInt16Property>::SetPropertyValue(params);
			break;
		case LUA_CASTCLASS_FDoubleProperty:
			FNumericPropertyDescr<double, FDoubleProperty>::SetPropertyValue(params);
			break;
		case LUA_CASTCLASS_FMapProperty:
			UnrealLua::FMapPropertyDescr::SetPropertyValue(params);
			break;
		case LUA_CASTCLASS_FSetProperty:
			FSetPropertyDescr::SetPropertyValue(params);
			break;
		case LUA_CASTCLASS_FEnumProperty:
			FEnumPropertyDescr::SetPropertyValue(params);
			break;
		default: ;
		}
	}
}
template void UnrealLua::PropertyHelper::SetPropertyValue(const TSetPropertyValueParams<sol::object>& params);
template void UnrealLua::PropertyHelper::SetPropertyValue(const TSetPropertyValueParams<sol::stack_object>& params);


bool UnrealLua::PropertyHelper::SetValueInScriptStructProperty(const sol::stack_object& key, FLuaScriptStructBase& strct, const sol::stack_object& luaValue)
{
	//FCPUCycleTimer timer{"SetValueInScriptStructProperty"};
	//FPlatformMisc::Prefetch(strct.PropertyMapping);;
	if(!key.valid() || key.get_type() != sol::type::string) [[unlikely]]
	{
		return false;
	}
	//FCPUCycleTimer timer{"Setting UStruct Property"};
	std::string_view strv = key.as<std::string_view>();
	const FHashedFieldMapping* found = strct.PropertyMapping->FindMapping(strv);
	if(found && found->IsProperty())
	{
		FProperty* prop = found->GetProperty();
		//FPlatformMisc::Prefetch(prop);
		//FPlatformMisc::Prefetch(strct.GetMemory());
		TSetPropertyValueParams params{ prop, strct.GetMemory(), 0, luaValue };
		UnrealLua::PropertyHelper::SetPropertyValue_InContainer(params);
		return true;			
	}
	return false;
}

template<typename LUAOBJ>
bool UnrealLua::PropertyHelper::SetValueInUObjectProperty(const LUAOBJ& key, FLuaUObjectItem& item, const LUAOBJ& value)
{
	/*
		Tries to set a script value.
		If no script value can be found, try to emplace a wrapper and set the value afterward
		If no property can bbe found to create a wrapper, set the value directly

		This is expensive the first time a Lua value is set (1x script, 1x properties, 1x setting wrapper, 1x setting actual value,
		but afterwards the value is directly accessible through the LuaScriptValue (1x script)
	*/
	//FCPUCycleTimer timer{FString::Printf(TEXT("Setting UObject property %hs"), strv.data())};
	item.SetScriptValue(key, value);
	return true;
}

template
bool UnrealLua::PropertyHelper::SetValueInUObjectProperty(const sol::stack_object& key, FLuaUObjectItem& item, const sol::stack_object& value);

template
bool UnrealLua::PropertyHelper::SetValueInUObjectProperty(const sol::object& key, FLuaUObjectItem& item, const sol::object& value);

bool UnrealLua::PropertyHelper::SetValueInUObjectProperty(const sol::string_view& key, UObject* obj, const FProperty* sourceProperty, void* sourceValueAddress, bool bCallRepNotify)
{
	if(!IsValid(obj))
	{
		return false;
	}
	FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(obj);
	return SetValueInUObjectProperty(key, item, sourceProperty, sourceValueAddress, bCallRepNotify);
}

bool UnrealLua::PropertyHelper::SetValueInUObjectProperty(const sol::string_view& key, FLuaUObjectItem& item, const FProperty* sourceProperty, void* sourceValueAddress, bool bCallRepNotify)
{
	item.SetScriptValue(key, sourceProperty, sourceValueAddress, bCallRepNotify);
	return true;
}

bool UnrealLua::PropertyHelper::SetValueInUObjectProperty(const sol::string_view& key, FLuaUObjectItem& item, const FLuaValue& value, bool bCallRepNotify)
{
	item.SetScriptValue(key, value, bCallRepNotify);
	return true;
}

bool UnrealLua::PropertyHelper::SetValueInUObjectProperty(const sol::object& key, FLuaUObjectItem& item, const sol::object& value, bool bCallRepNotify)
{
	item.SetScriptValue(key, value, bCallRepNotify);
	return true;
}

bool UnrealLua::PropertyHelper::SetValueInUObjectProperty(const sol::string_view& key, FLuaUObjectItem& item, const sol::object& value, bool bCallRepNotify)
{
	item.SetScriptValue(key, value, bCallRepNotify);
	return true;
}


template <typename LUAOBJ>
bool UnrealLua::PropertyHelper::SetValueInUObjectProperty(FProperty* prop, UObject* obj, const LUAOBJ& value)
{
	//Assumes that it has been tested that mapping is referring to a Property
	
	TSetPropertyValueParams params{ prop, obj, 0, value};
	UnrealLua::PropertyHelper::SetPropertyValue_InContainer(params);
	//If property is net prop, mark dirty and call rep notify
	UnrealLua::PropertyHelper::HandleSetPropertyNetBehavior(obj, prop);
	return true;
}

template
bool UnrealLua::PropertyHelper::SetValueInUObjectProperty(FProperty* prop, UObject* obj, const sol::stack_object& value);

template
bool UnrealLua::PropertyHelper::SetValueInUObjectProperty(FProperty* prop, UObject* obj, const sol::object& value);

void UnrealLua::PropertyHelper::InitializeStructFromTable(const UScriptStruct* structMetaData, void* containerPtr, sol::table& table, bool resetStruct)
{
	verify(table.valid())
	if (resetStruct)
	{
		structMetaData->InitializeStruct(containerPtr);	
	}
	table.for_each([structMetaData, containerPtr](const sol::object key_o, const sol::object val_o)
	{
		if(key_o.get_type() != sol::type::string)
		{
			return;
		}
		const FName key = UnrealLua::StringCache::GetFNameForStringLuaObject(key_o);
		FProperty* prop = structMetaData->FindPropertyByName(key);
		if(prop)
		{
			//uint8* memLoc = prop->ContainerPtrToValuePtr<uint8>(containerPtr);
			TSetPropertyValueParams params{prop, containerPtr, 0, val_o };
			UnrealLua::PropertyHelper::SetPropertyValue_InContainer(params);
		}
	});
	//If there are array-like arguments in the table, initialize them in order
	int32 numNumericArgs = table.size(); 
	if(numNumericArgs > 0)
	{
		int32 index = 1;
		for(TFieldIterator<FProperty> propIt(structMetaData); propIt && index <= numNumericArgs; ++propIt)
		{
			TSetPropertyValueParams params{*propIt, containerPtr, 0, table[index].get<sol::object>() };
			UnrealLua::PropertyHelper::SetPropertyValue_InContainer(params);
			index++;
		}			
	}
}

void UnrealLua::PropertyHelper::InitializeUObjectFromTable(UObject* obj, const sol::table& initPropsTbl, bool resetProperties)
{
	if (UUnrealLuaEngineSubsystem::IsGameSessionActive())
	{
		FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(obj);
		if(!initPropsTbl.valid()) [[unlikely]]
		{
			return;
		}
		initPropsTbl.for_each([&item, obj](const sol::object& key, const sol::object& val)
		{
			UnrealLua::PropertyHelper::SetValueInUObjectProperty(key, item, val);
		});		
	}
	else
	{
		if(!initPropsTbl.valid() || !IsValid(obj)) [[unlikely]]
		{
			return;
		}
		initPropsTbl.for_each([obj](const sol::object& key_o, const sol::object& val_o)
		{
			if(key_o.get_type() != sol::type::string)
			{
				return;
			}
			sol::string_view strv = key_o.as<sol::string_view>();
			if (strv.empty())
			{
				return;
			}
			const FName key = StringCast<TCHAR>(strv.data()).Get();
			FProperty* prop = obj->GetClass()->FindPropertyByName(key);
			if(prop)
			{
				//uint8* memLoc = prop->ContainerPtrToValuePtr<uint8>(containerPtr);
				TSetPropertyValueParams params{prop, obj, 0, val_o };
				UnrealLua::PropertyHelper::SetPropertyValue_InContainer(params);
			}
		});
	}
}

template<typename LUAOBJ>
void UnrealLua::PropertyHelper::SetPropertyValue_InContainer(TSetPropertyValueParams<LUAOBJ>& params)
{
	FProperty* prop = params.Prop;
	params.MemoryPtr = prop->ContainerPtrToValuePtr<void>(params.MemoryPtr);
	UnrealLua::PropertyHelper::SetPropertyValue(params);
}

template void UnrealLua::PropertyHelper::SetPropertyValue_InContainer(TSetPropertyValueParams<sol::object>& params);
template void UnrealLua::PropertyHelper::SetPropertyValue_InContainer(TSetPropertyValueParams<sol::stack_object>& params);

template<typename LUAOBJ>
void UnrealLua::PropertyHelper::SetPropertyValue_Direct(const TSetPropertyValueParams<LUAOBJ>& params)
{
	UnrealLua::PropertyHelper::SetPropertyValue(params);
}

template void UnrealLua::PropertyHelper::SetPropertyValue_Direct(const TSetPropertyValueParams<sol::object>& params);
template void UnrealLua::PropertyHelper::SetPropertyValue_Direct(const TSetPropertyValueParams<sol::stack_object>& params);

void UnrealLua::PropertyHelper::HandleSetPropertyNetBehavior(UObject* Object, FProperty* prop)
{
	if(prop->HasAnyPropertyFlags(CPF_Net))
	{
#ifdef WITH_PUSH_MODEL
		//mark property dirty
		MARK_PROPERTY_DIRTY(Object, prop);
#endif
		if(prop->HasAnyPropertyFlags(CPF_RepNotify))
		{
			//call rep notify function
			UFunction* RepNotifyFunc = Object->FindFunction(prop->RepNotifyFunc);

			if (RepNotifyFunc == nullptr)
			{
				LUA_LOG("FRepLayout::CallRepNotifies: Can't find RepNotify function %s for property %s on object %s.",
					*prop->RepNotifyFunc.ToString(), *prop->GetName(), *Object->GetName());
				return;
			}
			const int32 NumParms = RepNotifyFunc->NumParms;
			if(NumParms == 0)
			{
				Object->ProcessEvent(RepNotifyFunc, nullptr);
			}
			else
			{
				//For now, function arguments will be default values, aka NOT initialized with the previous/old values
				void* functionArgsMemory = (uint8*)FMemory_Alloca_Aligned(RepNotifyFunc->ParmsSize, RepNotifyFunc->GetMinAlignment());
				FMemory::Memzero( functionArgsMemory, RepNotifyFunc->ParmsSize );
				RepNotifyFunc->InitializeStruct(functionArgsMemory);
				//Copy rep property value to function args 
				prop->CopySingleValue(functionArgsMemory, prop->ContainerPtrToValuePtr<void>(Object));

				Object->ProcessEvent(RepNotifyFunc, functionArgsMemory);

				for (FProperty* P = RepNotifyFunc->DestructorLink; P; P = P->DestructorLinkNext)
				{
					if (!P->IsInContainer(RepNotifyFunc->ParmsSize))
					{
						P->DestroyValue_InContainer(functionArgsMemory);
					}
				}
			}
		}
	}
}
