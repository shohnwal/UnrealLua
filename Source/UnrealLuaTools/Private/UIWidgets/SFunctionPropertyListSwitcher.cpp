 // Fill out your copyright notice in the Description page of Project Settings.


#include "UIWidgets/SFunctionPropertyListSwitcher.h"

#include "SlateOptMacros.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SFunctionPropertyListSwitcher::Construct(const FArguments& InArgs)
{
	
	this->bSingleChoiceMode = InArgs._SingleChoiceMode;
	this->TargetStruct = TStrongObjectPtr<UStruct>(InArgs._TargetStruct);
	this->ItemListType = InArgs._ItemListType;
	TArray<UFunction*> allFunctions = InArgs._Functions;
	TArray<UFunction*> preselectedFunctions = InArgs._PreselectedFunctions;
	TArray<FName> preselectedFunctionnames = InArgs._PreselectedFunctionNames;
	TArray<FProperty*> preselectedProperties = InArgs._PreselectedProperties;
	TArray<FName> preselectedPropertyNames = InArgs._PreselectedPropertyNames;

	auto normalStyle = FAppStyle::GetWidgetStyle<FTextBlockStyle>("NormalText");
	
	bool hasFunctions = !allFunctions.IsEmpty();
	this->ChildSlot
	[
		SAssignNew(ContentVerticalBox, SVerticalBox)
	];
	
	this->ContentVerticalBox->AddSlot()
	[
		//Widget switcher buttons
		SAssignNew(WidgetSwitcherButtonsSection, SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			SNew(SButton)
			.OnClicked_Lambda([this]()
			{
				this->SetActiveWidget(EActiveWitgetListSelection::UFunction);
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(FText::FromString("Functions"))
				.Font(normalStyle.Font)
				.Justification(ETextJustify::Center)
				.Margin(2)
			]
		]
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Fill)
		.Padding(5)
		+ SHorizontalBox::Slot()
		[
			SNew(SButton)
			.OnClicked_Lambda([this]()
			{
				this->SetActiveWidget(EActiveWitgetListSelection::FProperty);
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(FText::FromString("Properties"))
				.Font(normalStyle.Font)
				.Justification(ETextJustify::Center)
				.Margin(2)
			]
		]
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Fill)
		.Padding(5)
	]
	.AutoHeight()
	.VAlign(VAlign_Top);
	
	this->ContentVerticalBox->AddSlot()
	[
		SNew(SBox)
		[
			ConstructWidgetSwitcher()
		]
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		.WidthOverride(300)
	]
	.FillHeight(1)
	.VAlign(VAlign_Fill);
	
	this->Switcher->AddSlot()
	[
		ConstructFunctionsList(allFunctions, preselectedFunctions, preselectedFunctionnames)
	]
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill);
	this->Switcher->AddSlot()
	[
		ConstructPropertyList()
	];
}

void SFunctionPropertyListSwitcher::SetViewedStruct(UStruct* ustruct, const TArray<FProperty*>& preselectedProperties, const TArray<FName>& preselectedPropertyNames)
{
	this->PropertyListWidget->SetViewedStruct(ustruct, preselectedProperties, preselectedPropertyNames);
	this->FunctionListWidget->SetViewedStruct(ustruct);
}

void SFunctionPropertyListSwitcher::SetActiveWidget(EActiveWitgetListSelection selection)
{
	if (selection == EActiveWitgetListSelection::FProperty)
	{
		this->Switcher->SetActiveWidget(this->PropertyListWidget.ToSharedRef());
	}
	else
	{
		this->Switcher->SetActiveWidget(this->FunctionListWidget.ToSharedRef());
	}
}

TSharedRef<SUFunctionSelectionList> SFunctionPropertyListSwitcher::ConstructFunctionsList(const TArray<UFunction*>& functions,const TArray<UFunction*>& preselectedFunctions, const TArray<FName>& preselectedFunctionNames)
{
	this->FunctionListWidget = SNew(SUFunctionSelectionList)
	.Functions(functions)
	.AddSeparators(true)
	.SingleChoiceMode(this->bSingleChoiceMode)
	.PreselectedFunctions(preselectedFunctions)
	.PreselectedFunctionNames(preselectedFunctionNames);
	this->FunctionListWidget->OnFunctionCheckboxChanged.BindRaw(this, &SFunctionPropertyListSwitcher::NotifyCheckedFunctionChanged);
	return this->FunctionListWidget.ToSharedRef();
}

TSharedRef<SPropertySelectionList> SFunctionPropertyListSwitcher::ConstructPropertyList()
{
	this->PropertyListWidget = SNew(SPropertySelectionList)
		.SingleChoiceMode(this->bSingleChoiceMode)
		.TargetStruct(this->TargetStruct.Get());
	this->PropertyListWidget->OnPropertySelectionChanged.BindRaw(this, &SFunctionPropertyListSwitcher::NotifyCheckedPropertyChanged);
	return this->PropertyListWidget.ToSharedRef();
}

TSharedRef<SWidgetSwitcher> SFunctionPropertyListSwitcher::ConstructWidgetSwitcher()
{
	this->Switcher = SNew(SWidgetSwitcher);
	return this->Switcher.ToSharedRef();
}


void SFunctionPropertyListSwitcher::NotifyCheckedFunctionChanged(UFunction* function, bool bIsChecked)
{
	if (bIsChecked && this->bSingleChoiceMode)
	{
		this->PropertyListWidget->ClearAllChecks();	
	}
	this->OnFunctionCheckboxChanged.ExecuteIfBound(function, bIsChecked);
}

void SFunctionPropertyListSwitcher::NotifyCheckedPropertyChanged(FProperty* property, bool bIsChecked)
{
	if (bIsChecked && this->bSingleChoiceMode)
	{
		this->FunctionListWidget->ClearAllChecks();
	}
	this->OnPropertyCheckboxChanged.ExecuteIfBound(property, bIsChecked);
}

void SFunctionPropertyListSwitcher::ClearAllChecked() const
{
	this->FunctionListWidget->ClearAllChecks();
	this->PropertyListWidget->ClearAllChecks();
}

UFunction* SFunctionPropertyListSwitcher::FindFunctionByString(const FString& funcName)
{
	return this->FunctionListWidget->FindFunctionByString(funcName);
}


END_SLATE_FUNCTION_BUILD_OPTIMIZATION
