#pragma once

class UPackage;
class UField;

namespace UnrealLua::Compiler
{
	struct FUnrealLuaCompilerUEnumPrototype;
	UField* CreateAndFillEnum(FUnrealLuaCompilerUEnumPrototype& proto, UPackage* destinationPackage);
}
