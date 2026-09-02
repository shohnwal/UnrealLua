// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_AddPinInterface.h"
#include "K2Node_LuaVariantFunction_SynchronizedInputOutput.h"
#include "K2Node_MakeLuaValue.generated.h"

/**
 * 
 */
UCLASS()
class UNREALLUAEDITOR_API UK2Node_MakeLuaValue : public UK2Node_LuaVariantFunction_SynchronizedInputOutput 
{
	GENERATED_BODY()
public:
	UK2Node_MakeLuaValue();
	virtual void PostInitProperties() override;
	virtual void PostLoad() override;
	virtual void AllocateDefaultPins() override;
protected:
	virtual FEdGraphPinType GetDefaultPinTypeForPin(UEdGraphPin* Pin) override;
};
