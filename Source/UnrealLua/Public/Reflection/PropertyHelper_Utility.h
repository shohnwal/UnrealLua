#pragma once
#include "CoreMinimal.h"
#include "UObject/UnrealType.h"
#include "sol/forward.hpp"

class FReferenceCollector;
class FProperty;
enum EPropertyFlags : uint64;

namespace UnrealLua::PropertyHelper
{
	UNREALLUA_API bool IsCompatibleType(FProperty* prop, sol::object luaValue);
	UNREALLUA_API bool IsCompatibleType(FProperty* prop, sol::stack_object luaValue);
	
	UNREALLUA_API inline bool IsInputParameter(FProperty* InParam)
	{
		const bool bIsParam = InParam->HasAnyPropertyFlags(CPF_Parm);
		const bool bIsReturnParam = InParam->HasAnyPropertyFlags(CPF_ReturnParm);
		const bool bIsReferenceParam = InParam->HasAnyPropertyFlags(CPF_ReferenceParm);
		const bool bIsOutParam = InParam->HasAnyPropertyFlags(CPF_OutParm) && !InParam->HasAnyPropertyFlags(CPF_ConstParm);
		return bIsParam && !bIsReturnParam && (!bIsOutParam || bIsReferenceParam);
	}
	UNREALLUA_API inline bool IsOutputParameter(FProperty* InParam)
	{
		const bool bIsReturnParam = InParam->HasAnyPropertyFlags(CPF_ReturnParm);
		const bool bIsOutParam = InParam->HasAnyPropertyFlags(CPF_OutParm) && !InParam->HasAnyPropertyFlags(CPF_ConstParm);
		return !bIsReturnParam && bIsOutParam;
	}

	UNREALLUA_API bool CanPropertyContainObjectReferences(FProperty* prop);
	UNREALLUA_API uint32 AddRefByProperty(FReferenceCollector& collector, FProperty* prop, void* memory, bool container = true);
	UNREALLUA_API uint32 AddRefByStruct(FReferenceCollector& collector, const UScriptStruct* scriptStruct, void* data);
	UNREALLUA_API FString GetPropertyFlagsString(EPropertyFlags Flags);
	
	UNREALLUA_API FString GetPropertyTypeName(FProperty* prop, bool luaStyle = true);
	//FLuaPropertyReference MakePropertyWrapper(FProperty* prop);
	UNREALLUA_API void HandleGetPropertyNetBehavior(UObject* Object, FProperty* prop);
	UNREALLUA_API void HandleSetPropertyNetBehavior(UObject* Object, FProperty* prop);
}