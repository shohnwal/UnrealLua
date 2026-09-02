// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ContextMenu/LuaScriptEditorContextMenuBuilder.h"
#include "LuaValue/LuaValue.h"
#include "UIWidgets/GamescreenDockableWindowWidget.h"
#include "Widgets/Input/SComboBox.h"

class SLuaScriptEditorTextBox;
class SMultiTabEdtitableLuaScriptSwitcher;
class SFunctionPropertyListSwitcher;
class SGridPanel;
class SMultiLineEditableTextBoxEx;
class SVerticalBox;
class SMultiLineEditableTextBox;
class SEditableTextBox;
struct FLuaScriptValue;
/**
 * 
 */
class UNREALLUATOOLS_API SLuaScriptValueEditor : public SGamescreenDockableWindowWidget
{
public:
	SLATE_BEGIN_ARGS(SLuaScriptValueEditor)
		: _Session(nullptr), _StartAsWindow(false)
		{
		}
	//If a LuaScriptValue is given, keyname will be fixed
	//If no LuaScriptValue is given, editor assumes it's a new value to be created
	SLATE_ARGUMENT(FLuaScriptValue*, LuaScriptValue)
	SLATE_ARGUMENT(TScriptInterface<ILuaToolsSession>, Session)
	SLATE_ARGUMENT(UObject*, ValueOwner)
	SLATE_EVENT(FOnLuaScriptValueChangedNativeDelegate, OnValueCreated)
	SLATE_ARGUMENT(FString, Title)
	SLATE_ATTRIBUTE(FAnchors, GameScreenAnchors)
	SLATE_ATTRIBUTE(FAnchors, ExternalWindowAnchors)
	SLATE_ATTRIBUTE(FVector2D, ExternalWindowSize)
	SLATE_ATTRIBUTE(FVector2D, ExternalWindowPosition)
	SLATE_ARGUMENT(ESizingRule, ExternalWindowSizingRule)
	SLATE_ATTRIBUTE(FLinearColor, BackgroundColor)
	SLATE_ARGUMENT(FVector2D, GameScreenAlignment)
	SLATE_ARGUMENT(bool, InitiallyHidden)
	SLATE_ARGUMENT(bool, StartAsWindow)
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	
	virtual EDockableWindowWidgetOnCloseExternalWindowBehavior GetOnCloseExternalWindowBehavior() const override;
	virtual EDockableWindowWidgetOnCloseGameScreenWidgetBehavior GetOnCloseGameScreenWidgetBehavior() const override;
	
	void NotifyLuaScriptValueChanged(FLuaValue LuaValue);
	void NotifyEditableKeyTextChanged(const FText& keyText);

	void NotifyOkButtonClicked();
	void NotifyCancelButtonClicked();
	
	void SetFromProperty(FProperty* prop);
	void SetFromUFunction(UFunction* func);
	void SetFromEmpty();
	void SetFromLuaScriptValue(FLuaScriptValue& val);
	void NotifyFunctionListButtonPressed(UFunction* function, bool bIsChecked);
	void NotifyPropertyListButtonPressed(FProperty* prop, bool bIsChecked);

	void ClearListenerDelegate();
private:
	FReply HandleEditKeyTextBoxKeyDown(const FGeometry& geometry, const FKeyEvent& keyEvent);
	void SetSuggestedKey(const FString& suggestionKey);
	void SetFromLuaValue(const FLuaValue& luaValue);
	FProperty* FindPropertyByString(FString searchStr, TArray<FProperty*>& outSuggestedProperties) const;
	UFunction* FindFunctionByString(const FString& funcName, TArray<UFunction*>& outSuggestedFunctions) const;
protected:
	virtual bool HasSettings() const override { return true; }
public:
	FString Key = "";
	TWeakObjectPtr<UObject> ValueOwner = {};
	TArray<TSharedPtr<FText>> TypeSelectOptions = {};
	
	FOnLuaScriptValueChangedNativeDelegate OnValueCreated = {};
	
	TSharedPtr<STextBlock> ValueOwnerText;
	
	//Current Value
	TSharedPtr<SGridPanel> CurrentValueInfoGrid;
	TSharedPtr<SHorizontalBox> CurrentValueSeparator;
	TSharedPtr<STextBlock> CurrentKeyTextBox = {};
	TSharedPtr<STextBlock> CurrentTypeTextBox = {};
	TSharedPtr<STextBlock> CurrentValueTextBox = {};
	
	//Edit value
	TSharedPtr<STextBlock> TypeRestrictedText;
	TSharedPtr<STextBlock> EditKeyTextLabel;
	TSharedPtr<SEditableTextBox> EditableKeyTextBox = {};
	
	TSharedPtr<STextBlock> EditTypeTextLabel = {};
	TSharedPtr<SComboBox<TSharedPtr<FText>>> NewTypeSelectComboBox = {};
	TSharedPtr<SLuaScriptEditorTextBox> EditValueTextBox = {};
	
	//PropertyListSwitcher, only on new script values
	TSharedPtr<SFunctionPropertyListSwitcher> PropertyFunctionListSwitcher = {};
	TSharedPtr<SLuaScriptMultiEditorSwitcher> ContentSwitcher = {};
	TSharedPtr<STextBlock> KeyHintText = {};
};
