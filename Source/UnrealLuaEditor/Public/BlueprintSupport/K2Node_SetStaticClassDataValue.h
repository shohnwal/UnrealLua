// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_CallFunction.h"
#include "UObject/Object.h"
#include "K2Node_SetStaticClassDataValue.generated.h"

/**
 * 
 */
UCLASS()
class UNREALLUAEDITOR_API UK2Node_SetStaticClassDataValue : public UK2Node_CallFunction
{
	GENERATED_BODY()
public:
	UK2Node_SetStaticClassDataValue();
	virtual void AllocateDefaultPins() override;
	virtual bool CanPasteHere(const UEdGraph* TargetGraph) const override;
	virtual bool IsActionFilteredOut(const FBlueprintActionFilter& Filter) override;
	virtual void EarlyValidation(FCompilerResultsLog& MessageLog) const override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual bool ShouldShowNodeProperties() const override { return true; }
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual void PinTypeChanged(UEdGraphPin* Pin) override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& propEditEvent) override;
	
	UPROPERTY(EditAnywhere, Category= "UnrealLua", meta=(GetOptions="GetPropertyNames"))
	FString StaticPropertyName;
	
	UPROPERTY(EditAnywhere, Category= "UnrealLua")
	bool bShowHiddenPins = false;
	
	UPROPERTY(EditAnywhere, Category= "UnrealLua", meta=(GetOptions="GetPropertyNames"))
	TArray<FString> StaticPropertyNames;
	
	UFUNCTION(CallInEditor)
	TArray<FString> GetPropertyNames() const;
};
