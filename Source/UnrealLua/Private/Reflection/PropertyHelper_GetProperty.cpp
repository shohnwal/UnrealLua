#include "Utility/UnrealVersion.h"
#if UNREALLUA_UE_VERSION_OLDER_THAN(5,8,0)
#include "InstancedStruct.h"
#endif
#include "Config/UnrealLuaConstants.h"
#include "Config/UnrealLua_CompilerFlags.h"
#include "LuaValue/LuaTable.h"
#include "LuaValue/LuaFunction.h"
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
#include "Reflection/PropertyDescr/SingleDelegatePropertyDescr.h"
#include "Reflection/PropertyDescr/StrPropertyDescr.h"
#include "Reflection/PropertyDescr/StructPropertyDescr.h"
#include "Reflection/PropertyDescr/TextPropertyDescr.h"


template<typename LUAOBJ>
sol::object UnrealLua::PropertyHelper::GetValueFromScriptStructProperty(const LUAOBJ& key, FLuaScriptStructBase& strct, sol::this_state lua)
{
	//FPlatformMisc::Prefetch(strct.PropertyMapping);
	if(!key.valid() || key.get_type() != sol::type::string) [[unlikely]]
	{
		return sol::nil;
	}

	void* mem = strct.GetMemory();
	
	if(!mem) [[unlikely]]
	{
		return sol::nil;
	}
//	FPlatformMisc::Prefetch(mem);
	sol::string_view strv = key.template as<sol::string_view>();

//	FCPUCycleTimer timer{FString("GetValueFromScriptStructProperty ") + strv.data()};
	
	const FHashedFieldMapping* found = strct.PropertyMapping->FindMapping(strv);
	if(found) //ScriptStructs only have properties, no need to check for found type (func or prop)
	{
		if(found->IsProperty())
		{
			FProperty* prop = found->GetProperty();
			//FPlatformMisc::Prefetch(prop);
			FGetPropertyValueParams params{prop, mem, 0, lua };
			return UnrealLua::PropertyHelper::GetPropertyValue_InContainer(params);
		}
		else if(UnrealLua::Compilation::WITH_SCRIPTSTRUCT_FUNCTION_LIBS)
		{
			verify(found->IsFunction())
			const FFunctionDescr* func = found->GetFunction();
			verify(IsValid(func->Func))
			verify(func->Func->IsValidLowLevel())
			return UnrealLua::LightUserdata::MakeFFunctionDescrReferenceObject(lua, func);
		}	
	}
	return sol::nil;	
}

template
sol::object UnrealLua::PropertyHelper::GetValueFromScriptStructProperty(const sol::object& key, FLuaScriptStructBase& strct, sol::this_state lua);
template 
sol::object UnrealLua::PropertyHelper::GetValueFromScriptStructProperty(const sol::stack_object& key, FLuaScriptStructBase& strct, sol::this_state lua);

int UnrealLua::PropertyHelper::GetValueFromScriptStructProperty(const sol::stack_object& key, FLuaScriptStructBase& strct)
{
	//FPlatformMisc::Prefetch(strct.PropertyMapping);
	sol::this_state lua{key.lua_state()};
	if(!key.valid() || key.get_type() != sol::type::string) [[unlikely]]
	{
		return sol::stack::push(lua, sol::nil);
	}

	void* mem = strct.GetMemory();
	
	if(!mem)
	{
		return sol::stack::push(lua, sol::nil);
	}
	//FPlatformMisc::Prefetch(mem);
	sol::string_view strv = key.template as<sol::string_view>();
	
	//FCPUCycleTimer timer{FString("PushValueFromScriptStructProperty ") + strv.data()};
	
	const FHashedFieldMapping* found = strct.PropertyMapping->FindMapping(strv);
	if(found)
	{
		if(found->IsProperty())
		{
			FProperty* prop = found->GetProperty();
			FPushPropertyValueParams params{prop, mem, 0, lua };
			return UnrealLua::PropertyHelper::GetPropertyValue_InContainer(params);
		}
		else if (UnrealLua::Compilation::WITH_SCRIPTSTRUCT_FUNCTION_LIBS)
		{
			verify(found->IsFunction())
			return UnrealLua::LightUserdata::PushFFunctionDescrReferenceObject(lua, found->GetFunction());
		}	
	}
	return sol::stack::push(lua, sol::nil);
}


sol::object UnrealLua::PropertyHelper::GetPropertyValue_InContainer(FGetPropertyValueParams& params)
{
	FProperty* prop = params.Prop;
	params.MemoryPtr = prop->ContainerPtrToValuePtr<void>(params.MemoryPtr);
	return UnrealLua::PropertyHelper::GetPropertyValue(params);
}

sol::object UnrealLua::PropertyHelper::GetPropertyValue(const FGetPropertyValueParams& params)
{
	uint64 interestingFlags = params.Prop->GetCastFlags() & supportedPropTypeFlags;
	if(!interestingFlags) [[unlikely]]
	{
		//no overlapping match with any supported properties
		LUA_LOG_ERROR("Can't get value for property %s, property type %s is not supported by UnrealLua", *params.Prop->GetFullName(), *params.Prop->GetCPPType())
		return sol::nil;
	}

	ELuaSupportedClassCastFlags supportedProperty = static_cast<ELuaSupportedClassCastFlags>(interestingFlags & ~(interestingFlags - 1));
	
	switch(supportedProperty)
	{
	case LUA_CASTCLASS_FInt8Property:
		return FNumericPropertyDescr<uint8, FByteProperty>::GetPropertyValue(params);
	case LUA_CASTCLASS_FByteProperty:
		{
			const FByteProperty* prop = CastField<FByteProperty>(params.Prop);
			if(prop->Enum)
			{
				return FEnumPropertyDescr::GetBytePropertyValue(params);
			}
			else
			{
				return FNumericPropertyDescr<uint8, FByteProperty>::GetPropertyValue(params);		
			}
		}
	case LUA_CASTCLASS_FIntProperty:
		return FNumericPropertyDescr<int32, FIntProperty>::GetPropertyValue(params);
	case LUA_CASTCLASS_FFloatProperty:
		return FNumericPropertyDescr<float, FFloatProperty>::GetPropertyValue(params);
	case LUA_CASTCLASS_FUInt64Property:
		return FNumericPropertyDescr<uint64, FUInt64Property>::GetPropertyValue(params);
	case LUA_CASTCLASS_FClassProperty:
		return FUObjectPropertyDescr::GetPropertyValue<FClassProperty>(params);
	case LUA_CASTCLASS_FUInt32Property:
		return FNumericPropertyDescr<uint32, FUInt32Property>::GetPropertyValue(params);
	case LUA_CASTCLASS_FInterfaceProperty:
		return FUObjectPropertyDescr::GetPropertyValue<FInterfaceProperty>(params);
	case LUA_CASTCLASS_FNameProperty:
		return FNamePropertyDescr::GetPropertyValue(params);
	case LUA_CASTCLASS_FStrProperty:
		return FStrPropertyDescr::GetPropertyValue(params);
	case LUA_CASTCLASS_FObjectProperty:
		return FUObjectPropertyDescr::GetPropertyValue<FObjectProperty>(params);
	case LUA_CASTCLASS_FBoolProperty:
		return FBoolPropertyDescr::GetPropertyValue(params);
	case LUA_CASTCLASS_FUInt16Property:
		return FNumericPropertyDescr<uint16, FUInt16Property>::GetPropertyValue(params);
	case LUA_CASTCLASS_FStructProperty:
		{
			FStructProperty* sprop = const_cast<FStructProperty*>(CastField<FStructProperty>(params.Prop));
			UScriptStruct* spropStruct = sprop->Struct;

			//will also check FVector_NetQuantize::StaticStruct and the other Vector_NetQuantize versions
			if (spropStruct->IsChildOf(UnrealLua::StaticPackages::VectorStruct))              
			{
				return FVectorPropertyDescr::GetPropertyValue(params);
			}
			if (spropStruct == UnrealLua::StaticPackages::Vector2DStruct)
			{
				return FVector2DPropertyDescr::GetPropertyValue(params);
			}
			if (spropStruct == UnrealLua::StaticPackages::RotatorStruct)
			{
				return FRotatorPropertyDescr::GetPropertyValue(params);
			}
			if(spropStruct == UnrealLua::StaticPackages::SharedStruct)
			{
				return FStructPropertyDescr::GetSharedStructPropertyValue(params);
			}
			if(spropStruct == UnrealLua::StaticPackages::InstancedStruct)
			{
				return FStructPropertyDescr::GetInstancedStructPropertyValue(params);
			}
			if (spropStruct == UnrealLua::StaticPackages::LuaValue)
			{
				return FStructPropertyDescr::GetLuaValuePropertyValue(params);
			}
			else if (spropStruct == UnrealLua::StaticPackages::LuaFunction)
			{
				return FStructPropertyDescr::GetLuaFunctionPropertyValue(params);
			}
			else if (spropStruct == UnrealLua::StaticPackages::LuaTable)
			{
				return FStructPropertyDescr::GetLuaTablePropertyValue(params);
			}
			if(UnrealLua::PropertyHelper::OnGetCustomStructPropertyValue.IsBound())
			{
				sol::object output{};
				if(UnrealLua::PropertyHelper::OnGetCustomStructPropertyValue.Execute(params, spropStruct, &output))
				{
					return output;
				}
			}
			return FStructPropertyDescr::GetPropertyValue(params);
		}
	case LUA_CASTCLASS_FArrayProperty:
		return FArrayPropertyDescr::GetPropertyValue(params);
	case LUA_CASTCLASS_FInt64Property:
		return FNumericPropertyDescr<int64, FInt64Property>::GetPropertyValue(params);
	case LUA_CASTCLASS_FSingleDelegateProperty:
		return FSingleDelegatePropertyDescr::GetPropertyValue(params);
	case LUA_CASTCLASS_FMulticastDelegateProperty:
		return FMulticastDelegatePropertyDescr::GetPropertyValue(params);
	case LUA_CASTCLASS_FWeakObjectProperty:
		return FUObjectPropertyDescr::GetPropertyValue<FWeakObjectProperty>(params);
	case LUA_CASTCLASS_FSoftObjectProperty:
		return FUObjectPropertyDescr::GetPropertyValue<FSoftObjectProperty>(params);
	case LUA_CASTCLASS_FTextProperty:
		return FTextPropertyDescr::GetPropertyValue(params);
	case LUA_CASTCLASS_FInt16Property:
		return FNumericPropertyDescr<int16, FInt16Property>::GetPropertyValue(params);
	case LUA_CASTCLASS_FDoubleProperty:
		return FNumericPropertyDescr<double, FDoubleProperty>::GetPropertyValue(params);
	case LUA_CASTCLASS_FMapProperty:
		return UnrealLua::FMapPropertyDescr::GetPropertyValue(params);
	case LUA_CASTCLASS_FSetProperty:
		return FSetPropertyDescr::GetPropertyValue(params);
	case LUA_CASTCLASS_FEnumProperty:
		return FEnumPropertyDescr::GetPropertyValue(params);
		
	default:
		return sol::nil;
	}
}

int UnrealLua::PropertyHelper::GetPropertyValue_InContainer(FPushPropertyValueParams& params)
{
	FProperty* prop = params.Prop;
	params.MemoryPtr = prop->ContainerPtrToValuePtr<void>(params.MemoryPtr);
	return UnrealLua::PropertyHelper::GetPropertyValue(params);
}

int UnrealLua::PropertyHelper::GetPropertyValue(FPushPropertyValueParams& params)
{
		uint64 interestingFlags = params.Prop->GetCastFlags() & supportedPropTypeFlags;
	if(!interestingFlags) [[unlikely]]
	{
		//no overlapping match with any supported properties
		LUA_LOG_ERROR("Can't get value for property %s, property type %s is not supported by UnrealLua", *params.Prop->GetFullName(), *params.Prop->GetCPPType())
		return sol::stack::push(params.Lua, sol::nil);
	}

	ELuaSupportedClassCastFlags supportedProperty = static_cast<ELuaSupportedClassCastFlags>(interestingFlags & ~(interestingFlags - 1));
	
	switch(supportedProperty)
	{
	case LUA_CASTCLASS_FInt8Property:
		return FNumericPropertyDescr<int8, FInt8Property>::GetPropertyValue(params);
	case LUA_CASTCLASS_FByteProperty:
		{
			const FByteProperty* prop = CastField<FByteProperty>(params.Prop);
			if(prop->Enum)
			{
				return FEnumPropertyDescr::GetBytePropertyValue(params);
			}
			else
			{
				return FNumericPropertyDescr<uint8, FByteProperty>::GetPropertyValue(params);		
			}
		}
	case LUA_CASTCLASS_FIntProperty:
		return FNumericPropertyDescr<int32, FIntProperty>::GetPropertyValue(params);
	case LUA_CASTCLASS_FFloatProperty:
		return FNumericPropertyDescr<float, FFloatProperty>::GetPropertyValue(params);
	case LUA_CASTCLASS_FUInt64Property:
		return FNumericPropertyDescr<uint64, FUInt64Property>::GetPropertyValue(params);
	case LUA_CASTCLASS_FClassProperty:
		return FUObjectPropertyDescr::GetPropertyValue<FClassProperty>(params);
	case LUA_CASTCLASS_FUInt32Property:
		return FNumericPropertyDescr<uint32, FUInt32Property>::GetPropertyValue(params);
	case LUA_CASTCLASS_FInterfaceProperty:
		return FUObjectPropertyDescr::GetPropertyValue<FInterfaceProperty>(params);
	case LUA_CASTCLASS_FNameProperty:
		return FNamePropertyDescr::GetPropertyValue(params);
	case LUA_CASTCLASS_FStrProperty:
		return FStrPropertyDescr::GetPropertyValue(params);
	case LUA_CASTCLASS_FObjectProperty:
		return FUObjectPropertyDescr::GetPropertyValue<FObjectProperty>(params);
	case LUA_CASTCLASS_FBoolProperty:
		return FBoolPropertyDescr::GetPropertyValue(params);
	case LUA_CASTCLASS_FUInt16Property:
		return FNumericPropertyDescr<uint16, FUInt16Property>::GetPropertyValue(params);
	case LUA_CASTCLASS_FStructProperty:
		{
			FStructProperty* sprop = const_cast<FStructProperty*>(CastField<FStructProperty>(params.Prop));
			UScriptStruct* spropStruct = sprop->Struct;

			//will also check FVector_NetQuantize::StaticStruct and the other Vector_NetQuantize versions
			if (spropStruct->IsChildOf(UnrealLua::StaticPackages::VectorStruct))              
			{
				return FVectorPropertyDescr::GetPropertyValue(params);
			}
			if (spropStruct == UnrealLua::StaticPackages::Vector2DStruct)
			{
				return FVector2DPropertyDescr::GetPropertyValue(params);
			}
			if (spropStruct == UnrealLua::StaticPackages::RotatorStruct)
			{
				return FRotatorPropertyDescr::GetPropertyValue(params);
			}
			if(spropStruct == UnrealLua::StaticPackages::SharedStruct)
			{
				return FStructPropertyDescr::GetSharedStructPropertyValue(params);
			}
			if(spropStruct == UnrealLua::StaticPackages::InstancedStruct)
			{
				return FStructPropertyDescr::GetInstancedStructPropertyValue(params);
			}
			if (spropStruct == UnrealLua::StaticPackages::LuaValue)
			{
				return FStructPropertyDescr::GetLuaValuePropertyValue(params);
			}
			else if (spropStruct == UnrealLua::StaticPackages::LuaFunction)
			{
				return FStructPropertyDescr::GetLuaFunctionPropertyValue(params);
			}
			else if (spropStruct == UnrealLua::StaticPackages::LuaTable)
			{
				return FStructPropertyDescr::GetLuaTablePropertyValue(params);
			}
			return FStructPropertyDescr::GetPropertyValue(params);
		}
	case LUA_CASTCLASS_FArrayProperty:
		return FArrayPropertyDescr::GetPropertyValue(params);
	case LUA_CASTCLASS_FInt64Property:
		return FNumericPropertyDescr<int64, FInt64Property>::GetPropertyValue(params);
	case LUA_CASTCLASS_FSingleDelegateProperty:
		return FSingleDelegatePropertyDescr::GetPropertyValue(params);
	case LUA_CASTCLASS_FMulticastDelegateProperty:
		return FMulticastDelegatePropertyDescr::GetPropertyValue(params);
	case LUA_CASTCLASS_FWeakObjectProperty:
		return FUObjectPropertyDescr::GetPropertyValue<FWeakObjectProperty>(params);
	case LUA_CASTCLASS_FSoftObjectProperty:
		return FUObjectPropertyDescr::GetPropertyValue<FSoftObjectProperty>(params);
	case LUA_CASTCLASS_FTextProperty:
		return FTextPropertyDescr::GetPropertyValue(params);
	case LUA_CASTCLASS_FInt16Property:
		return FNumericPropertyDescr<int16, FInt16Property>::GetPropertyValue(params);
	case LUA_CASTCLASS_FDoubleProperty:
		return FNumericPropertyDescr<double, FDoubleProperty>::GetPropertyValue(params);
	case LUA_CASTCLASS_FMapProperty:
		return UnrealLua::FMapPropertyDescr::GetPropertyValue(params);
	case LUA_CASTCLASS_FSetProperty:
		return FSetPropertyDescr::GetPropertyValue(params);
	case LUA_CASTCLASS_FEnumProperty:
		return FEnumPropertyDescr::GetPropertyValue(params);
		
	default:
		return sol::stack::push(params.Lua, sol::nil);
	}
}

void UnrealLua::PropertyHelper::HandleGetPropertyNetBehavior(UObject* Object, FProperty* prop)
{
	if(prop->HasAnyPropertyFlags(CPF_Net)) [[unlikely]]
	{
		//mark property dirty if its a struct or array, since we can't know whether they get changed
		//Primitive values get copied out and UObject references are replicated as their own objects

		//Note : Did not include set or Map properties, as they are not replicated (yet) @TODO : Fix it, if it changes
		if(prop->IsA<FStructProperty>() || prop->IsA<FArrayProperty>()/* || prop->IsA<FSetProperty>() || prop->IsA<FMapProperty>()*/) [[unlikely]]
		{
			MARK_PROPERTY_DIRTY(Object, prop);
		}
	}
}

/*
	uint64 interestingFlags = params.Prop->GetCastFlags() & supportedPropTypeFlags;
	//Count how many 0's there are right of the first 1 to find property type.
	//If it's zero 0's before the first 1, it's castflag 0x........1, which is index 1 in the CASTFLAG lookup table (index 0 is CASTCLASS_None with no 1's), hence the "+1" to get to the correct table index
	int index = std::_Countr_zero(interestingFlags) + 1; //0
	return GetPropertyValueLUT[index](params);
	*/
//ELuaSupportedClassCastFlags interestingFlags = static_cast<ELuaSupportedClassCastFlags>(params.Prop->GetCastFlags() & supportedPropTypeFlags);

//count how many 0s are before the first 1
//uint64 numLowerZeroes = std::_Countr_zero(interestingFlags);
//ELuaSupportedClassCastFlags supportedProperty = static_cast<ELuaSupportedClassCastFlags>(1ull << numLowerZeroes);

//@TODO : CHHECK! This could be better than relying on bitfields
/*
FName Type = params.Prop->GetID();
if (const EName* TagType = Type.ToEName(); TagType && Type.GetNumber() == NAME_NO_NUMBER_INTERNAL)
{
	switch (*TagType)
	{
	case NAME_StructProperty:
		break;
	}
}
*/



