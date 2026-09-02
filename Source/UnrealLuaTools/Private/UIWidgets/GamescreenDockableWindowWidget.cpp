// Fill out your copyright notice in the Description page of Project Settings.


#include "UIWidgets/GamescreenDockableWindowWidget.h"

#include "PropertyInfoViewStyle.h"
#include "SkeletalRenderPublic.h"
#include "SlateOptMacros.h"
#include "UnrealEngine.h"
#include "Components/SlateWrapperTypes.h"
#include "Engine/UserInterfaceSettings.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Overlay/DragBoxPosition.h"
#include "Session/UnrealLuaToolsSession.h"
#include "Styling/StyleColors.h"
#include "Textures/SlateIcon.h"
#include "UIWidgets/SLuaToolsOptionsButton.h"
#include "UIWidgets/Draggable/SUnrealLuaDraggableBoxOverlay.h"
#include "Utility/LuaLogMacros.h"
#include "Utility/WidgetStyles.h"
#include "Widgets/SCanvas.h"
#include "Widgets/SWindow.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/Anchors.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Text/STextBlock.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SGamescreenDockableWindowWidget::Construct(const FArguments& InArgs)
{
	this->GameScreenAlignment = InArgs._GameScreenAlignment;
	this->ExternalWindowAnchors = InArgs._ExternalWindowAnchors.IsSet() ? InArgs._ExternalWindowAnchors.Get() : FAnchors{0,0,1,1};
	this->GameScreenAnchors = InArgs._GameScreenAnchors.IsSet() ? InArgs._GameScreenAnchors.Get() : FAnchors{0,0,1,1};
	this->ExternalWindowSize = InArgs._ExternalWindowSize.IsSet() ? InArgs._ExternalWindowSize.Get() : FVector2D{0.5f, 0.5f};
	this->ExternalWindowPosition = InArgs._ExternalWindowPosition.IsSet() ? InArgs._ExternalWindowPosition.Get() : FVector2D{0.5f, 0.5f};
	FLinearColor backgroundColor = InArgs._BackgroundColor.IsSet() ? InArgs._BackgroundColor.Get() : USlateThemeManager::Get().GetColor(EStyleColor::Panel);
	this->Header = InArgs._Title;
	this->ExternalWindowSizingRule = InArgs._ExternalWindowSizingRule;
	this->bDraggableInViewport = InArgs._DraggableInGameScreen;
	this->bMinimizableInViewport = InArgs._MinimizableInGameScreen;
	this->Session = InArgs._Session.IsSet() ? InArgs._Session.Get() : nullptr;
	this->AllowViewModeSwitch = InArgs._AllowViewModeSwitch;
	verify(this->Session.IsValid())
	verify(this->Session->GetSessionType() == ELuaToolsSessionType::Game || InArgs._StartAsWindow )

	const FWindowStyle* windowStyle = &FCoreStyle::Get().GetWidgetStyle<FWindowStyle>("Window");

	TSharedRef<SImage> windowBackgroundImage =
		FSlateApplicationBase::Get().MakeImage(
			&windowStyle->BackgroundBrush,
			windowStyle->BackgroundColor,
			EVisibility::SelfHitTestInvisible
		);
	
	auto WindowBorder =
	FSlateApplicationBase::Get().MakeImage(
		&windowStyle->BorderBrush,
		windowStyle->BorderColor,
		EVisibility::SelfHitTestInvisible
	);

	auto WindowOutline = FSlateApplicationBase::Get().MakeImage(
			&windowStyle->OutlineBrush,
			windowStyle->OutlineColor,
			EVisibility::SelfHitTestInvisible
		);
	this->ChildSlot
	[
		SAssignNew(MainContentGrid, SGridPanel)
		.FillColumn(1,1)
		.FillRow(1,1)
		+ SGridPanel::Slot(1,1)
		[
			SAssignNew(MainContentSizeBox, SBox)
			[
				SAssignNew(MainContentOverlay, SOverlay)
				// window background
				+ SOverlay::Slot()
				[
					windowBackgroundImage
				]

				// window border
				+ SOverlay::Slot()
				[
					WindowBorder
				]

				// window outline
				+ SOverlay::Slot()
				.Padding(2.0f)
				[
					WindowOutline
				]	
			]
		]
	]
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill);
	//.Padding(10,10,10,10);;
	
	this->MainContentOverlay->AddSlot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	.Padding(5)
	[

		SAssignNew(WindowBodyVBox, SVerticalBox)
		+ SVerticalBox::Slot()
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SColorBlock)
				.Color(FLinearColor{0.01,0.01,0.01,1})
				.Visibility(EVisibility::SelfHitTestInvisible)
			]
			+ SOverlay::Slot()
			[
				SAssignNew(PseudoHeaderHBox, SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillContentWidth(1)
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Center)
				.Padding(10.f)
				[
					SAssignNew(PseudoHeaderText, STextBlock)
					.Text(FText::AsCultureInvariant(this->Header))
					.Justification(ETextJustify::Center)
					.Visibility(EVisibility::HitTestInvisible)					
				]
			]
		]
		.AutoHeight()
		.VAlign(VAlign_Top)
		.HAlign(HAlign_Fill)	
	];
	
	if (this->HasSettings())
	{
		this->PseudoHeaderHBox->AddSlot()
		[
			SNew(SLuaToolsOptionsButton)
		]
		.AutoWidth()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Padding(10.f);
	}

	this->PseudoHeaderHBox->AddSlot()
		[
			SAssignNew(SwitchViewModeButton, SButton)
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("[ ]"))
				.Visibility(EVisibility::HitTestInvisible)
			]
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Top)
			.Visibility(EVisibility::Visible)
			.OnClicked(this, &SGamescreenDockableWindowWidget::NotifySwitchViewModeButtonClicked)
		]
		.AutoWidth()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Padding(10.f);		
	
	if (!this->AllowViewModeSwitch)
	{
		this->SwitchViewModeButton->SetVisibility(EVisibility::Collapsed);
		this->SwitchViewModeButton->SetEnabled(false);
		this->SwitchViewModeButton->SetOnClicked({});
	}

	this->PseudoHeaderHBox->AddSlot()
	[
		SAssignNew(CloseWindowButton, SButton)
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant("X"))
			.Visibility(EVisibility::HitTestInvisible)
		]
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Visibility(EVisibility::Visible)
		.OnClicked(this, &SGamescreenDockableWindowWidget::NotifyGameScreenCloseButtonClicked)
	]
	.AutoWidth()
	.HAlign(HAlign_Right)
	.VAlign(VAlign_Top)
	.Padding(10.f);
	
	this->SetAsWindow(InArgs._StartAsWindow, true);
	this->SetisVisible(!InArgs._InitiallyHidden);
}

void SGamescreenDockableWindowWidget::Shutdown()
{
	TSharedRef<SWidget> This = this->AsShared();
	
	this->OnShutdown.Broadcast(SharedThis(this));
	
	this->SetAsWindow(false);
	
	this->RemoveFromViewport();
};

void SGamescreenDockableWindowWidget::SetisVisible(bool bIsVisible)
{
	if (bIsVisible)
	{
		this->Show();
	}
	else
	{
		this->Hide();
	}
}

void SGamescreenDockableWindowWidget::Show()
{
	bool wasVisible = this->IsVisible();
	this->SetVisibility(EVisibility::SelfHitTestInvisible);
	if (this->IsInWindow())
	{
		this->TryGetParentWindow()->ShowWindow();
	}
	if (!wasVisible)
	{
		this->UpdateInputModeRequest();
	}
}

void SGamescreenDockableWindowWidget::Hide()
{
	if (!this->IsVisible())
	{
		return;
	}
	this->SetVisibility(EVisibility::Collapsed);
	if (this->IsInWindow())
	{
		this->TryGetParentWindow()->HideWindow();
	}
	this->UpdateInputModeRequest();
}

void SGamescreenDockableWindowWidget::ToggleVisibility()
{
	if (this->GetVisibility() == EVisibility::Collapsed)
	{
		this->Show();
	}
	else
	{
		this->Hide();
	}
}

bool SGamescreenDockableWindowWidget::IsVisible() const
{
	return this->GetVisibility() != EVisibility::Collapsed && this->GetVisibility() != EVisibility::Hidden;
}

void SGamescreenDockableWindowWidget::SetAsWindow(bool bSwitchToWindow, bool force)
{
	if (bSwitchToWindow)
	{
		if (this->IsInWindow())
		{
			return;
		}
		
		TSharedRef<SWidget> This = this->AsShared();
		
		this->RemoveFromViewport();
		
		this->AddToWindow();
	}
	else
	{
		TSharedRef<SWidget> This = this->AsShared();
		
		if (!this->IsInWindow() && !force)
		{
			return;
		}
		
		this->RemoveFromWindow();
		
		if (!this->Session.IsValid() || this->Session->GetSessionType() != ELuaToolsSessionType::Game)
		{
			return;
		}
		this->AddToViewport();
	}
}

bool SGamescreenDockableWindowWidget::IsInWindow() const
{
	return this->Window.IsValid();
}

TSharedPtr<SWindow> SGamescreenDockableWindowWidget::TryGetParentWindow()
{
	if (this->Window.IsValid())
	{
		return this->Window.Pin().ToSharedRef(); 
	}
	return nullptr;
}

void SGamescreenDockableWindowWidget::IsInWindowChanged(bool bIsNowInWindow)
{
	this->UpdateInputModeRequest();
}

void SGamescreenDockableWindowWidget::BringToFrontInCanvas()
{
	if(!this->IsInWindow())
	{
		LUA_LOG("Bringing to front")
		TSharedRef<SWidget> This = this->AsShared();
		
		this->RemoveFromViewport();
		
		this->AddToViewport();
	}
}

FReply SGamescreenDockableWindowWidget::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent)
{
	if (this->IsInWindow())
	{
		return FReply::Handled();
	}
	else
	{
		this->BringToFrontInCanvas();
		return FReply::Handled();
	}
}

FReply SGamescreenDockableWindowWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (this->IsInWindow())
	{
		return FReply::Unhandled();
	}
	else
	{
		this->BringToFrontInCanvas();
		return FReply::Unhandled();
	}
}

FReply SGamescreenDockableWindowWidget::NotifySwitchViewModeButtonClicked()
{
	if (this->IsInWindow())
	{
		this->SetAsWindow(false);
	}
	else
	{
		this->SetAsWindow(true);
	}
	return FReply::Handled();
}

FReply SGamescreenDockableWindowWidget::NotifyGameScreenCloseButtonClicked()
{
	EDockableWindowWidgetOnCloseGameScreenWidgetBehavior behavior = this->GetOnCloseGameScreenWidgetBehavior();
	if (behavior == EDockableWindowWidgetOnCloseGameScreenWidgetBehavior::Hide)
	{
		this->SetisVisible(false);
	}
	else if (behavior == EDockableWindowWidgetOnCloseGameScreenWidgetBehavior::Remove)
	{
		this->Shutdown();
	}
	return FReply::Handled();
}


void SGamescreenDockableWindowWidget::NotifyExternalWindowClosed(const TSharedRef<SWindow>& hostWindow)
{
	hostWindow->GetOnWindowClosedEvent().Clear();
	
	if (!this->AllowViewModeSwitch)
	{
		this->Shutdown();
	}
	
	EDockableWindowWidgetOnCloseExternalWindowBehavior behavior = this->GetOnCloseExternalWindowBehavior();
	if (behavior == EDockableWindowWidgetOnCloseExternalWindowBehavior::RedockOnMainScreen)
	{
		this->SetAsWindow(false);
	}
	else if (behavior == EDockableWindowWidgetOnCloseExternalWindowBehavior::RedockOnMainScreenAndHide)
	{
		this->SetAsWindow(false);
		this->SetisVisible(false);
	}
	else
	{
		verify(behavior == EDockableWindowWidgetOnCloseExternalWindowBehavior::Remove)
		this->Shutdown();
	}
}

EDockableWindowWidgetOnCloseExternalWindowBehavior SGamescreenDockableWindowWidget::GetOnCloseExternalWindowBehavior() const
{
	return EDockableWindowWidgetOnCloseExternalWindowBehavior::Remove;
}

EDockableWindowWidgetOnCloseGameScreenWidgetBehavior SGamescreenDockableWindowWidget::GetOnCloseGameScreenWidgetBehavior() const
{
	return EDockableWindowWidgetOnCloseGameScreenWidgetBehavior::Hide;
}

bool SGamescreenDockableWindowWidget::ShouldRemoveFromParentOnExternalWindowClose()
{
	return false;
}

EDockableWindowWidgetInputMode SGamescreenDockableWindowWidget::GetViewportInputMode() const
{
	return EDockableWindowWidgetInputMode::GameAndUI;
}

void SGamescreenDockableWindowWidget::UpdateInputModeRequest()
{
	EDockableWindowWidgetInputMode inputmode = this->GetViewportInputMode();
	if (this->IsInWindow())
	{
		this->Session->RemoveInputModeOverride(this->SharedThis(this));
	}
	else if (this->IsVisible())
	{
		this->Session->AddInputModeOverride(this->SharedThis(this));
	}
	else
	{
		this->Session->RemoveInputModeOverride(this->SharedThis(this));
	}
}

void SGamescreenDockableWindowWidget::SetHeaderTitle(FString newTitle)
{
	this->Header = newTitle;
	if (this->IsInWindow())
	{
		this->TryGetParentWindow()->SetTitle(FText::AsCultureInvariant(this->Header));
	}
	else
	{
		this->PseudoHeaderText->SetText(FText::AsCultureInvariant(this->Header));
	}
}

bool SGamescreenDockableWindowWidget::HasSettings() const
{
	return false;
}

void SGamescreenDockableWindowWidget::AddToViewport()
{
	if (!this->Session.IsValid() || !this->Session->GetCanvas())
	{
		return;
	}
	verify(this->Session->GetSessionType() == ELuaToolsSessionType::Game)
	TSharedRef<SWidget> This = this->AsShared();
	
	this->CloseWindowButton->SetVisibility(EVisibility::Visible);
	this->PseudoHeaderText->SetVisibility(EVisibility::SelfHitTestInvisible);
	//this->PseudoHeaderHBox->SetVisibility(EVisibility::SelfHitTestInvisible);
	//this->PseudoHeaderHBox->SetEnabled(true);

	TSharedPtr<SConstraintCanvas> hostCanvas = this->Session->GetCanvas();
	
	if (this->bDraggableInViewport)
	{
		TSharedPtr<SUnrealLuaDraggableBoxOverlay> overlay = nullptr;
		if (this->DraggableOverlay.IsValid())
		{
			overlay = this->DraggableOverlay.Pin();
		}
		else
		{
			FIntPoint designSize = GetDefault<UUserInterfaceSettings>()->DesignScreenSize;
			
			FVector2f initialOffset;
			FVector2D viewportSize;
			this->Session->GetViewportClient()->GetViewportSize(viewportSize);
			initialOffset.X = viewportSize.X * 0.3f;//(this->GameScreenAnchors.Maximum.X + this->GameScreenAnchors.Minimum.X) * 0.5f; 
			initialOffset.Y = viewportSize.Y * 0.3f;//(this->GameScreenAnchors.Maximum.Y + this->GameScreenAnchors.Minimum.Y) * 0.5f; 
			
			this->MainContentSizeBox->SetMinDesiredHeight(designSize.Y * (this->GameScreenAnchors.Maximum.Y + this->GameScreenAnchors.Minimum.Y) * 0.5f);
			//this->MainContentSizeBox->SetMaxDesiredHeight(viewportSize.Y * 0.5);
			this->MainContentSizeBox->SetMinDesiredWidth(designSize.X * (this->GameScreenAnchors.Maximum.X + this->GameScreenAnchors.Minimum.X) * 0.5f);
			//this->MainContentSizeBox->SetMaxDesiredWidth(viewportSize.X * 0.5);
			
			overlay = SNew(SUnrealLuaDraggableBoxOverlay)
				.IsDraggable(true)
				.Visibility(EVisibility::SelfHitTestInvisible)
				.InitialAlignmentOffset(initialOffset)
				.Content()
				[
					this->AsShared()
				];
			this->DraggableOverlay = overlay;
			
		}

		hostCanvas->AddSlot()
		[
			overlay.ToSharedRef()
		]
		.Anchors(FAnchors{0, 0,1,1});
		//.AutoSize(true);
		if (this->LastDockedPosition.IsSet())
		{
			overlay->RestoreFromDragBoxPosition(this->LastDockedPosition.Get());
		}
	}
	else
	{
		hostCanvas->RemoveSlot(This);
		hostCanvas->AddSlot()
		[
			This
		]
		.Anchors(this->GameScreenAnchors)
		.Alignment(this->GameScreenAlignment)
		.AutoSize(true);
	}
	this->IsInWindowChanged(false);
}

void SGamescreenDockableWindowWidget::RemoveFromViewport()
{
	this->CloseWindowButton->SetVisibility(EVisibility::Collapsed);
	this->PseudoHeaderText->SetVisibility(EVisibility::Hidden);
	//this->PseudoHeaderHBox->SetVisibility(EVisibility::Collapsed);
	//this->PseudoHeaderHBox->SetEnabled(false);
		
	TSharedPtr<SConstraintCanvas> hostCanvas = nullptr;
	if (this->Session.IsValid())
	{
		hostCanvas = this->Session->GetCanvas();
	}
	
	if (this->bDraggableInViewport)
	{
		TSharedPtr<SUnrealLuaDraggableBoxOverlay> overlay = this->DraggableOverlay.Pin();
		if (overlay.IsValid())
		{
			this->LastDockedPosition = overlay->GetDragBoxPosition();
			
			if (hostCanvas.IsValid())
			{
				hostCanvas->RemoveSlot(overlay.ToSharedRef());
			}
		}				
	}
	else
	{
		if (hostCanvas.IsValid())
		{
			hostCanvas->RemoveSlot(this->AsShared());
		}
	}
}

void SGamescreenDockableWindowWidget::AddToWindow()
{
	if (this->IsInWindow())
	{
		return;
	}
	this->MainContentSizeBox->SetMinDesiredHeight({});
	this->MainContentSizeBox->SetMaxDesiredHeight({});
	this->MainContentSizeBox->SetMinDesiredWidth({});
	this->MainContentSizeBox->SetMaxDesiredWidth({});
				
		
	TSharedRef<SWindow> newWindow = SNew(SWindow)
	.SupportsMaximize(true)
	.SupportsMinimize(true)
	.IsPopupWindow(false)
	.SizingRule(this->ExternalWindowSizingRule)
	.ClientSize(FVector2D((float)(GSystemResolution.ResX) * this->ExternalWindowSize.X, (float)(GSystemResolution.ResY) * this->ExternalWindowSize.Y))
	.SupportsTransparency(EWindowTransparency::None)
	.InitialOpacity(1.0f)
	.FocusWhenFirstShown(true)
	.bDragAnywhere(true)
	.IsTopmostWindow(false)
	.Title(FText::AsCultureInvariant(this->Header))
	.ActivationPolicy(EWindowActivationPolicy::Always)
	.ScreenPosition(FVector2D((float)(GSystemResolution.ResX) * this->ExternalWindowPosition.X, (float)(GSystemResolution.ResY) * this->ExternalWindowPosition.Y))
	[
		this->AsShared()
	];
	FOnWindowClosed del;
	del.BindSP(this,  &SGamescreenDockableWindowWidget::NotifyExternalWindowClosed);
	newWindow->SetOnWindowClosed(del);
	this->Window = FSlateApplication::Get().AddWindow(newWindow, true);
		
	this->IsInWindowChanged(true);
}

void SGamescreenDockableWindowWidget::RemoveFromWindow()
{
	if (this->IsInWindow())
	{
		TSharedPtr<SWindow> window = this->TryGetParentWindow();
		if (window)
		{
			window->SetOnWindowClosed({});
			window->SetContent(SNullWidget::NullWidget);
			FSlateApplication::Get().RequestDestroyWindow(window.ToSharedRef());
		}
	}

	this->Window = nullptr;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
