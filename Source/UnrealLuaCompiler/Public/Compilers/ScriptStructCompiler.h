// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


class UPackage;
class UScriptStruct;
/**
 * 
 */
namespace UnrealLua::Compiler
{
	struct FUnrealLuaCompilerUScriptStructPrototype;
	
	extern UScriptStruct* CreateSkeletonScriptStruct(FUnrealLuaCompilerUScriptStructPrototype& prototype, UPackage* destinationPackage);

	extern UScriptStruct* CompileScriptStructPrototype(FUnrealLuaCompilerUScriptStructPrototype& prototype);
};
