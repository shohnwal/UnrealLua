#include "Utility/LuaFIleSystemLogMacros.h"

DEFINE_LOG_CATEGORY(LuaFiles)

void UnrealLua::FileSystem::Log::LogError(const FString& msg)
{
	UE_LOG(LuaFiles, Error, TEXT("%s"), *msg)
}

void UnrealLua::FileSystem::Log::LogWarning(const FString& msg)
{
	UE_LOG(LuaFiles, Warning, TEXT("%s"), *msg)
}

void UnrealLua::FileSystem::Log::Log(const FString& msg)
{
	UE_LOG(LuaFiles, Log, TEXT("%s"), *msg)
}
