// Fill out your copyright notice in the Description page of Project Settings.
#include "Compilers/InterfaceCompiler.h"

#include "Utility/LuaLogMacros.h"
#include "UObject/Class.h"
#include "UObject/Object.h"
#include "UObject/Package.h"
#include "UnrealLuaCompiler.h"
#include "UnrealLua/Public/UnrealOverrides/UnrealLuaCompiledUFunction.h"
#include "Compilers/FunctionCompiler.h"
#include "Prototypes/InterfacePrototype.h"


// From Bind_BlueprintEvent.cpp
extern UFunction* GetBlueprintEventByScriptName(UClass* Class, const FString& ScriptName);

static const FName NAME_ExposeOnSpawn(TEXT("ExposeOnSpawn"));
static const FName NAME_EditFixedSize(TEXT("EditFixedSize"));
static const FName NAME_DisplayName(TEXT("DisplayName"));
static const FName NAME_NoBlueprintsOfChildren(TEXT("NoBlueprintsOfChildren"));
static const FName NAME_Evt_ScriptName(TEXT("ScriptName"));
static const FName NAME_AllowPrivateAccess(TEXT("AllowPrivateAccess"));
static const FName NAME_Meta_EditorOnly(TEXT("EditorOnly"));

static const FName NAME_Class_DefaultConfig(TEXT("DefaultConfig"));
static const FName NAME_Actor_DefaultComponent(TEXT("DefaultComponent"));
static const FName NAME_Actor_OverrideComponent(TEXT("OverrideComponent"));
static const FName NAME_Actor_BindComponent(TEXT("BindComponent"));
static const FName NAME_Actor_RootComponent(TEXT("RootComponent"));
static const FName NAME_Actor_Attach(TEXT("Attach"));
static const FName NAME_Actor_AttachSocket(TEXT("AttachSocket"));
static const FName NAME_AnyStructRef(TEXT("__ANY_STRUCT_REF"));
static const FName NAME_Function_MixinArgument(TEXT("MixinArgument"));
static const FName NAME_Function_DefaultToSelf(TEXT("DefaultToSelf"));

const static FName FUNCMETA_BlueprintThreadSafe("BlueprintThreadSafe");
const static FName FUNCMETA_NotBlueprintThreadSafe("NotBlueprintThreadSafe");
const static FName FUNCMETA_BlueprintProtected("BlueprintProtected");
const static FName FUNCMETA_CrumbFunction("CrumbFunction");
const static FName FUNCMETA_ScriptNoOp("ScriptNoOp");

UClass* UnrealLua::Compiler::CreateSkeletonInterfaceClass(FUnrealLuaCompilerUInterfacePrototype& prototype, UPackage* destinationPackage)
{
	FString newClassName = prototype.TypeName.ToString();
	
	UObject* existing = FindObjectSafe<UObject>(destinationPackage, *newClassName);
	if (existing)
	{
		LUA_LOG_ERROR("Can not compile Lua-Compiled UInterface %s from file %s: A type with such a name already exists", *newClassName, *prototype.FileName);
		return nullptr;
	}
	
	UClass* baseInterfaceClass = UInterface::StaticClass();
	
	UClass* newClass = NewObject<UClass>(destinationPackage, *newClassName, RF_Public | RF_Standalone | RF_MarkAsRootSet /*| RF_Transient*/);
	newClass->SetSuperStruct(baseInterfaceClass);
	newClass->ClassWithin = baseInterfaceClass->ClassWithin ? ToRawPtr(baseInterfaceClass->ClassWithin) : ToRawPtr(UObject::StaticClass());
	//newClass->ClassFlags |= CLASS_NotPlaceable | CLASS_Hidden;
	//newClass->ClassFlags |= CLASS_CompiledFromBlueprint;
	newClass->ClassFlags |= (baseInterfaceClass->ClassFlags & CLASS_ScriptInherit);
	newClass->ClassFlags |= CLASS_Interface;
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
		newClass->ClassConfigName = baseInterfaceClass->ClassConfigName;
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
		newClass->SetMetaData(NAME_DisplayName, *DisplayString);
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

bool UnrealLua::Compiler::CompileInterfacePrototype(UnrealLua::Compiler::FUnrealLuaCompilerUInterfacePrototype& prototype)
{
	if (prototype.HasFinishedCompilation())
	{
		return true;
	}
	FString newClassName = prototype.TypeName.ToString();
	
	UClass* newClass = prototype.GetCompiledClass();
	verify(IsValid(newClass))
	
	if (!prototype.ParseBody())
	{
		return false;
	}
	
	if (!CompileUFunctions(newClass, prototype.GetFunctionPrototypes()))
	{
		newClass->ConditionalBeginDestroy();
		return false;		
	}
	
	newClass->Bind();
	newClass->AddToRoot();
	
	newClass->StaticLink(true);
	newClass->AssembleReferenceTokenStream(true);
	
	UObject* cdo = newClass->GetDefaultObject(true);
	if (!IsValid(cdo))
	{
		return false;
	}
	prototype.SetFinishedCompilation();
	return true;
}
