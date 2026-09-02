// Fill out your copyright notice in the Description page of Project Settings.


#include "UIWidgets/SMultiTabEdtitableLuaScriptSwitcher.h"

#include "NetworkMessage.h"
#include "SlateOptMacros.h"
#include "Components/VerticalBox.h"
#include "ContextMenu/LuaScriptEditorContextMenuBuilder.h"
#include "FileSystem/UnrealLuaFileSystemEntry.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/SToolBarButtonBlock.h"
#include "Misc/FileHelper.h"
#include "Subsystem/UnrealLuaFileSystem.h"
#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptBoxSubEditorNewDefaultScript.h"
#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptBoxSubEditorImportPrompt.h"
#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptBoxSubEditorNewObjectBase.h"
#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptBoxSubEditorNewUObject.h"
#include "UIWidgets/SLuaScriptMultiEditorSwitcher.h"
#include "UIWidgets/SMultiLineEditableTextBoxEx.h"
#include "UIWidgets/FileBrowser/SLuaScriptEditorFileBrowser.h"
#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptBoxSubEditorSaveFileDialog.h"
#include "Utility/LuaFIleSystemLogMacros.h"
#include "Utility/LuaLogMacros.h"
#include "Utility/WidgetStyles.h"
#include "Utility/WindowUIUtility.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "UnrealLuaTools"
BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SMultiTabEdtitableLuaScriptSwitcher::Construct(const FArguments& InArgs)
{
	this->CachedOnKeyDownHandler = InArgs._OnKeyDownHandler;
	this->OnContextMenuOpening = InArgs._OnContextMenuOpening;
	this->IsSingleFileOnly = InArgs._SingleTabOnly;
	this->Session = InArgs._Session;
	this->ChildSlot 
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		[
			SAssignNew(FilenameTabsScrollbar, SScrollBox)
			.Orientation(Orient_Horizontal)
		]
		.AutoHeight()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		+ SVerticalBox::Slot()
		[
			SAssignNew(TabBodyContent, SWidgetSwitcher)
		]
		.FillHeight(1)
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
	];
}

void SMultiTabEdtitableLuaScriptSwitcher::AddNewChildObjectEditor(TSharedRef<SLuaScriptBoxSubEditorNewObjectBase> newEditorObject)
{
	if (this->HasAnyTabOpen())
	{
		this->CurrentTab.Pin()->AddNewChildObjectEditor(newEditorObject);
	}
}

TSharedPtr<SMultiLineEditableTextBoxEx> SMultiTabEdtitableLuaScriptSwitcher::GetCurrentlyEditableTextBox() const
{
	return this->CurrentTab.IsValid() ? this->CurrentTab.Pin()->GetEditableText() : nullptr;
}

bool SMultiTabEdtitableLuaScriptSwitcher::HasAnyTabOpen()
{
	return this->CurrentTab.IsValid();
}


TSharedPtr<SMultiTabEdtiableLuaScriptSwitcherTab> SMultiTabEdtitableLuaScriptSwitcher::CreateTabFromFileHandle(const TSharedPtr<FUnrealLuaFileSystemEntry>& treeItemHandle)
{
	if (this->OpenTab(treeItemHandle))
	{
		//File already open
		return this->CurrentTab.Pin();
	}
	else
	{
		if (this->IsSingleFileOnly && this->CurrentTab.IsValid())
		{
			return nullptr;
		}
		
		if (!treeItemHandle.IsValid())
		{
			return nullptr;
		}
		
		const FString& fullPath = treeItemHandle->GetFullPath();
		const FString& displayName = treeItemHandle->GetDisplayName();
		FString fileContent{};
		if (!IFileManager::Get().FileExists(*fullPath))
		{
			LUA_FILES_LOG_ERROR("Trying to open file %s from file descriptor, but file does not exist on file system!", *fullPath)
			checkNoEntry()
			return nullptr;
		}
		//{
		//	LUA_LOG_WARNING("Trying to open file %s, but file can not be loaded, displayed content will be empty!", *fullPath)
		//}
		fileContent = treeItemHandle->LoadFileToString();
		//if (!FFileHelper::LoadFileToString(fileContent, *fullPath))
		//{
		//	//LUA_LOG_WARNING("Trying to open file %s, but file can not be loaded, displayed content will be empty!", *fullPath)
		//}
		
		auto newTab = this->CreateNewTabInternal(fullPath, fileContent);
		
		newTab->FileHandle = treeItemHandle;
		
		treeItemHandle->GetOnFileUpdatedDelegate().AddSP(newTab.ToSharedRef(), &SMultiTabEdtiableLuaScriptSwitcherTab::NotifyFileHandleUpdated);

		verify(this->OpenTab(newTab->FileHandle));
		
		return newTab;	
	}
}

TSharedPtr<SMultiTabEdtiableLuaScriptSwitcherTab> SMultiTabEdtitableLuaScriptSwitcher::CreateUnownedTab(const FString& fullPathOrDisplayName)
{
	if (this->IsSingleFileOnly && this->CurrentTab.IsValid())
	{
		return nullptr;
	}
	auto newTab = this->CreateNewTabInternal(fullPathOrDisplayName, "");
	
	newTab->bIsTemporaryFile = true;
	
	newTab->NotifyTextDirtyChanged(true);
	
	this->SetOpenTab(newTab);
	
	return newTab;
}

TSharedPtr<SMultiTabEdtiableLuaScriptSwitcherTab> SMultiTabEdtitableLuaScriptSwitcher::CreateNewDefaultLuaScriptTab(const FString& fullPathForDefaultLuaScriptFile, UClass* uclass)
{
	auto newTab = this->CreateUnownedTab(fullPathForDefaultLuaScriptFile);
	
	if (!newTab)
	{
		return nullptr;
	}
	
	TSharedRef<SLuaScriptBoxSubEditorNewDefaultScript> defaultLuaScriptEditor = SNew(SLuaScriptBoxSubEditorNewDefaultScript)
		.SelectedClass(uclass);
	newTab->AddNewChildObjectEditor(defaultLuaScriptEditor);
	
	defaultLuaScriptEditor->SetSelectedClass(uclass);
	
	return newTab;
}

void SMultiTabEdtitableLuaScriptSwitcher::NotifyFileTreeRebuilt(const TArray<TSharedPtr<FUnrealLuaFileSystemEntry>>& treeItems)
{
	TArray<TSharedPtr<SMultiTabEdtiableLuaScriptSwitcherTab>> tabscopy = this->OpenTabs;
	for (TSharedPtr<SMultiTabEdtiableLuaScriptSwitcherTab>& tab : tabscopy)
	{
		if (!tab->FileHandle.IsValid() && !tab->bIsTemporaryFile)
		{
			this->CloseTab(tab);
		}
	}
}

void SMultiTabEdtitableLuaScriptSwitcher::InsertAtCursorLocation(const FString& toInsert, bool withMetaSelection)
{
	TSharedPtr<SMultiTabEdtiableLuaScriptSwitcherTab> tab = this->GetCurrentTab();
	if (tab.IsValid())
	{
		tab->LinkedContentWidget->InsertTextAtCursorLocation(toInsert, withMetaSelection);
	}	
}

void SMultiTabEdtitableLuaScriptSwitcher::InsertWrappedTextAtCursorSelection(const FString& insertLeftFromSelection, const FString& insertRightFromSelection)
{
	TSharedPtr<SMultiTabEdtiableLuaScriptSwitcherTab> tab = this->GetCurrentTab();
	if (tab.IsValid())
	{
		tab->LinkedContentWidget->InsertWrappedTextAtCursorSelection(insertLeftFromSelection, insertRightFromSelection);
	}
}

TSharedPtr<SMultiTabEdtiableLuaScriptSwitcherTab> SMultiTabEdtitableLuaScriptSwitcher::CreateNewTabInternal(const FString& fullPathFileName, const FString& content)
{
	FString fileName;
	FString fullPath;;
	if (fullPathFileName.IsEmpty())
	{
		fileName = "untitled.lua";
		fullPath = "";
	}
	else if (fullPathFileName.Split(TEXT("/"), nullptr, &fileName, ESearchCase::CaseSensitive, ESearchDir::FromEnd ))
	{
		fullPath = fullPathFileName;
	}
	else
	{
		fileName = fullPathFileName;
		fullPath = "";
	}
	verify(fileName.EndsWith(".lua"));
	
	auto newTab = SNew(SMultiTabEdtiableLuaScriptSwitcherTab)
	.TabDisplayName(fileName)
	.OnCloseButtonClicked(this, &SMultiTabEdtitableLuaScriptSwitcher::NotifyTabCloseButtonClicked)
	.OnSelectThisTab(this, &SMultiTabEdtitableLuaScriptSwitcher::NotifySelectTabButtonClicked);
		
	TWeakPtr<SMultiTabEdtiableLuaScriptSwitcherTab> weakTab = newTab;
	auto newContent = SNew(SLuaScriptMultiEditorSwitcher)
		.Session(this->Session)
		.OnContextMenuOpening(this->OnContextMenuOpening)
		.OnCommitToTopLevelTextBox_Lambda([weakTab](const FReceivedSubEditorCommitData& commitData)
		{
			if (weakTab.IsValid())
			{
				weakTab.Pin()->NotifyTextEditorTopLevelCommit(commitData);
			}
		})
		.TextContent(content);
	
	newTab->LinkedContentWidget = newContent;
	newTab->FileHandle = nullptr;
	newTab->FileName = fileName;
	newTab->FullPath = fullPath;

	if (IsSingleFileOnly)
	{
		newTab->SetVisibility(EVisibility::Collapsed);
	}

	newContent->OnTextContentChanged.BindSPLambda(newTab, [newTab](const FText& newText){ newTab->NotifyTextDirtyChanged(true); });
	
	this->OpenTabs.Add(newTab);
	
	this->TabBodyContent->AddSlot()
	[
		newContent
	]
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill);
	this->FilenameTabsScrollbar->AddSlot()
	[
		newTab
	];
	
	return newTab;
}


void SMultiTabEdtitableLuaScriptSwitcher::CloseCurrentTab()
{
	if (this->HasAnyTabOpen())
	{
		this->CloseTab(this->CurrentTab.Pin());
	}
}

void SMultiTabEdtitableLuaScriptSwitcher::NotifyTabCloseButtonClicked(TSharedRef<SMultiTabEdtiableLuaScriptSwitcherTab> openFileInfo)
{
	this->CloseTab(openFileInfo);
}

void SMultiTabEdtitableLuaScriptSwitcher::NotifySelectTabButtonClicked(TSharedRef<SMultiTabEdtiableLuaScriptSwitcherTab> selectTabInfo)
{
	this->SetOpenTab(selectTabInfo);
}



bool SMultiTabEdtitableLuaScriptSwitcher::OpenTab(const TWeakPtr<FUnrealLuaFileSystemEntry>& treeItem)
{
	TSharedPtr<SMultiTabEdtiableLuaScriptSwitcherTab> existing = this->FindTab(treeItem);
	if (!existing)
	{
		return false;
	}
	this->SetOpenTab(existing);
	return true;
}

TSharedPtr<SMultiTabEdtiableLuaScriptSwitcherTab> SMultiTabEdtitableLuaScriptSwitcher::FindTab(const TWeakPtr<FUnrealLuaFileSystemEntry>& toSearch)
{
	for (TSharedPtr<SMultiTabEdtiableLuaScriptSwitcherTab> tab : this->OpenTabs)
	{
		if(tab->FileHandle == toSearch)
		{
			return tab;
		}
	}
	return nullptr;
}

TSharedPtr<SMultiTabEdtiableLuaScriptSwitcherTab> SMultiTabEdtitableLuaScriptSwitcher::GetCurrentTab()
{
	return this->CurrentTab.Pin();
}


void SMultiTabEdtitableLuaScriptSwitcher::SetOpenTab(const TSharedPtr<SMultiTabEdtiableLuaScriptSwitcherTab>& tab)
{
	if (!tab)
	{
		//this->FilenameTabsScrollbar->ScrollDescendantIntoView(nullptr);
		this->TabBodyContent->SetActiveWidgetIndex(-1);
		return;
	}
	this->FilenameTabsScrollbar->ScrollDescendantIntoView(tab);
	this->TabBodyContent->SetActiveWidget(tab->LinkedContentWidget.ToSharedRef());
	this->CurrentTab = tab;
	FSlateApplication::Get().SetKeyboardFocus(tab->GetEditableText());
	//this->bCanSupportFocus	
}

void SMultiTabEdtitableLuaScriptSwitcher::CloseTab(const TSharedPtr<SMultiTabEdtiableLuaScriptSwitcherTab>& toClose)
{
	if (!toClose.IsValid())
	{
		return;
	}
	if (this->IsSingleFileOnly)
	{
		//can't close tab if we're single file
		return;
	}
	
	toClose->PrepareClose();
	this->FilenameTabsScrollbar->RemoveSlot(toClose.ToSharedRef());
	this->TabBodyContent->RemoveSlot(toClose->LinkedContentWidget.ToSharedRef());
	if (toClose == this->CurrentTab)
	{
		this->CurrentTab.Reset();
	}
	int32 closedIndex = this->OpenTabs.IndexOfByKey(toClose);
	this->OpenTabs.Remove(toClose);
	if (!this->OpenTabs.IsEmpty())
	{
		if (this->OpenTabs.IsValidIndex(closedIndex))
		{
			this->SetOpenTab(this->OpenTabs[closedIndex]);
		}
		else if (this->OpenTabs.IsValidIndex(closedIndex - 1))
		{
			this->SetOpenTab(this->OpenTabs[closedIndex - 1]);
		}
	}
}

///////////////////////////////////////////////////////////////////////////
/// Tab Button
///////////////////////////////////////////////////////////////////////////

void SMultiTabEdtiableLuaScriptSwitcherTab::Construct(const FArguments& InArgs)
{
	this->OnCloseLuaEditScriptTab = InArgs._OnCloseButtonClicked;
	this->OnSelectThisTab = InArgs._OnSelectThisTab;
	this->ChildSlot
	[
		SNew(SBorder)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SAssignNew(SelectThisTabButton, SButton)
				.OnClicked(this, &SMultiTabEdtiableLuaScriptSwitcherTab::NotifySelectThisTabButtonClicked)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					[
						SAssignNew(FileDirtyMarker, STextBlock)
						.Text(FText::AsCultureInvariant("* "))
						.Visibility(EVisibility::Collapsed)						
					]
					.AutoWidth()
					+ SHorizontalBox::Slot()
					[
						SAssignNew(TabText, STextBlock)
						.Text(FText::AsCultureInvariant(InArgs._TabDisplayName))
						.Visibility(EVisibility::SelfHitTestInvisible)
					]
					.AutoWidth()
				]
			]
			.AutoWidth()
			+ SHorizontalBox::Slot()
			[
				SAssignNew(CloseTabButton, SButton)
				.OnClicked(this, &SMultiTabEdtiableLuaScriptSwitcherTab::NotifyTabCloseButtonClicked)
				[
					SNew(STextBlock)
					.Text(FText::AsCultureInvariant("X"))
					.Visibility(EVisibility::SelfHitTestInvisible)
				]
				.ButtonColorAndOpacity(FSlateColor{FLinearColor::Transparent})
			]
			.AutoWidth()			
		]
	];
}




FReply SMultiTabEdtiableLuaScriptSwitcherTab::NotifySelectThisTabButtonClicked()
{
	if (this->LinkedContentWidget.IsValid())
	{
		this->OnSelectThisTab.ExecuteIfBound(this->SharedThis(this));
	}
	return FReply::Handled();	
}

FReply SMultiTabEdtiableLuaScriptSwitcherTab::NotifyTabCloseButtonClicked()
{
	if (this->LinkedContentWidget.IsValid())
	{
		this->OnCloseLuaEditScriptTab.ExecuteIfBound(this->SharedThis(this));
	}
	return FReply::Handled();
}

void SMultiTabEdtiableLuaScriptSwitcherTab::NotifyTextDirtyChanged(bool isDirty)
{
	if (isDirty)
	{
		this->FileDirtyMarker->SetVisibility(EVisibility::SelfHitTestInvisible);	
	}
	else
	{
		this->FileDirtyMarker->SetVisibility(EVisibility::Collapsed);	
	}
}

void SMultiTabEdtiableLuaScriptSwitcherTab::ClearFileDirty()
{
	this->NotifyTextDirtyChanged(false);
}

void SMultiTabEdtiableLuaScriptSwitcherTab::NotifyFileHandleUpdated(TSharedRef<FUnrealLuaFileSystemEntry> fileHandle)
{
	verify(this->FileHandle == fileHandle);
	if (fileHandle->IsValid())
	{
		this->SetToolTipText(FText::AsCultureInvariant(fileHandle->GetFullPath()));
		FString content = fileHandle->LoadFileToString();
		this->LinkedContentWidget->GetEditableText()->SetText(FText::AsCultureInvariant(content));
	}
	else
	{
		this->NotifyTabCloseButtonClicked();
	}
}

void SMultiTabEdtiableLuaScriptSwitcherTab::PrepareClose()
{
	if (this->FileHandle.IsValid())
	{
		this->FileHandle.Pin()->GetOnFileUpdatedDelegate().RemoveAll(this);
	}
	else
	{
		verify(this->bIsTemporaryFile)
	}
}

TSharedPtr<SMultiLineEditableTextBoxEx> SMultiTabEdtiableLuaScriptSwitcherTab::GetEditableText()
{
	return this->LinkedContentWidget->GetEditableText();
}

void SMultiTabEdtiableLuaScriptSwitcherTab::SaveFile()
{
	TWeakPtr<FUnrealLuaFileSystemEntry> fileHandle = this->FileHandle;
	if (!fileHandle.IsValid())
	{
		FString filePath = this->FullPath;
		if (filePath.IsEmpty())
		{
			//auto saveDialog = SNew(SLuaScriptBoxSubEditorSaveFileDialog);
			
			//this->AddNewChildObjectEditor(saveDialog);
			//@TODO : ask for save location
			filePath = "";
			this->ClearFileDirty();
			LUA_LOG_WARNING("Can not save file to disk: unnamed file asking for file path not implemented yet!")
			return;
		}
		LUA_LOG_WARNING("Creating new file handle")
		this->FileHandle = UUnrealLuaFileSystem::Get()->CreateNewFile(this->FullPath);
		fileHandle = this->FileHandle;
	}
	
	if (!fileHandle.IsValid())
	{
		LUA_LOG_ERROR("Can not save file to %s, file handle not valid. Is the save path inside the Lua folder?", *this->FullPath);
		return;
	}
	FString fileContent = this->GetEditableText()->GetText().ToString();
	
	bool success = fileHandle.Pin()->SaveFile(fileContent);
	if (success)
	{
		this->ClearFileDirty();
	}
}

void SMultiTabEdtiableLuaScriptSwitcherTab::AddNewChildObjectEditor(TSharedRef<SLuaScriptBoxSubEditor> newEditor)
{
	this->LinkedContentWidget->AddNewChildObjectEditor(newEditor);
}

void SMultiTabEdtiableLuaScriptSwitcherTab::NotifyTextEditorTopLevelCommit(const FReceivedSubEditorCommitData& commitData)
{
	if (this->bIsTemporaryFile)
	{
		
	}
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
#undef LOCTEXT_NAMESPACE