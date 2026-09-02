// Fill out your copyright notice in the Description page of Project Settings.


#include "BlueprintSupport/K2Node_LuaVariantFunction_SynchronizedInputOutput.h"
// Fill out your copyright notice in the Description page of Project Settings.
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "LuaValue/LuaValue.h"


#define LOCTEXT_NAMESPACE "UnrealLuaEditor"


UK2Node_LuaVariantFunction_SynchronizedInputOutput::UK2Node_LuaVariantFunction_SynchronizedInputOutput()
{
	UScriptStruct* ss = FLuaValue::StaticStruct();
	verify(IsValid(ss))
}

void UK2Node_LuaVariantFunction_SynchronizedInputOutput::PostReconstructNode()
{
	Super::PostReconstructNode();

	//this->ProcessAndSyncPinNames();
	
	for (const FName& PinName : this->InputPinNames)
	{
		UEdGraphPin* argPin = FindArgumentPinChecked(PinName, EGPD_Input);
		SynchronizeArgumentPinTypeImpl(argPin);
	}
	
	for (const FName& PinName : this->OutputPinNames)
	{
		UEdGraphPin* argPin = FindArgumentPinChecked(PinName, EGPD_Output);
		SynchronizeArgumentPinTypeImpl(argPin);
	}
}

FEdGraphPinType UK2Node_LuaVariantFunction_SynchronizedInputOutput::GetDefaultPinTypeForPin(UEdGraphPin* Pin)
{
	FEdGraphPinType newType;
	newType.PinCategory =  UEdGraphSchema_K2::PC_Wildcard;
	return newType;
}

void UK2Node_LuaVariantFunction_SynchronizedInputOutput::ProcessAndSyncPinNames()
{
	this->OutputPinNames.Empty();
	
	//Make sure there are no empty input 
	for (FName& PinName : this->InputPinNames)
	{
		if (PinName == NAME_None)
		{
			int32 tryIndex = 1;
			FString tryName = FString::Printf(TEXT("Value_%d"), tryIndex);
			while (this->InputPinNames.Contains(tryName))
			{
				++tryIndex;
				tryName = FString::Printf(TEXT("Value_%d"), tryIndex);
			}
			PinName = *tryName;
		}
	}
	for (auto& basePinName : this->InputPinNames)
	{
		this->OutputPinNames.Emplace("Out " + basePinName.ToString());
	}
}

void UK2Node_LuaVariantFunction_SynchronizedInputOutput::SynchronizeArgumentPinType(UEdGraphPin* Pin)
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

void UK2Node_LuaVariantFunction_SynchronizedInputOutput::SynchronizeArgumentPinTypeImpl(UEdGraphPin* Pin)
{
	FEdGraphPinType NewPinType;
	if (Pin->LinkedTo.Num() > 0)
	{
		UEdGraphPin* otherPin = Pin->LinkedTo[0];
		NewPinType = otherPin->PinType;
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

UEdGraphPin* UK2Node_LuaVariantFunction_SynchronizedInputOutput::FindArgumentPin(const FName PinName, EEdGraphPinDirection PinDirection)
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

UEdGraphPin* UK2Node_LuaVariantFunction_SynchronizedInputOutput::FindArgumentPinChecked(const FName PinName, EEdGraphPinDirection PinDirection)
{
	UEdGraphPin* Pin = FindArgumentPin(PinName, PinDirection);
	check(Pin);
	return Pin;
}

void UK2Node_LuaVariantFunction_SynchronizedInputOutput::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = (PropertyChangedEvent.Property  ? PropertyChangedEvent.Property->GetFName() : NAME_None);
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UK2Node_LuaVariantFunction_SynchronizedInputOutput, InputPinNames))
	{
		this->ProcessAndSyncPinNames();
		
		ReconstructNode();
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
	GetGraph()->NotifyNodeChanged(this);
}


void UK2Node_LuaVariantFunction_SynchronizedInputOutput::PinConnectionListChanged(UEdGraphPin* Pin)
{
	// Potentially update an argument pin type
	SynchronizeArgumentPinType(Pin);
}

void UK2Node_LuaVariantFunction_SynchronizedInputOutput::PinTypeChanged(UEdGraphPin* Pin)
{
	// Potentially update an argument pin type
	SynchronizeArgumentPinType(Pin);

	Super::PinTypeChanged(Pin);
}

FText UK2Node_LuaVariantFunction_SynchronizedInputOutput::GetPinDisplayName(const UEdGraphPin* Pin) const
{
	if (/*(Inputs.Contains(Pin->PinName) || Outputs.Contains(Pin->PinName)) &&*/ !Pin->PinFriendlyName.IsEmpty())
	{
		return Pin->PinFriendlyName;
	}
	return Super::GetPinDisplayName(Pin);
}

void UK2Node_LuaVariantFunction_SynchronizedInputOutput::EarlyValidation(FCompilerResultsLog& MessageLog) const
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

	ValidatePinArray(this->InputPinNames);
	ValidatePinArray(this->OutputPinNames);
}

void UK2Node_LuaVariantFunction_SynchronizedInputOutput::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);
}

bool UK2Node_LuaVariantFunction_SynchronizedInputOutput::CanPasteHere(const UEdGraph* TargetGraph) const
{
	bool bCanPaste = Super::CanPasteHere(TargetGraph);
	//if (bCanPaste)
	//{
	//	bCanPaste &= FBlueprintEditorUtils::IsEditorUtilityBlueprint(FBlueprintEditorUtils::FindBlueprintForGraphChecked(TargetGraph));
	//}
	return bCanPaste;
}

bool UK2Node_LuaVariantFunction_SynchronizedInputOutput::IsActionFilteredOut(const FBlueprintActionFilter& Filter)
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

void UK2Node_LuaVariantFunction_SynchronizedInputOutput::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
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

bool UK2Node_LuaVariantFunction_SynchronizedInputOutput::CanAddPin() const
{
	return true;
}

void UK2Node_LuaVariantFunction_SynchronizedInputOutput::AddInputPin()
{
	this->InputPinNames.Add(NAME_None);
	this->ProcessAndSyncPinNames();
	ReconstructNode();
	GetGraph()->NotifyNodeChanged(this);
}

void UK2Node_LuaVariantFunction_SynchronizedInputOutput::OnPinRemoved(UEdGraphPin* InRemovedPin)
{
	this->InputPinNames.Remove(InRemovedPin->PinName);
	this->ProcessAndSyncPinNames();
	Super::OnPinRemoved(InRemovedPin);
}

#undef LOCTEXT_NAMESPACE
