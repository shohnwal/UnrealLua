#pragma once
#include "CoreMinimal.h"
#include "sol/sol.hpp"


DECLARE_MULTICAST_DELEGATE_OneParam(FLuaStateViewBroadcastDelegate, sol::state_view&);
struct UNREALLUA_API FLuaCoreDelegates
{
	static FLuaStateViewBroadcastDelegate OnRegisterLuaUsertypes;
	static FLuaStateViewBroadcastDelegate OnPerformSelfTest;
};