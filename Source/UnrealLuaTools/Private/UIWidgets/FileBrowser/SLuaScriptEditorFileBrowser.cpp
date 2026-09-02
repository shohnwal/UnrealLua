// Fill out your copyright notice in the Description page of Project Settings.


#include "UIWidgets/FileBrowser/SLuaScriptEditorFileBrowser.h"

#include <locale>

#include "SlateOptMacros.h"
#include "FileSystem/UnrealLuaFileSystemEntry.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Subsystem/UnrealLuaFileSystem.h"
#include "UIWidgets/Draggable/SUnrealLuaDraggableBoxOverlay.h"
#include "Utility/LuaLogMacros.h"
#include "VerseVM/VVMRuntimeError.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#define LOCTEXT_NAMESPACE "UnrealLuaTools"
BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

FUnrealLuaFileBrowserDragDropOp::FUnrealLuaFileBrowserDragDropOp()
{
	bCreateNewWindow =false;
	StartTime = FSlateApplicationBase::Get().GetCurrentTime();
}

FUnrealLuaFileBrowserDragDropOp::~FUnrealLuaFileBrowserDragDropOp()
{
	LUA_LOG("~FUnrealLuaFileBrowserDragDropOp");
}

void FUnrealLuaFileBrowserDragDropOp::OnDragged(const class FDragDropEvent& DragDropEvent)
{
	//FDragDropOperation::OnDragged(DragDropEvent);
	{
		FVector2D CachedDesiredSize = DraggedWidget->GetDesiredSize();

		FVector2D Position = DragDropEvent.GetScreenSpacePosition();
		Position -= CachedDesiredSize * FVector2D(0.5f, 0.5f);

		//Position -= CachedDesiredSize * FVector2D(0.5f, 0.5f);
//		switch ( this->DragPivot )
//		{
//		case EDragPivot::MouseDown:
//			Position +=  this->DragOffset;
//			break;
//
//		case EDragPivot::TopLeft:
//			// Position is already Top Left.
//			break;
//		case EDragPivot::TopCenter:
//			Position -= CachedDesiredSize * FVector2D(0.5f, 0);
//			break;
//		case EDragPivot::TopRight:
//			Position -= CachedDesiredSize * FVector2D(1, 0);
//			break;
//
//		case EDragPivot::CenterLeft:
//			Position -= CachedDesiredSize * FVector2D(0, 0.5f);
//			break;
//		case EDragPivot::CenterCenter:
//			Position -= CachedDesiredSize * FVector2D(0.5f, 0.5f);
//			break;
//		case EDragPivot::CenterRight:
//			Position -= CachedDesiredSize * FVector2D(1.0f, 0.5f);
//			break;
//
//		case EDragPivot::BottomLeft:
//			Position -= CachedDesiredSize * FVector2D(0, 1);
//			break;
//		case EDragPivot::BottomCenter:
//			Position -= CachedDesiredSize * FVector2D(0.5f, 1);
//			break;
//		case EDragPivot::BottomRight:
//			Position -= CachedDesiredSize * FVector2D(1, 1);
//			break;
//		}

		const double AnimationTime = 0.150;

		//double DeltaTime = FSlateApplicationBase::Get().GetCurrentTime() - StartTime;

		//if ( DeltaTime < AnimationTime )
		//{
		//	double T = DeltaTime / AnimationTime;
		//	FVector2D LerpPosition = ( Position - StartingScreenPos ) * T;
		//
		//	DecoratorPosition = StartingScreenPos + LerpPosition;
		//}
		//else
		{
			DecoratorPosition = Position;
			//LUA_LOG("Moving to position %s with size %s", *this->DecoratorPosition.ToString(), *CachedDesiredSize.ToString());
		}
	}
}

void SLuaScriptEditorFileBrowser::Construct(const FArguments& InArgs)
{
	UUnrealLuaFileSystem* fileSystem = UUnrealLuaFileSystem::Get();
	
	fileSystem->GetOnFileSystemChangedDelegate().AddSP(this, &SLuaScriptEditorFileBrowser::NotifyFileSystemChanged);
	this->OnFileDoubleClicked = InArgs._OnFileDoubleClicked;
	this->OnTreeRebuilt = InArgs._OnFileTreeRebuilt;
	this->OnRequestDefaultScriptFile = InArgs._OnRequestDefaultScriptFile;
	this->ChildSlot 
	[
		SNew(SOverlay)
		+SOverlay::Slot()
		[
			SAssignNew(FileTreeView,STreeView<TSharedPtr<FUnrealLuaFileSystemEntry>>)
			.TreeItemsSource(&fileSystem->GetRootArrayRef())
			.OnGenerateRow_Raw(this, &SLuaScriptEditorFileBrowser::NotifyGenerateRow)
			.OnGetChildren(this, &SLuaScriptEditorFileBrowser::NotifyGetChildren)
			.SelectionMode(ESelectionMode::Single)
			.ClearSelectionOnClick(true)
			.OnSelectionChanged(this, &SLuaScriptEditorFileBrowser::NotifySelectionChanged)
			.OnContextMenuOpening(this, &SLuaScriptEditorFileBrowser::RequestContextMenu)
			.OnItemsRebuilt(this, &SLuaScriptEditorFileBrowser::NotifyFileTreeRebuilt)
			.OnMouseButtonClick(this, &SLuaScriptEditorFileBrowser::NotifyTreeItemClicked)
			.OnMouseButtonDoubleClick(this, &SLuaScriptEditorFileBrowser::NotifyTreeItemDoubleClicked)
			.Visibility(EVisibility::SelfHitTestInvisible)
			.OnExpansionChanged(this, &SLuaScriptEditorFileBrowser::NotifyItemExpansionChanged)
		]
		+SOverlay::Slot()
		[
			SNew(SUnrealLuaDraggableBoxOverlay)
			.IsDraggable(true)
			.Visibility(EVisibility::SelfHitTestInvisible)
			.Content()
			[
				SNew(SBox)
				.Content()
				[
					SNew(SColorBlock)
					.Color(FLinearColor{0.8,0.2,0.8,1})
				]
				.HeightOverride(20)
				.WidthOverride(20)
			]
		]
	];
	this->FileTreeView->SetIsRightClickScrollingEnabled(false);
}

void SLuaScriptEditorFileBrowser::RebuildFileTree()
{
	UUnrealLuaFileSystem* fileSystem = UUnrealLuaFileSystem::Get();
	//Refresh
	if( FileTreeView.IsValid() )
	{
		FileTreeView->RequestTreeRefresh();
		this->FileTreeView->SetSingleExpandedItem(fileSystem->GetRoot());
	}
}

TSharedPtr<FUnrealLuaFileSystemEntry> SLuaScriptEditorFileBrowser::GetSelectedDirectory() const
{
	if( FileTreeView.IsValid() )
	{
		auto SelectedItems = FileTreeView->GetSelectedItems();
		if( SelectedItems.Num() > 0 )
		{
			const TSharedPtr<FUnrealLuaFileSystemEntry>& selectedItem = SelectedItems[0];
			return selectedItem;
		}
	}

	return NULL;
}

void SLuaScriptEditorFileBrowser::SelectDirectory(const TSharedPtr<FUnrealLuaFileSystemEntry>& categoryToSelect)
{
	if( ensure( categoryToSelect.IsValid() ) )
	{
		FileTreeView->SetSelection( categoryToSelect );
	}
}

bool SLuaScriptEditorFileBrowser::IsItemExpanded(const TSharedPtr<FUnrealLuaFileSystemEntry> item) const
{
	return FileTreeView->IsItemExpanded( item );
}

void SLuaScriptEditorFileBrowser::NotifyFileTreeRebuilt()
{
	UUnrealLuaFileSystem* fileSystem = UUnrealLuaFileSystem::Get();
	this->OnTreeRebuilt.ExecuteIfBound(fileSystem->GetRootArrayRef());
}

TSharedRef<ITableRow> SLuaScriptEditorFileBrowser::NotifyGenerateRow(TSharedPtr<FUnrealLuaFileSystemEntry> treeItem, const TSharedRef<STableViewBase>& tableViewBase)
{
	if(!treeItem.IsValid())
	{
		return SNew( STableRow< TSharedPtr<FUnrealLuaFileSystemEntry> >, tableViewBase )
		[
			SNew(STextBlock)
			.Text( FText::AsCultureInvariant("THIS WAS NULL SOMEHOW") )
		];
	}
	if (treeItem->IsFile())
	{
		TWeakPtr<FUnrealLuaFileSystemEntry> weakPtr = treeItem;
		auto widget = SNew( STableRow<TSharedPtr<FUnrealLuaFileSystemEntry> >, tableViewBase )
		.Visibility(EVisibility::SelfHitTestInvisible)
			.OnDragDetected_Lambda([this, weakPtr](const FGeometry& geometry, const FPointerEvent& pointerEvent)
			{
				return this->NotifyFileBrowserItemDragDetected(geometry, pointerEvent, weakPtr);
			})
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant(treeItem->GetDisplayName()) )
			//.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12))
			//.ColorAndOpacity(FLinearColor(1,1,1,1))
			//.ShadowColorAndOpacity(FLinearColor::Black)
			.ShadowOffset(FIntPoint(-2, 2))
		];		
		return widget;
	}
	else
	{
		TWeakPtr<FUnrealLuaFileSystemEntry> weakPtr = treeItem;
		
		TWeakPtr<SColorBlock> background = nullptr;
		TSharedRef<SOverlay> horizontalBox = SNew(SOverlay)
			+SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SAssignNew(background, SColorBlock)
				.Color(FLinearColor(0.3, 0.3, 0.7, 0.3f))
				.CornerRadius(FVector4{3, 3, 3, 3})
				.Visibility(EVisibility::Collapsed)
			]
			+ SOverlay::Slot()
			[
				SNew(SHorizontalBox)
				.Visibility(EVisibility::HitTestInvisible)
				+ SHorizontalBox::Slot()
				[
					SNew(SColorBlock)
					.Color(FLinearColor(0.6f, 0.6f, 0.1f))
					.Size(FVector2D(8.f, 8.f))
					.CornerRadius(FVector4{3, 3, 3, 3})
					.Visibility(EVisibility::HitTestInvisible)
				]
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				+ SHorizontalBox::Slot()
				[
					SNew(STextBlock)
					.Text(FText::AsCultureInvariant(treeItem->GetDisplayName()))
					//.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12))
					//.ColorAndOpacity(FLinearColor(1,1,1,1))
					//.ShadowColorAndOpacity(FLinearColor::Black)
					.ShadowOffset(FIntPoint(-2, 2))
					.Visibility(EVisibility::HitTestInvisible)
				]
				.FillWidth(1)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
			];
		TSharedRef<STableRow<TSharedPtr<FUnrealLuaFileSystemEntry>>> rowWidget = SNew(STableRow<TSharedPtr<FUnrealLuaFileSystemEntry>>, tableViewBase)
			.Visibility(EVisibility::Visible)
			.OnAcceptDrop_Raw(this, &SLuaScriptEditorFileBrowser::NotifyDropOnTreeItem)
			.OnDragEnter_Lambda([background](const FDragDropEvent& dragDropEvent)
			{
				if (background.IsValid() && dragDropEvent.GetOperation()->IsOfType<FUnrealLuaFileBrowserDragDropOp>())
				{
					background.Pin()->SetVisibility(EVisibility::SelfHitTestInvisible);
				}
			})
			.OnDragLeave_Lambda([background](const FDragDropEvent& dragDropEvent)
			{
				if (background.IsValid())
				{
					background.Pin()->SetVisibility(EVisibility::Collapsed);
				}
			})
			.OnDragDetected_Lambda([this, weakPtr](const FGeometry& geometry, const FPointerEvent& pointerEvent)
			{
				return this->NotifyFileBrowserItemDragDetected(geometry, pointerEvent, weakPtr);
			})
			[
				horizontalBox
			];
		return rowWidget;
	}
}

void SLuaScriptEditorFileBrowser::NotifyGetChildren(TSharedPtr<FUnrealLuaFileSystemEntry> treeItem, TArray<TSharedPtr<FUnrealLuaFileSystemEntry>>& outChildren)
{
	const TArray<TSharedPtr<FUnrealLuaFileSystemEntry>>& SubCategories = treeItem->GetSubDirectoriesAndFiles();
	outChildren.Append( SubCategories );
}

void SLuaScriptEditorFileBrowser::NotifySelectionChanged(TSharedPtr<FUnrealLuaFileSystemEntry> treeItem, ESelectInfo::Type arg)
{
	if (treeItem.IsValid())
	{
		if (treeItem->IsFile())
		{
			//FString fullPath = treeItem->BuildFullPath();
		}	
	}
}

void SLuaScriptEditorFileBrowser::NotfiyDeleteItem(TWeakPtr<FUnrealLuaFileSystemEntry> toDelete)
{
	if (toDelete.IsValid())
	{
		TSharedPtr<FUnrealLuaFileSystemEntry> ptr = toDelete.Pin();
		
		FMenuBuilder menuBuilder(true, nullptr);
		
		menuBuilder.SetSearchable(false);
	
		menuBuilder.BeginSection(NAME_None, {});

		const FString& path = toDelete.Pin()->GetFullPath();
		FString fullPath = FPaths::ConvertRelativePathToFull(path);
		TSharedRef<SVerticalBox> menuWidget = SNew(SVerticalBox);
		menuWidget->AddSlot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant("Confirm Delete") )
			.Justification(ETextJustify::Center)
		];
		
		menuWidget->AddSlot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(10,2,10,2)
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant(*fullPath) )
			.Justification(ETextJustify::Center)
		];
		
		TSharedPtr<SButton> yesButton = nullptr;
		TSharedRef<SHorizontalBox> buttonBox = SNew(SHorizontalBox);
		buttonBox->AddSlot()
		.AutoWidth()
		[
			SAssignNew(yesButton, SButton)
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("Yes") )
				.Visibility(EVisibility::SelfHitTestInvisible)
			]
			.OnClicked_Lambda([toDelete, menuWidget, this]() { this->NotifyCommitDelete(toDelete, ETextCommit::Type::OnEnter); FSlateApplication::Get().DismissMenuByWidget(menuWidget); return FReply::Handled();})
		];
		buttonBox->AddSlot()
		.AutoWidth()
		[
			SNew(SButton)
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("No") )
				.Visibility(EVisibility::SelfHitTestInvisible)
			]
			.OnClicked_Lambda([menuWidget](){ FSlateApplication::Get().DismissMenuByWidget(menuWidget); return FReply::Handled(); })
		];
		
		menuWidget->AddSlot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			buttonBox
		];

		menuBuilder.AddWidget(menuWidget, FText::GetEmpty(), true, false);

		menuBuilder.EndSection();


		// 4. Push the menu at the current cursor location
		TSharedPtr<SWidget> MenuWidget = menuBuilder.MakeWidget();
		FWidgetPath WidgetPath = FWidgetPath();
		
		FSlateApplication::Get().PushMenu(
			this->FileTreeView.ToSharedRef(),
			WidgetPath,
			MenuWidget.ToSharedRef(),
			FSlateApplication::Get().GetCursorPos() - FVector2D(50,50), 
			FPopupTransitionEffect::ContextMenu, false
		);
	
		FSlateApplication::Get().SetKeyboardFocus(yesButton);
		FSlateApplication::Get().SetAllUserFocus(yesButton);
	}
}

void SLuaScriptEditorFileBrowser::NotifyCommitDelete(const TWeakPtr<FUnrealLuaFileSystemEntry>& toDelete, ETextCommit::Type Type)
{
	if (!toDelete.IsValid() || Type != ETextCommit::Type::OnEnter)
	{
		return;
	}
	
	UUnrealLuaFileSystem::Get()->DeleteFileOrDirectory(toDelete.Pin().ToSharedRef());
}

void SLuaScriptEditorFileBrowser::NotifyCreateNewFile(const TWeakPtr<FUnrealLuaFileSystemEntry>& parentDirectoryPtr)
{
	if (!parentDirectoryPtr.IsValid())
	{
		return;
	}

	FMenuBuilder menuBuilder(true, nullptr);


	menuBuilder.SetSearchable(false);
	
	menuBuilder.BeginSection(NAME_None, {});

	TSharedRef<SEditableTextBox> widget = SNew(SEditableTextBox)
		.OnTextCommitted_Lambda([this, parentDirectoryPtr](const FText& text, ETextCommit::Type type)
		{
			this->NotifyCommitNewFile(parentDirectoryPtr, text, type);
		})
		.MinDesiredWidth(100);

	menuBuilder.AddWidget(widget, FText::AsCultureInvariant("New Filename"), true, false);

	menuBuilder.EndSection();


	// 4. Push the menu at the current cursor location
	TSharedPtr<SWidget> MenuWidget = menuBuilder.MakeWidget();
	FWidgetPath WidgetPath = FWidgetPath();

	FSlateApplication::Get().PushMenu(
		this->FileTreeView.ToSharedRef(),
		WidgetPath,
		MenuWidget.ToSharedRef(),
		FSlateApplication::Get().GetCursorPos() - FVector2D(10,10), 
		FPopupTransitionEffect::ContextMenu, false
	);
	
	FSlateApplication::Get().SetKeyboardFocus(widget);
	FSlateApplication::Get().SetAllUserFocus(widget);
}

void SLuaScriptEditorFileBrowser::NotifyCommitNewFile(const TWeakPtr<FUnrealLuaFileSystemEntry>& parentDirectoryPtr, const FText& fileNameText, ETextCommit::Type type)
{
	if (type != ETextCommit::Type::OnEnter)
	{
		UE_LOG(LogTemp, Log, TEXT("NOt enter"))
		return;
	}
	if (!parentDirectoryPtr.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("Parent dir not valid"))
		return;
	}
	TSharedPtr<FUnrealLuaFileSystemEntry> parentDirectory = parentDirectoryPtr.Pin();
	
	bool success = UUnrealLuaFileSystem::Get()->CreateNewFile(parentDirectory.ToSharedRef(), fileNameText.ToString());
	if (success)
	{
		UE_LOG(LogTemp, Log, TEXT("success"))
		this->FileTreeView->SetItemExpansion(parentDirectory, true);
	}
}

void SLuaScriptEditorFileBrowser::NotifyCreateNewDirectory(const TWeakPtr<FUnrealLuaFileSystemEntry>& parentDirectoryPtr)
{
	if (!parentDirectoryPtr.IsValid())
	{
		return;
	}

	FMenuBuilder menuBuilder(true, nullptr);

	menuBuilder.SetSearchable(false);
	
	menuBuilder.BeginSection(NAME_None, {});

	TSharedRef<SEditableTextBox> widget = SNew(SEditableTextBox)
		.OnTextCommitted_Lambda([this, parentDirectoryPtr](const FText& text, ETextCommit::Type type)
		{
			this->NotifyCommitNewDirectory(parentDirectoryPtr, text, type);
		})
		.MinDesiredWidth(100); 
	
	menuBuilder.AddWidget(widget, FText::AsCultureInvariant("New Directory"), true, false);

	menuBuilder.EndSection();


	// 4. Push the menu at the current cursor location
	TSharedPtr<SWidget> MenuWidget = menuBuilder.MakeWidget();
	FWidgetPath WidgetPath = FWidgetPath();

	FSlateApplication::Get().PushMenu(
		this->FileTreeView.ToSharedRef(),
		WidgetPath,
		MenuWidget.ToSharedRef(),
		FSlateApplication::Get().GetCursorPos() - FVector2D(10,10), 
		FPopupTransitionEffect::ContextMenu, false
	);
	
	FSlateApplication::Get().SetKeyboardFocus(widget);
	FSlateApplication::Get().SetAllUserFocus(widget);
}

void SLuaScriptEditorFileBrowser::NotifyCommitNewDirectory(const TWeakPtr<FUnrealLuaFileSystemEntry>& parentDirectoryPtr, const FText& directoryNameText, ETextCommit::Type type)
{
	if (!parentDirectoryPtr.IsValid() || type != ETextCommit::Type::OnEnter)
	{
		return;
	}
	
	bool success = UUnrealLuaFileSystem::Get()->CreateNewDirectory(parentDirectoryPtr.Pin().ToSharedRef(), directoryNameText.ToString());
	
	if (success)
	{
		this->FileTreeView->SetItemExpansion(parentDirectoryPtr.Pin(), true);
	}
}

void SLuaScriptEditorFileBrowser::NotifyRequestCreateNewDefaultScriptFile(const TWeakPtr<FUnrealLuaFileSystemEntry>& parentDirectoryPtr) const
{
	this->OnRequestDefaultScriptFile.ExecuteIfBound(parentDirectoryPtr.Pin());
}

void SLuaScriptEditorFileBrowser::NotifyItemExpansionChanged(TSharedPtr<FUnrealLuaFileSystemEntry> treeItem, bool bIsExpanded)
{
	//if (bIsExpanded)
	//{
	//	verify(treeItem.IsValid() && treeItem->IsDirectory())
	//}
	
	//TSet<TSharedPtr<FUnrealLuaFileSystemEntry>> expandedTreeItems;
	//this->FileTreeView->GetExpandedItems(expandedTreeItems);
	//
	//UUnrealLuaFileSystem::Get()->NotifyItemExpansionChanged(treeItem, bIsExpanded);
}

void SLuaScriptEditorFileBrowser::NotifyFileSystemChanged()
{
	if( FileTreeView.IsValid() )
	{
		FileTreeView->RequestTreeRefresh();
	}
}

void SLuaScriptEditorFileBrowser::NotifyTreeItemClicked(TSharedPtr<FUnrealLuaFileSystemEntry> entry)
{
	//LUA_LOG("clicked %s", *entry->GetFullPath());
}

void SLuaScriptEditorFileBrowser::NotifyTreeItemDoubleClicked(TSharedPtr<FUnrealLuaFileSystemEntry> entry)
{
	if (entry->IsFile())
	{
		this->OnFileDoubleClicked.ExecuteIfBound(entry);
	}
	else if (entry->IsDirectory() && entry->HasChildren())
	{
		bool isExpanded = this->FileTreeView->IsItemExpanded(entry);
		this->FileTreeView->SetItemExpansion(entry, !isExpanded);
	}
}

void SLuaScriptEditorFileBrowser::OnLuaFileDoubleClicked(const TWeakPtr<FUnrealLuaFileSystemEntry>& entry)
{
	checkNoEntry();
	if (!entry.IsValid())
	{
		return;
	}
	this->OnFileDoubleClicked.ExecuteIfBound(entry.Pin());
}

TSharedPtr<SWidget> SLuaScriptEditorFileBrowser::RequestContextMenu()
{
	TArray<TSharedPtr<FUnrealLuaFileSystemEntry>> selecteditems = this->FileTreeView->GetSelectedItems();
	if (selecteditems.IsEmpty())
	{
		LUA_LOG("No item selected")
		return nullptr;
	}
	TSharedPtr<FUnrealLuaFileSystemEntry> item = selecteditems[0];

	if (item->IsFile())
	{
		TWeakPtr<FUnrealLuaFileSystemEntry> weakItemPtr = item;
		FMenuBuilder menuBuilder{true, nullptr};
		menuBuilder.BeginSection(NAME_None, {});
		{
			if (item->CanBeDeleted())
			{
				menuBuilder.AddMenuEntry(LOCTEXT("Deletelabel", "Delete"),LOCTEXT("DeleteFileTip", "Delete File"), FSlateIcon(),FUIAction(FExecuteAction::CreateSPLambda(this, [this, weakItemPtr] (){ this->NotfiyDeleteItem(weakItemPtr);})));
			}
		}
		menuBuilder.EndSection();
		return menuBuilder.MakeWidget();
	}
	else
	{
		verify(item->IsDirectory());
		
		TWeakPtr<FUnrealLuaFileSystemEntry> weakItemPtr = item;
		FMenuBuilder menuBuilder{true, nullptr};
		menuBuilder.BeginSection(NAME_None, {});
		{
			if (item->IsInDefaultScriptDirectory())
			{
				menuBuilder.AddMenuEntry(LOCTEXT("NewFileLabel", "New Default Script"),LOCTEXT("NewFileTip", "Create new default Lua script"), FSlateIcon(),FUIAction(FExecuteAction::CreateSPLambda(this, [this, weakItemPtr] (){ this->NotifyRequestCreateNewDefaultScriptFile(weakItemPtr);})));
			}
			else
			{
				menuBuilder.AddMenuEntry(LOCTEXT("NewFileLabel", "New File"),LOCTEXT("NewFileTip", "Create new file"), FSlateIcon(),FUIAction(FExecuteAction::CreateSPLambda(this, [this, weakItemPtr] (){ this->NotifyCreateNewFile(weakItemPtr);})));	
				menuBuilder.AddMenuEntry(LOCTEXT("NewDirectoryLabel", "New Directory"),LOCTEXT("NewDirectoryTip", "Create new directory"), FSlateIcon(),FUIAction(FExecuteAction::CreateSPLambda(this, [this, weakItemPtr] (){ this->NotifyCreateNewDirectory(weakItemPtr);})));
			}
			if (item->CanBeDeleted())
			{
				menuBuilder.AddMenuEntry(LOCTEXT("Deletelabel", "Delete"),LOCTEXT("DeleteFileTip", "Delete Directory"), FSlateIcon(),FUIAction(FExecuteAction::CreateSPLambda(this, [this, weakItemPtr] (){ this->NotfiyDeleteItem(weakItemPtr);})));
			}
		}
		menuBuilder.EndSection();
		return menuBuilder.MakeWidget();
	}
	return nullptr;
}

void SLuaScriptEditorFileBrowser::ExpandItemChain(const TWeakPtr<FUnrealLuaFileSystemEntry>& item)
{
	if (!item.IsValid())
	{
		return;
	}
	TSharedPtr<FUnrealLuaFileSystemEntry> current = item.Pin();
	while (current.IsValid())
	{
		this->FileTreeView->SetItemExpansion(current, true);
		current = current->GetParentFolder();
	}
}

FReply SLuaScriptEditorFileBrowser::NotifyFileBrowserItemDragDetected(const FGeometry& geometry, const FPointerEvent& pointerEvent, const TWeakPtr<FUnrealLuaFileSystemEntry>& fileItem)
{
	if (!fileItem.IsValid())
	{
		return FReply::Handled();
	}
	// Calculate your drag offset here if necessary
	
	FVector2D dragOffset = geometry.AbsoluteToLocal(pointerEvent.GetScreenSpacePosition());
	
	auto file = fileItem.Pin();
	TSharedPtr<SWidget> widget = nullptr;
	if (file->IsFile())
	{
		widget = SNew(SBorder)
		.Visibility(EVisibility::SelfHitTestInvisible)
		.RenderOpacity(1.0f)
		.Content()
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SColorBlock)
				.Color(FLinearColor(1, 1, 1, 0.2f))
				.CornerRadius(FVector4{3, 3, 3, 3})
				.Visibility(EVisibility::HitTestInvisible)
			]
			+SOverlay::Slot()
			[
				SNew(SHorizontalBox)
				.Visibility(EVisibility::HitTestInvisible)
				+ SHorizontalBox::Slot()
				[
					SNew(STextBlock)
					.Text(FText::AsCultureInvariant(file->GetDisplayName()))
					//.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12))
					//.ColorAndOpacity(FLinearColor(1,1,1,1))
					//.ShadowColorAndOpacity(FLinearColor::Black)
					.ShadowOffset(FIntPoint(-2, 2))
					.Visibility(EVisibility::HitTestInvisible)
				]
				.FillWidth(1)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
			]
		];
	}
	else if (file->IsDirectory())
	{
		widget = SNew(SBorder)
			.Visibility(EVisibility::SelfHitTestInvisible)
			.RenderOpacity(1.0f)
			.Content()
			[
				SNew(SOverlay)
				+SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SColorBlock)
					.Color(FLinearColor(1, 1, 1, 0.2f))
					.CornerRadius(FVector4{3, 3, 3, 3})
					.Visibility(EVisibility::HitTestInvisible)
				]
				+SOverlay::Slot()
				[
					SNew(SHorizontalBox)
					.Visibility(EVisibility::HitTestInvisible)
					+ SHorizontalBox::Slot()
					[
						SNew(SColorBlock)
						.Color(FLinearColor(0.6f, 0.6f, 0.1f))
						.Size(FVector2D(8.f, 8.f))
						.CornerRadius(FVector4{3, 3, 3, 3})
						.Visibility(EVisibility::HitTestInvisible)
					]
					.AutoWidth()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					+ SHorizontalBox::Slot()
					[
						SNew(STextBlock)
						.Text(FText::AsCultureInvariant(file->GetDisplayName()))
						//.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12))
						//.ColorAndOpacity(FLinearColor(1,1,1,1))
						//.ShadowColorAndOpacity(FLinearColor::Black)
						.ShadowOffset(FIntPoint(-2, 2))
						.Visibility(EVisibility::HitTestInvisible)
					]
					.FillWidth(1)
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
				]
			];
	}

	//verify(this->DragDropWidget.IsValid())
	

	TSharedRef<FUnrealLuaFileBrowserDragDropOp> DragOperation = FUnrealLuaFileBrowserDragDropOp::New(widget.ToSharedRef(), dragOffset);
	DragOperation->FileSystemEntry = fileItem;
	//DragOperation->SetDecoratorVisibility(true);
	return FReply::Handled().BeginDragDrop(DragOperation);
}

FReply SLuaScriptEditorFileBrowser::NotifyDropOnTreeItem(const FDragDropEvent& dragDropEvent, EItemDropZone itemDropZone, TSharedPtr<FUnrealLuaFileSystemEntry> targetEntry)
{
	TSharedPtr<FDragDropOperation> opBase = dragDropEvent.GetOperation();
	if (opBase->IsOfType<FUnrealLuaFileBrowserDragDropOp>())
	{
		TSharedPtr<FUnrealLuaFileBrowserDragDropOp> op = dragDropEvent.GetOperationAs<FUnrealLuaFileBrowserDragDropOp>();
		
		UUnrealLuaFileSystem::Get()->MoveItemToDirectory(op->FileSystemEntry, targetEntry);
	}
	return FReply::Handled();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE