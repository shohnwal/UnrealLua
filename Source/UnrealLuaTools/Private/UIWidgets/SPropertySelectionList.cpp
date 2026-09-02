// Fill out your copyright notice in the Description page of Project Settings.


#include "UIWidgets/SPropertySelectionList.h"

#include "SlateOptMacros.h"
#include "Components/Widget.h"
#include "Framework/Application/SlateApplication.h"
#include "UObject/PropertyWithSetterAndGetter.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SPropertySelectionList::Construct(const FArguments& InArgs)
{
	this->bSingleChoiceMode = InArgs._SingleChoiceMode;
	TArray<FProperty*> preselectedProperties = InArgs._PreselectedProperties;
	TArray<FName> preselectedPropertyNames = InArgs._PreselectedPropertyNames;
	this->OnPropertySelectionChanged = InArgs._OnPropertyCheckboxChanged;
	SScrollBox::Construct(SScrollBox::FArguments()
		.Orientation(Orient_Vertical)
	);
	
	this->VerticalContentBox = SNew(SVerticalBox);
	this->AddSlot()
	[
		this->VerticalContentBox.ToSharedRef()	
	];
	//.AutoSize();
	
	this->SetViewedStruct(InArgs._TargetStruct, preselectedProperties, preselectedPropertyNames, InArgs._FilterListString);
}

void SPropertySelectionList::SetViewedStruct(UStruct* ustruct, const TArray<FProperty*>& preselectedProperties, const TArray<FName>& preselectedPropertyNames, FString filterListMustContain)
{
	if (this->Struct.Get() == ustruct)
	{
		return;
	}
	this->Struct.Reset(ustruct);
	this->VerticalContentBox->ClearChildren();
	this->SelectedProperties = {};
	this->CheckBoxes.Empty();
	
	if (this->Struct.IsValid())
	{
		UStruct* currentStruct = nullptr; 
		for (TFieldIterator<FProperty> propIt(this->Struct.Get()); propIt; ++propIt)
		{
			FProperty* prop = *propIt;
			if (prop->IsEditorOnlyProperty())
			{
				continue;
			}
			if (!filterListMustContain.IsEmpty() && !prop->GetName().Contains(filterListMustContain))
			{
				continue;
			}
			UStruct* ownerStruct = prop->GetOwnerStruct();
			if (ownerStruct != currentStruct)
			{
				currentStruct = ownerStruct;
				this->VerticalContentBox->AddSlot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Center)
				.Padding(10,10,10,10)
				[
					ConstructSeparator(currentStruct)
				];		
			}
		
			bool preselected = preselectedPropertyNames.Contains(prop->GetFName()) || preselectedProperties.Contains(prop);
			this->VerticalContentBox->AddSlot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			[
				this->ConstructListElement(prop, preselected)	
			];
			if (preselected)
			{
				this->SelectedProperties.AddUnique(prop);
			}
		}	
	}
}

TArray<FProperty*> SPropertySelectionList::GetSelectedProperties() const
{
	return this->SelectedProperties;
}

TSharedRef<SHorizontalBox> SPropertySelectionList::ConstructSeparator(UStruct* ustruct)
{
	TSharedRef<SHorizontalBox> widget = SNew(SHorizontalBox)
	+ SHorizontalBox::Slot()
	[
		SNew(SSeparator)
		.SeparatorImage(FAppStyle::GetBrush("Menu.Separator"))
		.Orientation(Orient_Horizontal)
	]
	.VAlign(VAlign_Center)
	+ SHorizontalBox::Slot()
	[
		SNew(STextBlock)
		.Text(FText::FromString(ustruct->GetAuthoredName()))
		.Justification(ETextJustify::Center)
	]
	.Padding(10, 0, 10, 0)
	.AutoWidth()
	+ SHorizontalBox::Slot()
	[
		SNew(SSeparator)
		.SeparatorImage(FAppStyle::GetBrush("Menu.Separator"))
		.Orientation(Orient_Horizontal)
	]
	.VAlign(VAlign_Center);
	return widget;
}

TSharedRef<SHorizontalBox> SPropertySelectionList::ConstructListElement(FProperty* prop, bool bIsSelected)
{
	if (this->bSingleChoiceMode)
	{
		TSharedRef<SHorizontalBox> elementHBox = SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		[
			SNew(SButton)
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant(prop->GetName()))
				.Visibility(EVisibility::SelfHitTestInvisible)
				.ToolTip(FSlateApplication::Get().MakeToolTip(FText::FromString(prop->GetName())))
			]
			.OnClicked_Lambda([this, prop]()
			{
				this->NotifyPropertyButtonClicked(prop);
				return FReply::Handled();
			})
		]	
		.FillWidth(1)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Fill);
		return elementHBox;
	}
	else
	{
		TSharedRef<SCheckBox> cb = SNew(SCheckBox)
						.IsChecked(bIsSelected ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
						.OnCheckStateChanged_Lambda([this, prop](ECheckBoxState state)
						{
							this->NotifyCheckedPropertyChanged(prop, state);
						});
	
		this->CheckBoxes.Emplace(prop, cb);
	
		TSharedRef<SHorizontalBox> elementHBox = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			cb
		]
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.AutoWidth()
		+ SHorizontalBox::Slot()
		[
			SNew(STextBlock)
			.Text(FText::FromString(prop->GetAuthoredName()))
			.ToolTip(FSlateApplication::Get().MakeToolTip(FText::FromString(prop->GetAuthoredName())))
		]
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Fill)
		.FillWidth(1);
	
		return elementHBox;
	}

}

void SPropertySelectionList::ClearAllChecks(FProperty* except)
{
	this->SelectedProperties.Empty();
	for (auto data : this->CheckBoxes)
	{
		if (data.Prop != except)
		{
			data.CheckBox->SetIsChecked(ECheckBoxState::Unchecked);
		}
	}	
}

void SPropertySelectionList::NotifyCheckedPropertyChanged(FProperty* prop, ECheckBoxState state)
{
	if (state == ECheckBoxState::Checked)
	{
		if (this->bSingleChoiceMode)
		{
			this->ClearAllChecks(prop);
		}
		this->SelectedProperties.AddUnique(prop);
	}
	else
	{
		this->SelectedProperties.Remove(prop);
	}
	this->OnPropertySelectionChanged.ExecuteIfBound(prop, state == ECheckBoxState::Checked);
}

void SPropertySelectionList::NotifyPropertyButtonClicked(FProperty* prop)
{
	this->OnPropertySelectionChanged.ExecuteIfBound(prop, true);
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
