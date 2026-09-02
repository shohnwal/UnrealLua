// Fill out your copyright notice in the Description page of Project Settings.


#include "Tools/LuaConfig/SUnrealLuaConfigEditor.h"
#include "SlateOptMacros.h"
#include "Components/VerticalBox.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Text/STextBlock.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SUnrealLuaConfigEditor::Construct(const FArguments& InArgs)
{
	this->LoadDataFromConfig();
	
	SGamescreenDockableWindowWidget::Construct(SGamescreenDockableWindowWidget::FArguments()
	.GameScreenAnchors(FAnchors{0.2,0.1,0.8,0.9})
	.GameScreenAlignment(FVector2D{0.5f})
	.ExternalWindowSize(FVector2D{0.5f,0.5f})
	.ExternalWindowPosition(FVector2D{0.5f,0.5f})
	.Title("Lua Config Editor")
	.Session(InArgs._Session)
	.InitiallyHidden(true)
	.StartAsWindow(false)
	.ExternalWindowSizingRule(ESizingRule::Autosized)
	.DraggableInGameScreen(true)
	);
	
	TSharedPtr<SHorizontalBox> ContentHBox;
	
	this->WindowBodyVBox->AddSlot()
	[
		SAssignNew(ContentHBox,SHorizontalBox)
	]
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill)
	.FillHeight(1)
	.Padding(10,10,10,10);
	
	this->WindowBodyVBox->AddSlot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Bottom)
	.AutoHeight()
	[
		SNew(SBorder)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(SButton)
				[
					SNew(STextBlock)
					.Text(FText::AsCultureInvariant("Ok"))
					.Font(FAppStyle::GetWidgetStyle<FTextBlockStyle>("NormalText").Font)
					.Visibility(EVisibility::SelfHitTestInvisible)
					.Margin(FMargin(2,2,2,2))
				]
				.ButtonStyle(&FButtonStyle::GetDefault())
				.OnClicked_Lambda([this]()
				{
					this->NotifyOkButtonClicked();	
					return FReply::Handled();;
				})
			]
			.AutoWidth()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.FillWidth(1)
			.Padding(10,10,10,10)
			+ SHorizontalBox::Slot()
			[
				SNew(SButton)
				[
					SNew(STextBlock).Text(FText::AsCultureInvariant("Apply"))
					.Font(FAppStyle::GetWidgetStyle<FTextBlockStyle>("NormalText").Font)
					.Visibility(EVisibility::SelfHitTestInvisible)
					.Margin(FMargin(2,2,2,2))
				]
				.ButtonStyle(&FButtonStyle::GetDefault())
				.OnClicked_Lambda([this]()
				{
					UUnrealLuaConfig::Get()->ApplySettingsToCache(this->TempConfigData);
					UUnrealLuaConfig::Get()->WriteConfigToLuaFile();
					this->LoadDataFromConfig();
					
					return FReply::Handled();
				})
			]
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.AutoWidth()
			.FillWidth(1)
			.Padding(10,10,10,10)
			+ SHorizontalBox::Slot()
			[
				SNew(SButton)
				[
					SNew(STextBlock).Text(FText::AsCultureInvariant("Reset"))
					.Font(FAppStyle::GetWidgetStyle<FTextBlockStyle>("NormalText").Font)
					.Visibility(EVisibility::SelfHitTestInvisible)
					.Margin(FMargin(2,2,2,2))
				]
				.ButtonStyle(&FButtonStyle::GetDefault())
				.OnClicked_Lambda([this]()
				{
					this->LoadDataFromConfig();
					return FReply::Handled();
				})
			]
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.AutoWidth()
			.FillWidth(1)
			.Padding(10,10,10,10)
			+ SHorizontalBox::Slot()
			[
				SNew(SButton)
				[
					SNew(STextBlock).Text(FText::AsCultureInvariant("Cancel"))
					.Font(FAppStyle::GetWidgetStyle<FTextBlockStyle>("NormalText").Font)
					.Visibility(EVisibility::SelfHitTestInvisible)
					.Margin(FMargin(2,2,2,2))
				]
				.ButtonStyle(&FButtonStyle::GetDefault())
				.OnClicked_Lambda([this]()
				{
					this->NotifyCancelButtonClicked();
					return FReply::Handled();
				})
			]
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.AutoWidth()
			.FillWidth(1)
			.Padding(10,10,10,10)	
		]
		.Padding(2)
		.VAlign(VAlign_Bottom)
		.HAlign(HAlign_Fill)
	];
	
	//Left side category buttons
	ContentHBox->AddSlot()
	.AutoWidth()
	[
		SNew(SBox)
		.WidthOverride(150)
		[
			SAssignNew(CategoryVBox, SVerticalBox)
		]
	];
	
	//Main content : category title + content
	ContentHBox->AddSlot()
	.FillWidth(1)
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	[
		SAssignNew(SelectedCategoryContentSwitcher, SWidgetSwitcher)
	];


	
	
	auto generalTemplate = this->MakeBodyTemplate(EConfigBodyType::General);
	auto generalBody = this->MakeCategoryBody(generalTemplate);
	
	auto advTemplate = this->MakeBodyTemplate(EConfigBodyType::Advanced);
	auto advBody = this->MakeCategoryBody(advTemplate);
	
	auto gcTemplate = this->MakeBodyTemplate(EConfigBodyType::GC);
	auto gcBody = this->MakeCategoryBody(gcTemplate);
	
	auto modsTemplate = this->MakeBodyTemplate(EConfigBodyType::Mods);
	auto modsBody = this->MakeCategoryBody(modsTemplate);
	
	this->AddCategory("General", "General Settings", generalBody);
	this->AddCategory("Advanced", "", advBody);
	this->AddCategory("Mods", "Lua script Mod Settings", modsBody);
	this->AddCategory("Garbage Collection", "Lua Garbage Collection Settings", gcBody);
}

void SUnrealLuaConfigEditor::NotifyOkButtonClicked()
{
	UUnrealLuaConfig::Get()->ApplySettingsToCache(this->TempConfigData);
	
	UUnrealLuaConfig::Get()->WriteConfigToLuaFile();
	
	this->LoadDataFromConfig();
	
	this->Hide();
}

void SUnrealLuaConfigEditor::NotifyCancelButtonClicked()
{
	this->LoadDataFromConfig();
	
	this->Hide();
}

void SUnrealLuaConfigEditor::NotifyDebugKeyTextBoxInput(const FKeyEvent& key)
{
	if (key.GetKey().IsValid())
	{
		this->DebugKeyTextBox->SetText(FText::AsCultureInvariant(key.GetKey().ToString()));
	
		this->TempConfigData.UnrealLuaToolsMenuKey = key.GetKey();	
	}
}


void SUnrealLuaConfigEditor::LoadDataFromConfig()
{
	//Copy currently cached settings
	this->TempConfigData = UUnrealLuaConfig::Get()->GetCachedSettings();
	
	//TODO : update widgets
}

void SUnrealLuaConfigEditor::AddCategory(FString buttonName, FString tooltip, TSharedRef<SWidget> content)
{
	int32 topPadding = this->CategoryVBox->NumSlots() == 0 ? 2 : 0;
	this->CategoryVBox->AddSlot()
	.HAlign(HAlign_Fill)
	.FillHeight(1)
	.AutoHeight()
	.Padding(2, topPadding, 2, 2)
	[
		SNew(SButton)
		.ToolTipText(FText::AsCultureInvariant(tooltip))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Fill)
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant(buttonName))
			.Justification(ETextJustify::Center)
		]
		.OnClicked_Lambda([this, content]() { this->SelectedCategoryContentSwitcher->SetActiveWidget(content); return FReply::Handled();})
	];	
	
	this->SelectedCategoryContentSwitcher->AddSlot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	[
		content
	];
}

TSharedRef<SBox> SUnrealLuaConfigEditor::MakeCategoryBody(const FLuaConfigEditorBodyTemplate& bodyTemplate)
{
	TSharedPtr<SGridPanel> grid = nullptr;
	TSharedPtr<STextBlock> DescriptionText = nullptr;
	TSharedRef<SBox> body = SNew(SBox)
		.HeightOverride(600)
		.WidthOverride(800)
		.Content()
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Top)
				[
					SNew(SBorder)
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Top)
					[
						SNew(STextBlock)
						.Text(FText::AsCultureInvariant(bodyTemplate.BodyTitle))
						.Justification(ETextJustify::Center)
					]
				]
				+ SVerticalBox::Slot()
				.FillHeight(1)
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SScrollBox)
					.Orientation(EOrientation::Orient_Vertical)
					+ SScrollBox::Slot()
					[
						SAssignNew(grid, SGridPanel)
						.FillColumn(1, 1) //Description column stretches
					]
					.FillSize(1)
				]
				+ SVerticalBox::Slot()
				.VAlign(VAlign_Bottom)
				.AutoHeight()
				[
					SNew(SBox)
					.MinDesiredHeight(100)
					[
						SNew(SBorder)
						[
							SAssignNew(DescriptionText, STextBlock)
						]
						.VAlign(VAlign_Fill)
					]
				]
			]
		];
	this->FillCategoryBody(grid.ToSharedRef(), bodyTemplate, DescriptionText.ToSharedRef());
	return body;
}

void SUnrealLuaConfigEditor::FillCategoryBody(TSharedRef<SGridPanel> contentGrid, const FLuaConfigEditorBodyTemplate& bodyTemplate, TSharedRef<STextBlock> descriptionText)
{
	for (auto it = bodyTemplate.Rows.CreateConstIterator(); it; ++it)
	{
		int32 index = it.GetIndex();
		const FLuaConfigEditorBodyTemplateRow& row = *it;
		TSharedPtr<SBox> box1 = nullptr;
		TSharedPtr<SBox> box2 = nullptr;
		contentGrid->AddSlot(0, index)
		[
			SAssignNew(box1, SBox)
			.MinDesiredHeight(50)
			.MinDesiredWidth(200)
			.MaxDesiredHeight(800)
			.Content()
			[
				SNew(SBorder)
				.Padding(1)
				.BorderBackgroundColor(FLinearColor(0.2f, 0.2f, 0.2f, 1))
				[
					SNew(SBox)
					.Padding(2)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Content()
					[
						SNew(STextBlock)
						.Text(FText::AsCultureInvariant(row.RowTitle))
						.ToolTip(FSlateApplication::Get().MakeToolTip(FText::AsCultureInvariant(row.Description)))
						.Justification(ETextJustify::Center)
					]
				]
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
			]
		];
		
		contentGrid->AddSlot(1, index)
		[
			SAssignNew(box2, SBox)
			.MinDesiredHeight(50)
			.MinDesiredWidth(50)
			.Content()
			[
				SNew(SBorder)
				.Padding(1)
				.BorderBackgroundColor(FLinearColor(0.2f, 0.2f, 0.2f, 1))
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Content()
				[
					row.Content.ToSharedRef()						
				]
			]
		];
		FNoReplyPointerEventHandler del;
		del.BindLambda([descriptionText, description =  row.Description](const FGeometry& geometry, const FPointerEvent& event)
		{
			descriptionText->SetText(FText::AsCultureInvariant(description));
		});
		FSimpleNoReplyPointerEventHandler del2;
		del2.BindLambda([descriptionText](const FPointerEvent& event){ descriptionText->SetText(FText::GetEmpty());});
		box1->SetOnMouseEnter(del);
		box1->SetOnMouseLeave(del2);
		box2->SetOnMouseEnter(del);
		box2->SetOnMouseLeave(del2);
		
		//contentGrid->AddSlot(2, index)
		//.HAlign(HAlign_Fill)
		//.VAlign(VAlign_Fill)
		//[
		//	SNew(SBorder)
		//	.Padding(1)
		//	.BorderBackgroundColor(FLinearColor(0.2f, 0.2f, 0.2f, 1))
		//	[
		//		SNew(SBox)
		//		.Content()
		//		[
		//			SNew(STextBlock)
		//			.Text(FText::AsCultureInvariant(row.Description))
		//			.AutoWrapText(true)
		//			.Justification(ETextJustify::Left)
		//		]
		//		.Padding(10)
		//		.HAlign(HAlign_Fill)
		//		.VAlign(VAlign_Center)
		//	]
		//	.HAlign(HAlign_Left)
		//	.VAlign(VAlign_Center)
		//];
	}
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
