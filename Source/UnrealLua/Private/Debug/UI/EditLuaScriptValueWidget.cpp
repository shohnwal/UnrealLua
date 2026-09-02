// Fill out your copyright notice in the Description page of Project Settings.


#include "Debug/UI/EditLuaScriptValueWidget.h"
#include "Components/CanvasPanel.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/TextBlock.h"

void UEditLuaScriptValueWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
}

void UEditLuaScriptValueWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
}

void UEditLuaScriptValueWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UEditLuaScriptValueWidget::NativeDestruct()
{
	Super::NativeDestruct();
}

void UEditLuaScriptValueWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}

APlayerController* UEditLuaScriptValueWidget::GetOwningPlayer() const
{
	return Super::GetOwningPlayer();
}

TSharedRef<SWidget> UEditLuaScriptValueWidget::RebuildWidget()
{
	auto rootWidget = Super::RebuildWidget();
	
	UWidgetTree* tree = this->WidgetTree;
	if (tree)
	{
		this->CanvasPanel = tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), "WidgetCanvasPanel");
	
		if (this->CanvasPanel)
		{
			UHorizontalBox* hbox = tree->ConstructWidget<UHorizontalBox>();
			if (hbox)
			{
				this->CanvasPanel->AddChildToCanvas(hbox);
				
				UTextBlock* keyNameWidget = tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), "KeyNameWidget");
				hbox->AddChildToHorizontalBox(keyNameWidget);				
				
				UMultiLineEditableTextBox* editTextBox = tree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(),"EditTextBox");
				hbox->AddChildToHorizontalBox(editTextBox);
			}
		}

	}

	
	return rootWidget;
}

void UEditLuaScriptValueWidget::OnWidgetRebuilt()
{
	Super::OnWidgetRebuilt();
}

void UEditLuaScriptValueWidget::GetSlotNames(TArray<FName>& SlotNames) const
{
	Super::GetSlotNames(SlotNames);
}

UWidget* UEditLuaScriptValueWidget::GetContentForSlot(FName SlotName) const
{
	return Super::GetContentForSlot(SlotName);
}

void UEditLuaScriptValueWidget::SetContentForSlot(FName SlotName, UWidget* Content)
{
	Super::SetContentForSlot(SlotName, Content);
}
