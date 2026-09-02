// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_LuaVariantFunction.h"
#include "K2Node_BroadcastLuaDelegateOnObject.generated.h"

/**
 * 
 */
UCLASS(HideCategories=(Output))
class UNREALLUAEDITOR_API UK2Node_BroadcastLuaDelegateOnObject : public UK2Node_LuaVariantFunction
{
	GENERATED_BODY()
	
	UK2Node_BroadcastLuaDelegateOnObject();
	
	//~ Begin UEdGraphNode Interface.
	virtual void AllocateDefaultPins() override;
	//~ End UEdGraphNode Interface.
};
