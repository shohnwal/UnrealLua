// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class SScrollBox;
/**
 * 
 */

struct FLuaIntellisenseItemWidget
{
	FString PropertyName = {};
	FString PropertyType = {};
};

class UNREALLUATOOLS_API SLuaIntelliSenseWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLuaIntelliSenseWidget)
		{
		}

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
	TSharedRef<ITableRow> OnGenerateTableRow(TSharedPtr<FLuaIntellisenseItemWidget> option, const TSharedRef<STableViewBase>& TableViewBase);
	FListRow GenerateRowForObject(TSharedPtr<FLuaIntellisenseItemWidget> option);

	bool ShowPropertiesOfStruct(UStruct* ustruct);
	bool ShowUFunctionsOfUClass(UClass* uclass);
	bool ShowImportOptions(UStruct* baseStruct);

	void MoveSelectionUp();
	void MoveSelectionDown();
	
	bool FilterResults(const FString& searchString);
	
	FString GetCurrentlySelectedItem();
	
	void Reset();
	void NotifyListItemSelected(const TSharedPtr<FLuaIntellisenseItemWidget>& option);
	void OnListRebuilt();
	
	FString CurrentFilter = "";
	TStrongObjectPtr<UStruct> Struct = nullptr;
	
	TArray<TSharedPtr<FLuaIntellisenseItemWidget>> AllOptionsList = {};
	TArray<TSharedPtr<FLuaIntellisenseItemWidget>> OptionsList = {};
	
	int32 CurrentlySelectedIndex = 0;
	TSharedPtr<SListView<TSharedPtr<FLuaIntellisenseItemWidget>>> OptionsListView = {};
};
