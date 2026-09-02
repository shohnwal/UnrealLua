// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"


class UUserWidget;

struct FUnrealLuaWatchedObjectHierarchyItem
{
	FName ObjectName = NAME_None;
	TWeakObjectPtr<UObject> Object = nullptr;
	bool SelectAsMainItem = false;
	TArray<TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem>> Children = {};
};
/**
 * 
 */

DECLARE_DELEGATE_OneParam(FUnrealLuaObjectTreeItemDelegate, TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem> item)

class UNREALLUATOOLS_API SUObjectHierarchyWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUObjectHierarchyWidget)
		: _OnItemDoubleClicked()
		{
		}
	SLATE_EVENT(FUnrealLuaObjectTreeItemDelegate, OnItemDoubleClicked);

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	
	void SetWatchedObject(UObject* object);
private:
	void SetWatchedActor(AActor* actor);
	void SetWatchedWorld(UWorld* actor);
	void SetWatchedGameInstance(UGameInstance* gameInstance);
	void SetWatchedWidget(UUserWidget* widget);
public:
	TSharedRef<ITableRow> NotifyGenerateRow(TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem> item, const TSharedRef<STableViewBase>& table);
	void NotifyGetChildren(TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem> parentItem, TArray<TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem>>& objectList);
	void NotifySelectionChanged(TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem> selectedItem, ESelectInfo::Type selectionMethod);
	void NotifyObjectTreeRebuilt();
	void NotifyTreeItemClicked(TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem> clickedItem);
	void NotifyTreeItemDoubleClicked(TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem> doubleClickedItem) const;
	void NotifyItemExpansionChanged(TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem> item, bool bIsExpanded);
private:
	TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem> GetParentItemForSubObject(UObject* object) const;
public:
	FUnrealLuaObjectTreeItemDelegate OnItemDoubleClicked = {};
	TArray<TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem>> RootObject = {};
	TArray<TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem>> ObjectsList = {};
	TSharedPtr<STreeView<TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem>>> ObjectsTreeView = {};
};
