// Fill out your copyright notice in the Description page of Project Settings.


#include "UIWidgets/SLuaToolsOptionsButton.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Utility/WidgetStyles.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SSpinBox.h"

void SLuaToolsOptionsButton::Construct(const FArguments& InArgs)
{
	this->ChildSlot
	[
		SAssignNew(SettingsButton, SButton)
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant(L"\x2699"))
				.Visibility(EVisibility::HitTestInvisible)
			]
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Top)
			.Visibility(EVisibility::Visible)
			.OnClicked(this, &SLuaToolsOptionsButton::NotifySettingsButtonClicked)
	];
	
	UnrealLuaTools::SlateStyles::GetOnStyleChangedDelegate().AddSP(this, &SLuaToolsOptionsButton::NotifyStyleChanged);
}

FReply SLuaToolsOptionsButton::NotifySettingsButtonClicked()
{
	FSlateIcon DummyIcon(NAME_None, NAME_None);
	
	FMenuBuilder menuBuilder(true, nullptr, nullptr, false, &FCoreStyle::Get(), false );

	TSharedRef<SHorizontalBox> hbox = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant("Code font size"))
		]
		.VAlign(VAlign_Center)
		.AutoWidth()
		.Padding(2,2,5,2);
	
	TSharedPtr<SSpinBox<int32>> slider = nullptr;
	
	FVector2f fontsizeLimits = UnrealLuaTools::SlateStyles::GetEditableTextBoxFontSizeLimits();
	TSharedRef<SBox> fontSizeSlider = SNew(SBox)
		.MinDesiredWidth(100)
		[
			//SAssignNew(slider, SSlider)
			//.MinValue(8)
			//.MaxValue(96)
			//.Value(UnrealLuaTools::SlateStyles::GetEditableTextBoxFontSize())
			//.OnValueChanged_Lambda([](float newValue) { UnrealLuaTools::SlateStyles::SetEditableTextBoxFontSize(static_cast<int32>(newValue)); })
			//.StepSize(1.f)
			//.Orientation(EOrientation::Orient_Horizontal)
			SAssignNew(slider, SSpinBox<int32>)
			.MinValue(fontsizeLimits.X)
			.MaxValue(fontsizeLimits.Y)
			.MinDesiredWidth(100)

			.Value(UnrealLuaTools::SlateStyles::GetEditableTextBoxFontSize())
			.OnValueChanged_Lambda(
				[this](int32 newValue)
				{
					UnrealLuaTools::SlateStyles::SetEditableTextBoxFontSize(static_cast<int32>(newValue));
				})
		];
	
	this->OnFontSizeChanged.AddSPLambda(slider.Get(), [slider](float newSize)
	{
		if (slider->GetValue() != static_cast<int32>(newSize))
		{
			slider->SetValue(static_cast<int32>(newSize));
		}
	});
	
	hbox->AddSlot()
	.Padding(2,2,2,2)
	[
		fontSizeSlider
	]
	.AutoWidth();
	
	menuBuilder.BeginSection(NAME_None, FText::AsCultureInvariant("Options"));
	{
		menuBuilder.AddWidget(hbox, FText::AsCultureInvariant("Font Size"), true, false);
	}
	
	FVector2D widgetPosition = this->SettingsButton->GetTickSpaceGeometry().GetAbsolutePosition();
	FVector2D widgetSize =  this->SettingsButton->GetTickSpaceGeometry().GetAbsoluteSize();
	FVector2D summonLocation = FVector2D{widgetPosition.X, widgetPosition.Y + widgetSize.Y}; 
	FSlateApplication::Get().PushMenu(
		this->AsShared(),
		FWidgetPath {},
		hbox, summonLocation
		/*FSlateApplication::Get().GetCursorPos()*/ ,
		FPopupTransitionEffect::ContextMenu, false
	);
	
	return FReply::Handled();
}

void SLuaToolsOptionsButton::NotifyFontSizeTextCommitted(const FText& text, ETextCommit::Type arg)
{
	if (text.IsNumeric())
	{
		float val = FCString::Atof(*text.ToString());
		UnrealLuaTools::SlateStyles::SetEditableTextBoxFontSize(static_cast<int32>(val));
	}
}

void SLuaToolsOptionsButton::NotifyStyleChanged()
{
	const FEditableTextBoxStyle* style = UnrealLuaTools::SlateStyles::GetEditableTextBoxStyle();
	this->OnFontSizeChanged.Broadcast(style->TextStyle.Font.Size);
}
