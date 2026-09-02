// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

class UPackage;
class UClass;
/**
 * 
 */
namespace UnrealLua::Compiler
{
	struct FUnrealLuaCompilerUClassPrototype;
	
	extern UClass* CreateSkeletonClass(UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype& prototype, UPackage* destinationPackage);
	extern bool CompileClassPrototype(UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype& prototype);
}