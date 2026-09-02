// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "Containers/Array.h"

/**
 * 
 */

class FProperty;
class UClass;
class UStruct;

namespace UnrealLua::Compiler
{
	struct FUnrealLuaCompilerUPropertyPrototype;
	struct FUnrealLuaCompilerUFunctionPrototype;
	
	extern bool HasFieldNameConflicts(const TArray<FUnrealLuaCompilerUPropertyPrototype>& propPrototypes, const TArray<FUnrealLuaCompilerUFunctionPrototype>& funcPrototypes, UStruct* parentStruct);
	extern bool HasPropertyNameConflicts(const TArray<FUnrealLuaCompilerUPropertyPrototype>& prototypes, UStruct* parentStruct);
	extern bool HasFunctionNameConflicts(const TArray<FUnrealLuaCompilerUFunctionPrototype>& prototypes, UClass* parentClass);
	
	extern bool CompileProperties(TArray<FUnrealLuaCompilerUPropertyPrototype>& prototypes, TArray<FProperty*>& outProperties, TArray<FProperty*>& outStaticProperties);
	
	//Add properties to a UStruct
	//This does not check for duplicate property names!
	//Will empty the properties array in the process
	extern bool AddProperties(TArray<FProperty*>& properties, TObjectPtr<UStruct> owner);
	
	extern bool HasField(const FName& nameToCheck, const TObjectPtr<UStruct> ustruct);
	extern bool HasFunction(const FName& nameToCheck, const TObjectPtr<UClass> uclass);
}