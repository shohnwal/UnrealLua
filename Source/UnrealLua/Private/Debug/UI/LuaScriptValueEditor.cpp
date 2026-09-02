// Fill out your copyright notice in the Description page of Project Settings.


#include "Debug/UI/LuaScriptValueEditor.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

TSharedRef<SWidget> ULuaScriptValueEditor::RebuildWidget()
{
	UCanvasPanel* canvasPanel = this->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), "UnrealLuaDebugCanvasPanel");
	canvasPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	this->WidgetTree->RootWidget = canvasPanel;
	this->NewValueString = "testkey";
	{
		USizeBox* sizeBox = this->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		sizeBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UCanvasPanelSlot* canvasPanelSlot = canvasPanel->AddChildToCanvas(sizeBox);
		canvasPanelSlot->SetAutoSize(true);
		sizeBox->SetHeightOverride(500);
		sizeBox->SetWidthOverride(500);
		sizeBox->SetMaxDesiredHeight(500);
		sizeBox->SetMaxDesiredWidth(500);
		FAnchorData AnchorData { FMargin(1.f, 1.f, 1.f, 1.f), FAnchors{0.5,0.5,0.5,0.5}, {0.5,0.5} };
		canvasPanelSlot->SetLayout(AnchorData);
		{
			UOverlay* background = this->WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
			background->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			auto sizeboxslot = sizeBox->AddChild(background);
			{
				UImage* image = this->WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
				image->SetVisibility(ESlateVisibility::HitTestInvisible);
				image->SetColorAndOpacity(FLinearColor{0.1,0.1,0.1,0.9});
				UOverlaySlot* backgroundSlot = background->AddChildToOverlay(image);
				backgroundSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
				backgroundSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Fill);
			}

			{
				UVerticalBox* vbox = this->WidgetTree->ConstructWidget<UVerticalBox>();
				UOverlaySlot* backgroundSlot = background->AddChildToOverlay(vbox);
				backgroundSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
				backgroundSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Fill);
				{
					//Key and value input fields
					UHorizontalBox* horizontalBox = this->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
					horizontalBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
					auto* vboxSlot = vbox->AddChildToVerticalBox(horizontalBox);
					vboxSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
					vboxSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Fill);
					{
						UTextBlock* valueKeyText = this->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
						valueKeyText->SetVisibility(ESlateVisibility::HitTestInvisible);
						valueKeyText->SetText(FText::FromString(this->NewValueString));
						UHorizontalBoxSlot* hboxslot1 = horizontalBox->AddChildToHorizontalBox(valueKeyText);
						hboxslot1->SetHorizontalAlignment(HAlign_Left);
						hboxslot1->SetVerticalAlignment(VAlign_Center);
						hboxslot1->SetPadding(FMargin(5.f, 1.f, 5.f, 1.f));
				
						this->KeyStringWidget = valueKeyText;
					}
					{
						UMultiLineEditableTextBox* editValueTextBox = this->WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), "NewValueTextBox");
						UHorizontalBoxSlot* hboxslot = horizontalBox->AddChildToHorizontalBox(editValueTextBox);
						hboxslot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
						hboxslot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
						hboxslot->SetSize(FSlateChildSize{ESlateSizeRule::Fill});
						hboxslot->SetPadding(FMargin(5.f, 1.f, 5.f, 1.f));
						editValueTextBox->SetVisibility(ESlateVisibility::Visible);
						editValueTextBox->SetHintText(FText::FromString("Enter value expression..."));
						editValueTextBox->SetIsReadOnly(false);
						FTextBlockStyle textstyle = editValueTextBox->WidgetStyle.TextStyle;
						textstyle.Font = FAppStyle::GetFontStyle( TEXT("PropertyWindow.NormalFont"));
						textstyle.ColorAndOpacity = FLinearColor::Black;
						
						editValueTextBox->WidgetStyle.SetTextStyle(textstyle);
				
						this->EditValueTextBox = editValueTextBox;
					}		
				}
				{
					//Cancel and apply buttons
					UHorizontalBox* horizontalBox = this->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
					horizontalBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
					auto* vboxSlot = vbox->AddChildToVerticalBox(horizontalBox);
					vboxSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
					vboxSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Bottom);
					{
						UButton* applyButton = this->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
						applyButton->SetVisibility(ESlateVisibility::Visible);
						UHorizontalBoxSlot* hboxSlot = horizontalBox->AddChildToHorizontalBox(applyButton);
						hboxSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
						hboxSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
						{
							UTextBlock* okButtonText = this->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
							okButtonText->SetVisibility(ESlateVisibility::HitTestInvisible);
							okButtonText->SetText(FText::FromString("Apply"));
							UPanelSlot* buttonSlot = applyButton->AddChild(okButtonText);
						}
					}
					{
						UButton* cancelButton = this->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
						cancelButton->SetVisibility(ESlateVisibility::Visible);
						UHorizontalBoxSlot* hboxSlot = horizontalBox->AddChildToHorizontalBox(cancelButton);
						hboxSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
						hboxSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
						{
							UTextBlock* cancelButtonText = this->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
							cancelButtonText->SetVisibility(ESlateVisibility::HitTestInvisible);
							cancelButtonText->SetText(FText::FromString("Cancel"));
							UPanelSlot* buttonSlot = cancelButton->AddChild(cancelButtonText);
						}
					}
				}
			}
		}
		
	}
	
	
	return Super::RebuildWidget();
}

void ULuaScriptValueEditor::NativeConstruct()
{
	Super::NativeConstruct();
	//this->EditValueTextBox->SetFocus();
}

void ULuaScriptValueEditor::InitializeLuaScriptEditor(FLuaScriptValue* luaScriptValue)
{
	this->OriginalValue = luaScriptValue->GetLuaValue().ToValueString();
	this->Key = luaScriptValue->GetKeyNameString();
}

void ULuaScriptValueEditor::UpdateCurrentScriptValue(const FString& currentValueString)
{
	this->OriginalValue = currentValueString;
}
