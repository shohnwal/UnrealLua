// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_AddPinInterface.h"
#include "K2Node_CallFunction.h"
#include "K2Node_LuaVariantFunction_SynchronizedInputOutput.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class UNREALLUAEDITOR_API UK2Node_LuaVariantFunction_SynchronizedInputOutput : public UK2Node_CallFunction, public IK2Node_AddPinInterface

{
	GENERATED_BODY()
	
public:	
	UK2Node_LuaVariantFunction_SynchronizedInputOutput();
	//~ Begin UObject Interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject Interface

	//~ Begin UEdGraphNode Interface.
	virtual bool ShouldShowNodeProperties() const override { return true; }
	virtual int32 GetNodeRefreshPriority() const override { return EBaseNodeRefreshPriority::Low_UsesDependentWildcard; }
	
	virtual void PostReconstructNode() override;
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual void PinTypeChanged(UEdGraphPin* Pin) override;
	virtual FText GetPinDisplayName(const UEdGraphPin* Pin) const override;
	//~ End UEdGraphNode Interface.

	//~ Begin UK2Node Interface.
	virtual void EarlyValidation(class FCompilerResultsLog& MessageLog) const override;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual bool CanPasteHere(const UEdGraph* TargetGraph) const override;
	virtual bool IsActionFilteredOut(const class FBlueprintActionFilter& Filter) override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	//~ End UK2Node Interface.

	
	virtual bool CanAddPin() const override;
	virtual void AddInputPin() override;
	virtual void OnPinRemoved(UEdGraphPin* InRemovedPin) override;
	
protected:
	//~ UK2Node_CallFunction interface
	virtual bool CanToggleNodePurity() const override { return false; }
	//~ End UK2Node_CallFunction interface
	
	virtual FEdGraphPinType GetDefaultPinTypeForPin(UEdGraphPin* Pin);

	void ProcessAndSyncPinNames();
private:
	/** Synchronize the type of the given argument pin with the type its connected to, or reset it to a wildcard pin if there's no connection */
	void SynchronizeArgumentPinType(UEdGraphPin* Pin);
	void SynchronizeArgumentPinTypeImpl(UEdGraphPin* Pin);

	/** Try and find a named argument pin - this is the same as FindPin except it searches the array from the end as that's where the argument pins will be */
	UEdGraphPin* FindArgumentPin(const FName PinName, EEdGraphPinDirection PinDirection = EGPD_MAX);
	UEdGraphPin* FindArgumentPinChecked(const FName PinName, EEdGraphPinDirection PinDirection = EGPD_MAX);

public:
	
	UPROPERTY(EditAnywhere, Category="Arguments")
	TArray<FName> InputPinNames;
	
	UPROPERTY(VisibleAnywhere, Category="Output")
	TArray<FName> OutputPinNames;
};
