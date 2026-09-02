// Fill out your copyright notice in the Description page of Project Settings.


#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptBoxSubEditorNewDefaultScript.h"

#include "SlateOptMacros.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "FileSystem/UnrealLuaFileSystemEntry.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Actor.h"
#include "Subsystem/UnrealLuaFileSystem.h"
#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptBoxSubEditorImportPrompt.h"
#include "ToolWidgets/SLuaNewDefaultScriptEditorWindow.h"
#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptEditorTextBox.h"
#include "UnrealOverrides/LuaClassOverrideRegistry.h"
#include "UObject/UObjectIterator.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"
#include "Utility/LuaLogMacros.h"
#include "Utility/ParseUObjectToLua.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SSpacer.h"
#define LOCTEXT_NAMESPACE "UnrealLuaTools"
BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SLuaScriptBoxSubEditorNewDefaultScript::Construct(const FArguments& InArgs)
{
	this->SelectedClass.Reset(InArgs._SelectedClass);	
	
	SLuaScriptBoxSubEditor::Construct(SLuaScriptBoxSubEditor::FArguments()
		.OnSubEditorCommit(InArgs._OnSubeditorCommit)
		.OnCancelEdit(InArgs._OnCancelEdit)
	);
	auto normalStyle = FAppStyle::GetWidgetStyle<FTextBlockStyle>("NormalText");
	
	FString fileSavingMsg = "Select Object class";
	//FString fileSavingMsg = fileAlreadyExists
	//? FString::Printf(TEXT("Warning: File already exists: %s"), *shortFilePath)
	//: FString::Printf(TEXT("File location: %s"), *shortFilePath);
	//FSlateColor fileSavingMsgColor = fileAlreadyExists ? FSlateColor(FLinearColor{1, 1, 0, 1}) : normalStyle.ColorAndOpacity;
	
	UnrealLuaTools::SlateStyles::GetOnStyleChangedDelegate().AddSPLambda(this, [this]()
	{
		this->GetLuaScriptEditorTextBox()->SetStyle(UnrealLuaTools::SlateStyles::GetEditableTextBoxStyle());
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
					.Text(FText::FromString("Create Default Lua Script"))					
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
		+ SVerticalBox::Slot()
		[
			SAssignNew(FileWarningText, STextBlock)
			.Text(FText::FromString("<warning>"))
			.Visibility(EVisibility::Collapsed)
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
				.OnTextChanged_Raw(this, &SLuaScriptBoxSubEditorNewDefaultScript::NotifyImportStructNameChanged)
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

		//Central part : [Editable Text box]|[PropertyList]
		+ SVerticalBox::Slot()
		[
			SAssignNew(CenterHBoxSplitter, SSplitter)
			.Orientation(EOrientation::Orient_Horizontal)
			+ SSplitter::Slot()
			.MinSize(200)
			.Value(0.8f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				[
					SAssignNew(EditTextBox, SLuaScriptEditorTextBox)
					.Text(FText::GetEmpty())
					.IsReadOnly(true)
					.Style(UnrealLuaTools::SlateStyles::GetEditableTextBoxStyle())
					.AllowContextMenu(false)
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
			SNew(SSpacer)
			.Size(FVector2D{1.f, 1.f})
			.Visibility(EVisibility::Collapsed)
		]
		.VAlign(VAlign_Bottom)
		.AutoHeight()
	]
	.HAlign(HAlign_Fill)
	.Padding(10,0,10,0);
	
	
	this->CenterHBoxSplitter->AddSlot()
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
			this->NotifyCommit();
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
			this->NotifyCancel();
			return FReply::Handled();
		})
	]
	.VAlign(VAlign_Center)
	.HAlign(HAlign_Center)
	.AutoWidth()
	.FillWidth(1)
	.Padding(10,10,10,10)
	);
	
	RebuildObjectList();
	RefilterObjectList();
	
	this->SetSelectedClass(this->SelectedClass.Get());
}

TSharedRef<SVerticalBox> SLuaScriptBoxSubEditorNewDefaultScript::ConstructSearchList()
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
				.OnTextChanged( this, &SLuaScriptBoxSubEditorNewDefaultScript::OnFilterTextChanged )
				//.DelayChangeNotificationsWhileTyping(true)
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
		SAssignNew(PropertyListSwitcherPage, SBox)
		[
			SAssignNew(PropertyListVBox, SVerticalBox)
			+SVerticalBox::Slot()
			[
				ConstructWidgetSwitcher(this->SelectedClass.Get(), {}, {}, {})
			]
		]
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		.MaxDesiredWidth(200)
	];
	//Search list

	this->RightSideWidgetSwitcher->AddSlot()
	[
		SAssignNew(SearchListSwitcherPage, SBox)
		[
			SAssignNew(SearchListWidget, SListView<FName>)
			.ListItemsSource(&FilteredObjectList)
			.OnGenerateRow(this, &SLuaScriptBoxSubEditorNewDefaultScript::OnGenerateTableRow )
			.Orientation(Orient_Vertical)
		]
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		.WidthOverride(300)
	];
	
	this->RightSideWidgetSwitcher->AddSlot()
	[
		SAssignNew(DefaultSwitcherPage, SBox)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("Type class name in search box to create a default Lua script for"))
				.AutoWrapText(true)
				.Justification(ETextJustify::Center)	
			]
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
		]
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		.WidthOverride(300)
	];
	
	this->RightSideWidgetSwitcher->SetActiveWidget(this->DefaultSwitcherPage.ToSharedRef());
	
	return searchlist;
}

TSharedRef<ITableRow> SLuaScriptBoxSubEditorNewDefaultScript::OnGenerateTableRow(FName inData, const TSharedRef<STableViewBase>& tableViewBase)
{
	FListRow GenerateRow = GenerateRowForObject(inData);
	return
		SNew( STableRow< FName >, tableViewBase )
		[
			GenerateRow.Widget
		];
}

SLuaScriptBoxSubEditorNewDefaultScript::FListRow SLuaScriptBoxSubEditorNewDefaultScript::GenerateRowForObject(FName InData)
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
				this->NotifyClassSelected(InData);
				return FReply::Handled();
			})
		);
}

TSharedRef<SFunctionPropertyListSwitcher> SLuaScriptBoxSubEditorNewDefaultScript::ConstructWidgetSwitcher(UStruct* ustruct, const TArray<UFunction*>& functions, const TArray<UFunction*>& preSelectedFunctions, const TArray<FName>& preSelectedFunctionNames)
{
	this->FunctionsPropertySwitcher = SNew(SFunctionPropertyListSwitcher)
		.TargetStruct(ustruct)
		.Functions(functions)
		.SingleChoiceMode(false)
		.PreselectedFunctions(preSelectedFunctions)
		.PreselectedFunctionNames(preSelectedFunctionNames)
		;
	this->FunctionsPropertySwitcher->OnFunctionCheckboxChanged.BindSP(this, &SLuaScriptBoxSubEditorNewDefaultScript::NotifyCheckedFunctionChanged);
	this->FunctionsPropertySwitcher->OnPropertyCheckboxChanged.BindSP(this, &SLuaScriptBoxSubEditorNewDefaultScript::NotifyCheckedPropertyChanged);
	return this->FunctionsPropertySwitcher.ToSharedRef();
}

void SLuaScriptBoxSubEditorNewDefaultScript::NotifyCheckedFunctionChanged(UFunction* Function, bool bIsChecked)
{
	this->RebuildTextboxContent();
}

void SLuaScriptBoxSubEditorNewDefaultScript::NotifyCheckedPropertyChanged(FProperty* Property, bool bIsChecked)
{
	this->RebuildTextboxContent();
}

void SLuaScriptBoxSubEditorNewDefaultScript::NotifyImportStructNameChanged(const FText& newImportName)
{
	this->RebuildTextboxContent();
}


void SLuaScriptBoxSubEditorNewDefaultScript::RebuildObjectList()
{
	LoadedObjectList.Empty();
	
	FARFilter Filter;
	Filter.PackagePaths.Add("/Game"); // Restrict to your content directory
	Filter.ClassPaths.Add(UObject::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	Filter.WithoutPackageFlags = PKG_UncookedOnly | PKG_EditorOnly | PKG_Developer;
	
	TArray<FAssetData> StructAssets;
	IAssetRegistry::Get()->GetAssets(Filter, StructAssets);

	for (const FAssetData& AssetData : StructAssets)
	{
		// UScriptStruct or UUserDefinedStruct asset found
		//UScriptStruct* StructAsset = Cast<UScriptStruct>(AssetData.GetAsset());
		//LoadedObjectList.Add(TStrongObjectPtr<UObject>(StructAsset));
		this->AllAssetData.Emplace(AssetData.GetFullName(), AssetData.ToSoftObjectPath());
		LoadedObjectList.Add(*AssetData.GetFullName());
	}

	for (TObjectIterator<UClass> it; it; ++it)
	{
		if (it->IsNative() && !it->IsEditorOnly())
		{
			//LoadedObjectList.Add(TStrongObjectPtr<UObject>(*it));	
			this->AllAssetData.Emplace(*it->GetFullName(), FSoftObjectPath{*it});
			LoadedObjectList.Add(*it->GetFullName());		
		}
	}
}

void SLuaScriptBoxSubEditorNewDefaultScript::OnFilterTextChanged(const FText& searchText)
{
	this->SetSelectedClass(nullptr);
	
	if (searchText.IsEmpty())
	{
		this->RightSideWidgetSwitcher->SetActiveWidget(this->DefaultSwitcherPage.ToSharedRef());
	}
	else if (this->RightSideWidgetSwitcher->GetActiveWidget() != this->SearchListSwitcherPage)
	{
		//LUA_LOG("Setting to search list with text %s", *searchText.ToString());
		this->RightSideWidgetSwitcher->SetActiveWidget(this->SearchListSwitcherPage.ToSharedRef());
	}

	ReapplyFilter();
}

void SLuaScriptBoxSubEditorNewDefaultScript::UpdateSectionVisibility(UClass* ustruct)
{
	if (ustruct)
	{
		//show property list
		this->RightSideWidgetSwitcher->SetActiveWidget(this->PropertyListSwitcherPage.ToSharedRef());
		this->FunctionsPropertySwitcher->SetViewedStruct(ustruct, {}, {});
	}
	else
	{
		//show search list again
		this->RightSideWidgetSwitcher->SetActiveWidget(this->SearchListSwitcherPage.ToSharedRef());
		this->FunctionsPropertySwitcher->SetViewedStruct(nullptr, {}, {});
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

void SLuaScriptBoxSubEditorNewDefaultScript::SetImportSectionEnabled(bool enabled)
{
	if (!enabled)
	{
		this->ImportNameTextBox->SetText(FText::GetEmpty());
		//this->GenerateImportLineCheckbox->SetIsChecked(ECheckBoxState::Unchecked);
	}
	this->ImportSection->SetEnabled(enabled);
	this->ImportSection->SetVisibility(enabled ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed);
}

void SLuaScriptBoxSubEditorNewDefaultScript::SetSelectedClass(UClass* uclass)
{
	this->SelectedClass.Reset(uclass);
	
	this->UpdateSectionVisibility(uclass);

	this->RebuildTextboxContent();
}

void SLuaScriptBoxSubEditorNewDefaultScript::SetSelectedClass(FName selectedStructFullPath)
{
	UClass* uclass = nullptr;
	
	if (selectedStructFullPath != NAME_None)
	{
		FSoftObjectPath& data = this->AllAssetData.FindChecked(selectedStructFullPath);
		uclass = Cast<UClass>(data.TryLoad());
		if (!uclass)
		{
			uclass = Cast<UClass>(StaticLoadClass(UObject::StaticClass(), nullptr, data.ToString() + "_C"));
		}	
	}
	this->SetSelectedClass(uclass);
}

namespace UnrealLua
{
	FString CreateImportPathForUClass(const FSoftObjectPath& path, ELuaImportFilter filter)
	{
		FStringBuilderBase importPath;
		;
		if (filter == ELuaImportFilter::UScriptStruct)
		{
			importPath << "import \"" + path.ToString() << "\"";	
		}
		else if (filter == ELuaImportFilter::UClass)
		{
			FString pathString = path.ToString();
			importPath = "import \"" + pathString;
			if (!pathString.EndsWith("_C"))
			{
				importPath.Append("_C");
			}
			importPath << "\"";
		}
		else if (filter == ELuaImportFilter::UEnum)
		{
			importPath << "import \"" + path.ToString() << "\"";	
		}
		return importPath.ToString();
	}
}
void SLuaScriptBoxSubEditorNewDefaultScript::RebuildTextboxContent()
{
	UClass* selected = Cast<UClass>(this->SelectedClass.Get());
	
	this->GetLuaScriptEditorTextBox()->SetText(FText::GetEmpty());
	
	if (selected)
	{
		bool isActor = selected->IsChildOf(AActor::StaticClass());
		
		FString tableName = "Script";
		
		FStringBuilderBase fileBuilder;
		
		if (!selected->IsNative())
		{
			fileBuilder << "local " << this->ImportNameTextBox->GetText().ToString() << " = " << UnrealLua::CreateImportPathForUClass(selected, ELuaImportFilter::UClass) << "\n\n";	
		}
		
		fileBuilder << "---@type ";
		
		char prefix = isActor ? 'A' : 'U';
		
		if (selected->IsNative())
		{
			fileBuilder << prefix << selected->GetName() << "\n";
		}
		else
		{
			//@TODO: Perhaps alink to parent window up to Script Editor could provide info whether this was already imported 
			fileBuilder << this->ImportNameTextBox->GetText().ToString() << "\n";
		}
		
		fileBuilder << "local " << tableName << " = ... or {}\n";
		fileBuilder << "\n";
	
		for (FProperty* prop : this->FunctionsPropertySwitcher->PropertyListWidget->GetSelectedProperties())
		{
			fileBuilder << UnrealLua::ParseUtility::ParsePropertyToLuaFunctionTemplateString(tableName, prop);
			fileBuilder << "\n";		
		}

		for (UFunction* func : this->FunctionsPropertySwitcher->FunctionListWidget->GetSelectedFunctions())
		{
			fileBuilder << UnrealLua::ParseUtility::ParseUFunctionToLuaFunctionTemplateString(tableName, func);
			fileBuilder << "\n";
		}
		fileBuilder << "return Script\n";

		this->GetLuaScriptEditorTextBox()->SetText(FText::FromString(fileBuilder.ToString()));	
		
		this->GenerateSaveFilePath();
	}
	else
	{
		this->GenerateSaveFilePath();
	}
}

void SLuaScriptBoxSubEditorNewDefaultScript::ReapplyFilter()
{
	//LUA_LOG("Reapplying filter");
	RefilterObjectList();

	if (SearchListWidget.IsValid())
	{
		SearchListWidget->RequestListRefresh();
	}
}

void SLuaScriptBoxSubEditorNewDefaultScript::RefilterObjectList()
{
	// Tokenize the search box text into a set of terms; all of them must be present to pass the filter
	TArray<FString> FilterTerms{};
	if (FilterTextBoxWidget.IsValid())
	{
		FilterTextBoxWidget->GetText().ToString().ParseIntoArray(FilterTerms, TEXT(" "), true);
	}

	if (FilterTerms.Num())
	{
		//LUA_LOG("%d filter terms", FilterTerms.Num());
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
				//LUA_LOG("Adding %s to object list", *key.ToString());
				FilteredObjectList.Add(key);
			}
		}
	}
	else
	{
		//LUA_LOG("No filter terms");
		// Nothing to filter, just copy the list
		//FilteredObjectList = LoadedObjectList; 
		FilteredObjectList = {}; 
	}
}

void SLuaScriptBoxSubEditorNewDefaultScript::InsertTextAtCursor(const FString& Text)
{
}

void SLuaScriptBoxSubEditorNewDefaultScript::NotifyCommitFromSubEditor(const FSubEditorCommitData& data)
{
}

TSharedPtr<SLuaScriptEditorTextBox> SLuaScriptBoxSubEditorNewDefaultScript::GetLuaScriptEditorTextBox()
{
	return this->EditTextBox;
}

void SLuaScriptBoxSubEditorNewDefaultScript::NotifyClassSelected(FName className)
{
	this->SetSelectedClass(className);
}

void SLuaScriptBoxSubEditorNewDefaultScript::GenerateSaveFilePath()
{
	if (!this->SelectedClass.IsValid())
	{
		this->FileWarningText->SetVisibility(EVisibility::Collapsed);
		this->SaveFilePath = "";
		return;
	}
	this->SaveFilePath = FLuaClassOverrideRegistry::GetDefaultLuaScriptPathForUClass_LuaFolderRelative(this->SelectedClass.Get(), true);

	bool fileAlreadyExists = false;
	
	TWeakPtr<FUnrealLuaFileSystemEntry> fileDescr = UUnrealLuaFileSystem::Get()->FindFileDescriptorForFilePath(this->SaveFilePath);
	
	if (fileDescr.IsValid())
	{
		fileAlreadyExists = true;
	}
	
	FString displayedLuaPath = "/Lua/" + this->SaveFilePath;
	auto normalStyle = FAppStyle::GetWidgetStyle<FTextBlockStyle>("NormalText");
	FString fileSavingMsg = fileAlreadyExists
		? FString::Printf(TEXT("Warning: File already exists: %s"), *displayedLuaPath)
		: FString::Printf(TEXT("File location: %s"), *displayedLuaPath);
	FSlateColor fileSavingMsgColor = fileAlreadyExists ? FSlateColor(FLinearColor{1, 1, 0, 1}) : normalStyle.ColorAndOpacity;
	
	this->FileWarningText->SetText(FText::AsCultureInvariant(fileSavingMsg));
	this->FileWarningText->SetColorAndOpacity(fileSavingMsgColor);
	this->FileWarningText->SetVisibility(EVisibility::HitTestInvisible);
}

void SLuaScriptBoxSubEditorNewDefaultScript::NotifyCommit()
{
	TWeakPtr<FUnrealLuaFileSystemEntry> fileDescr = UUnrealLuaFileSystem::Get()->FindFileDescriptorForFilePath(this->SaveFilePath);
	
	FSubEditorCommitData commitData {this->SelectedClass.Get(), this->GetLuaScriptEditorTextBox()->GetText().ToString()};
	LUA_LOG_WARNING("Trying to save default script %s to %s", *commitData.CommitText, *this->SaveFilePath)
	if (fileDescr.IsValid())
	{
		if (fileDescr.Pin()->SaveFile(commitData.CommitText))
		{
			if (fileDescr.Pin()->IsInDefaultScriptDirectory())
			{
				LUA_LOG_WARNING("Saved file, now overriding class %s", *GetNameSafe(this->SelectedClass.Get()));
				FLuaClassOverrideRegistry& registry = UnrealLua::UObjectRegistry::GetLuaClassOverrideRegistry();
				registry.RequestMakeUClassOverridable(this->SelectedClass.Get());
			}
			else
			{
				LUA_LOG_ERROR("File %s not in default script directory!", *fileDescr.Pin()->GetFullPath());
			}
		}
		else
		{
			LUA_LOG_ERROR("Failed to save file %s!", *fileDescr.Pin()->GetFullPath());
		}
	}
	else
	{
		TSharedPtr<FUnrealLuaFileSystemEntry> newFile = UUnrealLuaFileSystem::Get()->CreateNewFile(this->SaveFilePath);
		if (newFile.IsValid())
		{
			newFile->SaveFile(commitData.CommitText);
			LUA_LOG_WARNING("saved!")
			if (newFile->IsInDefaultScriptDirectory())
			{
				LUA_LOG_WARNING("Saved new file, now overriding class %s", *GetNameSafe(this->SelectedClass.Get()));
				FLuaClassOverrideRegistry& registry = UnrealLua::UObjectRegistry::GetLuaClassOverrideRegistry();
				registry.RequestMakeUClassOverridable(this->SelectedClass.Get());
			}
			else
			{
				LUA_LOG_ERROR("File %s not in default script directory!", *fileDescr.Pin()->GetFullPath());
			}
		}
		else
		{
			LUA_LOG_WARNING("not saved...")
		}
	}
	
	this->OnCommit.ExecuteIfBound(commitData);	
}

void SLuaScriptBoxSubEditorNewDefaultScript::NotifyCancel()
{
	this->OnCancelEdit.ExecuteIfBound(this->SharedThis(this));
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
#undef LOCTEXT_NAMESPACE