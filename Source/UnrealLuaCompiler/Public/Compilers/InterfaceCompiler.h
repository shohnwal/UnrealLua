
#pragma once

class UPackage;
class UClass;
/**
 * 
 */
namespace UnrealLua::Compiler
{
	struct FUnrealLuaCompilerUInterfacePrototype;
	
	extern UClass* CreateSkeletonInterfaceClass(UnrealLua::Compiler::FUnrealLuaCompilerUInterfacePrototype& prototype, UPackage* destinationPackage);
	extern bool CompileInterfacePrototype(UnrealLua::Compiler::FUnrealLuaCompilerUInterfacePrototype& prototype);
}