#pragma once
#include <source_location>
#include "Utility/LuaLogMacros.h"

#define LUA_LOG_STACK_CONVERSION false
#if LUA_LOG_STACK_CONVERSION
#define LOCAL_FUNC_LOG() LOCAL_FUNC_LOG_IMPL();
#else
#define LOCAL_FUNC_LOG() ;

#endif

inline void UNREALLUA_API LOCAL_FUNC_LOG_IMPL(const std::source_location location = std::source_location::current())
{
	LUA_LOG("Func : %hs (%lu: %lu) `%hs`",location.file_name(),location.line(),location.column(), location.function_name())
} 
