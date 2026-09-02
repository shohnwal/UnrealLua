#pragma once
#include "CoreMinimal.h"
#include "UnrealLuaToolDelegates.generated.h"

class UUnrealLuaTool;

UDELEGATE()

DECLARE_DYNAMIC_DELEGATE_OneParam(FUnrealLuaTooleActivateDynamicCallback, UUnrealLuaTool*, tool);
DECLARE_DELEGATE_OneParam(FUnrealLuaTooleActivateCallback, UUnrealLuaTool*);