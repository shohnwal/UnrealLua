// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UIWidgets/GamescreenDockableWindowWidget.h"
#include "Utility/UnrealLuaSimpleDelegateSignatures.h"
#include "Widgets/SCompoundWidget.h"

class ILuaToolsSession;
struct FScopedLuaContext;
struct FUnrealLuaFileSystemEntry;
class SLuaScriptEditorFileBrowser;
class SScrollBox;
class SLuaScriptEditor;
class SCheckBox;
class SWidgetSwitcher;
class SEditableTextBox;
class STextBlock;
class SMultiTabEdtitableLuaScriptSwitcher;
class SHorizontalBox;
class SBorder;
class SVerticalBox;
/**
 * 
 */

struct FWidgetDragDropOp : public FDragDropOperation
{
	TSharedPtr<SLuaScriptEditor> WidgetToDrag = {};
	FVector2D MouseOffset = {};
	TDelegate<void(const FVector2D&)> OnUIDropped = {};

	virtual void OnDragged(const FDragDropEvent& InDragDropEvent) override;

	virtual void OnDrop(bool bDropWasHandled, const FPointerEvent& InMouseEvent) override;
};

class UNREALLUATOOLS_API SLuaScriptEditor : public SGamescreenDockableWindowWidget
{
public:
	SLATE_BEGIN_ARGS(SLuaScriptEditor)
		: _Session(nullptr), _StartAsWindow(false)
		{
		}
		//GameScreenDockableWindowWidget
		SLATE_ARGUMENT(TScriptInterface<ILuaToolsSession>, Session)
		SLATE_ARGUMENT(FString, Title)
		SLATE_ATTRIBUTE(FAnchors, GameScreenAnchors)
		SLATE_ATTRIBUTE(FAnchors, ExternalWindowAnchors)
		SLATE_ATTRIBUTE(FVector2D, ExternalWindowSize)
		SLATE_ATTRIBUTE(FVector2D, ExternalWindowPosition)
		SLATE_ATTRIBUTE(FLinearColor, BackgroundColor)
		SLATE_ARGUMENT(FVector2D, GameScreenAlignment)
		SLATE_ARGUMENT(bool, InitiallyHidden)
		SLATE_ARGUMENT(bool, StartAsWindow)

		//LuascriptEditor
		SLATE_ARGUMENT(bool, SingleFileOnly)
		SLATE_ARGUMENT(FString, ShortFilePath)
		SLATE_ARGUMENT(FString, FullFilePath)
		SLATE_ARGUMENT(FString, FileContent)
		SLATE_ARGUMENT(UClass*, BaseScriptClass)
		SLATE_ARGUMENT(TArray<FName>, ForbiddenFunctions)
		SLATE_ARGUMENT(TArray<FName>, PreselectedFunctionNames)
		SLATE_ARGUMENT(TArray<UFunction*>, PreselectedFunctions)
		SLATE_EVENT(FSimpleStringDelegate, OnConfirmEditText)
	SLATE_END_ARGS()

	DECLARE_DELEGATE_OneParam(FOnDragComplete, const FVector2D& /*ScreenSpacePosition*/);
	
	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	
	virtual EDockableWindowWidgetOnCloseExternalWindowBehavior GetOnCloseExternalWindowBehavior() const override;
	virtual EDockableWindowWidgetOnCloseGameScreenWidgetBehavior GetOnCloseGameScreenWidgetBehavior() const override;
	virtual EDockableWindowWidgetInputMode GetViewportInputMode() const override;
	bool HasAnyTabOpen() const;
	TSharedRef<STextBlock> ConstructFileExistsBlock(const FArguments& InArgs);
	TSharedRef<SEditableTextBox> ConstructFilePathText(const FArguments& InArgs);
	void UpdateFilePath(FString newFilePath);
	void UpdateFileExistsMessage();
	
	FReply NotifyNewFileButtonPressed() const;
	FReply NotifyOpenFileButtonPressed();
	FReply NotifySaveFileButtonPressed();
	FReply NotifyReloadLuaScriptButtonClicked();
	FReply AddNewObjectEditorToCurrentTab();
	FReply AddNewStructEditorToCurrentTab();
	
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply HandleEditTextBoxKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent);

	void OpenFile(const FString& filePath, bool andShow = true);
	void OpenFiles(const TArray<FString>& filePaths, bool andShow = true);
	
	void OpenNewTabCreateDefaultScript(UClass* uclass);
	//virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	//virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	//void OnDragUpdate(const FPointerEvent& PointerEvent, bool bDropped);
	
	virtual void Shutdown() override;
private:
	void AddToolsTabAndContent(FString buttonName, TSharedRef<SScrollBox>& content);
	void CreateNewObjectsTab();
	void CreateValuesTab();
	void CreateUtilityTab();
	
	void NotifyRequestCreateDefaultScriptFile(TSharedPtr<FUnrealLuaFileSystemEntry> parentFolder);
	void NotifyFileBrowserFileDoubleClicked(TSharedPtr<FUnrealLuaFileSystemEntry> treeItem);
	void NotifyFileTreeRebuilt(const TArray<TSharedPtr<FUnrealLuaFileSystemEntry>>& treeItems);
protected:
	void InsertTextAtCursorLocation(const FString& insertText, bool withMetaSelection = false);
	void InsertWrappedTextAtCursorSelection(const FString& insertLeftFromSelection, const FString& insertRightFromSelection);
	
	virtual bool HasSettings() const;
public:
	FSlateFontInfo Font = {};
	
	TSharedPtr<SBorder> TopPartBorder = {};
//	TSharedPtr<SHorizontalBox> CenterHBox = {};
	TSharedPtr<SBorder> BottomPartBorder = {};
	
	FSimpleStringDelegate OnTextCommitted = {};
	TSharedPtr<SWidgetSwitcher> FilesBrowserAndToolsSwitcher = {};
	TSharedPtr<SMultiTabEdtitableLuaScriptSwitcher> FileTabsSwitcher = {};
	
	TSharedPtr<STextBlock> FileExistsMessage = {};
	TSharedPtr<SEditableTextBox> FilePathText = {};
	FString FilePath = "";
	
	TSharedPtr<SWidgetSwitcher> FileBrowserAndToolsSwitcher = {};
	
	TSharedPtr<SHorizontalBox> ToolsCategoryTabs = {};
	TSharedPtr<SWidgetSwitcher> ToolCategorySwitcher = {};
	TSharedPtr<SCheckBox> PrintCommentsCheckox = {};
	TSharedPtr<SLuaScriptEditorFileBrowser> FileBrowser = {};

	//used by dragdrop OP to remember mouse offset
	//FVector2D ScreenSpaceOffsetOfGrab = {};
	//FOnDragComplete OnDragComplete = {};
};
