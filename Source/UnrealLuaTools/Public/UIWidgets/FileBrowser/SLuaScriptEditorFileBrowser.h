// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Input/DragAndDrop.h"
#include "Widgets/Views/STreeView.h"

struct FUnrealLuaFileSystemEntry;

class FUnrealLuaFileBrowserDragDropOp : public FDragDropOperation
{
public:
	DRAG_DROP_OPERATOR_TYPE(FUnrealLuaFileBrowserDragDropOp, FDragDropOperation)
	FUnrealLuaFileBrowserDragDropOp();
	virtual ~FUnrealLuaFileBrowserDragDropOp() override;
	
	virtual void OnDragged(const class FDragDropEvent& DragDropEvent) override;
	
	virtual FVector2D GetDecoratorPosition() const override { return this->DecoratorPosition; }


	FVector2D StartingScreenPos;
	/** The absolute position of the decorator. */
	FVector2D DecoratorPosition;

	virtual TSharedPtr<SWidget> GetDefaultDecorator() const override { return DraggedWidget; }
	// The widget you are dragging
	TSharedPtr<SWidget> DraggedWidget;
	
	double StartTime;
	
	TWeakPtr<FUnrealLuaFileSystemEntry> FileSystemEntry = {};

	// Optional: The offset from the mouse cursor
	FVector2D DragOffset;

	static TSharedRef<FUnrealLuaFileBrowserDragDropOp> New(TSharedRef<SWidget> InWidget, FVector2D InOffset)
	{
		TSharedRef<FUnrealLuaFileBrowserDragDropOp> Operation = MakeShareable(new FUnrealLuaFileBrowserDragDropOp());
		Operation->DraggedWidget = InWidget;
		Operation->DragOffset = InOffset;
		return Operation;
	}
};

DECLARE_DELEGATE_OneParam(FUnrealLuaFileDelegate, TSharedPtr<FUnrealLuaFileSystemEntry> item)
DECLARE_DELEGATE_OneParam(FUnrealLuaFileTreeRebuiltDelegate, const TArray<TSharedPtr<FUnrealLuaFileSystemEntry>>& treeItems)
class SLuaScriptEditorFileBrowser : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLuaScriptEditorFileBrowser)
		{
		}
	SLATE_EVENT(FUnrealLuaFileDelegate, OnFileDoubleClicked)
	SLATE_EVENT(FUnrealLuaFileDelegate, OnFileDeletedClicked)
	SLATE_EVENT(FUnrealLuaFileDelegate, OnRequestDefaultScriptFile)
	SLATE_EVENT(FUnrealLuaFileTreeRebuiltDelegate, OnFileTreeRebuilt)
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	
	/** Rebuilds the category tree from scratch */
	void RebuildFileTree();
	
	/** @return Returns the currently selected category item */
	TSharedPtr<FUnrealLuaFileSystemEntry> GetSelectedDirectory() const;

	/** Selects the specified category */
	void SelectDirectory( const TSharedPtr<FUnrealLuaFileSystemEntry>& categoryToSelect );
	
	/** @return Returns true if the specified item is currently expanded in the tree */
	bool IsItemExpanded( const TSharedPtr<FUnrealLuaFileSystemEntry> item ) const;
	
	void NotifyFileTreeRebuilt();
	TSharedRef<ITableRow> NotifyGenerateRow(TSharedPtr<FUnrealLuaFileSystemEntry> treeItem, const TSharedRef<STableViewBase>& tableViewBase);
	void NotifyGetChildren(TSharedPtr<FUnrealLuaFileSystemEntry> treeItem, TArray<TSharedPtr<FUnrealLuaFileSystemEntry>>& outChildren);
	void NotifySelectionChanged(TSharedPtr<FUnrealLuaFileSystemEntry> treeItem, ESelectInfo::Type arg);

	void NotfiyDeleteItem(TWeakPtr<FUnrealLuaFileSystemEntry> toDelete);
	void NotifyCommitDelete(const TWeakPtr<FUnrealLuaFileSystemEntry>& toDelete, ETextCommit::Type Type);
	
	void NotifyCreateNewFile(const TWeakPtr<FUnrealLuaFileSystemEntry>& parentDirectoryPtr);
	void NotifyCommitNewFile(const TWeakPtr<FUnrealLuaFileSystemEntry>& parentDirectoryPtr, const FText& fileName, ETextCommit::Type type);
	void NotifyCreateNewDirectory(const TWeakPtr<FUnrealLuaFileSystemEntry>& parentDirectory);
	void NotifyCommitNewDirectory(const TWeakPtr<FUnrealLuaFileSystemEntry>& parentDirectory, const FText& fileName, ETextCommit::Type type);
	void NotifyRequestCreateNewDefaultScriptFile(const TWeakPtr<FUnrealLuaFileSystemEntry>& parentDirectoryPtr) const;
	
	void NotifyItemExpansionChanged(TSharedPtr<FUnrealLuaFileSystemEntry> treeItem, bool bIsExpanded);
	void NotifyFileSystemChanged();
	
	void NotifyTreeItemClicked(TSharedPtr<FUnrealLuaFileSystemEntry> entry);
	void NotifyTreeItemDoubleClicked(TSharedPtr<FUnrealLuaFileSystemEntry> entry);
	void OnLuaFileDoubleClicked(const TWeakPtr<FUnrealLuaFileSystemEntry>& entry);
	
	TSharedPtr<SWidget> RequestContextMenu();

	void ExpandItemChain(const TWeakPtr<FUnrealLuaFileSystemEntry>& item);
	
	FReply NotifyFileBrowserItemDragDetected(const FGeometry& geometry, const FPointerEvent& pointerEvent, const TWeakPtr<FUnrealLuaFileSystemEntry>& fileItem);
	FReply NotifyDropOnTreeItem(const FDragDropEvent& DragDropEvent, EItemDropZone ItemDropZone, TSharedPtr<FUnrealLuaFileSystemEntry> UnrealLuaFileSystemEntry);

	FUnrealLuaFileDelegate OnFileDoubleClicked = {};
	FUnrealLuaFileDelegate OnFileDeleted = {};
	FUnrealLuaFileDelegate OnRequestDefaultScriptFile = {};
	FUnrealLuaFileTreeRebuiltDelegate OnTreeRebuilt = {};

	TSharedPtr<STreeView<TSharedPtr<FUnrealLuaFileSystemEntry>>> FileTreeView = {};
	//TSharedPtr<SBox> DragDropWidget = {};
};
