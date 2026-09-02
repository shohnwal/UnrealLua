// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Draggable/UnrealLuaDragBoxPosition.h"
#include "Engine/GameViewportClient.h"
#include "UObject/WeakInterfacePtr.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SWindow.h"
#include "Widgets/Layout/Anchors.h"
#include "Widgets/Layout/SGridPanel.h"

class ILuaToolsSession;
class SBorder;
class SBox;
class SUnrealLuaDraggableBoxOverlay;
class STextBlock;
class UUnrealLuaToolsSession;
class SHorizontalBox;
class SVerticalBox;
class SConstraintCanvas;
class SButton;
class SColorBlock;
class SCanvas;
enum class ESizingRule : uint8;
class SGamescreenDockableWindowWidget;


enum class EDockableWindowWidgetOnCloseExternalWindowBehavior : uint8
{
	RedockOnMainScreen,
	RedockOnMainScreenAndHide,
	Remove
};

enum class EDockableWindowWidgetOnCloseGameScreenWidgetBehavior : uint8
{
	Hide,
	Remove
};

enum class EDockableWindowWidgetInputMode : uint8
{
	GameAndUI,
	UIOnly
};
DECLARE_MULTICAST_DELEGATE_OneParam(FDockableWindowWidgetMulticastDelegate, TSharedRef<SGamescreenDockableWindowWidget>)
/**
 * 
 */
class UNREALLUATOOLS_API SGamescreenDockableWindowWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGamescreenDockableWindowWidget)
		: _Session(nullptr), _ExternalWindowSizingRule(ESizingRule::UserSized), _AllowViewModeSwitch(true)
		{
		}
	SLATE_ATTRIBUTE(TScriptInterface<ILuaToolsSession>, Session)
	SLATE_ARGUMENT(FString, Title)
	SLATE_ATTRIBUTE(FAnchors, GameScreenAnchors)
	SLATE_ARGUMENT(FVector2D, GameScreenAlignment)
	SLATE_ATTRIBUTE(FAnchors, ExternalWindowAnchors)
	SLATE_ATTRIBUTE(FVector2D, ExternalWindowSize)
	SLATE_ARGUMENT(ESizingRule, ExternalWindowSizingRule)
	SLATE_ATTRIBUTE(FVector2D, ExternalWindowPosition)
	SLATE_ATTRIBUTE(FLinearColor, BackgroundColor)
	SLATE_ATTRIBUTE(bool, MirroredButtonPositions)
	SLATE_ARGUMENT(bool, InitiallyHidden)
	SLATE_ARGUMENT(bool, StartAsWindow)
	SLATE_ARGUMENT(bool, DraggableInGameScreen)
	SLATE_ARGUMENT(bool, MinimizableInGameScreen)
	SLATE_ARGUMENT(bool, AllowViewModeSwitch)
	SLATE_END_ARGS()
	
	FDockableWindowWidgetMulticastDelegate OnShutdown = {};

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	virtual void Shutdown();

	void SetisVisible(bool bIsVisible);
	virtual void Show();
	virtual void Hide();
	virtual void ToggleVisibility();
	bool IsVisible() const;
	void SetAsWindow(bool bSwitchToWindow, bool force = false);
	bool IsInWindow() const;
	TSharedPtr<SWindow> TryGetParentWindow();
	virtual void IsInWindowChanged(bool bIsNowInWindow);
	void BringToFrontInCanvas();
	
	virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply NotifySwitchViewModeButtonClicked();
	virtual FReply NotifyGameScreenCloseButtonClicked();
	void NotifyExternalWindowClosed(const TSharedRef<SWindow>& hostWindow);

	virtual EDockableWindowWidgetOnCloseExternalWindowBehavior GetOnCloseExternalWindowBehavior() const;
	virtual EDockableWindowWidgetOnCloseGameScreenWidgetBehavior GetOnCloseGameScreenWidgetBehavior() const;
	virtual bool ShouldRemoveFromParentOnExternalWindowClose();
	virtual EDockableWindowWidgetInputMode GetViewportInputMode() const;
	
	void UpdateInputModeRequest();
	
	void SetHeaderTitle(FString newTitle);
protected:
	virtual bool HasSettings() const;
private:
	void AddToViewport();
	void RemoveFromViewport();
	void AddToWindow();
	void RemoveFromWindow();
public:
	TWeakInterfacePtr<ILuaToolsSession> Session = nullptr;
	
	FString Header = "";
	FVector2D GameScreenAlignment = {};
	FAnchors GameScreenAnchors = {};
	FVector2D ExternalWindowSize = {};
	FVector2D ExternalWindowPosition = {};
	FAnchors ExternalWindowAnchors = {};
	ESizingRule ExternalWindowSizingRule = ESizingRule::UserSized;
	bool bDraggableInViewport = false;
	bool bMinimizableInViewport = false;
	bool AllowViewModeSwitch = true;
	
	TSharedPtr<SBox> MainContentSizeBox = {};
	TSharedPtr<SOverlay> MainContentOverlay = {};
	TSharedPtr<SGridPanel> MainContentGrid = {};
	TSharedPtr<SColorBlock> BackgroundColor = {};
	
	TSharedPtr<SButton> SettingsButton = {};
	TSharedPtr<SButton> SwitchViewModeButton = {};
	TSharedPtr<SButton> CloseWindowButton = {};
	TWeakPtr<SWindow> Window = {};
	TSharedPtr<SVerticalBox> WindowBodyVBox = {};
	TSharedPtr<SHorizontalBox> PseudoHeaderHBox = {};
	TSharedPtr<STextBlock> PseudoHeaderText;
	
	TWeakPtr<SUnrealLuaDraggableBoxOverlay> DraggableOverlay = {};
	
	TAttribute<FUnrealLuaDragBoxPosition> LastDockedPosition = {};
};
