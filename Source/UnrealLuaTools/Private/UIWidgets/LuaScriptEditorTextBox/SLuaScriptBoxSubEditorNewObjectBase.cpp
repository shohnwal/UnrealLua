// Fill out your copyright notice in the Description page of Project Settings.

#include "SlateOptMacros.h"
#include "ContextMenu/LuaScriptEditorContextMenuBuilder.h"
#include "Framework/Application/SlateApplication.h"
#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptEditorTextBox.h"
#include "UIWidgets/SFunctionPropertyListSwitcher.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "UIWidgets/SPropertySelectionList.h"
#include "Utility/WidgetStyles.h"
#include "Widgets/Layout/SSpacer.h"
#include "UIWidgets/SMultiLineEditableTextBoxEx.h"
#include "Widgets/Text/SRichTextBlock.h"

#define LOCTEXT_NAMESPACE "UnrealLuaTools"
BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SLuaScriptBoxSubEditorNewObjectBase::Construct(const FArguments& InArgs)
{
	this->SelectedStruct = {};
	this->Session = InArgs._Session;	
	SLuaScriptBoxSubEditor::Construct(SLuaScriptBoxSubEditor::FArguments()
		.OnSubEditorCommit(InArgs._OnSubEditorCommit)
		.OnCancelEdit(InArgs._OnCancelEdit)
	);
	
	UnrealLuaTools::SlateStyles::GetOnStyleChangedDelegate().AddSPLambda(this, [this]()
	{
		this->LuaScriptEditorTextBox->SetStyle(UnrealLuaTools::SlateStyles::GetEditableTextBoxStyle());
	});
	this->ChildSlot
	[
		SAssignNew(WindowBodyVBox, SVerticalBox)
		+ SVerticalBox::Slot()
		[
			SAssignNew(TopPartInfoHeader, SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(SBorder)
				[
					SAssignNew(SelectedStructText, STextBlock)
					.Text(FText::FromString("Create new object"))					
				]
			]
			.AutoWidth()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(10,0,10,0)
		]
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.AutoHeight()
		.Padding(10,10,10,10)
		+SVerticalBox::Slot()
		[
			SAssignNew(ImportSection, SHorizontalBox)
			.Visibility(EVisibility::Collapsed)
			+ SHorizontalBox::Slot()
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("Blueprint Asset requires import to be used in this file!"))
				.ColorAndOpacity(FLinearColor::Yellow)
				.ToolTipText(FText::AsCultureInvariant("The chosen item is a Blueprint asset.\nBlueprint assets are not automatically loaded into Lua\nand needs to be imported before it can be used.\nEnter a variable name this type should be used as in this script file.\n"))
				.Justification(ETextJustify::Center)
			]
			.AutoWidth()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(0,0,10,0)
			+ SHorizontalBox::Slot()
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("Variable name:"))
				.Justification(ETextJustify::Center)					
			]
			.AutoWidth()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(0,0,2,0)
			+ SHorizontalBox::Slot()
			[
				SAssignNew(ImportNameTextBox, SEditableTextBox)
				.OnTextChanged_Raw(this, &SLuaScriptBoxSubEditorNewObjectBase::NotifyImportStructNameChanged)
			]
			.AutoWidth()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			.FillWidth(1)
		]
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.AutoHeight()
		.Padding(10,0,10,10)
		+ SVerticalBox::Slot()
		[
			SAssignNew(CenterHBox, SSplitter)
			.Orientation(EOrientation::Orient_Horizontal)
			+ SSplitter::Slot()
			.MinSize(200)
			.Value(0.8f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				[
					SAssignNew(LuaScriptEditorTextBox, SLuaScriptEditorTextBox)
					.Session(InArgs._Session)
					.Text(FText::GetEmpty())
					.IsReadOnly(false)
					.Style(UnrealLuaTools::SlateStyles::GetEditableTextBoxStyle())
					.AllowContextMenu(true)
					.OnKeyDownHandler_Raw(this, &SLuaScriptBoxSubEditorNewObjectBase::HandleEditTextBoxKeyDown)
					.OnNewChildEditorCreated(this, &SLuaScriptBoxSubEditorNewObjectBase::AddNewChildObjectEditor)
				]
				.FillHeight(1)
				.VAlign(VAlign_Fill)
				.HAlign(HAlign_Fill)
				+SVerticalBox::Slot()
				[
					SAssignNew(ButtonsBorder, SBorder)
				]
				.VAlign(VAlign_Bottom)
				.HAlign(HAlign_Fill)
				.AutoHeight()
			]
		]
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		//Hide bottom part for now, as we don't need it 
		+SVerticalBox::Slot()
		[
			SAssignNew(BottomPartBorder, SSpacer)
			.Size(FVector2D{1.f, 1.f})
			.Visibility(EVisibility::Collapsed)
		]
		.VAlign(VAlign_Bottom)
		.AutoHeight()
	];
	
	RebuildObjectList();
	RefilterObjectList();
	
	this->CenterHBox->AddSlot()
	.MinSize(200)
	.Value(0.2f)
	[
		ConstructSearchList()
	];
	
	this->ButtonsBorder->SetContent(
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
				FSubEditorCommitData commitData {this->SelectedStruct.Get(), this->LuaScriptEditorTextBox->GetText().ToString()};
				this->OnCommit.ExecuteIfBound(commitData);	
				return FReply::Handled();;
			})
		]
		.AutoWidth()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.FillWidth(1)
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
				this->CancelEditing();
				return FReply::Handled();
			})
		]
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.AutoWidth()
		.FillWidth(1)
		.Padding(10,10,10,10)
	);
	
	this->RightSideWidgetSwitcher->SetActiveWidgetIndex(0);
	verify(this->RightSideWidgetSwitcher->GetActiveWidgetIndex() == 0);
	verify(this->RightSideWidgetSwitcher->GetNumWidgets() == 2);
}

void SLuaScriptBoxSubEditorNewObjectBase::InsertTextAtCursor(const FString& text)
{
	this->LuaScriptEditorTextBox->InsertTextAtCursor(text);
}

TSharedRef<SVerticalBox> SLuaScriptBoxSubEditorNewObjectBase::ConstructSearchList()
{
	TSharedRef<SVerticalBox> searchlist = SNew(SVerticalBox)
	// The filter line
	+SVerticalBox::Slot()
	.AutoHeight()
	[
		SNew(SHorizontalBox)
		// Filter text box
		+SHorizontalBox::Slot()
		.FillWidth(1)
		[
			SAssignNew(FilterTextBoxWidget, SSearchBox)
				.ToolTipText( LOCTEXT("SearchBox_ToolTip", "Type words to search for") )
				.OnTextChanged( this, &SLuaScriptBoxSubEditorNewObjectBase::OnFilterTextChanged )
				//.DelayChangeNotificationsWhileTyping(true)
		]

		// Refresh button (rescans for newly loaded objects; then reruns the filter on the new list)
		//hidden for now (coillapsed)
		+SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.ToolTipText( LOCTEXT("Refresh_ToolTip", "Search for new entries") )
			.OnClicked( this, &SLuaScriptBoxSubEditorNewObjectBase::OnRefreshButtonClicked )
			[
				SNew(SImage)
					.Image( FAppStyle::GetBrush(TEXT("AnimEditor.RefreshButton")) )
			]
			.Visibility(EVisibility::Collapsed)
		]
	]

	//switcher Searchlist <-> property/ufunction list
	+SVerticalBox::Slot()
	.FillHeight(1)
	.Padding(2.0f)
	[
		SAssignNew(RightSideWidgetSwitcher, SWidgetSwitcher)
	]
	.VAlign(VAlign_Fill);
	
	TSharedPtr<SVerticalBox> PropertyListVBox = nullptr;
	
	// property/ufunction list
	this->RightSideWidgetSwitcher->AddSlot()
	.Padding(2.0f)
	.VAlign(VAlign_Fill)
	[
		SAssignNew(PropertyListBorder, SBox)
		[
			SAssignNew(PropertyListVBox, SVerticalBox)
			+SVerticalBox::Slot()
			[
				SAssignNew(PropertyListWidget, SPropertySelectionList)
				.TargetStruct(this->SelectedStruct.Get())
				.PreselectedProperties({})
				.PreselectedPropertyNames({})
				.SingleChoiceMode(false)
			]
			+ SVerticalBox::Slot()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				[
					SAssignNew(AsMultilineCheckboxWidget, SCheckBox)
					.IsChecked(ECheckBoxState::Unchecked)
					.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { this->RebuildTextboxContent(); })
				]
				.AutoWidth()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Left)

				+ SHorizontalBox::Slot()
				[
					SNew(STextBlock)
					.Text(FText::AsCultureInvariant("Multiline Properties"))
				]
				.AutoWidth()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Fill)
			]
			.VAlign(VAlign_Bottom)
			.AutoHeight()
			.Padding(4, 10, 4, 0)
		]
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		.MaxDesiredWidth(200)
	];
	this->PropertyListWidget->SetOrientation(Orient_Vertical);
    this->PropertyListWidget->OnPropertySelectionChanged.BindRaw(this, &SLuaScriptBoxSubEditorNewObjectBase::NotifyPropertyCheckboxChanged);

	//Search list

	this->RightSideWidgetSwitcher->AddSlot()
	[
		SAssignNew(SearchListBorder, SBox)
		[
			SAssignNew(SearchListWidget, SListView<FName>)
			.ListItemsSource(&FilteredObjectList)
			.OnGenerateRow(this, &SLuaScriptBoxSubEditorNewObjectBase::OnGenerateTableRow )
			.Orientation(Orient_Vertical)
		]
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		.WidthOverride(300)
	];
	
	return searchlist;
}

FReply SLuaScriptBoxSubEditorNewObjectBase::OnRefreshButtonClicked()
{
	if (this->SelectedStruct.IsValid())
	{
		this->RightSideWidgetSwitcher->SetActiveWidget(this->PropertyListBorder.ToSharedRef());
	}
	//RebuildObjectList();
	//ReapplyFilter();
	return FReply::Handled();
}

void SLuaScriptBoxSubEditorNewObjectBase::OnFilterTextChanged( const FText& InFilterText )
{
	this->SetSelectedStruct(NAME_None);
	
	if (this->RightSideWidgetSwitcher->GetActiveWidget() != this->SearchListBorder)
	{
		this->RightSideWidgetSwitcher->SetActiveWidget(this->SearchListBorder.ToSharedRef());
	}

	ReapplyFilter();
}

void SLuaScriptBoxSubEditorNewObjectBase::ReapplyFilter()
{
	RefilterObjectList();

	if (SearchListWidget.IsValid())
	{
		SearchListWidget->RequestListRefresh();
	}
}

EVisibility SLuaScriptBoxSubEditorNewObjectBase::GetFilterStatusVisibility() const
{
	return IsFilterActive() ? EVisibility::Visible : EVisibility::Collapsed;
}


FText SLuaScriptBoxSubEditorNewObjectBase::GetFilterStatusText() const
{
	return FText::Format(LOCTEXT("FilterStatus_ShowingXOfYFmt", "Showing {0} of {1}"), FText::AsNumber(FilteredObjectList.Num()), FText::AsNumber(LoadedObjectList.Num()));
}

bool SLuaScriptBoxSubEditorNewObjectBase::IsFilterActive() const
{
	return FilteredObjectList.Num() != LoadedObjectList.Num();
}

FString SLuaScriptBoxSubEditorNewObjectBase::GetSearchableText(UObject* Object)
{
	return Object->GetName();
}


void SLuaScriptBoxSubEditorNewObjectBase::RefilterObjectList()
{
	// Tokenize the search box text into a set of terms; all of them must be present to pass the filter
	TArray<FString> FilterTerms{};
	if (FilterTextBoxWidget.IsValid())
	{
		FilterTextBoxWidget->GetText().ToString().ParseIntoArray(FilterTerms, TEXT(" "), true);
	}

	if (FilterTerms.Num())
	{
		FilteredObjectList.Empty();

		// Run thru each item in the list, checking it against the text filter
		//for (int32 ObjectIndex = 0; ObjectIndex < LoadedObjectList.Num(); ++ObjectIndex)
		for (auto& pair : this->AllAssetData)
		{
			//FMinimalAssetData& TestObject = *LoadedObjectList[ObjectIndex].Get();
			FName key = pair.Key;
			FSoftObjectPath& data = pair.Value;

			FString SearchText = data.GetAssetName(); //GetSearchableText(TestObject);

			bool bMatchesAllTerms = true;
			for (int32 FilterIndex = 0; (FilterIndex < FilterTerms.Num()) && bMatchesAllTerms; ++FilterIndex)
			{
				const bool bMatchesTerm = SearchText.Contains(FilterTerms[FilterIndex]);
				bMatchesAllTerms = bMatchesAllTerms && bMatchesTerm;
			}

			if (bMatchesAllTerms)
			{
				FilteredObjectList.Add(key);
			}
		}
	}
	else
	{
		// Nothing to filter, just copy the list
		//FilteredObjectList = LoadedObjectList; 
		FilteredObjectList = {}; 
	}
}


void SLuaScriptBoxSubEditorNewObjectBase::UpdateSectionVisibility(UStruct* ustruct)
{
	if (ustruct)
	{
		//show property list
		this->RightSideWidgetSwitcher->SetActiveWidget(this->PropertyListBorder.ToSharedRef());
		this->PropertyListWidget->SetViewedStruct(ustruct, {}, {});
	}
	else
	{
		//show search list again
		this->RightSideWidgetSwitcher->SetActiveWidget(this->SearchListBorder.ToSharedRef());
		this->PropertyListWidget->SetViewedStruct(nullptr, {}, {});
	}
	
	if (ustruct && !ustruct->IsNative())
	{
		this->SetImportSectionEnabled(true);
		this->ImportNameTextBox->SetText(FText::AsCultureInvariant(ustruct->GetAuthoredName()));
	}
	else
	{
		this->SetImportSectionEnabled(false);
	}
}

TSharedPtr<SLuaScriptEditorTextBox> SLuaScriptBoxSubEditorNewObjectBase::GetLuaScriptEditorTextBox()
{
	return this->LuaScriptEditorTextBox;
}

void SLuaScriptBoxSubEditorNewObjectBase::NotifyCommitFromSubEditor(const FSubEditorCommitData& commitData)
{
	this->InsertTextAtCursor(commitData.CommitText);
}

FReply SLuaScriptBoxSubEditorNewObjectBase::HandleEditTextBoxKeyDown(const FGeometry& geometry, const FKeyEvent& KeyEvent)
{
	if (KeyEvent.GetKey() == EKeys::Tab)
	{
		TSharedPtr<SLuaScriptEditorTextBox> currentText = this->LuaScriptEditorTextBox;
		if (!KeyEvent.GetModifierKeys().AnyModifiersDown())
		{
			currentText->InsertTextAtCursor(FString{"\t"});
			return FReply::Handled();
		}
		else if (KeyEvent.GetModifierKeys().IsShiftDown())
		{
			FTextSelection selection = currentText->GetTextSelection();
			FTextLocation currentLoc = currentText->GetCursorLocation();
			int32 line = currentLoc.GetLineIndex();
			int32 offset = currentLoc.GetOffset();
			bool handled = false;
			FString currentLine{};
			currentText->GetCurrentTextLine(currentLine);
			if (currentLine.Len() > 0)
			{
				if (offset > 0)
				{
					if (currentLine[offset] == '\t' || currentLine[offset] == ' ')
					{
						currentLine.RemoveAt(offset);
						handled = true;
					}
				}
				if (!handled)
				{
					if (currentLine[0] == '\t' || currentLine[0] == ' ')
					{
						currentLine.RemoveAt(0);
						handled = true;
					}
				}
			}
			//currentText->GoTo(ETextLocation::BeginningOfLine);
			return FReply::Handled();
		}
	}
	return FReply::Unhandled();
}

void SLuaScriptBoxSubEditorNewObjectBase::AddNewChildObjectEditor(TSharedRef<SLuaScriptBoxSubEditor> newEditorObject)
{
	this->OnRequestAddNewChildEditor.ExecuteIfBound(newEditorObject);
}

TSharedRef<ITableRow> SLuaScriptBoxSubEditorNewObjectBase::OnGenerateTableRow(FName InData, const TSharedRef<STableViewBase>& OwnerTable)
{
	FListRow GenerateRow = GenerateRowForObject(InData);
	return
		SNew( STableRow< FName >, OwnerTable )
		[
			GenerateRow.Widget
		];
}

SLuaScriptBoxSubEditorNewObjectBase::FListRow SLuaScriptBoxSubEditorNewObjectBase::GenerateRowForObject(FName InData)
{
	FSoftObjectPath& path = this->AllAssetData.FindChecked(InData);
	return
		FListRow(
			SNew(SButton)
			[
				SNew(STextBlock)
				.Text(FText::FromString(path.GetAssetName()))
				.ToolTip(FSlateApplication::Get().MakeToolTip(FText::AsCultureInvariant(InData.ToString())))
								
			].OnClicked_Lambda([this, InData]() 
			{
				this->NotifyListItemSelected(InData);
				return FReply::Handled();
			})
		);
}

void SLuaScriptBoxSubEditorNewObjectBase::SetImportSectionEnabled(bool enabled)
{
	if (!enabled)
	{
		this->ImportNameTextBox->SetText(FText::GetEmpty());
		//this->GenerateImportLineCheckbox->SetIsChecked(ECheckBoxState::Unchecked);
	}
	this->ImportSection->SetEnabled(enabled);
	this->ImportSection->SetVisibility(enabled ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed);
}

void SLuaScriptBoxSubEditorNewObjectBase::NotifyListItemSelected(FName selected)
{
	this->SetSelectedStruct(selected);
}

void SLuaScriptBoxSubEditorNewObjectBase::NotifyPropertyCheckboxChanged(FProperty* Property, bool bIsChecked)
{
	this->RebuildTextboxContent();
}

void SLuaScriptBoxSubEditorNewObjectBase::NotifyImportStructNameChanged(const FText& importName)
{
	this->RebuildTextboxContent();
}


END_SLATE_FUNCTION_BUILD_OPTIMIZATION
#undef LOCTEXT_NAMESPACE