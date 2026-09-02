// Fill out your copyright notice in the Description page of Project Settings.


#include "BlueprintSupport/K2Node_LuaVariantFunction.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "Kismet2/BlueprintEditorUtils.h"


#define LOCTEXT_NAMESPACE "UnrealLuaEditor"



void UK2Node_LuaVariantFunction::PostReconstructNode()
{
	Super::PostReconstructNode();
	auto SynchronizeUserDefinedPins = [this](const TArray<FName>& InPinNames, EEdGraphPinDirection InPinDirection)
	{
		for (const FName& PinName : InPinNames)
		{
			UEdGraphPin* ArgPin = FindArgumentPinChecked(PinName, InPinDirection);
			SynchronizeArgumentPinTypeImpl(ArgPin);
		}
	};
	SynchronizeUserDefinedPins(InputPinNames, EGPD_Input);
	SynchronizeUserDefinedPins(OutputPinNames, EGPD_Output);
}

FEdGraphPinType UK2Node_LuaVariantFunction::GetDefaultPinTypeForPin(UEdGraphPin* Pin)
{
	FEdGraphPinType newType;
	newType.PinCategory =  UEdGraphSchema_K2::PC_Wildcard;
	return newType;
}

void UK2Node_LuaVariantFunction::SynchronizeArgumentPinType(UEdGraphPin* Pin)
{
	auto SynchronizeUserDefinedPin = [this, Pin](const TArray<FName>& InPinNames, EEdGraphPinDirection InPinDirection)
	{
		if (Pin->Direction == InPinDirection && InPinNames.Contains(Pin->PinName))
		{
			// Try and find the argument pin and make sure we get the same result as the pin we were asked to update
			// If not we may have a duplicate pin name with another non-argument pin
			UEdGraphPin* ArgPin = FindArgumentPinChecked(Pin->PinName, InPinDirection);
			if (ArgPin == Pin)
			{
				SynchronizeArgumentPinTypeImpl(Pin);
			}
		}
	};
	SynchronizeUserDefinedPin(InputPinNames, EGPD_Input);
	SynchronizeUserDefinedPin(OutputPinNames, EGPD_Output);
}

void UK2Node_LuaVariantFunction::SynchronizeArgumentPinTypeImpl(UEdGraphPin* Pin)
{
	FEdGraphPinType NewPinType;
	if (Pin->LinkedTo.Num() > 0)
	{
		NewPinType = Pin->LinkedTo[0]->PinType;
	}
	else
	{
		NewPinType = GetDefaultPinTypeForPin(Pin);
	}
	
	if (Pin->PinType != NewPinType)
	{
		Pin->PinType = NewPinType;
	
		GetGraph()->NotifyNodeChanged(this);

		UBlueprint* Blueprint = GetBlueprint();
		if (!Blueprint->bBeingCompiled)
		{
			FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		}
	}
}

UEdGraphPin* UK2Node_LuaVariantFunction::FindArgumentPin(const FName PinName, EEdGraphPinDirection PinDirection)
{
	for (int32 PinIndex = Pins.Num() - 1; PinIndex >= 0; --PinIndex)
	{
		UEdGraphPin* Pin = Pins[PinIndex];
		if ((PinDirection == EGPD_MAX || PinDirection == Pin->Direction) && Pin->PinName == PinName)
		{
			return Pin;
		}
	}
	return nullptr;
}

UEdGraphPin* UK2Node_LuaVariantFunction::FindArgumentPinChecked(const FName PinName, EEdGraphPinDirection PinDirection)
{
	UEdGraphPin* Pin = FindArgumentPin(PinName, PinDirection);
	check(Pin);
	return Pin;
}

void UK2Node_LuaVariantFunction::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = (PropertyChangedEvent.Property  ? PropertyChangedEvent.Property->GetFName() : NAME_None);
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UK2Node_LuaVariantFunction, InputPinNames) || PropertyName == GET_MEMBER_NAME_CHECKED(UK2Node_LuaVariantFunction, OutputPinNames))
	{
		ReconstructNode();
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
	GetGraph()->NotifyNodeChanged(this);
}


void UK2Node_LuaVariantFunction::PinConnectionListChanged(UEdGraphPin* Pin)
{
	// Potentially update an argument pin type
	SynchronizeArgumentPinType(Pin);
}

void UK2Node_LuaVariantFunction::PinTypeChanged(UEdGraphPin* Pin)
{
	// Potentially update an argument pin type
	SynchronizeArgumentPinType(Pin);

	Super::PinTypeChanged(Pin);
}

FText UK2Node_LuaVariantFunction::GetPinDisplayName(const UEdGraphPin* Pin) const
{
	if (/*(Inputs.Contains(Pin->PinName) || Outputs.Contains(Pin->PinName)) &&*/ !Pin->PinFriendlyName.IsEmpty())
	{
		return Pin->PinFriendlyName;
	}
	return Super::GetPinDisplayName(Pin);
}

void UK2Node_LuaVariantFunction::EarlyValidation(FCompilerResultsLog& MessageLog) const
{
	Super::EarlyValidation(MessageLog);

	TSet<FString> AllPinNames;
	auto ValidatePinArray = [this, &AllPinNames, &MessageLog](const TArray<FName>& InPinNames)
	{
		for (const FName& PinName : InPinNames)
		{
			const FString PythonizedPinName = PinName.ToString();

			if (PinName == UEdGraphSchema_K2::PN_Execute ||
				PinName == UEdGraphSchema_K2::PN_Then ||
				PinName == UEdGraphSchema_K2::PN_ReturnValue
				)
			{
				MessageLog.Error(*FText::Format(LOCTEXT("InvalidPinName_RestrictedName", "Pin '{0}' ({1}) on @@ is using a restricted name."), FText::AsCultureInvariant(PinName.ToString()), FText::AsCultureInvariant(PythonizedPinName)).ToString(), this);
			}
			if (PythonizedPinName.Len() == 0)
			{
				MessageLog.Error(*LOCTEXT("InvalidPinName_EmptyName", "Empty pin name found on @@").ToString(), this);
			}
			else
			{
				bool bAlreadyUsed = false;
				AllPinNames.Add(PythonizedPinName, &bAlreadyUsed);

				if (bAlreadyUsed)
				{
					MessageLog.Error(*FText::Format(LOCTEXT("InvalidPinName_DuplicateName", "Pin '{0}' ({1}) on @@ has the same name as another pin."), FText::AsCultureInvariant(PinName.ToString()), FText::AsCultureInvariant(PythonizedPinName)).ToString(), this);
				}
			}
		}
	};

	ValidatePinArray(InputPinNames);
	ValidatePinArray(OutputPinNames);
}

void UK2Node_LuaVariantFunction::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);
}

bool UK2Node_LuaVariantFunction::CanPasteHere(const UEdGraph* TargetGraph) const
{
	bool bCanPaste = Super::CanPasteHere(TargetGraph);
	//if (bCanPaste)
	//{
	//	bCanPaste &= FBlueprintEditorUtils::IsEditorUtilityBlueprint(FBlueprintEditorUtils::FindBlueprintForGraphChecked(TargetGraph));
	//}
	return bCanPaste;
}

bool UK2Node_LuaVariantFunction::IsActionFilteredOut(const FBlueprintActionFilter& Filter)
{
	bool bIsFilteredOut = Super::IsActionFilteredOut(Filter);
	if (!bIsFilteredOut)
	{
		for (UEdGraph* TargetGraph : Filter.Context.Graphs)
		{
			bIsFilteredOut |= !CanPasteHere(TargetGraph);
		}
	}
	return bIsFilteredOut;
}

void UK2Node_LuaVariantFunction::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	// actions get registered under specific object-keys; the idea is that 
	// actions might have to be updated (or deleted) if their object-key is  
	// mutated (or removed)... here we use the node's class (so if the node 
	// type disappears, then the action should go with it)
	UClass* ActionKey = GetClass();
	// to keep from needlessly instantiating a UBlueprintNodeSpawner, first   
	// check to make sure that the registrar is looking for actions of this type
	// (could be regenerating actions for a specific asset, and therefore the 
	// registrar would only accept actions corresponding to that asset)
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

#undef LOCTEXT_NAMESPACE