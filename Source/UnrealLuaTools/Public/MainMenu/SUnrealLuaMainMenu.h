// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UIWidgets/GamescreenDockableWindowWidget.h"
#include "Widgets/SCompoundWidget.h"

class UUnrealLuaTool;
/**
 * 
 */
class UNREALLUATOOLS_API SUnrealLuaMainMenu : public SGamescreenDockableWindowWidget
{
public:
	SLATE_BEGIN_ARGS(SUnrealLuaMainMenu)
		: _Session(nullptr), _ExternalWindowSizingRule(ESizingRule::UserSized), _StartAsWindow(false)
		{
		}
	SLATE_ARGUMENT(UUnrealLuaToolsSession*, Session)
	SLATE_ARGUMENT(FString, Title)
	SLATE_ATTRIBUTE(FAnchors, GameScreenAnchors)
	SLATE_ATTRIBUTE(FAnchors, ExternalWindowAnchors)
	SLATE_ARGUMENT(FVector2D, GameScreenAlignment)
	SLATE_ATTRIBUTE(FVector2D, ExternalWindowSize)
	SLATE_ATTRIBUTE(FVector2D, ExternalWindowPosition)
	SLATE_ARGUMENT(ESizingRule, ExternalWindowSizingRule)
	SLATE_ATTRIBUTE(FLinearColor, BackgroundColor)
	SLATE_ATTRIBUTE(bool, MirroredButtonPositions)
	SLATE_ARGUMENT(bool, InitiallyHidden)
	SLATE_ARGUMENT(bool, StartAsWindow)

		
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	
	void NotifyInputKeyEvent(const FInputKeyEventArgs& InputKeyEventArgs);
	virtual EDockableWindowWidgetOnCloseExternalWindowBehavior GetOnCloseExternalWindowBehavior() const override;
	virtual EDockableWindowWidgetOnCloseGameScreenWidgetBehavior GetOnCloseGameScreenWidgetBehavior() const override;
	
	void AddTool(UUnrealLuaTool* newTool);
	
	virtual void Show() override;
	virtual void Hide() override;

	TMap<TSubclassOf<UUnrealLuaTool>, TObjectPtr<UUnrealLuaTool>> RegisteredTools = {};
	
	TSharedPtr<SVerticalBox> MainMenuButtonContainer = {};
};
