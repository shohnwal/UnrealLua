#include "Reflection/PropertyHelper_ToString.h"

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
#include "Utility/LuaLogMacros.h"

FString UnrealLua::PropertyHelper::GetPropertyValueAsLuaSyntaxValidString_InContainer(FGetPropertyValueAsLuaSyntaxStringParams& params)
{
	void* valuePtr = params.Prop->ContainerPtrToValuePtr<void>(params.MemoryPtr);
	params.MemoryPtr = valuePtr;
	return GetPropertyValueAsLuaSyntaxValidString(params);
}

FString UnrealLua::PropertyHelper::GetPropertyValueAsLuaSyntaxValidString(FGetPropertyValueAsLuaSyntaxStringParams& params)
{
		//Setting read-only properties not allowed, unlss they are explicitly editable or a function parameter
		FProperty* prop = params.Prop; 

	uint64 interestingFlags = prop->GetCastFlags() & supportedPropTypeFlags;

		if(!interestingFlags) [[unlikely]]
		{
			//no overlapping match with any supported properties
			LUA_LOG_ERROR("Can't get value strong for property %s, property type %s is not supported by UnrealLua", *prop->GetFullName(), *prop->GetCPPType())
			return "nil";
		}

		ELuaSupportedClassCastFlags supportedProperty = static_cast<ELuaSupportedClassCastFlags>(interestingFlags & ~(interestingFlags - 1));

		switch(supportedProperty)
		{
		case LUA_CASTCLASS_FInt8Property:
			return FNumericPropertyDescr<int8, FInt8Property>::GetPropertyValueAsLuaSyntaxValidString(params);
		case LUA_CASTCLASS_FByteProperty:
			{
				const FByteProperty* bprop = CastField<FByteProperty>(params.Prop);
				if(bprop->Enum)
				{
					UEnum* uenum = bprop->Enum;
					uint64 val = bprop->GetUnsignedIntPropertyValue(params.MemoryPtr);
					return uenum->GetNameByValue(val).ToString();
				}
				else
				{
					return  FNumericPropertyDescr<uint8, FByteProperty>::GetPropertyValueAsLuaSyntaxValidString(params);
				}
			}
		case LUA_CASTCLASS_FIntProperty:
			return FNumericPropertyDescr<int32, FIntProperty>::GetPropertyValueAsLuaSyntaxValidString(params);
		case LUA_CASTCLASS_FFloatProperty:
			return FNumericPropertyDescr<float, FFloatProperty>::GetPropertyValueAsLuaSyntaxValidString(params);
		case LUA_CASTCLASS_FUInt64Property:
			return FNumericPropertyDescr<uint64, FUInt64Property>::GetPropertyValueAsLuaSyntaxValidString(params);
			
		case LUA_CASTCLASS_FClassProperty:
			return FUObjectPropertyDescr::GetClassPropertyValueAsLuaSyntaxValidString(params);
		case LUA_CASTCLASS_FUInt32Property:
			return FNumericPropertyDescr<uint32, FUInt32Property>::GetPropertyValueAsLuaSyntaxValidString(params);
		case LUA_CASTCLASS_FInterfaceProperty:
			return FUObjectPropertyDescr::GetInterfacePropertyValueAsLuaSyntaxValidString(params);
		case LUA_CASTCLASS_FNameProperty:
			return FNamePropertyDescr::GetPropertyValueAsLuaSyntaxValidString(params);
		case LUA_CASTCLASS_FStrProperty:
			return FStrPropertyDescr::GetPropertyValueAsLuaSyntaxValidString(params);
		case LUA_CASTCLASS_FObjectProperty:
			return FUObjectPropertyDescr::GetObjectPropertyValueAsLuaSyntaxValidString(params);
		case LUA_CASTCLASS_FBoolProperty:
			return FBoolPropertyDescr::GetPropertyValueAsLuaSyntaxValidString(params);
		case LUA_CASTCLASS_FUInt16Property:
			return FNumericPropertyDescr<uint16, FUInt16Property>::GetPropertyValueAsLuaSyntaxValidString(params);
		case LUA_CASTCLASS_FStructProperty:
			{
				FStructProperty* sprop = CastField<FStructProperty>(params.Prop);
				const UScriptStruct* spropStruct = sprop->Struct;

				if (spropStruct->IsChildOf(UnrealLua::StaticPackages::VectorStruct))              
				{
					return FVectorPropertyDescr::GetPropertyValueAsLuaSyntaxValidString(params);
				}
				else if (sprop->Struct == UnrealLua::StaticPackages::Vector2DStruct)
				{
					return FVector2DPropertyDescr::GetPropertyValueAsLuaSyntaxValidString(params);
				}
				else if (sprop->Struct == UnrealLua::StaticPackages::RotatorStruct)
				{
					return FRotatorPropertyDescr::GetPropertyValueAsLuaSyntaxValidString(params);
				}
				else if(spropStruct == UnrealLua::StaticPackages::InstancedStruct)
				{
					return FStructPropertyDescr::GetInstancedStructPropertyValueAsLuaSyntaxValidString(params);
				}
				else if (spropStruct == UnrealLua::StaticPackages::SharedStruct)
				{
					return FStructPropertyDescr::GetSharedStructPropertyValueAsLuaSyntaxValidString(params);
				}
				else if (spropStruct == UnrealLua::StaticPackages::LuaValue)
				{
					return FStructPropertyDescr::GetLuaValuePropertyValueAsLuaSyntaxValidString(params);
				}
				else if (spropStruct == UnrealLua::StaticPackages::LuaFunction)
				{
					return FStructPropertyDescr::GetLuaFunctionPropertyValueAsLuaSyntaxValidString(params);
				}
				else if (spropStruct == UnrealLua::StaticPackages::LuaTable)
				{
					return FStructPropertyDescr::GetLuaTablePropertyValueAsLuaSyntaxValidString(params);
				}
				else
				{
					return FStructPropertyDescr::GetPropertyValueAsLuaSyntaxValidString(params);
				}
			}
			break;
		case LUA_CASTCLASS_FArrayProperty:
			return FArrayPropertyDescr::GetPropertyValueAsLuaSyntaxValidString(params);
			break;
		case LUA_CASTCLASS_FInt64Property:
			return FNumericPropertyDescr<int64, FInt64Property>::GetPropertyValueAsLuaSyntaxValidString(params);
			break;
		case LUA_CASTCLASS_FMulticastDelegateProperty:
			return FMulticastDelegatePropertyDescr::GetPropertyValueAsLuaSyntaxValidString(params);
			break;
		case LUA_CASTCLASS_FWeakObjectProperty:
			return FUObjectPropertyDescr::GetWeakObjectPropertyValueAsLuaSyntaxValidString(params);
			break;
		case LUA_CASTCLASS_FSoftObjectProperty:
			return FUObjectPropertyDescr::GetSoftObjectPropertyValueAsLuaSyntaxValidString(params);
			break;
		case LUA_CASTCLASS_FTextProperty:
			return FTextPropertyDescr::GetPropertyValueAsLuaSyntaxValidString(params);
			break;
		case LUA_CASTCLASS_FInt16Property:
			return FNumericPropertyDescr<int16, FInt16Property>::GetPropertyValueAsLuaSyntaxValidString(params);
			break;
		case LUA_CASTCLASS_FDoubleProperty:
			return FNumericPropertyDescr<double, FDoubleProperty>::GetPropertyValueAsLuaSyntaxValidString(params);
			break;
		case LUA_CASTCLASS_FMapProperty:
			return UnrealLua::FMapPropertyDescr::GetPropertyValueAsLuaSyntaxValidString(params);
			break;
		case LUA_CASTCLASS_FSetProperty:
			return FSetPropertyDescr::GetPropertyValueAsLuaSyntaxValidString(params);
			break;
		case LUA_CASTCLASS_FEnumProperty:
			return FEnumPropertyDescr::GetPropertyValueAsLuaSyntaxValidString(params);
			break;
		default: ;
		}
	return "nil";
}
