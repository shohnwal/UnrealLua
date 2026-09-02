// Fill out your copyright notice in the Description page of Project Settings.


#include "UIWidgets/IntelliSense/SLuaIntelliSenseWidget.h"

#include "SlateOptMacros.h"
#include "Framework/Application/SlateApplication.h"
#include "Utility/WidgetStyles.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SLuaIntelliSenseWidget::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBox)
		.MaxDesiredWidth(700)
		.MaxDesiredHeight(300)
		[
			SNew(SBorder)
			.BorderBackgroundColor(UnrealLuaTools::SlateStyles::GetEditableTextBoxStyle()->ReadOnlyForegroundColor)
			.Padding(1)
			.Content()
			[
				SAssignNew(OptionsListView, SListView<TSharedPtr<FLuaIntellisenseItemWidget>>)
				.ListItemsSource(&OptionsList)
				.OnGenerateRow(this, &SLuaIntelliSenseWidget::OnGenerateTableRow )
				.Orientation(Orient_Vertical)
				.OnItemsRebuilt_Raw(this, &SLuaIntelliSenseWidget::OnListRebuilt)
			]
		]
	];
	this->ShowPropertiesOfStruct(nullptr);
}

TSharedRef<ITableRow> SLuaIntelliSenseWidget::OnGenerateTableRow(TSharedPtr<FLuaIntellisenseItemWidget> option, const TSharedRef<STableViewBase>& TableViewBase)
{
	FListRow GenerateRow = GenerateRowForObject(option);
	return
		SNew( STableRow< TSharedPtr<FString> >, TableViewBase )
		[
			GenerateRow.Widget
		];
}

SLuaIntelliSenseWidget::FListRow SLuaIntelliSenseWidget::GenerateRowForObject(TSharedPtr<FLuaIntellisenseItemWidget> option)
{
	return
		FListRow(
			SNew(SBox)
			.WidthOverride(600)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				.HAlign(HAlign_Fill)
				.Padding(2,1,30,1)
				[
					SNew(STextBlock)
					.Text(FText::AsCultureInvariant(option->PropertyName))
					.ToolTip(FSlateApplication::Get().MakeToolTip(FText::AsCultureInvariant(option->PropertyName)))
					.OnDoubleClicked_Lambda([this, option](	const FGeometry&, const FPointerEvent&)
					{
						this->NotifyListItemSelected(option);
						return FReply::Handled();
					})
				]
				+ SHorizontalBox::Slot()
				.Padding(0,1,2,1)
				.AutoWidth()
				.HAlign(HAlign_Right)
				[
					SNew(STextBlock)
					.Text(FText::AsCultureInvariant(*option->PropertyType))
					.Justification(ETextJustify::Right)
				]
			]
		);
}


bool SLuaIntelliSenseWidget::ShowPropertiesOfStruct(UStruct* ustruct)
{
	this->Struct.Reset(ustruct);
	this->AllOptionsList.Empty();
	this->OptionsList.Empty();
	this->CurrentlySelectedIndex = 0;
	if (this->Struct.IsValid())
	{
		for (TFieldIterator<FProperty> propIt(this->Struct.Get()); propIt; ++propIt)
		{
			FProperty* prop = *propIt;
			if (prop->IsEditorOnlyProperty())
			{
				continue;
			}
			auto item =	MakeShared<FLuaIntellisenseItemWidget>(prop->GetName(), prop->GetCPPType());
			this->AllOptionsList.Emplace(item);
		}	
	}
	this->OptionsList = this->AllOptionsList;
	this->OptionsListView->RequestListRefresh();
	return !this->OptionsList.IsEmpty();
}

bool SLuaIntelliSenseWidget::ShowUFunctionsOfUClass(UClass* ustruct)
{
	this->Struct.Reset(ustruct);
	this->AllOptionsList.Empty();
	this->OptionsList.Empty();
	this->CurrentlySelectedIndex = 0;
	if (this->Struct.IsValid())
	{
		for (TFieldIterator<UFunction> propIt(this->Struct.Get()); propIt; ++propIt)
		{
			UFunction* func = *propIt;
			if (func->IsEditorOnly())
			{
				continue;
			}
			auto item = MakeShared<FLuaIntellisenseItemWidget>(func->GetName(), "UFunction");
			this->AllOptionsList.Emplace(item);
		}	
	}
	this->OptionsList = this->AllOptionsList;
	this->OptionsListView->RequestListRefresh();
	return !this->OptionsList.IsEmpty();
}

bool SLuaIntelliSenseWidget::ShowImportOptions(UStruct* baseStruct)
{
	this->Struct.Reset(baseStruct);
	this->OptionsList.Empty();
	this->CurrentlySelectedIndex = 0;
	return !this->OptionsList.IsEmpty();
}

void SLuaIntelliSenseWidget::MoveSelectionUp()
{
	if (this->OptionsList.IsEmpty())
	{
		return;
	}
	verify(this->OptionsList.IsValidIndex(this->CurrentlySelectedIndex))
	auto item = this->OptionsList[this->CurrentlySelectedIndex];
	this->OptionsListView->SetItemHighlighted(item, false);
	this->CurrentlySelectedIndex = FMath::Clamp(this->CurrentlySelectedIndex - 1, 0, this->OptionsList.Num() - 1);
	this->OptionsListView->SetItemHighlighted(this->OptionsList[this->CurrentlySelectedIndex], true);
	this->OptionsListView->RequestScrollIntoView(this->OptionsList[this->CurrentlySelectedIndex]);
}

void SLuaIntelliSenseWidget::MoveSelectionDown()
{
	if (this->OptionsList.IsEmpty())
	{
		return;
	}
	verify(this->OptionsList.IsValidIndex(this->CurrentlySelectedIndex))
	auto item = this->OptionsList[this->CurrentlySelectedIndex];
	this->OptionsListView->SetItemHighlighted(item, false);
	this->CurrentlySelectedIndex = FMath::Clamp(this->CurrentlySelectedIndex + 1, 0, this->OptionsList.Num() - 1);
	this->OptionsListView->SetItemHighlighted(this->OptionsList[this->CurrentlySelectedIndex], true);
	this->OptionsListView->RequestScrollIntoView(this->OptionsList[this->CurrentlySelectedIndex]);
}

bool SLuaIntelliSenseWidget::FilterResults(const FString& searchString)
{
	if (searchString.Equals(this->CurrentFilter))
	{
		return !this->OptionsList.IsEmpty();
	}
	this->CurrentFilter = searchString;
	
	UE_LOG(LogTemp, Log, TEXT("New Filter: %s"), *searchString);
	this->OptionsList = this->AllOptionsList;
	
	if (searchString.IsEmpty())
	{
		return !this->OptionsList.IsEmpty();
	}
	
	this->OptionsList.RemoveAll([&searchString](const TSharedPtr<FLuaIntellisenseItemWidget>& item)
	{
		return !item->PropertyName.Contains(searchString);
	});
	this->OptionsListView->RequestListRefresh();
	this->CurrentlySelectedIndex = 0;
	return !this->OptionsList.IsEmpty();
}

FString SLuaIntelliSenseWidget::GetCurrentlySelectedItem()
{
	if (this->OptionsList.IsEmpty())
	{
		return FString();
	}
	else
	{
		verify(this->OptionsList.IsValidIndex(this->CurrentlySelectedIndex));
		return *this->OptionsList[this->CurrentlySelectedIndex].Get()->PropertyName;
	}
}

void SLuaIntelliSenseWidget::Reset()
{
}

void SLuaIntelliSenseWidget::NotifyListItemSelected(const TSharedPtr<FLuaIntellisenseItemWidget>& option)
{
	
}

void SLuaIntelliSenseWidget::OnListRebuilt()
{
	if (this->OptionsList.IsEmpty())
	{
		return;
	}
	else
	{
		this->OptionsListView->SetItemHighlighted(0, true);
	}
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
