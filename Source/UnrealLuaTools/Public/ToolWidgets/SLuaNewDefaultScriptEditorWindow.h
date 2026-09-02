// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UIWidgets/GamescreenDockableWindowWidget.h"
#include "UIWidgets/SFunctionPropertyListSwitcher.h"
#include "Utility/UnrealLuaSimpleDelegateSignatures.h"
#include "Widgets/SWindow.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"

/**
 * 
 */
class UNREALLUATOOLS_API SLuaNewDefaultScriptEditorWindow : public SGamescreenDockableWindowWidget
{
public:
	SLATE_BEGIN_ARGS(SLuaNewDefaultScriptEditorWindow)
		{
		}
		SLATE_ARGUMENT(TScriptInterface<ILuaToolsSession>, Session)
		SLATE_ARGUMENT(FString, Title)
		SLATE_ATTRIBUTE(FAnchors, GameScreenAnchors)
		SLATE_ATTRIBUTE(FAnchors, ExternalWindowAnchors)
		SLATE_ATTRIBUTE(FVector2D, ExternalWindowSize)
		SLATE_ATTRIBUTE(FVector2D, ExternalWindowPosition)
		SLATE_ATTRIBUTE(FLinearColor, BackgroundColor)
		SLATE_ARGUMENT(FVector2D, GameScreenAlignment)
		SLATE_ARGUMENT(bool, InitiallyHidden)
		SLATE_ARGUMENT(bool, StartAsWindow)
		SLATE_EVENT(FSimpleDelegate, OnDefaultScriptCreated)
				
		SLATE_ARGUMENT(UClass*, UClass)
		SLATE_ARGUMENT(TArray<FName>, ForbiddenFunctions)
		SLATE_ARGUMENT(TArray<FName>, PreselectedFunctionNames)
		SLATE_ARGUMENT(TArray<UFunction*>, PreselectedFunctions)
		SLATE_EVENT(FSimpleStringDelegate, OnConfirmEditText)
	SLATE_END_ARGS()


	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);


private:
	void FillTextBox();
	TSharedRef<SMultiLineEditableTextBox> ConstructEditableTextBox(const FArguments& InArgs);
	TSharedRef<SFunctionPropertyListSwitcher> ConstructWidgetSwitcher(UStruct* ustruct, const TArray<UFunction*>& functions, const TArray<UFunction*>& preSelectedFunctions, const TArray<FName>& preSelectedFunctionNames);
	void NotifyCheckedFunctionChanged(UFunction* Function, bool bIsChecked);
	void NotifyCheckedPropertyChanged(FProperty* property, bool bIsChecked);
public:
	FString FileName = "";
	TStrongObjectPtr<UClass> ScriptClass = {};
	FSimpleStringDelegate OnConfirmEditText = {};
	TSharedPtr<SFunctionPropertyListSwitcher> FunctionsPropertySwitcher = {};
	TSharedPtr<SMultiLineEditableTextBox> EditTextBox = {};
	
	FSimpleDelegate OnDefaultScriptCreated = {};
};
