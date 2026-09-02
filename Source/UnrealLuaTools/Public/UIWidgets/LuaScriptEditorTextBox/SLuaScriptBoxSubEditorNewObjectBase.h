// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SLuaScriptBoxSubEditor.h"
#include "Utility/UnrealLuaSimpleDelegateSignatures.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Views/SListView.h"

class SLuaScriptEditorTextBox;
class SRichTextBlock;
class SMultiLineEditableTextBoxEx;
class SMultiLineEditableTextBox;
class SEditableTextBox;
class SCheckBox;
class SSearchBox;
class SPropertySelectionList;
class STextBlock;
class SLuaScriptBoxSubEditorNewObjectBase;
/**
 * 
 */

class UNREALLUATOOLS_API SLuaScriptBoxSubEditorNewObjectBase : public SLuaScriptBoxSubEditor
{
public:
	SLATE_BEGIN_ARGS(SLuaScriptBoxSubEditorNewObjectBase)
		{
		}
		SLATE_ARGUMENT(TScriptInterface<ILuaToolsSession>, Session)
		SLATE_EVENT(FSubEditorCommitDelegate, OnSubEditorCommit)
		SLATE_EVENT(FOnCreateNewChildObjectEditorDelegate, OnAddNewChildEditor)
		SLATE_EVENT(FSimpleWidgetDelegate, OnCancelEdit)
	SLATE_END_ARGS()
	
	/** Return value for GenerateRowForObject */
	struct FListRow
	{
		FListRow( const TSharedRef<SWidget>& InWidget)
		: Widget(InWidget)
		{}
		TSharedRef<SWidget> Widget;
	};

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	TStrongObjectPtr<UStruct> SelectedStruct = nullptr;
	TSharedPtr<SRichTextBlock> EditTextSectionRich = {};

	void InsertTextAtCursor(const FString& text);
	virtual TSharedRef<SVerticalBox> ConstructSearchList();
	FReply OnRefreshButtonClicked();
	void OnFilterTextChanged(const FText& InFilterText);
	void ReapplyFilter();
	EVisibility GetFilterStatusVisibility() const;
	FText GetFilterStatusText() const;
	bool IsFilterActive() const;
	FString GetSearchableText(UObject* Object);
	TSharedRef<ITableRow> OnGenerateTableRow(FName InData, const TSharedRef<STableViewBase>& OwnerTable);
	FListRow GenerateRowForObject(FName InData);
	virtual void SetImportSectionEnabled(bool enabled);
	void NotifyListItemSelected(FName selected);
	void NotifyPropertyCheckboxChanged(FProperty* Property, bool bIsChecked);
	void NotifyImportStructNameChanged(const FText& importName);
	virtual void RefilterObjectList();
	virtual void RebuildObjectList() = 0;
	virtual void SetSelectedStruct(FName selectedStructFullPath) = 0;	
	virtual void SetSelectedStruct(UStruct* selectedStruct) = 0;	
	void UpdateSectionVisibility(UStruct* ustruct);
	virtual void RebuildTextboxContent() = 0;
	virtual TSharedPtr<SLuaScriptEditorTextBox> GetLuaScriptEditorTextBox() override;
	
	virtual void NotifyCommitFromSubEditor(const FSubEditorCommitData& commitData);
	
	FReply HandleEditTextBoxKeyDown(const FGeometry& geometry, const FKeyEvent& keyEvent);
	
	virtual void AddNewChildObjectEditor(TSharedRef<SLuaScriptBoxSubEditor> newEditorObject);
	
	/*Basic layout*/
	
	TWeakInterfacePtr<ILuaToolsSession> Session = {};	
	
	TSharedPtr<SVerticalBox> WindowBodyVBox = {};
	TSharedPtr<SHorizontalBox> TopPartInfoHeader = {};
	TSharedPtr<SSplitter> CenterHBox = {};
	TSharedPtr<SLuaScriptEditorTextBox> LuaScriptEditorTextBox = {};
	TSharedPtr<SSpacer> BottomPartBorder = {};
	TSharedPtr<SBorder> ButtonsBorder = {};
	
	/** List of objects that can be shown */
	TArray<FName> LoadedObjectList = {};
	TMap<FName, FSoftObjectPath> AllAssetData = {};
	/** List of objects to show that have passed the keyword filtering */
	TSharedPtr<STextBlock> SelectedStructText = {};
	TArray<FName> FilteredObjectList = {};
	TSharedPtr<SBox> SearchListBorder = {};
	/* Widget containing the filtering text box */
	TSharedPtr< SSearchBox > FilterTextBoxWidget = nullptr;
	TSharedPtr< SListView<FName> > SearchListWidget = nullptr;
	TSharedPtr<SWidgetSwitcher> RightSideWidgetSwitcher = {};
	TSharedPtr<SHorizontalBox> ImportSection = {};
	//TSharedPtr<SCheckBox> GenerateImportLineCheckbox = {};
	TSharedPtr<SBox> PropertyListBorder = {};
	TSharedPtr<SEditableTextBox> ImportNameTextBox = {};
	TSharedPtr<SPropertySelectionList> PropertyListWidget = {};
	TSharedPtr<SCheckBox> AsMultilineCheckboxWidget = {};
};
