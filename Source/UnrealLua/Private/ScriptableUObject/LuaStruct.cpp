#include "ScriptableUObject/LuaStruct.h"

void ULuaStructLib::ExecuteLuaFunction(FString luaFunction, TArray<FLuaValue>& args)
{
}

void ULuaStructLib::ExecuteLuaFunctionNative(const char* const funcName, sol::variadic_args& args)
{
	
}

FLuaScriptSettings FLuaStruct::GetLuaScriptSettings()
{
	return this->LuaScriptSettings;
}
