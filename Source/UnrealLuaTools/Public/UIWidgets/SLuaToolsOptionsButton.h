// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SButton;
/**
 * 
 */

DECLARE_MULTICAST_DELEGATE_OneParam(FSimpleFloatDelegate, float);
class UNREALLUATOOLS_API SLuaToolsOptionsButton : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLuaToolsOptionsButton)
	{
	}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	FReply NotifySettingsButtonClicked();
	void NotifyFontSizeTextCommitted(const FText& text, ETextCommit::Type arg);
	void NotifyStyleChanged();
	
	TSharedPtr<SButton> SettingsButton = {};
	
	FSimpleFloatDelegate OnFontSizeChanged = {};
};
