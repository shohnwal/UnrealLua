// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SPropertySelectionList.h"
#include "SUFunctionSelectionList.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

/**
 * 
 */

enum class EFunctionPropertyListSwitcherSelectionType : uint8
{
	CheckBox,
	Button
};

class UNREALLUATOOLS_API SFunctionPropertyListSwitcher : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFunctionPropertyListSwitcher)
		{
		}
		SLATE_ARGUMENT(UStruct*, TargetStruct)
		SLATE_ARGUMENT(TArray<FProperty*>, PreselectedProperties)
		SLATE_ARGUMENT(TArray<FName>, PreselectedPropertyNames)
		SLATE_ARGUMENT(TArray<UFunction*>, Functions)
		SLATE_ARGUMENT(TArray<UFunction*>, PreselectedFunctions)
		SLATE_ARGUMENT(TArray<FName>, PreselectedFunctionNames)
		SLATE_ARGUMENT(bool, SingleChoiceMode)
		SLATE_ARGUMENT(EFunctionPropertyListSwitcherSelectionType, ItemListType)
	SLATE_END_ARGS()

	enum class EActiveWitgetListSelection
	{
		UFunction,
		FProperty
	};

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	
	void SetViewedStruct(UStruct* ustruct, const TArray<FProperty*>& preselectedProperties, const TArray<FName>& preselectedPropertyNames);
	
	void SetActiveWidget(EActiveWitgetListSelection selection);
	TSharedRef<SUFunctionSelectionList> ConstructFunctionsList(const TArray<UFunction*>& functions,
	                                                           const TArray<UFunction*>& preselectedFunctions,
	                                                           const TArray<FName>& preselectedFunctionNames);
	TSharedRef<SPropertySelectionList> ConstructPropertyList();
	TSharedRef<SWidgetSwitcher> ConstructWidgetSwitcher();

	void NotifyCheckedFunctionChanged(UFunction* function, bool bIsChecked);
	void NotifyCheckedPropertyChanged(FProperty* property, bool bIsChecked);
	void ClearAllChecked() const;
	UFunction* FindFunctionByString(const FString& funcName);

	bool bSingleChoiceMode = false;
	EFunctionPropertyListSwitcherSelectionType ItemListType = EFunctionPropertyListSwitcherSelectionType::Button;

	TSharedPtr<SVerticalBox> ContentVerticalBox = {};
	TStrongObjectPtr<UStruct> TargetStruct = nullptr;
	TSharedPtr<SWidgetSwitcher> Switcher = {};
	TSharedPtr<SUFunctionSelectionList> FunctionListWidget = {};
	TSharedPtr<SPropertySelectionList> PropertyListWidget = {};
	FOnFunctionCheckboxChangedDelegate OnFunctionCheckboxChanged = {};
	FOnPropertyCheckboxChangedDelegate OnPropertyCheckboxChanged = {};
	TSharedPtr<SHorizontalBox> WidgetSwitcherButtonsSection = {};
};
