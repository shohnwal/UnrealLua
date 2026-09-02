#pragma once
#include "CoreMinimal.h"

template <typename BaseScriptStruct>
	requires requires { BaseScriptStruct::StaticStruct();}
struct TUnrealLuaScriptStructTypeTraitsBase
{
	enum
	{
		bPushAsLuaScriptStruct = true //Struct will be pushed to Lua as FLuaScriptStruct
	};
};

template <typename BaseScriptStruct>
struct TUnrealLuaScriptStructTypeTraits : public TUnrealLuaScriptStructTypeTraitsBase<BaseScriptStruct>
{

};