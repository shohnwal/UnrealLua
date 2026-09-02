#pragma once
#include "CoreMinimal.h"
#include "LuaContext/LuaPath.h"

class ILuaContext;
struct FScopedLuaContext;

/*
 * basePath is based on ProjectDir/Game/[basePath arg]
 * Basepath prefix-"/" and postfix-"/" is optional, will get corrected before scan
 *
 * FLuaFileFinder finder = (ctx, "Skill/") //looks in FPaths::ProjectDir()/Lua/[Directories]/Skill/
 * Directories are (in order):
 * ===Mods with specific gamemode have highest priority===
 * - ModsDir()/Mod1/Lua/<GameMode>/ 
 * - ProjectDir()/Lua/<GameMode>/
 * - ModsDir()/Mods/Mod1/Lua/Game/
 * - ProjectDir/Lua/Game/
 * ===Unmodded base gamemode have lowest priority===
 */
struct UNREALLUA_API FLuaFileLister
{
	FLuaFileLister( const TArray<FString>& excludeStartsWith = {}, const TArray<FString>& excludeEndsWith = {});
	FLuaFileLister(FScopedLuaContext& ctx, const FString& appendedPath, bool bRecursiveSearch = false, const TArray<FString>& excludeStartsWith = {}, const TArray<FString>& excludeEndsWith = {});
	FLuaFileLister(ILuaContext& ctx, const FString& appendedPath, bool bRecursiveSearch = false, const TArray<FString>& excludeStartsWith = {}, const TArray<FString>& excludeEndsWith = {});
	FLuaFileLister(const FLuaPath& paths, const FString& appendedPath, bool bRecursiveSearch = false, const TArray<FString>& excludeStartsWith = {}, const TArray<FString>& excludeEndsWith = {});
	
	FString LuaSubPath;
	const TArray<FString>& StartsWithFilter;
	const TArray<FString>& EndsWithFilter;
	FLuaPath LuaPaths;

	TSet<FString> UniqueFileNames;
	TSet<FString> FoundFullFilePaths;

	void RemoveAllUniqueFilenames(const TFunction<bool(FString&)>& func);
	
private:
	void FindFiles(bool recursive);
	bool DoesFileNamePassFilter(const FString& fileName) const;
};
