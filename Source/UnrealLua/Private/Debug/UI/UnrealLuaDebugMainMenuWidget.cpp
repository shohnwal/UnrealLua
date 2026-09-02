// Fill out your copyright notice in the Description page of Project Settings.


#include "Debug/UI/UnrealLuaDebugMainMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Debug/DebugTools/UnrealLuaDebugTool.h"
#include "Utility/LuaLogMacros.h"

TSharedRef<SWidget> UUnrealLuaDebugMainMenuWidget::RebuildWidget()
{
	UCanvasPanel* canvasPanel = this->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), "UnrealLuaDebugCanvasPanel");
	this->WidgetTree->RootWidget = canvasPanel;
	//this->SetContentForSlot("CanvasPanel", canvasPanel);
	canvasPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	this->Canvas = canvasPanel;
	
	{
		//overlay for background and buttons
		UOverlay* mainoverlay = this->WidgetTree->ConstructWidget<UOverlay>();
		UCanvasPanelSlot* slot = this->Canvas->AddChildToCanvas(mainoverlay);
		slot->SetAnchors(FAnchors{0,0,0.3f,0.3f});
		{
			UImage* background = this->WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),"actorPickerImage");
			background->SetVisibility(ESlateVisibility::HitTestInvisible);
			UOverlaySlot* backgroundSlot = mainoverlay->AddChildToOverlay(background);
			backgroundSlot->SetHorizontalAlignment(HAlign_Fill);
			backgroundSlot->SetVerticalAlignment(VAlign_Fill);
			background->SetColorAndOpacity(FLinearColor{0,0,0, 0.5});
		}
		{
			UVerticalBox* buttonContainerVBox = this->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),"ButtonContainerVBox");
			this->MainMenuBUttonContainer = buttonContainerVBox;
		}
	}

	{
		UImage* actorPickerImage = this->WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),"actorPickerImage");
		actorPickerImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* slot = this->AddWidgetToMainCanvas(actorPickerImage);
		slot->SetAnchors(FAnchors{0,0,0.3f,0.3f});
		actorPickerImage->SetColorAndOpacity(FLinearColor{0,0,0, 0.1});
	}

	{
		UTextBlock* actorPickerText = this->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),"actorPickerText");
		actorPickerText->SetVisibility(ESlateVisibility::HitTestInvisible);
		actorPickerText->SetText(FText::AsCultureInvariant("Picking actors"));
		UCanvasPanelSlot* textSlot = this->AddWidgetToMainCanvas(actorPickerText);
		textSlot->SetAnchors(FAnchors{0,0,1,1});
		textSlot->SetPosition({100, 100});
	}	

	return Super::RebuildWidget();
}

void UUnrealLuaDebugMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

}

UCanvasPanelSlot* UUnrealLuaDebugMainMenuWidget::AddWidgetToMainCanvas(UWidget* widget)
{
	if (!widget)
	{
		return nullptr;
	}
	return this->Canvas->AddChildToCanvas(widget);
}

void UUnrealLuaDebugMainMenuWidget::BeginDestroy()
{
	if (!this->IsTemplate())
	{
		LUA_LOG("Destroying UUnrealLuaDebugUICanvas");
	}
	Super::BeginDestroy();
}

void UUnrealLuaDebugMainMenuWidget::NotifyToolButtonClicked()
{
	
}

void UUnrealLuaDebugMainMenuWidget::AddTool(UUnrealLuaDebugTool* tool)
{
	if (!tool)
	{
		return;
	}
	FName toolname = tool->GetToolMainMenuButtonName();
	if (toolname == NAME_None)
	{
		return;
	}
	if (this->RegisteredTools.Contains(tool->GetClass()))
	{
		return;
	}
	verify(!this->RegisteredToolButtons.Contains(toolname));
	this->RegisteredTools.Emplace(tool->GetClass(), tool);
	FString toolNameStr = toolname.ToString();
	
	UButton* toolButton = this->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(),*(toolNameStr + "ToolButton"));
	toolButton->OnClicked.AddDynamic(this, &UUnrealLuaDebugMainMenuWidget::NotifyToolButtonClicked);
	
	UTextBlock* toolButtonText = this->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),*(toolNameStr + "ToolButtonText"));
	toolButtonText->SetText(FText::AsCultureInvariant(toolNameStr));
	toolButtonText->SetVisibility(ESlateVisibility::Visible);
	UButtonSlot* buttonSlot = Cast<UButtonSlot>(toolButton->AddChild(toolButtonText));
	buttonSlot->SetHorizontalAlignment(HAlign_Center);
	buttonSlot->SetVerticalAlignment(VAlign_Center);
	
}

void UUnrealLuaDebugMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
}
