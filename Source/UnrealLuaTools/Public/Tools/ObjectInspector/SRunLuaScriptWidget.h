// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UIWidgets/GamescreenDockableWindowWidget.h"
#include "Widgets/SCompoundWidget.h"

class SLuaScriptMultiEditorSwitcher;
/**
 * 
 */
class UNREALLUATOOLS_API SRunLuaScriptWidget : public SGamescreenDockableWindowWidget
{
public:
	SLATE_BEGIN_ARGS(SRunLuaScriptWidget)
		{
		}
	SLATE_ARGUMENT(TScriptInterface<ILuaToolsSession>, Session)
	SLATE_ARGUMENT(UObject*, SelfParam)
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
	
	void NotifyRunScriptButtonClicked();
	
	void NotifyCloseButtonClicked();
protected:
	virtual bool HasSettings() const override { return true; }
	
	TWeakObjectPtr<UObject> SelfParamObj = {};
	TSharedPtr<STextBlock> SelfParamObjectName = {};
	TSharedPtr<SLuaScriptMultiEditorSwitcher> ContentSwitcher;
};
