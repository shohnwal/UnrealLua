// Fill out your copyright notice in the Description page of Project Settings.


#include "Compilers/ScriptStructCompiler.h"
#include "CoreMinimal.h"
#include "Utility/LuaLogMacros.h"
#include "UnrealLuaCompiler.h"
#include "UObject/Object.h"
#include "UObject/Package.h"
#include "Compilers/PropertyCompiler.h"

#include "Prototypes/UScriptStructPrototype.h"
#include "Prototypes/UFunctionPrototype.h"

UScriptStruct* UnrealLua::Compiler::CreateSkeletonScriptStruct(FUnrealLuaCompilerUScriptStructPrototype& prototype, UPackage* destinationPackage)
{
	const FString newStructName = prototype.TypeName.ToString();;
	
	UObject* existing = FindObjectSafe<UObject>(destinationPackage, *newStructName);
	if (existing)
	{
		prototype.Compiler->SetError(FString::Printf(TEXT("Can not compile Lua-Compiled ScriptSturct %s from file %s: A type with such a name already exists"), *newStructName, *prototype.FileName));
		return nullptr;
	}

	UScriptStruct* parent = prototype.GetParentStruct();
	if (HasFieldNameConflicts(prototype.GetPropertyPrototypes(), {}, parent))
	{
		prototype.Compiler->SetError(FString::Printf(TEXT("Can not compile Lua-Compiled ScriptSturct %s from file %s: The parent ScriptStruct %s has a conflicting property name!"), *newStructName, *prototype.FileName, *GetNameSafe(parent)));
		return nullptr;
	}
	UScriptStruct* newScriptStruct = NewObject<UScriptStruct>(destinationPackage, *newStructName, RF_Public | RF_MarkAsNative);
	prototype.SetCompiledStruct(newScriptStruct);
	return newScriptStruct;
}

UScriptStruct* UnrealLua::Compiler::CompileScriptStructPrototype(FUnrealLuaCompilerUScriptStructPrototype& prototype)
{
	const FString newStructName = prototype.TypeName.ToString();;
	LUA_LOG_WARNING("Compilation of ScriptStruct prototype %s", *newStructName)
	verify(IsValid(prototype.GetCompiledScriptStruct()))
	//no duplicate fields, safe to create FProperties now!
	
	TArray<FProperty*> properties{};
	TArray<FProperty*> staticProperties{};
	if (!CompileProperties(prototype.GetPropertyPrototypes(), properties, staticProperties))
	{
		verify(properties.IsEmpty());
		verify(staticProperties.IsEmpty());
		return nullptr;
	}
	//structs should not have any static properties
	verify(staticProperties.IsEmpty());
	
	TObjectPtr<UScriptStruct> newScriptStruct = prototype.GetCompiledScriptStruct();
#if WITH_METADATA
	newScriptStruct->SetMetaData(TEXT("BlueprintType"), TEXT("true"));
#endif
	
	AddProperties(properties, newScriptStruct);
	
	newScriptStruct->SetSuperStruct(nullptr);
	newScriptStruct->Bind();
	newScriptStruct->StaticLink(/*RelinkExistingProperties*/true);
	
	prototype.SetFinishedCompilation();
	
	return newScriptStruct;
}
