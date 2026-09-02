#pragma once

#include "CoreMinimal.h"

namespace UnrealLua::ParseUtility
{
    FString ParseUFunctionToLuaFunctionTemplateString(const FString& tableName, UFunction* func);
    FString ParsePropertyToLuaFunctionTemplateString(const FString& tableName, FProperty* prop);
    
    FString ParseUFunctionToLuaFunctionAnnontation(UFunction* func, bool signatureOnly, bool withFuncName, bool withAnnotations);
}
