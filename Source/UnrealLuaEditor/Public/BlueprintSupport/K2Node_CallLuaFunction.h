// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_LuaVariantFunction.h"
#include "K2Node_CallLuaFunction.generated.h"

/**
 * 
 */
UCLASS()
class UNREALLUAEDITOR_API UK2Node_CallLuaFunction : public UK2Node_LuaVariantFunction
{
	GENERATED_BODY()
public:
	UK2Node_CallLuaFunction();
	
	//~ Begin UEdGraphNode Interface.
	virtual void AllocateDefaultPins() override;
	//~ End UEdGraphNode Interface.
};
