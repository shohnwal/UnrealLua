#pragma once

#include "CoreMinimal.h"
#include "sol/sol.hpp"
#include "LoadedLuaScriptResult.generated.h"

USTRUCT()
struct UNREALLUA_API FLoadedLuaFileInfo
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere)
	FString FullPathOnDisk = {};
	UPROPERTY(VisibleAnywhere)
	FDateTime TimeStamp = {};
	UPROPERTY(VisibleAnywhere)
	FString LoadError;
	bool IsValid() const { return !FullPathOnDisk.IsEmpty() && TimeStamp.GetTicks() >= 0 && LoadError.IsEmpty(); }
};

struct	FLoadLuaScriptResult
{
	FString OriginalFileRequestPath = {};
	bool bIsAbsolutePath = false;
	sol::table FinalResult = {};
	FString ScriptPathName = {};
	TArray<FLoadedLuaFileInfo> MainFileInfo = {};
	TArray<FLoadedLuaFileInfo> ModFileInfos = {};
	TArray<FString> ErrorMessages = {};

	bool IsValid() const { return FinalResult.valid() ;}
};