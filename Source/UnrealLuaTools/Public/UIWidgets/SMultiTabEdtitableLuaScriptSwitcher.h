// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Utility/UnrealLuaSimpleDelegateSignatures.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Text/SRichTextBlock.h"

class ILuaToolsSession;
struct FReceivedSubEditorCommitData;
class SLuaScriptBoxSubEditor;
class SLuaScriptBoxSubEditorNewObjectBase;
struct FUnrealLuaFileSystemEntry;
class SLuaScriptEditorWindow;
class STextBlock;
class SButton;
class SScrollBox;
class SWidgetSwitcher;
class SLuaScriptMultiEditorSwitcher;
class SMultiTabEdtiableLuaScriptSwitcherTab;
class SMultiLineEditableTextBoxEx;
struct FScopedLuaContext;

class UNREALLUATOOLS_API SMultiTabEdtitableLuaScriptSwitcher : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMultiTabEdtitableLuaScriptSwitcher)
		: _Session(nullptr)
		{
		}
		SLATE_ARGUMENT(TScriptInterface<ILuaToolsSession>, Session)
		SLATE_EVENT(FOnKeyDown, OnKeyDownHandler)
		SLATE_ARGUMENT(FText, Text)
		SLATE_ARGUMENT(bool, IsReadOnly)
		SLATE_ARGUMENT(bool, AllowContextMenu)
		SLATE_ARGUMENT(bool, SingleTabOnly)
		/** Delegate to call before a context menu is opened. User returns the menu c ontent or null to the disable context menu */
		SLATE_EVENT(FOnContextMenuOpening, OnContextMenuOpening)
		/** Callback delegate to have first chance handling of the OnKeyDown event */
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	void AddNewChildObjectEditor(TSharedRef<SLuaScriptBoxSubEditorNewObjectBase> newEditorObject);

	TSharedPtr<SMultiLineEditableTextBoxEx> GetCurrentlyEditableTextBox() const; 
	bool HasAnyTabOpen();

	
	void CloseCurrentTab();
	void NotifyTabCloseButtonClicked(TSharedRef<SMultiTabEdtiableLuaScriptSwitcherTab> openFileInfo);
	void NotifySelectTabButtonClicked(TSharedRef<SMultiTabEdtiableLuaScriptSwitcherTab> selectTabInfo);

	bool OpenTab(const TWeakPtr<FUnrealLuaFileSystemEntry>& treeItem);

	TSharedPtr<SMultiTabEdtiableLuaScriptSwitcherTab> FindTab(const TWeakPtr<FUnrealLuaFileSystemEntry>& Weak);
	
	TSharedPtr<SMultiTabEdtiableLuaScriptSwitcherTab> GetCurrentTab();
	
	TSharedPtr<SMultiTabEdtiableLuaScriptSwitcherTab> CreateTabFromFileHandle(const TSharedPtr<FUnrealLuaFileSystemEntry>& treeItemHandle);
	TSharedPtr<SMultiTabEdtiableLuaScriptSwitcherTab> CreateUnownedTab(const FString& fullPathOrDisplayName = "untitled.lua");
	TSharedPtr<SMultiTabEdtiableLuaScriptSwitcherTab> CreateNewDefaultLuaScriptTab(const FString& fullPathForDefaultLuaScriptFile, UClass* uclass);

	void NotifyFileTreeRebuilt(const TArray<TSharedPtr<FUnrealLuaFileSystemEntry>>& treeItems);

	void InsertAtCursorLocation(const FString& toInsert, bool withMetaSelection = false);
	void InsertWrappedTextAtCursorSelection(const FString& insertLeftFromSelection, const FString& insertRightFromSelection);

protected:
	
	TSharedPtr<SMultiTabEdtiableLuaScriptSwitcherTab> CreateNewTabInternal(const FString& fullPathFileName, const FString& content);
	
	void SetOpenTab(const TSharedPtr<SMultiTabEdtiableLuaScriptSwitcherTab>& tab);
	void CloseTab(const TSharedPtr<SMultiTabEdtiableLuaScriptSwitcherTab>& tab);

	TSharedPtr<SWidgetSwitcher> TabBodyContent;
	TSharedPtr<SScrollBox> FilenameTabsScrollbar;
	
	FOnKeyDown CachedOnKeyDownHandler = {};
	TArray<TSharedPtr<SMultiTabEdtiableLuaScriptSwitcherTab>> OpenTabs = {};
	TWeakPtr<SMultiTabEdtiableLuaScriptSwitcherTab> CurrentTab = {};
	bool IsSingleFileOnly = false;
	TScriptInterface<ILuaToolsSession> Session = nullptr;

private:
	FOnContextMenuOpening OnContextMenuOpening = {};
};

DECLARE_DELEGATE_OneParam(FOnLuaEditScriptTabClickedDelegate, TSharedRef<SMultiTabEdtiableLuaScriptSwitcherTab>)

class UNREALLUATOOLS_API SMultiTabEdtiableLuaScriptSwitcherTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMultiTabEdtiableLuaScriptSwitcherTab)
	{
	}
	SLATE_ARGUMENT(FString, TabDisplayName)
	SLATE_EVENT(FOnLuaEditScriptTabClickedDelegate, OnCloseButtonClicked)
	SLATE_EVENT(FOnLuaEditScriptTabClickedDelegate, OnSelectThisTab)
SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	
	FReply NotifySelectThisTabButtonClicked();
	FReply NotifyTabCloseButtonClicked();
	void NotifyTextDirtyChanged(bool isDirty);
	void ClearFileDirty();
	void NotifyFileHandleUpdated(TSharedRef<FUnrealLuaFileSystemEntry> fileHandle);
	void PrepareClose();
	
	TSharedPtr<SMultiLineEditableTextBoxEx> GetEditableText();
	void SaveFile();
	
	void AddNewChildObjectEditor(TSharedRef<SLuaScriptBoxSubEditor> newEditor);
	void NotifyTextEditorTopLevelCommit(const FReceivedSubEditorCommitData& commitData);

	TWeakPtr<FUnrealLuaFileSystemEntry> FileHandle = {};
	FString FileName = "";
	FString FullPath = "";
	bool bFileIsDirty = false;
	bool bIsTemporaryFile = false;

	FOnLuaEditScriptTabClickedDelegate OnSelectThisTab = {};
	FOnLuaEditScriptTabClickedDelegate OnCloseLuaEditScriptTab = {};
	TSharedPtr<SButton> SelectThisTabButton = {};
	TSharedPtr<SButton> CloseTabButton = {};
	TSharedPtr<STextBlock> TabText = {};
	TSharedPtr<STextBlock> FileDirtyMarker = {};
	
	TSharedPtr<SLuaScriptMultiEditorSwitcher> LinkedContentWidget = {};
};

