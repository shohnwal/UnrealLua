#include "Config/UnrealLuaConstants.h"
#include "GameFramework/Actor.h"
#include "Engine/NetSerialization.h"
#include "Reflection/PropertyHelper.h"
#include "Reflection/PropertyHelperTypes.h"
#include "Reflection/PropertyDescr/FBoolPropertyDescr.h"
#include "Reflection/PropertyDescr/FObjectPropertyDescr.h"

FString UnrealLua::PropertyHelper::GetPropertyTypeName(FProperty* prop, bool luaStyle)
{
	/*
	uint64 interestingFlags = params.Prop->GetCastFlags() & supportedPropTypeFlags;
	//Count how many 0's there are right of the first 1 to find property type.
	//If it's zero 0's before the first 1, it's castflag 0x........1, which is index 1 in the CASTFLAG lookup table (index 0 is CASTCLASS_None with no 1's), hence the "+1" to get to the correct table index
	int index = std::_Countr_zero(interestingFlags) + 1; //0
	return GetPropertyValueLUT[index](params);
	*/
	//ELuaSupportedClassCastFlags interestingFlags = static_cast<ELuaSupportedClassCastFlags>(params.Prop->GetCastFlags() & supportedPropTypeFlags);
	
	uint64 interestingFlags = prop->GetCastFlags() & supportedPropTypeFlags;
	if(!interestingFlags) [[unlikely]]
	{
		//no overlapping match with any supported properties
		//LUA_LOG_ERROR("Can't get value for property %s, property type %s is not supported by UnrealLua", *prop->GetFullName(), *prop->GetCPPType())
		return "any";
	}

	ELuaSupportedClassCastFlags supportedProperty = static_cast<ELuaSupportedClassCastFlags>(interestingFlags & ~(interestingFlags - 1));
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
	
	switch(supportedProperty)
	{
	case LUA_CASTCLASS_FInt8Property:
		return "int8";
	case LUA_CASTCLASS_FByteProperty:
		{
			const FByteProperty* eprop = CastField<FByteProperty>(prop);
			if(eprop->Enum)
			{
				return eprop->Enum->GetName();
			}
			else
			{
				return "uint8";		
			}
		}
	case LUA_CASTCLASS_FIntProperty:
		return "int32";
	case LUA_CASTCLASS_FFloatProperty:
		return "float";
	case LUA_CASTCLASS_FUInt64Property:
		return "uint64";
	case LUA_CASTCLASS_FClassProperty:
		{
			UClass* clazz = CastField<FClassProperty>(prop)->MetaClass;
			bool bIsActor = clazz ? clazz->IsChildOf<AActor>() : false;
			FString classStr = (clazz ? (bIsActor ? "A" : "U") + clazz->GetName() : "UObject");
			FString str = "TSubclassOf<" + classStr + ">";
			return str;// + "|" + classStr + "|string";					
		}
	case LUA_CASTCLASS_FUInt32Property:
		return "uint32";
	case LUA_CASTCLASS_FInterfaceProperty:
		{
			UClass* clazz = CastField<FInterfaceProperty>(prop)->InterfaceClass;
			if(clazz)
			{
				FString str = "I" + clazz->GetName();//FString::Printf(TEXT("TInterface<%s>"), *clazz->GetName());
				return str;
			}
			return "Interface";
		}
	case LUA_CASTCLASS_FNameProperty:
		return "FName";
	case LUA_CASTCLASS_FStrProperty:
		return "FString";
	case LUA_CASTCLASS_FObjectProperty:
		{
			UClass* clazz = CastField<FObjectProperty>(prop)->PropertyClass.Get();
			bool bIsActor = clazz ? clazz->IsChildOf<AActor>() : false;
			return clazz ? (bIsActor ? "A" : "U") + clazz->GetName() : "Null";
		}
	case LUA_CASTCLASS_FBoolProperty:
		return "boolean";
	case LUA_CASTCLASS_FUInt16Property:
		return "uint16";
	case LUA_CASTCLASS_FStructProperty:
		{
			const FStructProperty* sprop = CastField<FStructProperty>(prop);
			const UScriptStruct* const spropStruct = sprop->Struct;
                
			if (spropStruct == UnrealLua::StaticPackages::VectorStruct || spropStruct == FVector_NetQuantize::StaticStruct() || spropStruct == FVector_NetQuantize10::StaticStruct() || spropStruct == FVector_NetQuantize100::StaticStruct() || spropStruct == FVector_NetQuantizeNormal::StaticStruct())              
			{
				return "FVector";
			}
			if (sprop->Struct == UnrealLua::StaticPackages::Vector2DStruct)
			{
				return "FVector2D";
			}
			if (sprop->Struct == UnrealLua::StaticPackages::RotatorStruct)
			{
				return "FRotator";
			}
			if(sprop->Struct == UnrealLua::StaticPackages::SharedStruct)
			{
				return "TSharedStruct";
			}
			if(sprop->Struct == UnrealLua::StaticPackages::InstancedStruct)
			{
				return "TInstancedStruct";
			}
			return "F" + sprop->Struct->GetName();
		}
	case LUA_CASTCLASS_FArrayProperty:
		{
			if (luaStyle)
			{
				return FString::Printf(TEXT("TArray(%s)"), *UnrealLua::PropertyHelper::GetPropertyTypeName(CastField<FArrayProperty>(prop)->Inner, true));
			}
			else
			{
				return FString::Printf(TEXT("TArray<%s>"), *UnrealLua::PropertyHelper::GetPropertyTypeName(CastField<FArrayProperty>(prop)->Inner, true));
			}
		}
	case LUA_CASTCLASS_FInt64Property:
		return "int64";
	case LUA_CASTCLASS_FSingleDelegateProperty:
		{
			FString params = "";
			const FDelegateProperty* delProp = CastField<FDelegateProperty>(prop);
			UFunction* signatureFunc = delProp->SignatureFunction;
			FString returnProp = "";
			for(TFieldIterator<FProperty> propIt(signatureFunc); propIt;)
			{
				FProperty* sprop = *propIt;
				++propIt;
				if(sprop->HasAllPropertyFlags(CPF_ReturnParm))
				{
					returnProp = " -> " + UnrealLua::PropertyHelper::GetPropertyTypeName(sprop, true);
					continue;
				}
				params += UnrealLua::PropertyHelper::GetPropertyTypeName(sprop, true);
				params += " " + sprop->GetName();
				if(propIt)
				{
					params += ", ";
				}
			}
			//FString out = FString::Printf(TEXT("FDelegate #(<%s(%s)>)"), *returnProp, *params);
			if (luaStyle)
			{
				return FString::Printf(TEXT("FDelegate()"));
			}
			else
			{
				return FString::Printf(TEXT("FDelegate<%s(%s)>"), *returnProp, *params);
			}
		}
	case LUA_CASTCLASS_FMulticastDelegateProperty:
		{
			FString params = "";
			const FMulticastDelegateProperty* delProp = CastField<FMulticastDelegateProperty>(prop);
			UFunction* signatureFunc = delProp->SignatureFunction;
			for(TFieldIterator<FProperty> propIt(signatureFunc); propIt;)
			{
				params += UnrealLua::PropertyHelper::GetPropertyTypeName(*propIt, true);
				params += " " + propIt->GetName();
				++propIt;
				if(propIt)
				{
					params += ", ";
				}
			}
			//FString out = FString::Printf(TEXT("FMulticastDelegate #(<void (%s)>)"), *params);
			if (luaStyle)
			{
				return FString::Printf(TEXT("FMulticastDelegate()"));
			}
			else
			{
				return FString::Printf(TEXT("FMulticastDelegate<(%s)>"), *params);
			}
		}
	case LUA_CASTCLASS_FWeakObjectProperty:
		{
			UClass* clazz = CastField<FWeakObjectProperty>(prop)->PropertyClass;
			bool bIsActor = clazz ? clazz->IsChildOf<AActor>() : false;
			FString classStr = (clazz ? (bIsActor ? "A" : "U") + clazz->GetName() : "UObject");
			FString str = "TWeakObjectPtr<" + classStr + ">";
			return str;
		}
	case LUA_CASTCLASS_FSoftObjectProperty:
		{
			UClass* clazz = CastField<FSoftObjectProperty>(prop)->PropertyClass;
			bool bIsActor = clazz ? clazz->IsChildOf<AActor>() : false;
			FString classStr = (clazz ? (bIsActor ? "A" : "U") + clazz->GetName() : "UObject");
			FString str = "TSoftObjectPtr<" + classStr + ">";
			return str;		
		}
	case LUA_CASTCLASS_FTextProperty:
		return "FText";
	case LUA_CASTCLASS_FInt16Property:
		return "int16";
	case LUA_CASTCLASS_FDoubleProperty:
		return "double";
	case LUA_CASTCLASS_FMapProperty:
		{
			if (luaStyle)
			{
				return FString::Printf(TEXT("TMap(%s, %s)"), *UnrealLua::PropertyHelper::GetPropertyTypeName(CastField<FMapProperty>(prop)->KeyProp, true), *UnrealLua::PropertyHelper::GetPropertyTypeName(CastField<FMapProperty>(prop)->ValueProp, true));
			}
			else
			{
				return FString::Printf(TEXT("TMap<%s, %s>"), *UnrealLua::PropertyHelper::GetPropertyTypeName(CastField<FMapProperty>(prop)->KeyProp, true), *UnrealLua::PropertyHelper::GetPropertyTypeName(CastField<FMapProperty>(prop)->ValueProp, true));
			}
		}
	case LUA_CASTCLASS_FSetProperty:
		{
			if (luaStyle)
			{
				return FString::Printf(TEXT("TSet(%s)"), *UnrealLua::PropertyHelper::GetPropertyTypeName(CastField<FSetProperty>(prop)->ElementProp, true));
			}
			else
			{
				return FString::Printf(TEXT("TSet<%s>"), *UnrealLua::PropertyHelper::GetPropertyTypeName(CastField<FSetProperty>(prop)->ElementProp, true));
			}
		}
	case LUA_CASTCLASS_FEnumProperty:
		{
			const FEnumProperty* eprop = CastField<FEnumProperty>(prop);
			if(eprop->GetEnum())
			{
				return eprop->GetEnum()->GetName();
			}
			return "int64";
		}
		default:
				return "unknown";
	}
}