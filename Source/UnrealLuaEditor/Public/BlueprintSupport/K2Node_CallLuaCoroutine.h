// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_LuaVariantFunction.h"
#include "K2Node_CallLuaCoroutine.generated.h"

/**
 * 
 */
UCLASS()
class UNREALLUAEDITOR_API UK2Node_CallLuaCoroutine : public UK2Node_LuaVariantFunction
{
	GENERATED_BODY()
	
	UK2Node_CallLuaCoroutine();
public:
	//~ Begin UEdGraphNode Interface.
	virtual void AllocateDefaultPins() override;
	//~ End UK2Node Interface.
};
