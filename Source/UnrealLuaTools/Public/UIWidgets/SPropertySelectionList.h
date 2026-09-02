// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaScriptEditorTextBox/SLuaScriptEditorTextBox.h"
#include "Widgets/Layout/SScrollBox.h"

class SCheckBox;
DECLARE_DELEGATE_TwoParams(FOnPropertyCheckboxChangedDelegate, FProperty*, bool);
/**
 * 
 */

namespace UnrealLua
{
	struct UNREALLUATOOLS_API FPropertyCheckboxPair
	{
		FProperty* Prop = nullptr;
		TSharedPtr<SCheckBox> CheckBox = {};
	};
}
class UNREALLUATOOLS_API SPropertySelectionList : public SScrollBox
{
public:
	SLATE_BEGIN_ARGS(SPropertySelectionList)
		{
		}
	SLATE_ARGUMENT(UStruct*, TargetStruct)
	SLATE_ARGUMENT(bool, SingleChoiceMode);
	SLATE_ARGUMENT(FString, FilterListString);
	SLATE_ARGUMENT(TArray<FProperty*>, PreselectedProperties)
	SLATE_ARGUMENT(TArray<FName>, PreselectedPropertyNames)
	SLATE_EVENT(FOnPropertyCheckboxChangedDelegate, OnPropertyCheckboxChanged)
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	
	void SetViewedStruct(UStruct* ustruct, const TArray<FProperty*>& preselectedProperties = {}, const TArray<FName>& preselectedPropertyNames = {}, FString filterListMustContain = TEXT(""));
	
	TArray<FProperty*> GetSelectedProperties() const;
	
	TSharedRef<SHorizontalBox> ConstructSeparator(UStruct* ustruct);
	TSharedRef<SHorizontalBox> ConstructListElement(FProperty* prop, bool bPreselected);

	void ClearAllChecks(FProperty* except = nullptr);
	
	void NotifyCheckedPropertyChanged(FProperty* prop, ECheckBoxState state);
	void NotifyPropertyButtonClicked(FProperty* prop);

	bool bSingleChoiceMode = false;
	TStrongObjectPtr<UStruct> Struct = nullptr;
	TSharedPtr<SVerticalBox> VerticalContentBox = {};
	
	TArray<FProperty*> SelectedProperties = {};
	
	TArray<UnrealLua::FPropertyCheckboxPair> CheckBoxes = {};
	
	FOnPropertyCheckboxChangedDelegate OnPropertySelectionChanged = {};
};
