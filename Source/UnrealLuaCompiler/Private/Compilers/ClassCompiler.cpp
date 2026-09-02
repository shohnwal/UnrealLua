// Fill out your copyright notice in the Description page of Project Settings.
#include "Compilers/ClassCompiler.h"

#include "Reflection/PropertyHelper.h"
#include "Reflection/PropertyHelperTypes.h"
#include "Utility/LuaLogMacros.h"
#include "UObject/Class.h"
#include "Prototypes/UClassPrototype.h"
#include "UObject/Object.h"
#include "UObject/Package.h"
#include "UnrealLuaCompiler.h"
#include "UnrealLuaCompilerConstants.h"
#include "UnrealLua/Public/UnrealOverrides/UnrealLuaCompiledUFunction.h"
#include "Compilers/FunctionCompiler.h"
#include "Compilers/PropertyCompiler.h"


// From Bind_BlueprintEvent.cpp
extern UFunction* GetBlueprintEventByScriptName(UClass* Class, const FString& ScriptName);


namespace UnrealLua::CompiledUClass
{
	void UClassConstructor(const FObjectInitializer& initializer)
	{
		UObject* Object = initializer.GetObj();
		// We need to run the C++ constructor first so everything is valid
		UClass* objectClass = Object->GetClass();
		UClass* constructorClass = objectClass;
		const bool isCDO = Object->HasAnyFlags(RF_ClassDefaultObject);
		UObject* cdo = objectClass->GetDefaultObject<UObject>();
		while (true)
		{
			if (objectClass->ClassConstructor != UnrealLua::CompiledUClass::UClassConstructor)
			{
				break;
			}
			if (!isCDO)
			{
				for (TFieldIterator<FProperty> it(objectClass, EFieldIterationFlags::None); it; ++it)
				{
					FProperty* prop = *it;
					prop->CopyCompleteValue_InContainer(Object, cdo);
				}
			}
			objectClass = objectClass->GetSuperClass();
		}
		if(!isCDO)
		{

		}

		verify(IsValid(objectClass));
		objectClass->ClassConstructor(initializer);
		
		if (!isCDO)
		{
			/*
			static const FName EventTickName(TEXT("ReceiveTick"));
			if (UFunction* func = objectClass->FindFunctionByName(EventTickName))
			{
				if(AActor* actor = Cast<AActor>(cdo))
				{
					actor->PrimaryActorTick.bCanEverTick = true;
				}
				else if (UActorComponent* cmp = Cast<UActorComponent>(cdo))
				{
					cmp->PrimaryComponentTick.bCanEverTick = true;
				}
			}
				
			 */
		}
	
		//@TODO : Call lua script initialization UserConstructionScript?
	}
}
UClass* UnrealLua::Compiler::CreateSkeletonClass(FUnrealLuaCompilerUClassPrototype& prototype, UPackage* destinationPackage)
{
	FString newClassName = prototype.TypeName.ToString();
	
	UObject* existing = FindObjectSafe<UObject>(destinationPackage, *newClassName);
	if (existing)
	{
		LUA_LOG_ERROR("Can not compile Lua-Compiled UClass %s from file %s: A type with such a name already exists", *newClassName, *prototype.FileName);
		return nullptr;
	}
	
	UClass* parentClass = prototype.ParentClassPath.TryLoadClass<UObject>();
	if (!parentClass)
	{
		//parent class not being able to load is not an error, perhaps it is not compiled yet
		return nullptr;
	}
	prototype.ParentClass = parentClass;
	if (HasFieldNameConflicts(prototype.GetPropertyPrototypes(), prototype.GetFunctionPrototypes(), parentClass))
	{
		LUA_LOG_ERROR("Can not compile Lua-Compiled UClass %s from file %s: The class has a conflicting property name with a property or function of parent UClass %s!", *newClassName, *prototype.FileName, *GetNameSafe(parentClass));
		return nullptr;
	}
	UClass* newClass = NewObject<UClass>(destinationPackage, *newClassName, RF_Public | RF_Standalone | RF_MarkAsRootSet | RF_MarkAsNative /*| RF_Transient*/);
	newClass->SetSuperStruct(parentClass);
	newClass->ClassConstructor = &UnrealLua::CompiledUClass::UClassConstructor;
	newClass->ClassWithin = parentClass->ClassWithin ? ToRawPtr(parentClass->ClassWithin) : ToRawPtr(UObject::StaticClass());
	//newClass->ClassFlags |= CLASS_NotPlaceable | CLASS_Hidden;
	newClass->ClassFlags |= CLASS_CompiledFromBlueprint;
	newClass->ClassFlags |= (parentClass->ClassFlags & CLASS_ScriptInherit);
	newClass->ClassFlags |= CLASS_Native;
#if WITH_METADATA
	newClass->SetMetaData(TEXT("BlueprintType"), TEXT("true"));
	newClass->SetMetaData(TEXT("IsBlueprintBase"), TEXT("true"));
#endif
	FString configName = "";
	
	if (!configName.IsEmpty())
	{
		newClass->ClassFlags |= CLASS_Config;
		newClass->ClassConfigName = FName(*configName);
	}
	else
	{
		newClass->ClassConfigName = parentClass->ClassConfigName;
	}
	
	if ((newClass->ClassFlags & CLASS_Config) != 0)
	{
		//if (ClassDesc->Meta.Contains(NAME_Class_DefaultConfig))
		//	NewClass->ClassFlags |= CLASS_DefaultConfig;
	}
	
#if WITH_EDITOR
	//CopyClassInheritedMetaData(SuperClass, NewClass);
	
	if (!prototype.Meta.Contains("DisplayName"))
	{
		FString DisplayString = newClass->GetName();
		DisplayString = FName::NameToDisplayString(DisplayString, false);
		newClass->SetMetaData(CompilerConstants::NAME_DisplayName, *DisplayString);
	}
	
	for (auto& Elem : prototype.Meta)
	{
		newClass->SetMetaData(*Elem.Key, *Elem.Value);
	}
	
	if (prototype.Meta.Contains(TEXT("NotBlueprintable")))
	{
		newClass->SetMetaData(TEXT("IsBlueprintBase"), TEXT("false"));
		newClass->RemoveMetaData(TEXT("Blueprintable"));
	}
	else if (prototype.Meta.Contains(TEXT("Blueprintable")))
	{
		newClass->SetMetaData(TEXT("IsBlueprintBase"), TEXT("true"));
		newClass->SetMetaData(TEXT("BlueprintType"), TEXT("true"));
		newClass->RemoveMetaData(TEXT("NotBlueprintable"));
	}
	// Don't inherit BlueprintThreadSafe ever, at least not until we have multithreading in Lua
	if (!prototype.Meta.Contains("BlueprintThreadSafe"))
	{
		newClass->RemoveMetaData("BlueprintThreadSafe");
	}
#endif

	
	prototype.SetCompiledClass(newClass);
	return newClass;
}

bool UnrealLua::Compiler::CompileClassPrototype(FUnrealLuaCompilerUClassPrototype& prototype)
{
	if (prototype.HasFinishedCompilation())
	{
		return true;
	}
	if (prototype.ParentClassPrototype != nullptr)
	{
		check(prototype.Compiler->UnrealLuaCompilerUTypePackage == prototype.ParentClass->GetPackage())
		if (!CompileClassPrototype(*prototype.ParentClassPrototype))
		{
			return false;
		}
		verify(prototype.ParentClassPrototype->HasFinishedCompilation())
	}
	else
	{
		check(prototype.Compiler->UnrealLuaCompilerUTypePackage != prototype.ParentClass->GetPackage())
	}
	FString newClassName = prototype.TypeName.ToString();
	
	UClass* newClass = prototype.GetCompiledClass();
	verify(IsValid(newClass));
	
	UClass* parentClass = prototype.GetParentClass();
	verify(IsValid(parentClass));
	verify(parentClass->IsNative());

	verify(newClass->GetSuperStruct() == parentClass);
	
	for (TObjectPtr<UClass> interfaceClass : prototype.Interfaces)
	{
		verify(IsValid(interfaceClass));
		verify(interfaceClass->IsChildOf(UInterface::StaticClass()));
		
		//Check if parent implements that interface already
		if (parentClass->ImplementsInterface(interfaceClass))
		{
			continue;
		}
		
		//if no, we must check all interface function for compatibility issues
		for (TFieldIterator<UFunction> interface_it(interfaceClass); interface_it; ++interface_it)
		{
			UFunction* interfaceFunc = *interface_it;
			FName interfaceFuncName = interfaceFunc->GetFName();
			UFunction* parentFunc = parentClass->FindFunctionByName(interfaceFuncName);
			if (parentFunc)
			{
				if (!parentFunc->IsSignatureCompatibleWith(interfaceFunc))
				{
					FString incompatibilityReason = "";
					bool compatible = UnrealLua::Compiler::ExamineUFunctionSignatureCompatibility(interfaceFunc, parentFunc, incompatibilityReason);
					verify(!compatible);
					UUnrealLuaCompiler::SetIsErrorWithArgs(TEXT("Error while trying to compile class prototype {0]:\nInterface UFunction {1}\n\n({2})\n\nhas signature incompatible with parent UFunction {3}\n\n({4})\n\nReason:\n{5}!"), *prototype.GetTypeNameString(), *GetNameSafe(interfaceFunc), *GetFullNameSafe(interfaceFunc), *GetNameSafe(parentFunc), *GetFullNameSafe(parentFunc), *incompatibilityReason);
					return false;	
				}
			}
		}
		
		FImplementedInterface interface{interfaceClass, 0, true};
		newClass->Interfaces.Add(interface);
	}
	
	//no duplicate fields, safe to create FProperties now!
	
	if (!prototype.ParseBody())
	{
		return false;
	}
	
	TArray<FProperty*> properties{};
	TArray<FProperty*> staticProperties{};
	if (!CompileProperties(prototype.GetPropertyPrototypes(), properties, staticProperties))
	{
		verify(properties.IsEmpty());
		verify(staticProperties.IsEmpty());
		return false;
	}
	
	if (!CompileUFunctions(newClass, prototype.GetFunctionPrototypes()))
	{
		for (FProperty* prop : properties)
		{
			delete prop;
		}
		properties.Empty();
		newClass->ConditionalBeginDestroy();
		return false;		
	}
	
	//Handle static data/sparse class data
	
	void* staticData = nullptr;
	UScriptStruct* staticDataStruct = nullptr;
	
	if (!staticProperties.IsEmpty())
	{
		UScriptStruct* parentStaticDataStruct = parentClass->GetSparseClassDataStruct();
		staticDataStruct = NewObject<UScriptStruct>(newClass,"StaticData", RF_Public | RF_MarkAsNative);
		staticDataStruct->SetSuperStruct(parentStaticDataStruct);
		
		//For now, static properties can not be saved
		for (FProperty* staticProperty : staticProperties)
		{
			//staticProperty->PropertyFlags |= CPF_Transient;
			staticProperty->PropertyFlags &= ~CPF_BlueprintReadOnly;
			staticProperty->PropertyFlags |= CPF_BlueprintVisible;
			staticProperty->PropertyFlags |= CPF_Edit;
		}
		AddProperties(staticProperties, staticDataStruct);;
		
		staticDataStruct->StaticLink(true);
		staticDataStruct->Bind();
		staticDataStruct->AddToCluster(newClass);
		newClass->SetSparseClassDataStruct(staticDataStruct);
		staticData = newClass->GetOrCreateSparseClassData();
		LUA_LOG("Created static data: %s", *staticDataStruct->GetPathName())
		for (TFieldIterator<FProperty> it(staticDataStruct); it; ++it)
		{
			LUA_LOG("Static property: %s %s", *it->GetFullName(), *it->GetPathName());
		}
	}
	
	newClass->PropertyLink = parentClass->PropertyLink;
	
	AddProperties(properties, newClass);
	
	if (newClass->ChildProperties)
	{
		//newClass->PropertyLink = newClass->ChildProperties;
	}
	newClass->Bind();
	newClass->AddToRoot();
	
	/*
	if(!prototype.bPlaceable)
		newClass->ClassFlags |= CLASS_NotPlaceable; 
	else
		newClass->ClassFlags &= ~CLASS_NotPlaceable; 

	if (prototype.bAbstract)
		newClass->ClassFlags |= CLASS_Abstract;

	if (prototype.bTransient)
		newClass->ClassFlags |= CLASS_Transient;

	if (prototype.bHideDropdown)
		newClass->ClassFlags |= CLASS_HideDropDown;

	if (prototype.bDefaultToInstanced)
		newClass->ClassFlags |= CLASS_DefaultToInstanced;

	if (prototype.bEditInlineNew)
		newClass->ClassFlags |= CLASS_EditInlineNew;

	if (prototype.bIsDeprecatedClass)
		newClass->ClassFlags |= CLASS_Deprecated;
	*/
	newClass->StaticLink(true);
	newClass->AssembleReferenceTokenStream(true);
	prototype.SetFinishedCompilation();
	
	UObject* cdo = newClass->GetDefaultObject(true);
	if (!IsValid(cdo))
	{
		return false;
	}
	
	FUnrealLuaCompilerUClassPrototype* fieldContainerProto = &prototype;
	do
	{
		verify(fieldContainerProto->HasFinishedCompilation())
		for (FUnrealLuaCompilerUPropertyPrototype& propProto : fieldContainerProto->GetProperties())
		{
			if(!propProto.EvaluatedDefaultValue.valid())
			{
				continue;
			}
			FProperty* prop = propProto.CompiledProperty;
			verify(prop != nullptr);
			if (propProto.IsStaticProperty())
			{
				verify(staticData != nullptr);
				verify(staticDataStruct != nullptr);
				verify(prop->Owner == fieldContainerProto->GetCompiledClass()->GetSparseClassDataStruct());
				
				prop->InitializeValue_InContainer(staticData);
				TSetPropertyValueParams params{prop, staticData, 0, propProto.EvaluatedDefaultValue};
				UnrealLua::PropertyHelper::SetPropertyValue_InContainer(params);
			}
			else
			{
				verify(prop->Owner == fieldContainerProto->GetCompiledClass());
				prop->InitializeValue_InContainer(cdo);
				TSetPropertyValueParams params{prop, cdo, 0, propProto.EvaluatedDefaultValue};
				UnrealLua::PropertyHelper::SetPropertyValue_InContainer(params);
			}
			 

		}	
		fieldContainerProto = fieldContainerProto->ParentClassPrototype;
	}
	while (fieldContainerProto);
	/*
	static const FName EventTickName(TEXT("ReceiveTick"));
	if (UFunction* func = newClass->FindFunctionByName(EventTickName))
	{
		if(AActor* actor = Cast<AActor>(cdo))
		{
			actor->PrimaryActorTick.bCanEverTick = true;
		}
		else if (UActorComponent* cmp = Cast<UActorComponent>(cdo))
		{
			cmp->PrimaryComponentTick.bCanEverTick = true;
		}
	}*/
	
	return true;
}
