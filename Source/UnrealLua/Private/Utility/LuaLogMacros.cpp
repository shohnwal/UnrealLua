
#include "Utility/LuaLogMacros.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY(LuaLog)

void UnrealLua::Log::LogError(const FString& msg)
{
	
	UE_LOG(LuaLog, Error, TEXT("%s"), *msg)
}

void UnrealLua::Log::LogWarning(const FString& msg)
{
	//::UE::Logging::Private::BasicLog<::ELogVerbosity::Warning>(LOG_Static, &LuaLog, ##__VA_ARGS__);
	UE_LOG(LuaLog, Warning, TEXT("%s"), *msg)
}

void UnrealLua::Log::Log(const FString& msg)
{
	UE_LOG(LuaLog, Log, TEXT("%s"), *msg)
};
