#pragma once

class UUnrealLuaCompiler;

namespace UnrealLua::Compiler
{
	bool CompilePrototypes(UUnrealLuaCompiler* compiler);
	
	void LoadMissingTypesFromBackups(UUnrealLuaCompiler* compiler);
	void BackupCompiledPrototypes(UUnrealLuaCompiler* compiler);
}
