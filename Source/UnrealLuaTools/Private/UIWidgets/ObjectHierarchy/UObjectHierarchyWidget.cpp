// Fill out your copyright notice in the Description page of Project Settings.


#include "UIWidgets/ObjectHierarchy/UObjectHierarchyWidget.h"

#include "SlateOptMacros.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Widgets/SOverlay.h"
#include "Engine/GameInstance.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Widgets/Layout/SScrollBox.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SUObjectHierarchyWidget::Construct(const FArguments& InArgs)
{
	this->OnItemDoubleClicked = InArgs._OnItemDoubleClicked;
	this->ChildSlot 
	[
		SNew(SOverlay)
		+SOverlay::Slot()
		[
			SNew(SVerticalBox)
			//+ SVerticalBox::Slot()
			//[
			//	SNew(STextBlock)
			//	.Text(FText::AsCultureInvariant("ObjectTree"))
			//]
			//.AutoHeight()
			+ SVerticalBox::Slot()
			[
				SNew(SBox)
				.WidthOverride(300)
				.Content()
				[
					SAssignNew(ObjectsTreeView,STreeView<TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem>>)
					.TreeItemsSource(&RootObject)
					.OnGenerateRow_Raw(this, &SUObjectHierarchyWidget::NotifyGenerateRow)
					.OnGetChildren(this, &SUObjectHierarchyWidget::NotifyGetChildren)
					.SelectionMode(ESelectionMode::Single)
					.ClearSelectionOnClick(true)
					.OnSelectionChanged(this, &SUObjectHierarchyWidget::NotifySelectionChanged)
					.OnContextMenuOpening(nullptr)
					.OnItemsRebuilt(this, &SUObjectHierarchyWidget::NotifyObjectTreeRebuilt)
					.OnMouseButtonClick(this, &SUObjectHierarchyWidget::NotifyTreeItemClicked)
					.OnMouseButtonDoubleClick(this, &SUObjectHierarchyWidget::NotifyTreeItemDoubleClicked)
					.Visibility(EVisibility::SelfHitTestInvisible)
					.OnExpansionChanged(this, &SUObjectHierarchyWidget::NotifyItemExpansionChanged)
					.OnItemToString_Debug_Lambda([](TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem> item) { return item->ObjectName.ToString();})	
				]
			]
			.FillHeight(1)
		]
	];
}

void SUObjectHierarchyWidget::SetWatchedActor(AActor* actor)
{
	this->ObjectsList.Empty();
	this->RootObject.Empty();
	if (IsValid(actor))
	{
		this->RootObject.Emplace(MakeShared<FUnrealLuaWatchedObjectHierarchyItem>(actor->GetFName(), actor, true));
		TArray<UObject*> allObjects{};
		GetObjectsWithOuter(actor, allObjects);
		for (auto obj : allObjects)
		{
			auto item = MakeShared<FUnrealLuaWatchedObjectHierarchyItem>(obj->GetFName(), obj, false);
			this->ObjectsList.Emplace(item);
			TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem> outer = this->GetParentItemForSubObject(obj);
			if (outer.IsValid())
			{
				outer->Children.Emplace(item);
			}
		}
		this->ObjectsTreeView->RequestListRefresh();
	}
	else
	{
		this->ObjectsTreeView->RequestListRefresh();
	}
}

void SUObjectHierarchyWidget::SetWatchedWorld(UWorld* world)
{
	this->ObjectsList.Empty();
	this->RootObject.Empty();
	if (IsValid(world))
	{
		auto worldItem = MakeShared<FUnrealLuaWatchedObjectHierarchyItem>(world->GetFName(), world, true);
		this->RootObject.Emplace(worldItem);
		
		UGameInstance* gi = world->GetGameInstance();
		if (gi)
		{
			auto gameInstanceItem = MakeShared<FUnrealLuaWatchedObjectHierarchyItem>(gi->GetFName(), gi, true);
			this->ObjectsList.Emplace(gameInstanceItem);
		}
		
		TArray<AActor*> actors;
		for( int32 LevelIndex=0; LevelIndex<world->GetNumLevels(); LevelIndex++ )
		{
			ULevel* Level = world->GetLevel(LevelIndex);
			actors.Reserve(Level->Actors.Num());
			for (auto actor : Level->Actors)
			{
				if (IsValid(actor))
				{
					actors.Add(actor);
				}
			}
		}
		
		for (AActor* actorInWorld : actors)
		{
			auto item = MakeShared<FUnrealLuaWatchedObjectHierarchyItem>(actorInWorld->GetFName(), actorInWorld, true);
			this->ObjectsList.Emplace(item);
			worldItem->Children.Add(item);
		}
		this->ObjectsTreeView->RequestListRefresh();
	}
	else
	{
		this->ObjectsTreeView->RequestListRefresh();
	}
}

void SUObjectHierarchyWidget::SetWatchedGameInstance(UGameInstance* gameInstance)
{
	this->ObjectsList.Empty();
	this->RootObject.Empty();
	if (IsValid(gameInstance))
	{
		auto gameInstanceItem = MakeShared<FUnrealLuaWatchedObjectHierarchyItem>(gameInstance->GetFName(), gameInstance, true);
		this->RootObject.Emplace(gameInstanceItem);
		
		UWorld* world = gameInstance->GetWorld();
		auto worldItem = MakeShared<FUnrealLuaWatchedObjectHierarchyItem>(world->GetFName(), world, true);
		this->ObjectsList.Emplace(worldItem);
		
		for (UGameInstanceSubsystem* subsystem : gameInstance->GetSubsystemArrayCopy<UGameInstanceSubsystem>())
		{
			auto subsystemitem = MakeShared<FUnrealLuaWatchedObjectHierarchyItem>(subsystem->GetFName(), subsystem, false);
			this->ObjectsList.Emplace(subsystemitem);
		}
		
		this->ObjectsTreeView->RequestListRefresh();
	}
	else
	{
		this->ObjectsTreeView->RequestListRefresh();
	}
}

void SUObjectHierarchyWidget::SetWatchedWidget(UUserWidget* widget)
{
	
}

void SUObjectHierarchyWidget::SetWatchedObject(UObject* object)
{
	this->ObjectsList.Empty();
	this->RootObject.Empty();
	if(!object)
	{
		this->ObjectsTreeView->RequestListRefresh();
		return;
	}
	if (object->IsA<AActor>())
	{
		this->SetWatchedActor(Cast<AActor>(object));
		this->ObjectsTreeView->SetSingleExpandedItem(this->RootObject.Last());
	}
	else if (object->IsA<UWorld>())
	{
		this->SetWatchedWorld(Cast<UWorld>(object));
		this->ObjectsTreeView->SetSingleExpandedItem(this->RootObject.Last());
	}
}

TSharedRef<ITableRow> SUObjectHierarchyWidget::NotifyGenerateRow(TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem> item, const TSharedRef<STableViewBase>& table)
{
	if(!item.IsValid())
	{
		return SNew( STableRow< TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem> >, table )
		[
			SNew(STextBlock)
			.Text( FText::AsCultureInvariant("THIS WAS NULL SOMEHOW") )
		];
	}
	TWeakPtr<FUnrealLuaWatchedObjectHierarchyItem> weakPtr = item;
	auto widget = SNew( STableRow<TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem> >, table )
	.Visibility(EVisibility::SelfHitTestInvisible)
	[
		SNew(STextBlock)
		.Text(FText::AsCultureInvariant(item->ObjectName.ToString()) )
		//.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12))
		//.ColorAndOpacity(FLinearColor(1,1,1,1))
		//.ShadowColorAndOpacity(FLinearColor::Black)
		.ShadowOffset(FIntPoint(-2, 2))
	];		
	return widget;
}

void SUObjectHierarchyWidget::NotifyGetChildren(TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem> parentItem, TArray<TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem>>& objectList)
{
	objectList = parentItem->Children;
}

void SUObjectHierarchyWidget::NotifySelectionChanged(TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem> selectedItem, ESelectInfo::Type selectionMethod)
{
	
}

void SUObjectHierarchyWidget::NotifyObjectTreeRebuilt()
{
}

void SUObjectHierarchyWidget::NotifyTreeItemClicked(TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem> clickedItem)
{
	
}

void SUObjectHierarchyWidget::NotifyTreeItemDoubleClicked(TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem> doubleClickedItem) const
{
	this->OnItemDoubleClicked.ExecuteIfBound(doubleClickedItem);
}

void SUObjectHierarchyWidget::NotifyItemExpansionChanged(TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem> item, bool bIsExpanded)
{
	
}

TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem> SUObjectHierarchyWidget::GetParentItemForSubObject(UObject* object) const
{
	UObject* outer = object->GetOuter();
	if (outer)
	{
		if (outer == this->RootObject.Last()->Object)
		{
			return this->RootObject.Last();	
		}
		auto found = this->ObjectsList.FindByPredicate([outer](const TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem>& item)
		{
			return outer == item->Object;
		});
		if (found)
		{
			return *found;
		}
	}
	return nullptr;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
