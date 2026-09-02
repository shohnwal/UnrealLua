// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_CallLuaFunction.h"
#include "UObject/Object.h"
#include "K2Node_CallLuaRPCFunction.generated.h"

/**
 * 
 */
UCLASS(HideCategories=(Output))
class UNREALLUAEDITOR_API UK2Node_CallLuaRPCFunction : public UK2Node_LuaVariantFunction
{
	GENERATED_BODY()
public:
	UK2Node_CallLuaRPCFunction();
	
	virtual void AllocateDefaultPins() override;
	
};
