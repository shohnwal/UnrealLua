// Fill out your copyright notice in the Description page of Project Settings.


#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptBoxSubEditorImportPrompt.h"

#include "SlateOptMacros.h"
#include "UnrealEngine.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptBoxSubEditorNewObjectBase.h"
#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptEditorTextBox.h"
#include "Utility/LuaLogMacros.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SComboBox.h"

#define LOCTEXT_NAMESPACE "UnrealLuaTools"
BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SLuaScriptBoxSubEditorImportPrompt::Construct(const FArguments& InArgs)
{
	this->ImportFilter = InArgs._ImportFilter;
	this->ImportFilter = ELuaImportFilter::UClass;
	
	SLuaScriptBoxSubEditor::Construct(SLuaScriptBoxSubEditor::FArguments()
	.OnSubEditorCommit(InArgs._OnSubEditorCommit)
	.OnCancelEdit(InArgs._OnCancelEdit)
	);
	
	TypeOptions.Add(MakeShareable( new FString("UObject")));
	TypeOptions.Add(MakeShareable( new FString("Struct")));
	TypeOptions.Add(MakeShareable( new FString("Enum")));
	CurrentlySelectedType = TypeOptions[0];
	this->ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.HAlign(HAlign_Center)
		.AutoHeight()
		.Padding(0,0,0,10)
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant("Create Lua Import for Blueprint asset"))
			.Justification(ETextJustify::Center)
		]
		+ SVerticalBox::Slot()
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		.FillHeight(1)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			[
				SAssignNew(TypeOptionsComboBox, SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&TypeOptions)
				.InitiallySelectedItem(CurrentlySelectedType)
				.OnSelectionChanged(this, &SLuaScriptBoxSubEditorImportPrompt::OnSelectionChanged)
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> item)
				{
					return SNew(STextBlock).Text(FText::AsCultureInvariant(*item));
				})
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						return FText::AsCultureInvariant(*this->CurrentlySelectedType);
					})
				]
			]
			.HAlign(HAlign_Center)
			.AutoHeight()
			+SVerticalBox::Slot()
			[
				SNew(SBox)
				.WidthOverride(200)
				.Content()
				[
					ConstructSearchList()
				]
				.VAlign(VAlign_Fill)
				.HAlign(HAlign_Fill)
			]
			.AutoHeight()
			.FillHeight(1)
			.VAlign(VAlign_Fill)
			.HAlign(HAlign_Fill)
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Bottom)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.FillWidth(1)
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
					this->Commit();
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.AutoWidth()
			.FillWidth(1)
			.Padding(10,10,10,10)
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
		]
	];
	
	RebuildObjectList();
	RefilterObjectList();
}

auto DoSomething(int x, int y = 123, int z = 456) -> int;

TSharedRef<SVerticalBox> SLuaScriptBoxSubEditorImportPrompt::ConstructSearchList()
{
	TSharedRef<SVerticalBox> searchlist = SNew(SVerticalBox)
	// The filter line
	+SVerticalBox::Slot()
	.VAlign(VAlign_Top)
	.HAlign(HAlign_Center)
	.AutoHeight()
	[
		SNew(SBox)
		.WidthOverride(300)
		.Content()
		[
			SAssignNew(FilterTextBoxWidget, SSearchBox)
			.ToolTipText( LOCTEXT("SearchBox_ToolTip", "Type words to search for") )
			.OnTextChanged( this, &SLuaScriptBoxSubEditorImportPrompt::OnFilterTextChanged )
			//.DelayChangeNotificationsWhileTyping(true)	
		]
	]
	+SVerticalBox::Slot()
	.FillHeight(1)
	.Padding(2.0f)
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Center)
	[
		SAssignNew(WidgetSwitcher, SWidgetSwitcher)
		+ SWidgetSwitcher::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant("Use search text box to select an asset to import"))
			.Justification(ETextJustify::Center)
		]
		+SWidgetSwitcher::Slot()
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			SNew(SBox)
			[
				SAssignNew(SearchListWidget, SListView<FName>)
				.ListItemsSource(&FilteredObjectList)
				.OnGenerateRow(this, &SLuaScriptBoxSubEditorImportPrompt::OnGenerateTableRow )
				.Orientation(Orient_Vertical)
			]
			.VAlign(VAlign_Fill)
			.HAlign(HAlign_Fill)
			.WidthOverride(600)
		]
		+SWidgetSwitcher::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("local"))
			]
			.AutoWidth()
			.VAlign(VAlign_Center)
			+ SHorizontalBox::Slot()
			[
				SNew(SBox)
				[
					SAssignNew(ImportNameText, SEditableTextBox)
				]
				.MinDesiredWidth(100)
			]
			.Padding(2,0,2,0)
			.AutoWidth()
			.VAlign(VAlign_Center)
			+ SHorizontalBox::Slot()
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("="))
			]
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0,0,2,0)
			+SHorizontalBox::Slot()
			[
				SNew(SBox)
				[
					SNew(SBorder)
					[
						SAssignNew(ImportStringText, STextBlock)
						.Text(FText::GetEmpty())
					]
					.VAlign(VAlign_Center)
				]	
				.MinDesiredWidth(200)
				.VAlign(VAlign_Center)
			]
			.AutoWidth()
			.Padding(5)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Fill)	
		]
	];
	
	this->WidgetSwitcher->SetActiveWidgetIndex(0);
	
	return searchlist;
}


void SLuaScriptBoxSubEditorImportPrompt::RebuildObjectList()
{
	LoadedObjectList.Empty();
	this->AllAssetData.Empty();
	
	FARFilter Filter;
	Filter.PackagePaths.Add("/Game"); // Restrict to your content directory
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	Filter.WithoutPackageFlags = PKG_UncookedOnly | PKG_EditorOnly | PKG_Developer;

	TArray<FAssetData> StructAssets;
	
	if (this->ImportFilter == ELuaImportFilter::UScriptStruct)
	{
		Filter.ClassPaths.Add(UScriptStruct::StaticClass()->GetClassPathName());
	}
	else if (this->ImportFilter == ELuaImportFilter::UClass)
	{
		Filter.ClassPaths.Add(UObject::StaticClass()->GetClassPathName());
	}
	else
	{
		verify(this->ImportFilter == ELuaImportFilter::UEnum)
		Filter.ClassPaths.Add(UEnum::StaticClass()->GetClassPathName());
	}
	
	IAssetRegistry::Get()->GetAssets(Filter, StructAssets);

	for (const FAssetData& AssetData : StructAssets)
	{
		this->AllAssetData.Emplace(AssetData.GetFullName(), AssetData.ToSoftObjectPath());
		LoadedObjectList.Add(*AssetData.GetFullName());
	}
}



TSharedRef<ITableRow> SLuaScriptBoxSubEditorImportPrompt::OnGenerateTableRow(FName InData, const TSharedRef<STableViewBase>& OwnerTable)
{
	FListRow GenerateRow = GenerateRowForObject(InData);
	return
		SNew( STableRow< FName >, OwnerTable )
		[
			GenerateRow.Widget
		];
}


SLuaScriptBoxSubEditorImportPrompt::FListRow SLuaScriptBoxSubEditorImportPrompt::GenerateRowForObject(FName InData)
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

void SLuaScriptBoxSubEditorImportPrompt::NotifyListItemSelected(FName selected)
{
	FSoftObjectPath& path = this->AllAssetData.FindChecked(selected);
	LUA_LOG("%s", *path.ToString())
	
	FStringBuilderBase importPath;
	
	if (this->ImportFilter == ELuaImportFilter::UScriptStruct)
	{
		importPath << "import \"" + path.ToString() << "\"";	
	}
	else if (this->ImportFilter == ELuaImportFilter::UClass)
	{
		 FString pathString = path.ToString();
		importPath = "import \"" + pathString;
		if (!pathString.EndsWith("_C"))
		{
			importPath.Append("_C");
		}
		importPath << "\"";
	}
	else if (this->ImportFilter == ELuaImportFilter::UEnum)
	{
		importPath << "import \"" + path.ToString() << "\"";	
	}

	this->ImportStringText->SetText(FText::AsCultureInvariant(importPath));
	this->ImportNameText->SetText(FText::AsCultureInvariant(path.GetAssetName()));
	this->WidgetSwitcher->SetActiveWidgetIndex(2);
}


void SLuaScriptBoxSubEditorImportPrompt::NotifyImportStructNameChanged(const FText& importName)
{
	//this->RebuildOutput();
}

void SLuaScriptBoxSubEditorImportPrompt::Commit()
{
	FStringBuilderBase importPath;
	if (!this->ImportNameText->GetText().IsEmpty())
	{
		importPath << "local " << this->ImportNameText->GetText().ToString() << " = ";
	}
	importPath << this->ImportStringText->GetText().ToString();
	FSubEditorCommitData commitData{nullptr, importPath.ToString()};
	this->CommmitEditing(commitData);
}

void SLuaScriptBoxSubEditorImportPrompt::InsertTextAtCursor(const FString& Text)
{
	
}

void SLuaScriptBoxSubEditorImportPrompt::NotifyCommitFromSubEditor(const FSubEditorCommitData& data)
{
	
}

TSharedPtr<SLuaScriptEditorTextBox> SLuaScriptBoxSubEditorImportPrompt::GetLuaScriptEditorTextBox()
{
	return nullptr;
}

void SLuaScriptBoxSubEditorImportPrompt::OnSelectionChanged(TSharedPtr<FString> option, ESelectInfo::Type selecitonType)
{
	this->CurrentlySelectedType = option;
	this->TypeOptionsComboBox->SetSelectedItem(this->CurrentlySelectedType);
	
	if (*option == "UObject")
	{
		this->ImportFilter = ELuaImportFilter::UClass;
	}
	else if (*option == "Struct")
	{
		this->ImportFilter = ELuaImportFilter::UScriptStruct;
	}
	else
	{
		this->ImportFilter = ELuaImportFilter::UEnum;
	}
	RebuildObjectList();
	
	this->FilterTextBoxWidget->SetText(FText::GetEmpty());

	this->OnFilterTextChanged(FText::GetEmpty());
}

void SLuaScriptBoxSubEditorImportPrompt::OnFilterTextChanged(const FText& Text)
{
	if (Text.IsEmpty())
	{
		this->WidgetSwitcher->SetActiveWidgetIndex(0);
		this->ImportStringText->SetText(FText::GetEmpty());
		this->ImportNameText->SetText(FText::GetEmpty());		
	}
	else if (this->WidgetSwitcher->GetActiveWidgetIndex() != 1)
	{
		this->WidgetSwitcher->SetActiveWidgetIndex(1);
		this->ImportStringText->SetText(FText::GetEmpty());
		this->ImportNameText->SetText(FText::GetEmpty());
	}
	ReapplyFilter();
}


void SLuaScriptBoxSubEditorImportPrompt::ReapplyFilter()
{
	RefilterObjectList();

	if (SearchListWidget.IsValid())
	{
		SearchListWidget->RequestListRefresh();
	}
}


void SLuaScriptBoxSubEditorImportPrompt::RefilterObjectList()
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

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
#undef LOCTEXT_NAMESPACE