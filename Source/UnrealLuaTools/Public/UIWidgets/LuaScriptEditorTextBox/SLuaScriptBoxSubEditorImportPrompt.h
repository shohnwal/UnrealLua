// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SLuaScriptBoxSubEditorNewObjectBase.h"
#include "Utility/UnrealLuaSimpleDelegateSignatures.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SWindow.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Views/SListView.h"

class SWidgetSwitcher;
class SEditableTextBox;
class SSearchBox;
class ITableRow;
class STableViewBase;

enum class ELuaImportFilter
{
	UClass,
	UScriptStruct,
	UEnum
};
/**
 * 
 */
class UNREALLUATOOLS_API SLuaScriptBoxSubEditorImportPrompt : public SLuaScriptBoxSubEditor
{
public:
	SLATE_BEGIN_ARGS(SLuaScriptBoxSubEditorImportPrompt)
		{
		}
		SLATE_ARGUMENT(ELuaImportFilter, ImportFilter)
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

	void RebuildObjectList();
	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	void OnFilterTextChanged(const FText& Text);
	void ReapplyFilter();
	void RefilterObjectList();
	TSharedRef<SVerticalBox> ConstructSearchList();
	TSharedRef<ITableRow> OnGenerateTableRow(FName InData, const TSharedRef<STableViewBase>& OwnerTable);
	FListRow GenerateRowForObject(FName InData);
	void NotifyListItemSelected(FName selected);
	void NotifyImportStructNameChanged(const FText& importName);
	
	virtual void Commit();

	virtual void InsertTextAtCursor(const FString& Text) override;
	virtual void NotifyCommitFromSubEditor(const FSubEditorCommitData& data) override;
	virtual TSharedPtr<SLuaScriptEditorTextBox> GetLuaScriptEditorTextBox() override;

	TArray<FName> FilteredObjectList = {};
	TArray<FName> LoadedObjectList = {};
	TMap<FName, FSoftObjectPath> AllAssetData = {};
	TSharedPtr< SSearchBox > FilterTextBoxWidget = nullptr;
	TArray<TSharedPtr<FString>> TypeOptions = {};
	TSharedPtr<FString> CurrentlySelectedType = {};
	TSharedPtr< SListView<FName> > SearchListWidget = nullptr;
	
	ELuaImportFilter ImportFilter = {};
	
	TSharedPtr<STextBlock> ImportStringText = {};
	TSharedPtr<SEditableTextBox> ImportNameText = {};
	TSharedPtr<SWidgetSwitcher> WidgetSwitcher = {};
	TSharedPtr<SComboBox<TSharedPtr<FString>>> TypeOptionsComboBox;
	void OnSelectionChanged(TSharedPtr<FString> option, ESelectInfo::Type seleciton);
};
