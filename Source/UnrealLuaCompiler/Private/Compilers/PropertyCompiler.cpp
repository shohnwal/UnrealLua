// Fill out your copyright notice in the Description page of Project Settings.


#include "Compilers/PropertyCompiler.h"

#include "Utility/LuaLogMacros.h"
#include "LuaTypes/LuaArray.h"
#include "LuaTypes/LuaInstancedStruct.h"
#include "LuaTypes/LuaLightUserdata.h"
#include "LuaTypes/LuaMap.h"
#include "LuaTypes/LuaPrimitives.h"
#include "LuaTypes/LuaScriptStruct.h"
#include "LuaTypes/LuaSet.h"
#include "LuaTypes/LuaSharedStruct.h"
#include "LuaTypes/LuaUClass.h"
#include "LuaTypes/LuaUStruct.h"
#include "LuaTypes/TLuaSubclassOf.h"
#include "Reflection/PropertyHelper.h"
#include "UnrealLuaCompilerConstants.h"
#include "Prototypes/UFunctionPrototype.h"
#include "Prototypes/UPropertyPrototype.h"

bool UnrealLua::Compiler::HasFieldNameConflicts(const TArray<FUnrealLuaCompilerUPropertyPrototype>& propPrototypes, const TArray<FUnrealLuaCompilerUFunctionPrototype>& funcPrototypes, UStruct* parentStruct)
{
	if (!IsValid(parentStruct))
	{
		return false;
	}	
	
	TSet<FName> handledNames = {};
	TSet<FName> conflictingNames = {};
	for(const FUnrealLuaCompilerUPropertyPrototype& prop : propPrototypes)
	{
		bool alreadyInSet = false;
		handledNames.Add(prop.TypeName, &alreadyInSet);
		if (alreadyInSet)
		{
			conflictingNames.Add(prop.TypeName);
		}
	}	

	for (const FUnrealLuaCompilerUFunctionPrototype& funcProto : funcPrototypes)
	{
		bool alreadyInSet = false;
		handledNames.Add(funcProto.TypeName, &alreadyInSet);
		if (alreadyInSet)
		{
			conflictingNames.Add(funcProto.TypeName);
		}	
	}
	
	if(!conflictingNames.IsEmpty())
	{
		LUA_LOG_ERROR("Conflicting property- or function name(s):")
		for (const auto& conflictingName : conflictingNames)
		{
			LUA_LOG_ERROR("%s", *conflictingName.ToString())
		}
		return true;
	}
	UClass* uclass = Cast<UClass>(parentStruct);
	
	if (HasPropertyNameConflicts(propPrototypes, parentStruct))
	{
		return true;
	}
	
	if(uclass && HasFunctionNameConflicts(funcPrototypes, uclass))
	{
		return true;
	}
	return false;
}

bool UnrealLua::Compiler::HasPropertyNameConflicts(const TArray<FUnrealLuaCompilerUPropertyPrototype>& prototypes, UStruct* parentStruct)
{
	if (!IsValid(parentStruct))
	{
		return false;
	}
	UClass* uclass = Cast<UClass>(parentStruct);
	for(const FUnrealLuaCompilerUPropertyPrototype& prop : prototypes)
	{
		if (HasField(prop.TypeName, parentStruct) || HasFunction(prop.TypeName, uclass))
		{
			return true;
		}
	}
	return false;
}

bool UnrealLua::Compiler::HasFunctionNameConflicts(const TArray<FUnrealLuaCompilerUFunctionPrototype>& prototypes, UClass* parentClass)
{
	if (!IsValid(parentClass))
	{
		return false;
	}
	for(const FUnrealLuaCompilerUFunctionPrototype& func : prototypes)
	{
		//UClasses only have name conflict if a Property and UFunction have same names
		//A duplicate function name in parent class is ok, as we can just override the function in the child UClass  
		if (HasField(func.TypeName, parentClass))
		{
			return true;
		}
	}
	return false;
}
namespace UnrealLua::Compiler
{
	FProperty* CompileProperty(FUnrealLuaCompilerUPropertyPrototype& prototype)
	{
		//@TODO : Support delegates
		
		verify(prototype.AssignedTypeOrValue.valid())
		verify(!prototype.TypeName.IsNone())
		verify(prototype.OwnerPrototype != nullptr)
		const EUnrealLuaCompilerPrototypeType ownerType = prototype.OwnerPrototype->GetPrototypeCategory();
		verify(ownerType == EUnrealLuaCompilerPrototypeType::Function || ownerType == EUnrealLuaCompilerPrototypeType::ScriptStruct || ownerType == EUnrealLuaCompilerPrototypeType::Class)

		
		FString method = "";
		//LUA_LOG_WARNING("compiling property %s", *prototype.TypeName.ToString())
		
		
		/*This can be
		 * - default value
		 * - type
		 */
		sol::object assignedTypeOrValue = prototype.AssignedTypeOrValue;
		
		sol::object typeToUse{};
		sol::object defaultValue{};
	
		//single value. Can be either value or type
		//if value, figure out type
		//if type, leave defaultValue nil
		
		//First, check for Lua primitive values
		if(assignedTypeOrValue.is<int>())
		{
			FLuaPrimitiveCPPType primitiveType{ELuaSupportedClassCastFlags::LUA_CASTCLASS_FInt64Property};
			typeToUse = sol::make_object<FLuaPrimitiveCPPType>(assignedTypeOrValue.lua_state(), primitiveType);
			defaultValue = assignedTypeOrValue;
		}
		else if (assignedTypeOrValue.is<double>())
		{
			FLuaPrimitiveCPPType primitiveType{ELuaSupportedClassCastFlags::LUA_CASTCLASS_FDoubleProperty};
			typeToUse = sol::make_object<FLuaPrimitiveCPPType>(assignedTypeOrValue.lua_state(), primitiveType);
			defaultValue = assignedTypeOrValue;
		}
		else if (assignedTypeOrValue.get_type() == sol::type::boolean)
		{
			FLuaPrimitiveCPPType primitiveType{ELuaSupportedClassCastFlags::LUA_CASTCLASS_FBoolProperty};
			typeToUse = sol::make_object<FLuaPrimitiveCPPType>(assignedTypeOrValue.lua_state(), primitiveType);
			defaultValue = assignedTypeOrValue;
		}
		else if (assignedTypeOrValue.get_type() == sol::type::string)
		{
			sol::string_view strv = assignedTypeOrValue.as<sol::string_view>();
			if (strv.starts_with("TSubclassOf<") && strv.ends_with('>'))
			{
				//create FClassProperty
				typeToUse = assignedTypeOrValue;
			}
			else
			{
				//use raw string
				FLuaPrimitiveCPPType primitiveType{ELuaSupportedClassCastFlags::LUA_CASTCLASS_FStrProperty};
				typeToUse = sol::make_object<FLuaPrimitiveCPPType>(assignedTypeOrValue.lua_state(), primitiveType);
				defaultValue = assignedTypeOrValue;
			}
		}		
		//check usertype, might be value or type
		else if (assignedTypeOrValue.get_type() == sol::type::userdata)
		{
			if (   assignedTypeOrValue.is<FLuaPrimitiveCPPType>()
				|| assignedTypeOrValue.is<FLuaUClass>()
				|| assignedTypeOrValue.is<FLuaUStruct>()
				)
			{
				typeToUse = assignedTypeOrValue;
			}
			else if (assignedTypeOrValue.is<FVector>()	|| assignedTypeOrValue.is<FRotator>() || assignedTypeOrValue.is<FTransform>())
			{
				typeToUse = assignedTypeOrValue;
				defaultValue = assignedTypeOrValue;
			}
			else if(assignedTypeOrValue.is<FLuaScriptStruct>())
			{
				FLuaScriptStruct& ss = assignedTypeOrValue.as<FLuaScriptStruct&>();
				typeToUse = sol::make_object<FLuaUStruct>(assignedTypeOrValue.lua_state(), ss.GetScriptStruct());
				defaultValue = assignedTypeOrValue;
			}
			else if (assignedTypeOrValue.is<FLuaInstancedStruct>())
			{
				FLuaInstancedStruct& instancedStruct = assignedTypeOrValue.as<FLuaInstancedStruct&>();
				typeToUse = assignedTypeOrValue;
				if (instancedStruct.IsValid())
				{
					defaultValue = assignedTypeOrValue;
				}
			}
			else if (assignedTypeOrValue.is<FLuaSharedStruct>())
			{
				FLuaSharedStruct& sharedStruct = assignedTypeOrValue.as<FLuaSharedStruct&>();
				typeToUse = assignedTypeOrValue;
				if (sharedStruct.IsValid())
				{
					defaultValue = assignedTypeOrValue;
				}
			}
			else if (assignedTypeOrValue.is<FLuaArray>())
			{
				typeToUse = assignedTypeOrValue;
				defaultValue = assignedTypeOrValue;
			}
			else if (assignedTypeOrValue.is<FLuaSet>())
			{
				typeToUse = assignedTypeOrValue;
				defaultValue = assignedTypeOrValue;
			}
			else if (assignedTypeOrValue.is<FLuaMap>())
			{
				typeToUse = assignedTypeOrValue;
				defaultValue = assignedTypeOrValue;
			}
			else if (assignedTypeOrValue.is<TLuaSubclassOf>())
			{
				typeToUse = assignedTypeOrValue;
			}
			else
			{
				checkNoEntry();
				return nullptr;
			}
		}
		else if (assignedTypeOrValue.get_type() == sol::type::lightuserdata)
		{
			if(UnrealLua::LightUserdata::IsUObject(assignedTypeOrValue))
			{
				typeToUse = assignedTypeOrValue;		
			}
			else if(UnrealLua::LightUserdata::IsEnum(assignedTypeOrValue))
			{
				typeToUse = assignedTypeOrValue;
			}
		}
		else if (assignedTypeOrValue.get_type() == sol::type::table)
		{
			sol::state_view lua{assignedTypeOrValue.lua_state()};
			const sol::table fVector = lua["FVector"];
			const sol::table fRotator = lua["FRotator"];
			const sol::table fTransform = lua["FTransform"];
			const sol::table fInstancedStruct = lua["TInstancedStruct"];
			const sol::table fSharedStruct = lua["TSharedStruct"];
			
			sol::table tbl = assignedTypeOrValue.as<sol::table>();
			if (tbl == fVector)
			{
        		typeToUse = sol::make_object<FVector>(assignedTypeOrValue.lua_state(), FVector::ZeroVector);
			}
			else if (tbl == fRotator)
			{
        		typeToUse = sol::make_object<FRotator>(assignedTypeOrValue.lua_state(), FRotator::ZeroRotator);
			}
			else if (tbl == fTransform)
			{
        		typeToUse = sol::make_object<FTransform>(assignedTypeOrValue.lua_state(), FTransform::Identity);
			}
			else if (tbl == fInstancedStruct)
			{
        		typeToUse = sol::make_object<FLuaInstancedStruct>(assignedTypeOrValue.lua_state(), FLuaInstancedStruct());
			}
			else if (tbl == fSharedStruct)
			{
        		typeToUse = sol::make_object<FLuaSharedStruct>(assignedTypeOrValue.lua_state(), FLuaSharedStruct());
			}
			else
			{
				typeToUse = tbl[1];
				defaultValue = tbl[2];
			}
		}
		prototype.PropertyCompilerEvaluatedType = typeToUse;
		prototype.EvaluatedDefaultValue = defaultValue;
		
		if (!prototype.PropertyCompilerEvaluatedType.valid())
		{
			std::string propType = UnrealLua::LuaTypes::TypeInfo::UTypeInternal(prototype.PropertyCompilerEvaluatedType, true, -1); 
			LUA_LOG_ERROR("Invalid or unknown type in property %s : %hs", *prototype.GetTypeNameString(), propType.c_str())
			return nullptr;			
		}
		std::string propType = UnrealLua::LuaTypes::TypeInfo::UTypeInternal(typeToUse, true, -1);
		
		FString replicationFunc = "";

		bool bIsReplicated = false;
		
		if (ownerType == EUnrealLuaCompilerPrototypeType::Function)
		{
			
		}
		else
		{
			verify(ownerType == EUnrealLuaCompilerPrototypeType::Class || ownerType == EUnrealLuaCompilerPrototypeType::ScriptStruct);
			
			//Check EditAnywhere-like flags
			uint8 numEditFlagsTrue = 0;
			
			bool bIsStaticProp = false;
			bool bIsBlueprintReadWrite = false;
			bool bIsBlueprintReadOnly = false;
			bool bIsVisibleAnywhere = false;
			bool bIsVisibleDefaultsOnly = false;
			bool bIsVisibleInstanceOnly = false;
			bool bIsEditAnywhere = false;
			bool bIsEditDefaultsOnly = false;
			bool bIsEditInstanceOnly = false;
			bool bisBlueprintAssignable = false;
			
			for (const auto& flagstr : prototype.PropFlags)
			{
				FStringView flag = flagstr;
				flag.TrimStartAndEndInline();
				if (flag.Equals(UnrealLua::CompilerConstants::PROPFLAG_Static, ESearchCase::IgnoreCase))
				{
					bIsStaticProp = true;
				}
				else if (flag.Equals(UnrealLua::CompilerConstants::PROPFLAG_BlueprintReadOnly, ESearchCase::IgnoreCase))
				{
					bIsBlueprintReadOnly = true;
				}
				else if (flag.Equals(UnrealLua::CompilerConstants::PROPFLAG_BlueprintReadWrite, ESearchCase::IgnoreCase))
				{
					bIsBlueprintReadWrite = true;
				}
				else if (flag.Equals(UnrealLua::CompilerConstants::PROPFLAG_EditAnywhere, ESearchCase::IgnoreCase))
				{
					bIsEditAnywhere = true;
					numEditFlagsTrue++;
				}
				else if (flag.Equals(UnrealLua::CompilerConstants::PROPFLAG_VisibleAnywhere, ESearchCase::IgnoreCase))
				{
					bIsVisibleAnywhere = true;
					numEditFlagsTrue++;
				}
				else if (flag.Equals(UnrealLua::CompilerConstants::PROPFLAG_VisibleDefaultsOnly, ESearchCase::IgnoreCase))
				{
					bIsVisibleDefaultsOnly = true;
					numEditFlagsTrue++;
				}
				else if (flag.Equals(UnrealLua::CompilerConstants::PROPFLAG_VisibleInstanceOnly, ESearchCase::IgnoreCase))
				{
					bIsVisibleInstanceOnly = true;
					numEditFlagsTrue++;
				}
				else if (flag.Equals(UnrealLua::CompilerConstants::PROPFLAG_EditDefaultsOnly, ESearchCase::IgnoreCase))
				{
					bIsEditDefaultsOnly = true;
					numEditFlagsTrue++;
				}
				else if (flag.Equals(UnrealLua::CompilerConstants::PROPFLAG_EditInstanceOnly, ESearchCase::IgnoreCase))
				{
					bIsEditInstanceOnly = true;
					numEditFlagsTrue++;
				}
				else if (flag.Equals(UnrealLua::CompilerConstants::PROPFLAG_BlueprintAssignable, ESearchCase::IgnoreCase))
				{
					bisBlueprintAssignable = true;
				}
				else if (flag.Equals(UnrealLua::CompilerConstants::PROPFLAG_Replicated, ESearchCase::IgnoreCase))
				{
					bIsReplicated = true;
				}
				else if (flag.StartsWith(UnrealLua::CompilerConstants::PROPFLAG_ReplicatedUsing, ESearchCase::IgnoreCase))
				{
					bIsReplicated = true;
					int32 equalsIndex = INDEX_NONE;
					flag.FindChar(TEXT('='), equalsIndex);
					if (equalsIndex != INDEX_NONE)
					{
						FStringView replicatedUsingStr = flag.RightChop(equalsIndex+1);
						replicatedUsingStr.TrimStartAndEndInline();
						replicationFunc = replicatedUsingStr;
					}
					else
					{
						prototype.SetIsError("Invalid ReplicatedUsing flag : " + FString(flag));	
					}
				}
				else
				{
					prototype.SetIsError("Unknown UProperty flag : " + FString(flag));
					return nullptr;
				}
			}
			
			//Check BlueprintReadWrite-like flags
			if (bIsBlueprintReadWrite && bIsBlueprintReadOnly)
			{
				prototype.SetIsError("Property is marked ReadWrite and ReadOnly:" + prototype.GetTypeNameString() + ": " + prototype.PropFlagsString);
				return nullptr;
			}
			
			if (numEditFlagsTrue > 1)
			{
				prototype.SetIsError("Property has conflicting Visible/Edit flags: " + prototype.GetTypeNameString() + ": " + prototype.PropFlagsString);
				return nullptr;
			}
			
			if (bIsBlueprintReadWrite)
			{
				prototype.PropertyFlags |= CPF_BlueprintVisible;
			}
			else if (bIsBlueprintReadOnly)
			{
				prototype.PropertyFlags |= CPF_BlueprintVisible;
				prototype.PropertyFlags |= CPF_BlueprintReadOnly;
			}
			else
			{
				//by default, properties are ReadWrite
				prototype.PropertyFlags |= CPF_BlueprintVisible;
			}
			
			if (bIsEditAnywhere)
			{
				prototype.PropertyFlags |= CPF_Edit;
			}
			else if (bIsEditDefaultsOnly)
			{
				prototype.PropertyFlags |= CPF_Edit;
				prototype.PropertyFlags |= CPF_DisableEditOnInstance;
			}
			else if (bIsEditInstanceOnly)
			{
				prototype.PropertyFlags |= CPF_Edit;
				prototype.PropertyFlags |= CPF_DisableEditOnTemplate;
			}
			else if (bIsVisibleDefaultsOnly)
			{
				prototype.PropertyFlags |= CPF_DisableEditOnInstance;
			}
			else if (bIsVisibleInstanceOnly)
			{
				prototype.PropertyFlags |= CPF_DisableEditOnTemplate;
			}
			else if (bIsVisibleAnywhere)
			{
				
			}
			else
			{
				//By default, properties are editanywhere
				prototype.PropertyFlags |= CPF_Edit;
			}
			
			if (bIsStaticProp)
			{
				prototype.bIsStaticProperty = true;
				if (ownerType != EUnrealLuaCompilerPrototypeType::Class)
				{
					prototype.SetIsError("Static Properties only allowed on classes! " + prototype.GetFullPathString());
				}
			}
		}

		
		//LUA_LOG("Using prop type %hs for property %s", propType.c_str(), *prototype.GetTypeNameString())
		FProperty* newProperty = UnrealLua::PropertyHelper::CreateNewProperty(prototype.PropertyCompilerEvaluatedType, prototype.TypeName, prototype.PropertyFlags);
		if (!newProperty)
		{
			return nullptr;
		}
	
		if (bIsReplicated && !replicationFunc.IsEmpty())
		{
			newProperty->RepNotifyFunc = *replicationFunc;
		}
		
		newProperty->PropertyFlags |= prototype.PropertyFlags;
		prototype.CompiledProperty = newProperty;
		return newProperty;
	}
}

bool UnrealLua::Compiler::CompileProperties(TArray<FUnrealLuaCompilerUPropertyPrototype>& prototypes, TArray<FProperty*>& outProperties, TArray<FProperty*>& outStaticProperties)
{
	bool success = true;
	
	//Use reverse order, since properties will get applied to parent in reverse order during UnrealLua::Compiler::AddProperties()
	Algo::Sort(prototypes, [](const FUnrealLuaCompilerUPropertyPrototype& a, const FUnrealLuaCompilerUPropertyPrototype& b)
	{
		return a.LineDefined > b.LineDefined;
	});
	
	for(FUnrealLuaCompilerUPropertyPrototype& propPrototype : prototypes)
	{
		FProperty* newProperty = CompileProperty(propPrototype);
		if (!newProperty)
		{
			success = false;
			break;
		}
		if (propPrototype.IsStaticProperty())
		{
			outStaticProperties.Add(newProperty);
		}
		else
		{
			outProperties.Add(newProperty);
		}
	}
	
	if (!success)
	{
		for (FProperty* prop : outProperties)
		{
			delete prop;
		}
		outProperties.Empty();
		for (FProperty* prop : outStaticProperties)
		{
			delete prop;
		}
		outStaticProperties.Empty();
	}
	
	return success;
}

bool UnrealLua::Compiler::AddProperties(TArray<FProperty*>& properties, TObjectPtr<UStruct> owner)
{
	bool everythingOk = true;
	if (!IsValid(owner))
	{
		everythingOk = false;;
	}
	if (!everythingOk)
	{
		for (FProperty* prop : properties)
		{
			delete prop;
		}
		properties.Empty();
		return false;
	}
	
	for (FProperty* prop : properties)
	{
		owner->AddCppProperty(prop);
		prop->Owner = owner;
		if (FArrayProperty* arrProp = CastField<FArrayProperty>(prop))
		{
			verify(arrProp->Inner != nullptr);
			arrProp->Inner->Owner = arrProp;
		}
		else if (FSetProperty* setProp = CastField<FSetProperty>(prop))
		{
			verify(setProp->ElementProp != nullptr);
			setProp->ElementProp->Owner = setProp;
		}
		else if (FMapProperty* mapProp = CastField<FMapProperty>(prop))
		{
			verify(mapProp->KeyProp != nullptr);
			verify(mapProp->ValueProp != nullptr);
			mapProp->KeyProp->Owner = mapProp;
			mapProp->ValueProp->Owner = mapProp;
		}
	}
	properties.Empty();
	return true;
}

bool UnrealLua::Compiler::HasField(const FName& nameToCheck, const TObjectPtr<UStruct> ustruct)
{
	return ustruct->FindPropertyByName(nameToCheck) != nullptr;
}

bool UnrealLua::Compiler::HasFunction(const FName& nameToCheck, const TObjectPtr<UClass> uclass)
{
	if(!uclass)
	{
		return false;
	}
	return uclass->FindFunctionByName(nameToCheck, EIncludeSuperFlag::IncludeSuper) != nullptr;
}
