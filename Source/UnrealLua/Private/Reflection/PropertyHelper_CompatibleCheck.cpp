#include "Utility/LuaLogMacros.h"
#include "Config/UnrealLuaConstants.h"
#include "Engine/NetSerialization.h"
#include "Reflection/PropertyHelper.h"
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

bool UnrealLua::PropertyHelper::IsCompatibleType(FProperty* prop, sol::object luaValue)
{
	/*
	const uint64 interestingFlags = prop->GetCastFlags() & supportedPropTypeFlags;
	const int index = std::_Countr_zero(interestingFlags) + 1; //0
	return isCombatibleTypeLUT[index](prop, luaValue);
	*/

	/*
	uint64 interestingFlags = params.Prop->GetCastFlags() & supportedPropTypeFlags;
	//Count how many 0's there are right of the first 1 to find property type.
	//If it's zero 0's before the first 1, it's castflag 0x........1, which is index 1 in the CASTFLAG lookup table (index 0 is CASTCLASS_None with no 1's), hence the "+1"
	int index = std::_Countr_zero(interestingFlags) + 1;
	SetPropertyValueLUT[index](params);
	*/
	//ELuaSupportedClassCastFlags interestingFlags = static_cast<ELuaSupportedClassCastFlags>(params.Prop->GetCastFlags() & supportedPropTypeFlags);
	//ELuaSupportedClassCastFlags interestingFlags = static_cast<ELuaSupportedClassCastFlags>(params.Prop->StaticClassCastFlagsPrivate() & supportedPropTypeFlags);
	uint64 interestingFlags = prop->GetCastFlags() & supportedPropTypeFlags;
	
	if(!interestingFlags)
	{
		//no overlapping match with any supported properties
		LUA_LOG_ERROR("Property %s not compatible by default, property type %s is not supported by UnrealLua", *prop->GetFullName(), *prop->GetCPPType())
		return false;
	}

	ELuaSupportedClassCastFlags supportedProperty = static_cast<ELuaSupportedClassCastFlags>(interestingFlags & ~(interestingFlags - 1));
	
	//count how many 0s are before the first 1
	//uint64 numLowerZeroes = std::_Countr_zero(interestingFlags);
	//ELuaSupportedClassCastFlags supportedProperty = static_cast<ELuaSupportedClassCastFlags>(1ull << numLowerZeroes);
	switch(supportedProperty)
	{
		case LUA_CASTCLASS_FInt8Property:
			return FNumericPropertyDescr<int8, FInt8Property>::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FByteProperty:
			return FNumericPropertyDescr<uint8, FByteProperty>::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FIntProperty:
			return FNumericPropertyDescr<int32, FIntProperty>::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FFloatProperty:
			return FNumericPropertyDescr<float, FFloatProperty>::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FUInt64Property:
			return FNumericPropertyDescr<uint64, FUInt64Property>::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FClassProperty:
			return FUObjectPropertyDescr::IsCompatibleType<FClassProperty>(prop, luaValue);
		case LUA_CASTCLASS_FUInt32Property:
			return FNumericPropertyDescr<uint32, FUInt32Property>::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FInterfaceProperty:
			return FUObjectPropertyDescr::IsCompatibleType<FInterfaceProperty>(prop, luaValue);
		case LUA_CASTCLASS_FNameProperty:
			return FNamePropertyDescr::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FStrProperty:
			return FStrPropertyDescr::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FObjectProperty:
			return FUObjectPropertyDescr::IsCompatibleType<FObjectProperty>(prop, luaValue);
		case LUA_CASTCLASS_FBoolProperty:
			return FBoolPropertyDescr::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FUInt16Property:
			return FNumericPropertyDescr<uint16, FUInt16Property>::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FStructProperty:
			{
				const FStructProperty* sprop = CastField<FStructProperty>(prop);
				const UScriptStruct* const spropStruct = sprop->Struct;
                
				static UPackage* CoreUObjectPackage = UObject::StaticClass()->GetOutermost();
				if (spropStruct == UnrealLua::StaticPackages::VectorStruct || spropStruct == FVector_NetQuantize::StaticStruct() || spropStruct == FVector_NetQuantize10::StaticStruct() || spropStruct == FVector_NetQuantize100::StaticStruct() || spropStruct == FVector_NetQuantizeNormal::StaticStruct())              
				{
					return FVectorPropertyDescr::IsCompatibleType(prop, luaValue);
				}
				if (sprop->Struct == UnrealLua::StaticPackages::Vector2DStruct)
				{
					return FVector2DPropertyDescr::IsCompatibleType(prop, luaValue);
				}
				if (sprop->Struct == UnrealLua::StaticPackages::RotatorStruct)
				{
					return FRotatorPropertyDescr::IsCompatibleType(prop, luaValue);
				}
				if(sprop->Struct == UnrealLua::StaticPackages::SharedStruct)
				{
					return FStructPropertyDescr::IsCompatibleType(prop, luaValue);
				}
				if(sprop->Struct == UnrealLua::StaticPackages::InstancedStruct)
				{
					return FStructPropertyDescr::IsCompatibleType(prop, luaValue);
				}
				return FStructPropertyDescr::IsCompatibleType(prop, luaValue);
			}
		case LUA_CASTCLASS_FArrayProperty:
			return FArrayPropertyDescr::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FInt64Property:
			return FNumericPropertyDescr<int64, FInt64Property>::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FMulticastDelegateProperty:
			return FMulticastDelegatePropertyDescr::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FWeakObjectProperty:
			return FUObjectPropertyDescr::IsCompatibleType<FWeakObjectProperty>(prop, luaValue);
		case LUA_CASTCLASS_FSoftObjectProperty:
			return FUObjectPropertyDescr::IsCompatibleType<FSoftObjectProperty>(prop, luaValue);
		case LUA_CASTCLASS_FTextProperty:
			return FTextPropertyDescr::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FInt16Property:
			return FNumericPropertyDescr<int16, FInt16Property>::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FDoubleProperty:
			return FNumericPropertyDescr<double, FDoubleProperty>::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FMapProperty:
			return UnrealLua::FMapPropertyDescr::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FSetProperty:
			return FSetPropertyDescr::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FEnumProperty:
			return FEnumPropertyDescr::IsCompatibleType(prop, luaValue);
		
		default:
				return false;
	}
}


bool UnrealLua::PropertyHelper::IsCompatibleType(FProperty* prop, sol::stack_object luaValue)
{
	/*
	const uint64 interestingFlags = prop->GetCastFlags() & supportedPropTypeFlags;
	const int index = std::_Countr_zero(interestingFlags) + 1; //0
	return isCombatibleTypeLUT[index](prop, luaValue);
	*/

	/*
	uint64 interestingFlags = params.Prop->GetCastFlags() & supportedPropTypeFlags;
	//Count how many 0's there are right of the first 1 to find property type.
	//If it's zero 0's before the first 1, it's castflag 0x........1, which is index 1 in the CASTFLAG lookup table (index 0 is CASTCLASS_None with no 1's), hence the "+1"
	int index = std::_Countr_zero(interestingFlags) + 1;
	SetPropertyValueLUT[index](params);
	*/
	//ELuaSupportedClassCastFlags interestingFlags = static_cast<ELuaSupportedClassCastFlags>(params.Prop->GetCastFlags() & supportedPropTypeFlags);
	//ELuaSupportedClassCastFlags interestingFlags = static_cast<ELuaSupportedClassCastFlags>(params.Prop->StaticClassCastFlagsPrivate() & supportedPropTypeFlags);
	uint64 interestingFlags = prop->GetCastFlags() & supportedPropTypeFlags;
	
	if(!interestingFlags)
	{
		//no overlapping match with any supported properties
		LUA_LOG_ERROR("Property %s not compatible by default, property type %s is not supported by UnrealLua", *prop->GetFullName(), *prop->GetCPPType())
		return false;
	}

	ELuaSupportedClassCastFlags supportedProperty = static_cast<ELuaSupportedClassCastFlags>(interestingFlags & ~(interestingFlags - 1));
	
	//count how many 0s are before the first 1
	//uint64 numLowerZeroes = std::_Countr_zero(interestingFlags);
	//ELuaSupportedClassCastFlags supportedProperty = static_cast<ELuaSupportedClassCastFlags>(1ull << numLowerZeroes);
	switch(supportedProperty)
	{
		case LUA_CASTCLASS_FInt8Property:
			return FNumericPropertyDescr<int8, FInt8Property>::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FByteProperty:
			return FNumericPropertyDescr<uint8, FByteProperty>::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FIntProperty:
			return FNumericPropertyDescr<int32, FIntProperty>::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FFloatProperty:
			return FNumericPropertyDescr<float, FFloatProperty>::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FUInt64Property:
			return FNumericPropertyDescr<uint64, FUInt64Property>::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FClassProperty:
			return FUObjectPropertyDescr::IsCompatibleType<FClassProperty>(prop, luaValue);
		case LUA_CASTCLASS_FUInt32Property:
			return FNumericPropertyDescr<uint32, FUInt32Property>::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FInterfaceProperty:
			return FUObjectPropertyDescr::IsCompatibleType<FInterfaceProperty>(prop, luaValue);
		case LUA_CASTCLASS_FNameProperty:
			return FNamePropertyDescr::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FStrProperty:
			return FStrPropertyDescr::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FObjectProperty:
			return FUObjectPropertyDescr::IsCompatibleType<FObjectProperty>(prop, luaValue);
		case LUA_CASTCLASS_FBoolProperty:
			return FBoolPropertyDescr::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FUInt16Property:
			return FNumericPropertyDescr<uint16, FUInt16Property>::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FStructProperty:
			{
				const FStructProperty* sprop = CastField<FStructProperty>(prop);
				const UScriptStruct* const spropStruct = sprop->Struct;
                
				static UPackage* CoreUObjectPackage = UObject::StaticClass()->GetOutermost();
				if (spropStruct == UnrealLua::StaticPackages::VectorStruct || spropStruct == FVector_NetQuantize::StaticStruct() || spropStruct == FVector_NetQuantize10::StaticStruct() || spropStruct == FVector_NetQuantize100::StaticStruct() || spropStruct == FVector_NetQuantizeNormal::StaticStruct())              
				{
					return FVectorPropertyDescr::IsCompatibleType(prop, luaValue);
				}
				if (sprop->Struct == UnrealLua::StaticPackages::Vector2DStruct)
				{
					return FVector2DPropertyDescr::IsCompatibleType(prop, luaValue);
				}
				if (sprop->Struct == UnrealLua::StaticPackages::RotatorStruct)
				{
					return FRotatorPropertyDescr::IsCompatibleType(prop, luaValue);
				}
				if(sprop->Struct == UnrealLua::StaticPackages::SharedStruct)
				{
					return FStructPropertyDescr::IsCompatibleType(prop, luaValue);
				}
				if(sprop->Struct == UnrealLua::StaticPackages::InstancedStruct)
				{
					return FStructPropertyDescr::IsCompatibleType(prop, luaValue);
				}
				return FStructPropertyDescr::IsCompatibleType(prop, luaValue);
			}
		case LUA_CASTCLASS_FArrayProperty:
			return FArrayPropertyDescr::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FInt64Property:
			return FNumericPropertyDescr<int64, FInt64Property>::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FMulticastDelegateProperty:
			return FMulticastDelegatePropertyDescr::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FWeakObjectProperty:
			return FUObjectPropertyDescr::IsCompatibleType<FWeakObjectProperty>(prop, luaValue);
		case LUA_CASTCLASS_FSoftObjectProperty:
			return FUObjectPropertyDescr::IsCompatibleType<FSoftObjectProperty>(prop, luaValue);
		case LUA_CASTCLASS_FTextProperty:
			return FTextPropertyDescr::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FInt16Property:
			return FNumericPropertyDescr<int16, FInt16Property>::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FDoubleProperty:
			return FNumericPropertyDescr<double, FDoubleProperty>::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FMapProperty:
			return UnrealLua::FMapPropertyDescr::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FSetProperty:
			return FSetPropertyDescr::IsCompatibleType(prop, luaValue);
		case LUA_CASTCLASS_FEnumProperty:
			return FEnumPropertyDescr::IsCompatibleType(prop, luaValue);
		
		default:
				return false;
	}	
}
