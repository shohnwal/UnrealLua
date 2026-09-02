// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SScrollBox.h"

class SCheckBox;
DECLARE_DELEGATE_TwoParams(FOnFunctionCheckboxChangedDelegate, UFunction*, bool);
/**
 * 
 */

namespace UnrealLua
{
	struct UNREALLUATOOLS_API FFunctionCheckboxPair
	{
		UFunction* Func = nullptr;
		TSharedPtr<SCheckBox> CheckBox = {};
	};
}

class UNREALLUATOOLS_API SUFunctionSelectionList : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUFunctionSelectionList)
	{
	}
	SLATE_ARGUMENT(TArray<UFunction*>, Functions)
	SLATE_ARGUMENT(TArray<UFunction*>, PreselectedFunctions)
	SLATE_ARGUMENT(TArray<FName>, PreselectedFunctionNames)
	SLATE_ARGUMENT(bool, AddSeparators)
	SLATE_ARGUMENT(bool, SingleChoiceMode)
	SLATE_END_ARGS()
	
	static TArray<UFunction*> CreateDefaultFunctionSelectionList(UClass* uclass, const TArray<FName>& forbiddenFunctions = {});
	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	TSharedRef<SHorizontalBox> ConstructClassSeparator(UClass* className);

	TSharedRef<SHorizontalBox> ConstructListElement(UFunction* function, bool bIsSelected);
	
	TArray<UFunction*> GetSelectedFunctions() const;
	void ClearAllChecks(UFunction* except = nullptr);

	UFunction* FindFunctionByString(const FString& funcName);
	void SetViewedStruct(UStruct* ustruct);

protected:
	void NotifyFunctionButtonClicked(UFunction* func);
	void NotifyCheckedFunctionChanged(UFunction* Function, ECheckBoxState State);
public:
	TSharedPtr<SScrollBox> MainBodyScrollBox = {};
	bool bSingleChoiceMode = false;
	FOnFunctionCheckboxChangedDelegate OnFunctionCheckboxChanged = {};
	
	TArray<UnrealLua::FFunctionCheckboxPair> CheckBoxes = {};
	
	TArray<UFunction*> SelectedFunctions = {};
};
