// Fill out your copyright notice in the Description page of Project Settings.


#include "UIWidgets/SSearchableStructComboBox.h"

#include "SlateOptMacros.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/Input/SEditableTextBox.h"

#define LOCTEXT_NAMESPACE "UnrealLuaTools"
BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION


void SSearchableStructComboBox::Construct(const FArguments& InArgs)
{
	check(InArgs._ComboBoxStyle);

	ItemStyle = InArgs._ItemStyle;
	MenuRowPadding = InArgs._ComboBoxStyle->MenuRowPadding;

	// Work out which values we should use based on whether we were given an override, or should use the style's version
	const FComboButtonStyle& OurComboButtonStyle = InArgs._ComboBoxStyle->ComboButtonStyle;
	const FButtonStyle* const OurButtonStyle = InArgs._ButtonStyle ? InArgs._ButtonStyle : &OurComboButtonStyle.ButtonStyle;

	this->OnComboBoxOpening = InArgs._OnComboBoxOpening;
	this->OnSelectionChanged = InArgs._OnSelectionChanged;
	this->bAlwaysSelectItem = InArgs._bAlwaysSelectItem;
	this->OnGenerateWidget = InArgs._OnGenerateWidget;

	//UScriptStruct* baseStruct = InArgs._BaseStruct ? InArgs._BaseStruct : UScriptStruct::();
	
	this->BaseStruct.Reset(nullptr);
	
	CustomScrollbar = InArgs._CustomScrollbar;

	RebuildObjectList();
	
	TAttribute<EVisibility> SearchVisibility = InArgs._SearchVisibility;
	const EVisibility CurrentSearchVisibility = SearchVisibility.Get();

	TSharedRef<SWidget> ComboBoxMenuContent =
		SNew(SBox)
		.MaxDesiredHeight(InArgs._MaxListHeight)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(this->SearchField, SEditableTextBox)
				.HintText(LOCTEXT("Search", "Search"))
				.OnTextChanged(this, &SSearchableStructComboBox::OnSearchTextChanged)
				.OnTextCommitted(this, &SSearchableStructComboBox::OnSearchTextCommitted)
				.Visibility(SearchVisibility)
			]

			+ SVerticalBox::Slot()
			[
				SAssignNew(this->ComboListView, SListView<UScriptStruct*>)
				.ListItemsSource(&FilteredOptionsSource)
				.OnGenerateRow(this, &SSearchableStructComboBox::GenerateMenuItemRow)
				.OnSelectionChanged(this, &SSearchableStructComboBox::OnSelectionChanged_Internal)
				.OnKeyDownHandler(this, &SSearchableStructComboBox::OnKeyDownHandler)
				.SelectionMode(ESelectionMode::Single)
				.ExternalScrollbar(InArgs._CustomScrollbar)
			]
		];

	// Set up content
	TSharedPtr<SWidget> ButtonContent = InArgs._Content.Widget;
	if (InArgs._Content.Widget == SNullWidget::NullWidget)
	{
		SAssignNew(ButtonContent, STextBlock)
			.Text(NSLOCTEXT("SSearchableComboBox", "ContentWarning", "No Content Provided"))
			.ColorAndOpacity(FLinearColor::Red);
	}


	SComboButton::Construct(SComboButton::FArguments()
		.ComboButtonStyle(&OurComboButtonStyle)
		.ButtonStyle(OurButtonStyle)
		.Method(InArgs._Method)
		.ButtonContent()
		[
			ButtonContent.ToSharedRef()
		]
		.MenuContent()
		[
			ComboBoxMenuContent
		]
		.HasDownArrow(InArgs._HasDownArrow)
		.ContentPadding(InArgs._ContentPadding)
		.ForegroundColor(InArgs._ForegroundColor)
		.OnMenuOpenChanged(this, &SSearchableStructComboBox::OnMenuOpenChanged)
		.IsFocusable(true)
		);

	if (CurrentSearchVisibility == EVisibility::Visible)
	{
		SetMenuContentWidgetToFocus(SearchField);
	}
	else
	{
		SetMenuContentWidgetToFocus(ComboListView);
	}

	// Need to establish the selected item at point of construction so its available for querying
	// NB: If you need a selection to fire use SetItemSelection rather than setting an IntiallySelectedItem
	SelectedItem.Reset(nullptr);
	if (TListTypeTraits<UScriptStruct*>::IsPtrValid(SelectedItem.Get()))
	{
		ComboListView->Private_SetItemSelection(SelectedItem.Get(), true);
	}

}

void SSearchableStructComboBox::RebuildObjectList()
{
	LoadedObjectList.Empty();
	
	for (TObjectIterator<UScriptStruct> it; it; ++it)
	{
		if (it->IsChildOf(this->BaseStruct.Get()))
		{
			LoadedObjectList.Add(*it);
		}
	}
}

void SSearchableStructComboBox::ClearSelection()
{
	ComboListView->ClearSelection();
	SelectedItem = nullptr;
}

void SSearchableStructComboBox::SetSelectedItem(UScriptStruct* InSelectedItem, ESelectInfo::Type InSelectInfo)
{
	if (TListTypeTraits<UScriptStruct*>::IsPtrValid(InSelectedItem))
	{
		ComboListView->SetSelection(InSelectedItem, InSelectInfo);
	}
	else
	{
		ComboListView->SetSelection(SelectedItem.Get(), InSelectInfo);
	}
}

UScriptStruct* SSearchableStructComboBox::GetSelectedItem()
{
	return SelectedItem.Get();
}

void SSearchableStructComboBox::RefreshOptions()
{
	// Need to refresh filtered list whenever options change
	FilteredOptionsSource.Reset();
	
	
	TArray<FString> FilterTerms;
	SearchText.ToString().ParseIntoArray(FilterTerms, TEXT(" "), true);

	if (FilterTerms.IsEmpty())
	{
		// Nothing to filter, just copy the list
		FilteredOptionsSource = LoadedObjectList; 
	}
	else
	{
		// Run thru each item in the list, checking it against the text filter
		for (int32 ObjectIndex = 0; ObjectIndex < LoadedObjectList.Num(); ++ObjectIndex)
		{
			UScriptStruct* TestObject = LoadedObjectList[ObjectIndex];

			FString searchText = TestObject->GetName();

			bool bMatchesAllTerms = true;
			for (int32 FilterIndex = 0; (FilterIndex < FilterTerms.Num()) && bMatchesAllTerms; ++FilterIndex)
			{
				const bool bMatchesTerm = searchText.Contains(FilterTerms[FilterIndex]);
				bMatchesAllTerms = bMatchesAllTerms && bMatchesTerm;
			}

			if (bMatchesAllTerms)
			{
				FilteredOptionsSource.Add(TestObject);
			}
		}
	}

	ComboListView->RequestListRefresh();
}

TSharedRef<ITableRow> SSearchableStructComboBox::GenerateMenuItemRow(UScriptStruct* InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SComboRow<UScriptStruct*>, OwnerTable)
		.Style(ItemStyle)
		.Padding(MenuRowPadding)
		[
			SNew(STextBlock).Text(FText::FromString(*GetNameSafe(InItem)))
		];
}

void SSearchableStructComboBox::OnMenuOpenChanged(bool bOpen)
{
	if (bOpen == false)
	{
		if (TListTypeTraits<UScriptStruct*>::IsPtrValid(SelectedItem.Get()))
		{
			// Ensure the ListView selection is set back to the last committed selection
			ComboListView->SetSelection(SelectedItem.Get(), ESelectInfo::OnNavigation);
		}

		// Set focus back to ComboBox for users focusing the ListView that just closed
		FSlateApplication::Get().ForEachUser([this](FSlateUser& User) 
		{
			TSharedRef<SWidget> ThisRef = this->AsShared();
			if (User.IsWidgetInFocusPath(this->ComboListView))
			{
				User.SetFocus(ThisRef);
			}
		});

	}
}


void SSearchableStructComboBox::OnSelectionChanged_Internal(UScriptStruct* ProposedSelection, ESelectInfo::Type SelectInfo)
{
	if (!ProposedSelection)
	{
		return;
	}

	// Ensure that the proposed selection is different from selected
	if (ProposedSelection != SelectedItem.Get() || bAlwaysSelectItem)
	{
		SelectedItem.Reset(ProposedSelection);
		OnSelectionChanged.ExecuteIfBound(ProposedSelection);
	}

	// close combo as long as the selection wasn't from navigation
	if (SelectInfo != ESelectInfo::OnNavigation)
	{
		this->SetIsOpen(false);
	}
	else
	{
		ComboListView->RequestScrollIntoView(SelectedItem.Get(), 0);
	}
}

void SSearchableStructComboBox::OnSearchTextChanged(const FText& ChangedText)
{
	SearchText = ChangedText;

	RefreshOptions();
}

void SSearchableStructComboBox::OnSearchTextCommitted(const FText& InText, ETextCommit::Type InCommitType)
{
	if ((InCommitType == ETextCommit::Type::OnEnter) && FilteredOptionsSource.Num() > 0)
	{
		ComboListView->SetSelection(FilteredOptionsSource[0], ESelectInfo::OnKeyPress);
	}
}

FReply SSearchableStructComboBox::OnButtonClicked()
{
	// if user clicked to close the combo menu
	if (this->IsOpen())
	{
		// Re-select first selected item, just in case it was selected by navigation previously
		TArray<UScriptStruct*> SelectedItems = ComboListView->GetSelectedItems();
		if (SelectedItems.Num() > 0)
		{
			OnSelectionChanged_Internal(SelectedItems[0], ESelectInfo::Direct);
		}
	}
	else
	{
		SearchField->SetText(FText::GetEmpty());
		OnComboBoxOpening.ExecuteIfBound();
	}

	return SComboButton::OnButtonClicked();
}

FReply SSearchableStructComboBox::OnKeyDownHandler(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Enter)
	{
		// Select the first selected item on hitting enter
		TArray<UScriptStruct*> SelectedItems = ComboListView->GetSelectedItems();
		if (SelectedItems.Num() > 0)
		{
			OnSelectionChanged_Internal(SelectedItems[0], ESelectInfo::OnKeyPress);
			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}


END_SLATE_FUNCTION_BUILD_OPTIMIZATION
#undef LOCTEXT_NAMESPACE
