// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaValue/LuaValue.h"
#include "Utility/UnrealLuaSimpleDelegateSignatures.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SButton.h"

class SBorder;
class STextBlock;
struct FLuaScriptValue;
/**
 * 
 */
class UNREALLUATOOLS_API SLuaScriptValueListElementWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLuaScriptValueListElementWidget)
		: _InitiallyOpen(false)
		{
		}
	SLATE_ARGUMENT(FLuaScriptValue*, LuaScriptValue)
	SLATE_ARGUMENT(bool, InitiallyOpen)
	SLATE_EVENT(FSimpleStringDelegate, OnRequestEditValue)
	SLATE_EVENT(FSimpleStringDelegate, OnSelectUObject)
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	FReply ToggleContentVisbility();
	FReply NotifyValueContentButtonPressed();
	FReply NotifySelectUObjectButtonClicked();
	void NotifyLuaScriptValueChanged(FLuaValue luaValue);
	bool IsOpen() const;
	
	void UpdateValueWidgets(const FLuaValue& luaValue);

	FString KeyString = "";
	TSharedPtr<STextBlock> KeyLabelText = {};
	TSharedPtr<STextBlock> ValueText = {};
	TSharedPtr<SBorder> ValueContentBorder = {};
	TSharedPtr<STextBlock> TypeText = {};
	TSharedPtr<SButton> SelectUObjectButton = {};

	FSimpleStringDelegate OnSelectUObject;
	FSimpleStringDelegate OnRequestEditValue;
};
