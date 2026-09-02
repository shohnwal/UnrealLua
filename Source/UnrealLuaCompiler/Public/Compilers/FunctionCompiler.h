// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "Containers/Array.h"
class UClass;
/**
 * 
 */
namespace UnrealLua::Compiler
{
	struct FUnrealLuaCompilerUFunctionPrototype;
	extern bool CompileUFunctions(UClass* owningClass, TArray<FUnrealLuaCompilerUFunctionPrototype>& prototypes);
	extern bool ExamineUFunctionSignatureCompatibility(UFunction* func1, UFunction* func2, FString& outMismaches);
}
