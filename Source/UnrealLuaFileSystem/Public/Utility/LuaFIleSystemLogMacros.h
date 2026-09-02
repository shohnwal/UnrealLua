#pragma once
#include "Logging/LogMacros.h"
#define LUA_ANY_LOGGING

UNREALLUAFILESYSTEM_API DECLARE_LOG_CATEGORY_EXTERN(LuaFiles, Log, All); 

#ifdef LUA_ANY_LOGGING
#include "Logging/LogVerbosity.h"
#define LUA_LOGGING
#endif

namespace UnrealLua::FileSystem::Log
{
	UNREALLUAFILESYSTEM_API void LogError(const FString& msg); 
	UNREALLUAFILESYSTEM_API void LogWarning(const FString& msg); 
	UNREALLUAFILESYSTEM_API void Log(const FString& msg); 
}

#ifdef LUA_LOGGING
#define LUA_FILES_LOG_ERROR(str, ...) UnrealLua::FileSystem::Log::LogError(FString::Printf(TEXT(str), ##__VA_ARGS__));
#define LUA_FILES_LOG_WARNING(str, ...) UnrealLua::FileSystem::Log::LogWarning(FString::Printf(TEXT(str), ##__VA_ARGS__));
#define LUA_FILES_LOG(str, ...)  UnrealLua::FileSystem::Log::Log(FString::Printf(TEXT(str), ##__VA_ARGS__));
#define LUA_FILES_LOG_CONDITIONAL(cond, str, ...) if(cond) { UUnrealLua::Log::Log(FString::Printf(TEXT(str), ##__VA_ARGS__));}
//SET_WARN_COLOR(COLOR_CYAN);\
//\
//CLEAR_WARN_COLOR();
#else
#define LUA_FILES_LOG_ERROR(str, ...)
#define LUA_FILES_LOG_WARNING(str, ...)
#define LUA_FILES_LOG(str, ...)
#define LUA_FILES_LOG_CONDITIONAL(cond, str, ...) if(cond) { UUnrealLua::Log::Log(FString::Printf(TEXT(str), ##__VA_ARGS__));}
#endif

#ifndef __LINE__
#define __builtin_LINE() __LINE__
#endif
#ifndef __FILE__
#define __builtin_FILE() __FILE__
#endif

