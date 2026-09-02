#pragma once
#include "LuaTypes/UnrealLuaScriptStructTypeTraits.h"
#include "StructUtils/SharedStruct.h"
#include "StructUtils/InstancedStruct.h"

template<>
struct TUnrealLuaScriptStructTypeTraits<FInstancedStruct> : public TUnrealLuaScriptStructTypeTraitsBase<FInstancedStruct>
{
	enum
	{
		bPushAsLuaScriptStruct = false,
	};
};

template<>
struct TUnrealLuaScriptStructTypeTraits<FSharedStruct> : public TUnrealLuaScriptStructTypeTraitsBase<FSharedStruct>
{
	enum
	{
		bPushAsLuaScriptStruct = false,
	};
};

