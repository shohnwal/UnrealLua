// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SLuaScriptBoxSubEditorNewObjectBase.h"

class SFunctionPropertyListSwitcher;
class SLuaScriptEditorTextBox;
/**
 * 
 */
class UNREALLUATOOLS_API SLuaScriptBoxSubEditorNewDefaultScript : public SLuaScriptBoxSubEditor
{
public:
	SLATE_BEGIN_ARGS(SLuaScriptBoxSubEditorNewDefaultScript)
		{
		}
		SLATE_EVENT(FSimpleWidgetDelegate, OnCancelEdit)
		SLATE_EVENT(FSubEditorCommitDelegate, OnSubeditorCommit)
		SLATE_ARGUMENT(UClass*, SelectedClass)
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
	TSharedRef<SVerticalBox> ConstructSearchList();
	TSharedRef<ITableRow> OnGenerateTableRow(FName inData, const TSharedRef<STableViewBase>& tableViewBase);
	FListRow GenerateRowForObject(FName InData);

	TSharedRef<SFunctionPropertyListSwitcher> ConstructWidgetSwitcher(UStruct* ustruct,
	                                                                  const TArray<UFunction*>& functions,
	                                                                  const TArray<UFunction*>& preSelectedFunctions,
	                                                                  const TArray<FName>& preSelectedFunctionNames);
	void NotifyCheckedFunctionChanged(UFunction* Function, bool bIsChecked);
	void NotifyCheckedPropertyChanged(FProperty* Property, bool bIsChecked);
	
	void NotifyImportStructNameChanged(const FText& newImportName);
	void RebuildObjectList();

	void OnFilterTextChanged(const FText& searchText);
	void UpdateSectionVisibility(UClass* Class);
	void SetImportSectionEnabled(bool enabled);
	void SetSelectedClass(UClass* uclass);
	void SetSelectedClass(FName selectedStructFullPath);

	void RebuildTextboxContent();

	void ReapplyFilter();
	void RefilterObjectList();
	virtual void InsertTextAtCursor(const FString& Text) override;
	virtual void NotifyCommitFromSubEditor(const FSubEditorCommitData& data) override;
	virtual TSharedPtr<SLuaScriptEditorTextBox> GetLuaScriptEditorTextBox() override;
	void NotifyClassSelected(FName className);

	void GenerateSaveFilePath();
	
	void NotifyCommit();
	void NotifyCancel();
	TMap<FName, FSoftObjectPath> AllAssetData = {};
	
	TStrongObjectPtr<UClass> SelectedClass = nullptr;
	TArray<FName> LoadedObjectList = {};
	TArray<FName> FilteredObjectList = {};
	
	FString SaveFilePath = "";
	
	TSharedPtr<SVerticalBox> WindowBodyVBox;
	TSharedPtr<SLuaScriptEditorTextBox> EditTextBox;
	TSharedPtr<SFunctionPropertyListSwitcher> FunctionsPropertySwitcher;
	TSharedPtr<SSplitter> CenterHBoxSplitter;
	TSharedPtr<SEditableTextBox> ImportNameTextBox;
	TSharedPtr<SHorizontalBox> ImportSection;
	TSharedPtr<SHorizontalBox> TopPartInfoHeader;
	TSharedPtr<STextBlock> SelectedStructText;
	TSharedPtr<SBorder> ButtonsBorder;
	TSharedPtr<SSearchBox> FilterTextBoxWidget;
	TSharedPtr<SWidgetSwitcher> RightSideWidgetSwitcher;
	TSharedPtr<SBox> PropertyListSwitcherPage;
	TSharedPtr<SBox> SearchListSwitcherPage;
	TSharedPtr<SBox> DefaultSwitcherPage;
	TSharedPtr<SListView<FName>> SearchListWidget;
	TSharedPtr<STextBlock> FileWarningText;
};
