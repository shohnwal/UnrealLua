
#include "UObject/UnrealType.h"
#include "Utility/LuaLogMacros.h"
#include "Config/UnrealLuaConstants.h"
#include "LuaTypes/LuaPrimitives.h"
#include "LuaTypes/LuaUClass.h"
#include "LuaTypes/LuaUStruct.h"
#include "Reflection/PropertyHelper.h"
#include "UObject/TextProperty.h"
#include "LuaTypes/LuaArray.h"
#include "LuaTypes/LuaInstancedStruct.h"
#include "LuaTypes/LuaLightUserdata.h"
#include "LuaTypes/LuaMap.h"
#include "LuaTypes/LuaSet.h"
#include "LuaTypes/LuaSharedStruct.h"
#include "LuaTypes/TLuaSubclassOf.h"
#include "Utility/UnrealVersion.h"

#if UNREALLUA_UE_VERSION_NEWER_THAN_OR_EQUAL(5,8,0)
#define SET_FLAGS(flags)
#else
#define SET_FLAGS(flags) ,flags
#endif

#pragma message ("Still need to add support for all types in " __FILE__ )

//@TODO : fix up flags and properties according to PropertyBag.cpp FProperty* CreatePropertyFromDesc
FProperty* UnrealLua::PropertyHelper::CreateNewProperty(ELuaSupportedClassCastFlags typeFlags, UObject* propClass, FName propName, EObjectFlags propFlags)
{
	FProperty* prop = nullptr;
	switch(typeFlags)
	{
	case LUA_CASTCLASS_FInt8Property:
		prop = new FInt8Property(nullptr, propName SET_FLAGS(propFlags));
		prop->SetPropertyFlags(CPF_HasGetValueTypeHash);
		break;
	case LUA_CASTCLASS_FByteProperty:
		prop = new FByteProperty(nullptr, propName SET_FLAGS(propFlags));
		prop->SetPropertyFlags(CPF_HasGetValueTypeHash);
		break;
	case LUA_CASTCLASS_FIntProperty:
		prop = new FIntProperty(nullptr, propName SET_FLAGS(propFlags));
		prop->SetPropertyFlags(CPF_HasGetValueTypeHash);
		break;
	case LUA_CASTCLASS_FFloatProperty:
		prop = new FFloatProperty(nullptr, propName SET_FLAGS(propFlags));
		prop->SetPropertyFlags(CPF_HasGetValueTypeHash);
		break;
	case LUA_CASTCLASS_FUInt64Property:
		prop = new FUInt64Property(nullptr, propName SET_FLAGS(propFlags));
		prop->SetPropertyFlags(CPF_HasGetValueTypeHash);
		break;
	case LUA_CASTCLASS_FUInt32Property:
		prop = new FUInt32Property(nullptr, propName SET_FLAGS(propFlags));
		prop->SetPropertyFlags(CPF_HasGetValueTypeHash);
		break;
	case LUA_CASTCLASS_FNameProperty:
		prop = new FNameProperty(nullptr, propName SET_FLAGS(propFlags));
		prop->SetPropertyFlags(CPF_HasGetValueTypeHash);
		break;
	case LUA_CASTCLASS_FStrProperty:
		prop = new FStrProperty(nullptr, propName SET_FLAGS(propFlags));
		prop->SetPropertyFlags(CPF_HasGetValueTypeHash);
		break;
	case LUA_CASTCLASS_FBoolProperty:
		prop = new FBoolProperty(nullptr, propName SET_FLAGS(propFlags));
		CastField<FBoolProperty>(prop)->SetBoolSize(sizeof(bool), true);
		break;
	case LUA_CASTCLASS_FUInt16Property:
		prop = new FUInt16Property(nullptr, propName SET_FLAGS(propFlags));
		prop->SetPropertyFlags(CPF_HasGetValueTypeHash);
		break;
	case LUA_CASTCLASS_FInt64Property:
		prop = new FInt64Property(nullptr, propName SET_FLAGS(propFlags));
		prop->SetPropertyFlags(CPF_HasGetValueTypeHash);
		break;
	case LUA_CASTCLASS_FTextProperty:
		prop = new FTextProperty(nullptr, propName SET_FLAGS(propFlags));
		break;
	case LUA_CASTCLASS_FInt16Property:
		prop = new FInt16Property(nullptr, propName SET_FLAGS(propFlags));
		prop->SetPropertyFlags(CPF_HasGetValueTypeHash);
		break;
	case LUA_CASTCLASS_FDoubleProperty:
		prop = new FDoubleProperty(nullptr, propName SET_FLAGS(propFlags));
		prop->SetPropertyFlags(CPF_HasGetValueTypeHash);
		break;
	case LUA_CASTCLASS_FObjectProperty:
		{
			UClass* clazz = CastChecked<UClass>(propClass);
			prop = new FObjectProperty(nullptr, propName SET_FLAGS(propFlags));
			
			prop->SetPropertyFlags(CPF_HasGetValueTypeHash);
			if (clazz->HasAnyClassFlags(CLASS_DefaultToInstanced))
			{
				prop->SetPropertyFlags(CPF_InstancedReference);
			}
			CastField<FObjectProperty>(prop)->SetPropertyClass(clazz);			
		}
		break;
	case LUA_CASTCLASS_FSoftObjectProperty:
		{
			UClass* clazz = CastChecked<UClass>(propClass);
			prop = new FSoftObjectProperty(nullptr, propName SET_FLAGS(propFlags));
			prop->SetPropertyFlags(CPF_HasGetValueTypeHash);
			if (clazz->HasAnyClassFlags(CLASS_DefaultToInstanced))
			{
				prop->SetPropertyFlags(CPF_InstancedReference);
			}
			CastField<FSoftObjectProperty>(prop)->SetPropertyClass(clazz);
		}
		break;
	case LUA_CASTCLASS_FInterfaceProperty:
		{
			UClass* clazz = CastChecked<UClass>(propClass);
			prop = new FInterfaceProperty(nullptr, propName SET_FLAGS(propFlags));
			prop->SetPropertyFlags(CPF_HasGetValueTypeHash);
			CastField<FInterfaceProperty>(prop)->InterfaceClass = clazz;
		}
		break;
	default:
		prop = nullptr;
	}
	
	return prop;
}

namespace UnrealLua::PropertyHelper
	{	
	FProperty* CreateNewProperty(uint64 classFlags, UObject* propClass, FName propName, EObjectFlags propFlags)
	{
		uint64 interestingFlags = classFlags & supportedPropTypeFlags;
		ELuaSupportedClassCastFlags supportedProperty = static_cast<ELuaSupportedClassCastFlags>(interestingFlags & ~(interestingFlags - 1));

		//@TODO : test same optimization as in GetProperty
		
		return CreateNewProperty(supportedProperty, propClass, propName, propFlags);
	}
	
	FProperty* CreateObjectProperty(UClass* uclass, bool bMakeTSubClassProp, FName propName, EObjectFlags propFlags)
	{
		FObjectProperty* prop = nullptr;
		if(bMakeTSubClassProp)
		{
			prop = new FClassProperty(nullptr, propName SET_FLAGS(propFlags));
			CastField<FClassProperty>(prop)->SetMetaClass(uclass ? uclass : UObject::StaticClass());
			prop->PropertyClass = uclass ? uclass : UClass::StaticClass();
			prop->SetPropertyFlags(CPF_HasGetValueTypeHash);		
		}
		else
		{
			prop = new FObjectProperty(nullptr, propName SET_FLAGS(propFlags));
			prop->SetPropertyClass(uclass ? uclass : UClass::StaticClass());
			prop->SetPropertyFlags(CPF_HasGetValueTypeHash);		
		}

		return prop;
	}
}

FProperty* UnrealLua::PropertyHelper::CreateNewProperty(FFieldClass* fieldClass, UObject* propClass, FName propName, EObjectFlags propFlags)
{
	return UnrealLua::PropertyHelper::CreateNewProperty(fieldClass->GetCastFlags(), propClass, propName, propFlags);
}

FProperty* UnrealLua::PropertyHelper::CreateNewProperty(FProperty* otherProp, FName propName, EObjectFlags propFlags)
{
	UClass* propClass = nullptr;
	if(const FObjectProperty* op = CastField<FObjectProperty>(otherProp))
	{
		propClass = op->PropertyClass;
	}
	else if(const FClassProperty* cp = CastField<FClassProperty>(otherProp))
	{
		propClass = cp->PropertyClass;
	}
	else if(const FSoftClassProperty* sp = CastField<FSoftClassProperty>(otherProp))
	{
		propClass = sp->MetaClass;
	}
	else if(const FSoftObjectProperty* so = CastField<FSoftObjectProperty>(otherProp))
	{
		propClass = so->PropertyClass;
	}
	else if(const FInterfaceProperty* ip = CastField<FInterfaceProperty>(otherProp))
	{
		propClass = ip->InterfaceClass;
	}
	return otherProp ? UnrealLua::PropertyHelper::CreateNewProperty(otherProp->GetCastFlags(), propClass, propName, propFlags) : nullptr;
}

//Called by LuaContainer Constructors like FArray("T<ACharacter>") or TSet("int32") to parse inner element type
//The optional UClass/UScriptStruct class will be derived depending on the inner element type by looking up the UE type table
FProperty* UnrealLua::PropertyHelper::CreateNewProperty(const sol::object& luaParamArg, FName propName, EPropertyFlags propFlags, EObjectFlags objectFlags)
{
	FProperty* prop = nullptr;
	
	sol::object luaParam = luaParamArg;

	bool bIsTSubclassOfParam = false;
	if(luaParam.get_type() == sol::type::string)
	{
		sol::string_view strv = luaParam.as<sol::string_view>();
		if(!strv.empty())
		{
			if(strv.size() > 3)
			{
				if(strv.ends_with(">"))
				{
					//Some kind of template property
					if(strv.starts_with("T<"))
					{
						strv = {strv.begin()+2, strv.end()-1};
						bIsTSubclassOfParam = true;					
					}
					else if(strv.size() > 13 && strv.starts_with("TSubclassOf<"))
					{
						strv = {strv.begin()+13, strv.end()-1};
						bIsTSubclassOfParam = true;					
					}
				}
			}
			
			sol::state_view luaStateView = luaParam.lua_state();
			luaParam = luaStateView["UE"][strv];//luaStateView["UE"][strv];
		}
	}
	else if (luaParam.get_type() == sol::type::table)
	{
		sol::state_view lua{luaParam.lua_state()};
		const sol::table fVector = lua["FVector"];
		const sol::table fRotator = lua["FRotator"];
		const sol::table fTransform = lua["FTransform"];
		const sol::table fInstancedStruct = lua["TInstancedStruct"];
		const sol::table fSharedStruct = lua["TSharedStruct"];
			
		sol::table tbl = luaParam.as<sol::table>();
		if (tbl == fVector)
		{
			 luaParam = sol::make_object<FVector>(luaParam.lua_state(), FVector::ZeroVector);
		}
		else if (tbl == fRotator)
		{
			luaParam = sol::make_object<FRotator>(luaParam.lua_state(), FRotator::ZeroRotator);
		}
		else if (tbl == fTransform)
		{
			luaParam = sol::make_object<FTransform>(luaParam.lua_state(), FTransform::Identity);
		}
		else if (tbl == fInstancedStruct)
		{
			luaParam = sol::make_object<FLuaInstancedStruct>(luaParam.lua_state(), FLuaInstancedStruct());
		}
		else if (tbl == fSharedStruct)
		{
			luaParam = sol::make_object<FLuaSharedStruct>(luaParam.lua_state(), FLuaSharedStruct());
		}
	}
	
	if(luaParam.is<TLuaSubclassOf>())
	{
		TLuaSubclassOf& tsubclassOf = luaParam.as<TLuaSubclassOf&>();
		if (tsubclassOf.IsValid())
		{
			UClass* clazz = luaParam.as<FLuaUClass&>().TryLoadClass();
			prop = UnrealLua::PropertyHelper::CreateObjectProperty(clazz, true, propName, objectFlags);
		}
		else
		{
			UClass* clazz = UClass::StaticClass();
			prop = UnrealLua::PropertyHelper::CreateObjectProperty(clazz, true, propName, objectFlags);
		}
	}
	else if(luaParam.is<FLuaPrimitiveCPPType>())
	{
		prop= UnrealLua::PropertyHelper::CreateNewProperty(luaParam.as<FLuaPrimitiveCPPType>().Type, nullptr, propName, objectFlags);
	}
	else if(luaParam.is<FLuaUClass>())
	{
		UClass* clazz = luaParam.as<FLuaUClass&>().TryLoadClass();
		prop = UnrealLua::PropertyHelper::CreateObjectProperty(clazz, bIsTSubclassOfParam, propName, objectFlags);
	}
	else if(luaParam.is<FLuaUStruct>())
	{
		FStructProperty* Prop = new FStructProperty(nullptr, propName SET_FLAGS(RF_Public));
		UScriptStruct* ss = Cast<UScriptStruct>(luaParam.as<FLuaUStruct&>().TryLoad());
		Prop->Struct = ss;

		if (ss->GetCppStructOps() && ss->GetCppStructOps()->HasGetTypeHash())
		{
			Prop->SetPropertyFlags(CPF_HasGetValueTypeHash);
		}
				
		if (ss->StructFlags & STRUCT_HasInstancedReference)
		{
			Prop->SetPropertyFlags(CPF_ContainsInstancedReference);
		}

		if(ss->StructFlags & STRUCT_IsPlainOldData)
		{
			Prop->SetPropertyFlags(CPF_IsPlainOldData);
		}
		prop = Prop;
	}
	else if (luaParam.is<FVector>())
	{
		FStructProperty* newProp = new FStructProperty(nullptr, propName SET_FLAGS(objectFlags));
		UScriptStruct* ss = UnrealLua::StaticPackages::VectorStruct;
		newProp->Struct = ss;
		if (ss->GetCppStructOps() && ss->GetCppStructOps()->HasGetTypeHash())
		{
			newProp->SetPropertyFlags(CPF_HasGetValueTypeHash);
		}
				
		if (ss->StructFlags & STRUCT_HasInstancedReference)
		{
			newProp->SetPropertyFlags(CPF_ContainsInstancedReference);
		}

		if(ss->StructFlags & STRUCT_IsPlainOldData)
		{
			newProp->SetPropertyFlags(CPF_IsPlainOldData);
		}
		prop = newProp;
	}
	else if (luaParam.is<FRotator>())
	{
		FStructProperty* newProp = new FStructProperty(nullptr, propName SET_FLAGS(objectFlags));
		UScriptStruct* ss = UnrealLua::StaticPackages::RotatorStruct;
		newProp->Struct = ss;
		if (ss->GetCppStructOps() && ss->GetCppStructOps()->HasGetTypeHash())
		{
			newProp->SetPropertyFlags(CPF_HasGetValueTypeHash);
		}
				
		if (ss->StructFlags & STRUCT_HasInstancedReference)
		{
			newProp->SetPropertyFlags(CPF_ContainsInstancedReference);
		}

		if(ss->StructFlags & STRUCT_IsPlainOldData)
		{
			newProp->SetPropertyFlags(CPF_IsPlainOldData);
		}
		prop = newProp;
	}
	else if (luaParam.is<FLuaArray>())
	{
		const FLuaArray& arr = luaParam.as<FLuaArray&>();
		FArrayProperty* arrProp = new FArrayProperty(nullptr, propName SET_FLAGS(objectFlags));
		FProperty* inner = CreateNewProperty(arr.GetInner(), propName, objectFlags);
		arrProp->AddCppProperty(inner);
		inner->Owner = arrProp;
		prop = arrProp;
	}
	else if (luaParam.is<FLuaSet>())
	{
		const FLuaSet& arr = luaParam.as<FLuaSet&>();
		FSetProperty* containerProp = new FSetProperty(nullptr, propName SET_FLAGS(objectFlags));
		FProperty* inner = CreateNewProperty(arr.GetInner(), propName, objectFlags);
		containerProp->ElementProp = inner;
		inner->Owner = containerProp;
		prop = containerProp;
	}
	else if (luaParam.is<FLuaMap>())
	{
		const FLuaMap& container = luaParam.as<FLuaMap&>();
		FMapProperty* containerProp = new FMapProperty(nullptr, propName SET_FLAGS(objectFlags));
		containerProp->KeyProp = CreateNewProperty(container.GetKeyProperty());
		containerProp->ValueProp = CreateNewProperty(container.GetValueProperty());
		containerProp->KeyProp->Owner = containerProp;
		containerProp->ValueProp->Owner = containerProp;
		prop = containerProp;
	}
	else if (luaParam.is<FLuaInstancedStruct>())
	{
		FStructProperty* Prop = new FStructProperty(nullptr, propName SET_FLAGS(objectFlags));
		UScriptStruct* ss = UnrealLua::StaticPackages::InstancedStruct;
		
		Prop->Struct = ss;

		if (ss->GetCppStructOps() && ss->GetCppStructOps()->HasGetTypeHash())
		{
			Prop->SetPropertyFlags(CPF_HasGetValueTypeHash);
		}
				
		if (ss->StructFlags & STRUCT_HasInstancedReference)
		{
			Prop->SetPropertyFlags(CPF_ContainsInstancedReference);
		}

		if(ss->StructFlags & STRUCT_IsPlainOldData)
		{
			Prop->SetPropertyFlags(CPF_IsPlainOldData);
		}
		prop = Prop;
	}
	
	else if (luaParam.is<FLuaSharedStruct>())
	{
		FStructProperty* Prop = new FStructProperty(nullptr, propName SET_FLAGS(objectFlags));
		UScriptStruct* ss = UnrealLua::StaticPackages::SharedStruct;
		
		Prop->Struct = ss;

		if (ss->GetCppStructOps() && ss->GetCppStructOps()->HasGetTypeHash())
		{
			Prop->SetPropertyFlags(CPF_HasGetValueTypeHash);
		}
				
		if (ss->StructFlags & STRUCT_HasInstancedReference)
		{
			Prop->SetPropertyFlags(CPF_ContainsInstancedReference);
		}

		if(ss->StructFlags & STRUCT_IsPlainOldData)
		{
			Prop->SetPropertyFlags(CPF_IsPlainOldData);
		}
		prop = Prop;
	}
	
	else if (luaParam.get_type() == sol::type::lightuserdata)
	{
		if(UnrealLua::LightUserdata::IsUObject(luaParam))
		{
			UObject* obj = UnrealLua::LightUserdata::GetUObject(luaParam);
			if (obj)
			{
				UClass* clazz = obj ? obj->GetClass() : UObject::StaticClass();
				prop = UnrealLua::PropertyHelper::CreateObjectProperty(clazz, bIsTSubclassOfParam, propName, objectFlags);			
			}			
		}
		else if(UnrealLua::LightUserdata::IsEnum(luaParam))
		{
			UEnum* uenum = UnrealLua::LightUserdata::GetUEnum(luaParam);
			if(uenum)
			{
				FEnumProperty* Prop = new FEnumProperty(nullptr, propName SET_FLAGS(objectFlags));
				FByteProperty* UnderlyingProp = new FByteProperty(Prop, "UnderlyingType" SET_FLAGS(objectFlags)); // HACK: Hardwire to byte property for now for BP compatibility
				Prop->SetEnum(uenum);
				Prop->AddCppProperty(UnderlyingProp);
				prop = Prop;	
			}
		}
	}
	if(prop == nullptr)
	{
		sol::state_view lua{luaParam.lua_state()};
		std::string objType = UnrealLua::LuaTypes::TypeInfo::UTypeInternal(luaParam, true, -1);
		LUA_LOG_ERROR("Unexpected type for Property creation : %hs, %d", objType.data(), luaParam.get_type())
		return nullptr;
	}
	prop->PropertyFlags |= propFlags;
	return prop;
}