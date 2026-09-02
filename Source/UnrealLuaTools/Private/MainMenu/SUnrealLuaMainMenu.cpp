// Fill out your copyright notice in the Description page of Project Settings.


#include "MainMenu/SUnrealLuaMainMenu.h"

#include "SlateOptMacros.h"
#include "Components/VerticalBox.h"
#include "Config/UnrealLuaConfig.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Session/UnrealLuaToolsSession.h"
#include "Subsystem/UnrealLuaTools.h"
#include "Tools/UnrealLuaTool.h"
#include "Utility/LuaLogMacros.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#define LOCTEXT_NAMESPACE "UnrealLuaTools"
BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SUnrealLuaMainMenu::Construct(const FArguments& InArgs)
{
	SGamescreenDockableWindowWidget::Construct(SGamescreenDockableWindowWidget::FArguments()
	.ExternalWindowAnchors(InArgs._ExternalWindowAnchors)
	.ExternalWindowSize(InArgs._ExternalWindowSize)
	.ExternalWindowPosition(InArgs._ExternalWindowPosition)
	.GameScreenAnchors(InArgs._GameScreenAnchors)
	.BackgroundColor(InArgs._BackgroundColor)
	.Title(InArgs._Title)
	.Session(InArgs._Session)
	.GameScreenAlignment(InArgs._GameScreenAlignment)
	.InitiallyHidden(InArgs._InitiallyHidden)
	.StartAsWindow(InArgs._StartAsWindow)
	.ExternalWindowSizingRule(InArgs._ExternalWindowSizingRule)
	.DraggableInGameScreen(false)
	.AllowViewModeSwitch(false)
	);
	
	this->WindowBodyVBox->AddSlot()
	[
		SAssignNew(MainMenuButtonContainer, SVerticalBox)
	]
	.AutoHeight()
	.Padding(FMargin(10))
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Center);
	
	if (this->Session->GetSessionType() == ELuaToolsSessionType::Game)
	{
		this->Session->GetOninputKeyEvent().AddSP(this, &SUnrealLuaMainMenu::NotifyInputKeyEvent);
	}
}

void SUnrealLuaMainMenu::NotifyInputKeyEvent(const FInputKeyEventArgs& inputEvent)
{
	FKey key = inputEvent.Key;
	EInputEvent eventType = inputEvent.Event;
	
	if (eventType != EInputEvent::IE_Pressed)
	{
		return;
	}
	if (key == UUnrealLuaConfig::GetMainMenuKey())
   {
   		this->ToggleVisibility();
   }
}

EDockableWindowWidgetOnCloseExternalWindowBehavior SUnrealLuaMainMenu::GetOnCloseExternalWindowBehavior() const
{
	return EDockableWindowWidgetOnCloseExternalWindowBehavior::RedockOnMainScreen;
}

EDockableWindowWidgetOnCloseGameScreenWidgetBehavior SUnrealLuaMainMenu::GetOnCloseGameScreenWidgetBehavior() const
{
	return EDockableWindowWidgetOnCloseGameScreenWidgetBehavior::Hide;
}

void SUnrealLuaMainMenu::AddTool(UUnrealLuaTool* tool)
{
	if (!tool)
	{
		return;
	}
	if (this->RegisteredTools.Contains(tool->GetClass()))
	{
		return;
	}
	FString toolname = tool->GetToolMainMenuButtonLabel();
	if (toolname.IsEmpty())
	{
		return;
	}
	this->RegisteredTools.Emplace(tool->GetClass(), tool);
	
	TWeakObjectPtr<UUnrealLuaTool> weakToolPtr{tool};
	this->MainMenuButtonContainer->AddSlot()
	.AutoHeight()
	.VAlign(VAlign_Top)
	.Padding(2.f)
	[
		SNew(SButton)
		.OnClicked_Lambda([weakToolPtr]()
		{
			if (weakToolPtr.IsValid())
			{
				weakToolPtr->NotifyMainMenuButtonClicked();
			}
			return FReply::Handled();
		})
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant(toolname))
		]
		.DesiredSizeScale(2)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
	];
	tool->NotifyAddedToMainMenu();
}

void SUnrealLuaMainMenu::Show()
{
	SGamescreenDockableWindowWidget::Show();
}

void SUnrealLuaMainMenu::Hide()
{
	SGamescreenDockableWindowWidget::Hide();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
#undef LOCTEXT_NAMESPACE