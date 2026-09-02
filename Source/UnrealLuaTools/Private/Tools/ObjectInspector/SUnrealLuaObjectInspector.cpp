// Fill out your copyright notice in the Description page of Project Settings.


#include "Tools/ObjectInspector/SUnrealLuaObjectInspector.h"

#include "UnrealEngine.h"
#include "BlueprintSupport/UnrealLuaUtility.h"
#include "Components/VerticalBox.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "LuaContext/LuaScripts/LoadedLuaScriptCollection.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Session/UnrealLuaToolsSession.h"
#include "Slate/SObjectWidget.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"
#include "Tools/LuaScriptEditor/SLuaScriptEditor.h"
#include "Tools/LuaScriptEditor/UnrealLuaScriptEditorTool.h"
#include "Tools/ObjectInspector/SLuaScriptValueEditor.h"
#include "Tools/ObjectInspector/SRunLuaScriptWidget.h"
#include "ToolWidgets/SLuaNewDefaultScriptEditorWindow.h"
#include "UIWidgets/SLuaScriptValueListElementWidget.h"
#include "UIWidgets/SMultiLineEditableTextBoxEx.h"
#include "UIWidgets/SSubobjectListWidget.h"
#include "UIWidgets/ObjectHierarchy/UObjectHierarchyWidget.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"
#include "Widgets/SWindow.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"


void SUnrealLuaObjectInspector::Construct(const FArguments& InArgs)
{
	SGamescreenDockableWindowWidget::Construct(SGamescreenDockableWindowWidget::FArguments()
		.ExternalWindowAnchors(InArgs._ExternalWindowAnchors)
		.ExternalWindowSize(InArgs._ExternalWindowSize)
		.ExternalWindowPosition(InArgs._ExternalWindowPosition)
		.GameScreenAnchors(InArgs._GameScreenAnchors)
		.BackgroundColor(InArgs._BackgroundColor)
		.Title(InArgs._Title)
		.Session(InArgs._Session)
		.GameScreenAlignment(InArgs._GameScreenAlignment)
		.InitiallyHidden(InArgs._InitiallyHidden)
		.StartAsWindow(InArgs._StartAsWindow)
		.DraggableInGameScreen(false)
	);
	
	this->OnRequestEditLuaScriptValue = InArgs._OnRequestEditLuaScriptValue;
	this->OnRequestActorSelection = InArgs._OnRequestActorSelection;
	
	this->MainContentSizeBox->SetWidthOverride(250);
	this->WindowBodyVBox->AddSlot()
	[
		SNew(SSeparator)
		.SeparatorImage(FAppStyle::GetBrush("Menu.Separator"))
		.Orientation(Orient_Horizontal)
	]
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Center)
	.AutoHeight()
	.Padding(10.f);
	
	this->WindowBodyVBox->AddSlot()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			SNew(SButton)
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("Select Actor"))
				.Visibility(EVisibility::HitTestInvisible)
			]
			.Visibility(EVisibility::Visible)
			.OnClicked(this, &SUnrealLuaObjectInspector::NotifySelectActorButtonClicked)						
		]
		+ SHorizontalBox::Slot()
		[
			SNew(SButton)
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("Select World"))
				.Visibility(EVisibility::HitTestInvisible)
			]
			.Visibility(EVisibility::Visible)
			.OnClicked(this, &SUnrealLuaObjectInspector::NotifySelectWorldButtonClicked)						
		]
	]
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Center)
	.AutoHeight()
	.Padding(10.f);
	
	this->WindowBodyVBox->AddSlot()
	[
		SNew(SSeparator)
		.SeparatorImage(FAppStyle::GetBrush("Menu.Separator"))
		.Orientation(Orient_Horizontal)
	]
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Center)
	.AutoHeight()
	.Padding(10.f);
	
	//this->MainContentGrid->AddSlot(0,1)
	//[
	//	SAssignNew(SubobjectsBrowser, SUObjectHierarchyWidget)
	//	.OnItemDoubleClicked_Raw(this, &SUnrealLuaObjectInspector::NotifyHierarchyItemSelected)
	//]
	//.Padding(FMargin(1))
	//.VAlign(VAlign_Top)
	//.HAlign(HAlign_Right);;
	
	this->WindowBodyVBox->AddSlot()
	[
		SAssignNew(WatchedObjectOwnerLabel, STextBlock)
		.Text(FText::AsCultureInvariant("No main object selected"))	
	]
	.Padding(FMargin(10))
	.AutoHeight()
	.VAlign(VAlign_Top)
	.HAlign(HAlign_Center);
	
	TSharedPtr<SVerticalBox> ObjectDetailsVBox = nullptr;
	this->WindowBodyVBox->AddSlot()
	[
		SNew(SSplitter)
		.Visibility(EVisibility::Visible)
		.Orientation(EOrientation::Orient_Vertical)
		+ SSplitter::Slot()
		.MinSize(10)
		.Value(0.2f)
		//.SizeRule(SSplitter::FractionOfParent)
		[
			SNew(SBorder)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.FillHeight(1)
				.VAlign(VAlign_Fill)
				.HAlign(HAlign_Fill)
				[
					SAssignNew(SubobjectsBrowser, SUObjectHierarchyWidget)
					.OnItemDoubleClicked_Raw(this, &SUnrealLuaObjectInspector::NotifyHierarchyItemSelected)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.VAlign(VAlign_Top)
				.HAlign(HAlign_Fill)
				[
					SAssignNew(ParentHBox, SHorizontalBox)
					+ SHorizontalBox::Slot()
					[
						SAssignNew(SelectParentButton, SButton)
						[
							SAssignNew(OuterObjectText, STextBlock)
							.Text(FText::AsCultureInvariant("No outer"))
							.Visibility(EVisibility::HitTestInvisible)
						]
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						.Visibility(EVisibility::Visible)
						.OnClicked(this, &SUnrealLuaObjectInspector::NotifySelectParentButtonClicked)	
					]
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Bottom)
				]
			]
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
		]
		+ SSplitter::Slot()
		.Value(0.8f)
		.MinSize(10)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			.AutoHeight()
			.Padding(FMargin(0,0,0,0))
			[
				SNew(SSeparator)
				.SeparatorImage(FAppStyle::GetBrush("Menu.Separator"))
				.Orientation(Orient_Horizontal)
			]
			+ SVerticalBox::Slot()
			.Padding(FMargin(0,5,0,5))
			.AutoHeight()
			.VAlign(VAlign_Top)
			.HAlign(HAlign_Center)
			[
				SAssignNew(WatchedObjectLabel, STextBlock)
				.Text(FText::AsCultureInvariant("No object selected"))				
			]
			+ SVerticalBox::Slot()
			.FillHeight(1)
			.Padding(FMargin(0,0,0,5))
			.VAlign(VAlign_Fill)
			.HAlign(HAlign_Fill)
			[
				SNew(SScrollBox)
				.Orientation(EOrientation::Orient_Vertical)
				+ SScrollBox::Slot()
				[
					SAssignNew(ObjectDetailsVBox, SVerticalBox)
				]
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				.FillSize(1)	
			]
		]
	]
	.FillHeight(1)
	.Padding(FMargin(10))
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill);
		
	ObjectDetailsVBox->AddSlot()
	.Padding(0,2,0,2)
	[
		SNew(SExpandableArea)
		.AreaTitle(FText::AsCultureInvariant("Lua Script Values"))
		.Padding(0.0f)
		.InitiallyCollapsed(true)
		.BodyContent()
		[
			SAssignNew(LuaValuesListBorder, SBorder)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Top)
				[
					SAssignNew(LuaValuesListScrollBox, SScrollBox)
					.Orientation(EOrientation::Orient_Vertical)
				]
				+SVerticalBox::Slot()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					[
						SNew(SButton)
						[
							SNew(STextBlock)
							.Text(FText::AsCultureInvariant("New"))
							.Visibility(EVisibility::SelfHitTestInvisible)
						]
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.OnClicked(this, &SUnrealLuaObjectInspector::NotifyNewScriptValueButtonClicked)
					]
					.AutoWidth()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
				]
				.AutoHeight()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
			]
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
		]
	]
	.AutoHeight()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill);
	
	ObjectDetailsVBox->AddSlot()
	.Padding(0,2,0,2)
	[
		SNew(SExpandableArea)
		.AreaTitle(FText::AsCultureInvariant("Lua Info"))
		.Padding(0.0f)
		.InitiallyCollapsed(true)
		.BodyContent()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			[
				SAssignNew(CreateDefaultLuaScriptButton, SButton)
				.OnClicked_Raw(this, &SUnrealLuaObjectInspector::NotifyCreateDefaultLuaScriptButtonClicked)
				[
					SNew(STextBlock)
					.Text(FText::AsCultureInvariant("Create Default Lua Script"))
					.Visibility(EVisibility::SelfHitTestInvisible)
					//.IsEnabled(false)
				]
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
			]
			.AutoHeight()
			+ SVerticalBox::Slot()
			[
				SAssignNew(ReloadLuaScriptButton, SButton)
				.OnClicked_Raw(this, &SUnrealLuaObjectInspector::NotifyRequestReloadLuaScriptButtonClicked)
				[
					SNew(STextBlock)
					.Text(FText::AsCultureInvariant("Reload lua script"))
					.Visibility(EVisibility::SelfHitTestInvisible)
					//.IsEnabled(false)
				]
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
			]
			.AutoHeight()
			+ SVerticalBox::Slot()
			[
				SAssignNew(EditLoadedLuaScriptButton, SButton)
				.OnClicked_Raw(this, &SUnrealLuaObjectInspector::NotifyEditLoadedLuaScript)
				[
					SNew(STextBlock)
					.Text(FText::AsCultureInvariant("Edit Loaded Script"))
					.Visibility(EVisibility::SelfHitTestInvisible)
					//.IsEnabled(false)
				]
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
			]
			.AutoHeight()
			+SVerticalBox::Slot()
			[
				SAssignNew(RunScriptButton, SButton)
				.OnClicked_Raw(this, &SUnrealLuaObjectInspector::NotifyRunLuaScriptButtonClicked)
				[
					SNew(STextBlock)
					.Text(FText::AsCultureInvariant("Run Script"))
					.Visibility(EVisibility::SelfHitTestInvisible)
				]
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)					
			]
			.AutoHeight()
			+SVerticalBox::Slot()
			[
				SNew(SExpandableArea)
				.AreaTitle(FText::AsCultureInvariant("Loaded Files"))
				.Padding(2.0f)
				.InitiallyCollapsed(true)
				.BodyContent()
				[
					SAssignNew(LoadedFilesVBox, SVerticalBox)
				]
			]
			.AutoHeight()
		]
	]
	.AutoHeight()
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill);
	
	this->SelectMainObject(nullptr);
	
	UnrealLua::UObjectRegistry::GetLuaClassOverrideRegistry().OnClassOverrideFinished.AddSP(this, &SUnrealLuaObjectInspector::NotifyUClassOverrideFinished);
}

void SUnrealLuaObjectInspector::Shutdown()
{
	this->ClearWatchedData();
	SGamescreenDockableWindowWidget::Shutdown();
}

EDockableWindowWidgetOnCloseExternalWindowBehavior SUnrealLuaObjectInspector::GetOnCloseExternalWindowBehavior() const
{
	return EDockableWindowWidgetOnCloseExternalWindowBehavior::RedockOnMainScreen;
}

EDockableWindowWidgetOnCloseGameScreenWidgetBehavior SUnrealLuaObjectInspector::GetOnCloseGameScreenWidgetBehavior() const
{
	return EDockableWindowWidgetOnCloseGameScreenWidgetBehavior::Hide;
}

void SUnrealLuaObjectInspector::NotifyHierarchyItemSelected(TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem> subobjectHierarchyItem)
{
	//
	UObject* obj = subobjectHierarchyItem->Object.Get();
	if (!obj || !this->SelectedMainObject.IsValid())
	{
		this->SelectMainObject(nullptr);
		return;
	}
	if (subobjectHierarchyItem->SelectAsMainItem)
	{
		this->SelectMainObject(obj);
	}
	else
	{
		this->SetWatchedUObject(obj);
	}
}

/*
 TODO:
 - has lua script loaded
 if yes:
 - text: lua script path
 - button: edit lua script -> opens lua script window
 - button: reload lua script
if no:
 - button : "Create default script"
 
 Button: Add Lua script value
 Button: Run script on self -> create function(self) [editable textbox] end
Button: Select subobject
 */
FReply SUnrealLuaObjectInspector::NotifyGameScreenCloseButtonClicked()
{
	this->SelectMainObject(nullptr);
	return SGamescreenDockableWindowWidget::NotifyGameScreenCloseButtonClicked();
}

FReply SUnrealLuaObjectInspector::NotifySelectParentButtonClicked()
{
	UObject* currentObj = this->WatchedObject.Get();
	if (!currentObj)
	{
		this->SelectMainObject(nullptr);
	}
	else
	{
		UObject* parentObject = currentObj->GetOuter();
		this->SelectMainObject(parentObject);	
	}
	return FReply::Handled();
}

FReply SUnrealLuaObjectInspector::NotifySelectActorButtonClicked()
{
	this->OnRequestActorSelection.ExecuteIfBound();
	
	this->Session->GetOninputKeyEvent().AddSP(this, &SUnrealLuaObjectInspector::NotifyInputKeyEvent);
	return FReply::Handled();
}

FReply SUnrealLuaObjectInspector::NotifySelectWorldButtonClicked()
{
	if (!this->Session.IsValid())
	{
		this->SelectMainObject(nullptr);
	}
	auto session = this->Session.Get();
	UWorld* world = session->GetWorld();
	this->SelectMainObject(world);
	return FReply::Handled();
}

FReply SUnrealLuaObjectInspector::NotifyLuaContextButtonClicked()
{
	return FReply::Handled();
}

FReply SUnrealLuaObjectInspector::NotifyCreateDefaultLuaScriptButtonClicked()
{
	UObject* watchedObject = this->WatchedObject.Get();
	if (watchedObject)
	{
		verify(Cast<UClass>(watchedObject) == nullptr) 
		UClass* uclass = watchedObject->GetClass();
		if (uclass)
		{
			UUnrealLuaScriptEditorTool* tool = Cast<UUnrealLuaToolsSession>(this->Session.GetObject())->GetTool<UUnrealLuaScriptEditorTool>();
			tool->UnrealEditorLuaScriptEditor->OpenNewTabCreateDefaultScript(uclass);
		}
		else
		{
			FString popupMessage = "UClass not valid";
			FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, FText::FromString(popupMessage));
			LUA_LOG_ERROR("%s", *popupMessage)
		}
	}
	return FReply::Handled();
}

FReply SUnrealLuaObjectInspector::NotifyRunLuaScriptButtonClicked()
{
	UObject* object = this->WatchedObject.Get();
	
	auto runWidget = SNew(SRunLuaScriptWidget)
		.SelfParam(object)
		.Visibility(EVisibility::Collapsed)
		.Session(this->Session.ToScriptInterface())
		.Title("Lua Script Value Editor")
		.ExternalWindowAnchors(FAnchors{0.f,0.f,1.f,1.0f})
		.GameScreenAnchors(FAnchors{0.3,0.2,0.7,0.8})
		.GameScreenAlignment(FVector2D(0.5f,0.5f))
		.ExternalWindowSize(FVector2D{0.4f, 0.6f})
		.ExternalWindowPosition(FVector2D{0.5f, 0.5f})
		.ExternalWindowSizingRule(ESizingRule::UserSized)
		.InitiallyHidden(false)
		.StartAsWindow(false);
	
	return FReply::Handled();
}

FReply SUnrealLuaObjectInspector::NotifyRequestReloadLuaScriptButtonClicked()
{
	if (this->HasLuaScriptableObject())
	{
		UnrealLua::UObjectRegistry::LoadLuaScript(this->WatchedObject.Get(), true);
	}
	return FReply::Handled();
}

void SUnrealLuaObjectInspector::NotifyRequestEditLuaScriptValue(FString key)
{
	UObject* object = this->WatchedObject.Get();
	if (!object)
	{
		return;
	}
	
	FLuaScriptValue* scriptValue = nullptr;
	
	if (!key.IsEmpty())
	{
		auto scriptValueEditor = this->FindLuaScriptValueEditor(key);
		if (scriptValueEditor.IsValid())
		{
			//there is already an editor for this object-key open
			if (scriptValueEditor->IsInWindow())
			{
				scriptValueEditor->TryGetParentWindow()->BringToFront();
				scriptValueEditor->TryGetParentWindow()->FlashWindow();
			}
			else
			{
				scriptValueEditor->BringToFrontInCanvas();
			}
			return;		
		}		
		//no editor found
		FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(object);

		scriptValue = item.GetLuaScriptValue(*key);
	}
	
	//no existing editor for this object-key found, create a new one
	
	auto newLuaScriptValueEditor = SNew(SLuaScriptValueEditor)
		.ValueOwner(object)
		.LuaScriptValue(scriptValue)
		.Visibility(EVisibility::Collapsed)
		.Session(this->Session.ToScriptInterface())
		.Title("Lua Script Value Editor")
		.ExternalWindowAnchors(FAnchors{0.f,0.f,1.f,1.0f})
		.GameScreenAnchors(FAnchors{0.3,0.2,0.7,0.8})
		.GameScreenAlignment(FVector2D(0.5f,0.5f))
		.ExternalWindowSize(FVector2D{0.4f, 0.6f})
		.ExternalWindowPosition(FVector2D{0.5f, 0.5f})
		.ExternalWindowSizingRule(ESizingRule::UserSized)
		.InitiallyHidden(false)
		.StartAsWindow(false)
		;
	
	newLuaScriptValueEditor->OnShutdown.AddSP(this, &SUnrealLuaObjectInspector::NotifyLuaScriptValueEditorShutdown);
	this->LuaScriptValueEditors.Emplace(newLuaScriptValueEditor);
}

void SUnrealLuaObjectInspector::NotifySelectUObjectFromLuaScriptValueKey(FString key)
{
	UObject* object = this->WatchedObject.Get();
	if (!object)
	{
		return;
	}
	
	FLuaScriptValue* scriptValue = nullptr;
	
	if (!key.IsEmpty())
	{
		FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(object);

		scriptValue = item.GetLuaScriptValue(*key);
	}
	if (!scriptValue || !scriptValue->IsType<TObjectPtr<UObject>>())
	{
		return;
	}
	UObject* val = scriptValue->Get<TObjectPtr<UObject>>().Get();
	this->SelectMainObject(val);
}

void SUnrealLuaObjectInspector::NotifyLuaScriptValueEditorShutdown(TSharedRef<SGamescreenDockableWindowWidget> endingEditor)
{
	TSharedRef<SLuaScriptValueEditor> ref = StaticCastSharedRef<SLuaScriptValueEditor>(endingEditor);
	this->LuaScriptValueEditors.RemoveSingleSwap(ref);
}

FReply SUnrealLuaObjectInspector::NotifyEditLoadedLuaScript()
{
	const FReply reply = FReply::Handled();

	UUnrealLuaScriptEditorTool* tool = Cast<UUnrealLuaToolsSession>(this->Session.GetObject())->GetTool<UUnrealLuaScriptEditorTool>();
	if (!tool)
	{
		return reply;
	}
	
	UObject* obj = this->WatchedObject.Get();
	if (!obj)
	{
		return reply;
	}
	FLuaUObjectItem* item = UnrealLua::UObjectRegistry::TryGetUObjectItem(obj);
	if (!item)
	{
		return reply;
	}
	FLuaScriptInstanceHandle& handle = item->GetLuaScriptHandle();
	if (!handle.IsValid())
	{
		return reply;
	}
	ULoadedLuaScriptCollection* coll = handle.GetLuaScriptCollection();
	if (!coll)
	{
		return reply;
	}
	if (coll->FileInfo.MainFileInfo.IsEmpty())
	{
		return reply;
	}
	TArray<FString> filesToOpen = {};
	for (const FLoadedLuaFileInfo& fileInfo : coll->FileInfo.MainFileInfo)
	{
		if (fileInfo.IsValid())
		{
			filesToOpen.Emplace(fileInfo.FullPathOnDisk);
			//there is always only one valid main file
			break;
		}
	}
	
	verify(filesToOpen.Num() <= 1);
	
	for (const FLoadedLuaFileInfo& fileInfo : coll->FileInfo.ModFileInfos)
	{
		if (fileInfo.IsValid())
		{
			filesToOpen.Emplace(fileInfo.FullPathOnDisk);
		}
	}

	TSharedRef<SLuaScriptEditor> editor = tool->GetLuaScriptEditor();
	editor->OpenFiles(filesToOpen);
	
	return reply;
}

void SUnrealLuaObjectInspector::NotifyUClassOverrideFinished(UClass* uclass)
{
	this->UpdateLuaScriptableObjectFunctionality();
}

void SUnrealLuaObjectInspector::UpdateLuaScriptableObjectFunctionality()
{
	bool hasLuaScriptableObject = this->HasLuaScriptableObject();
	this->CreateDefaultLuaScriptButton->SetVisibility(hasLuaScriptableObject ? EVisibility::SelfHitTestInvisible : EVisibility::Visible);
	this->CreateDefaultLuaScriptButton->SetEnabled(!hasLuaScriptableObject);
	
	this->ReloadLuaScriptButton->SetVisibility(hasLuaScriptableObject ? EVisibility::Visible : EVisibility::SelfHitTestInvisible);
	this->ReloadLuaScriptButton->SetEnabled(hasLuaScriptableObject);
	
	this->EditLoadedLuaScriptButton->SetVisibility(hasLuaScriptableObject ? EVisibility::Visible : EVisibility::SelfHitTestInvisible);
	this->EditLoadedLuaScriptButton->SetEnabled(hasLuaScriptableObject);
	
	this->LoadedFilesVBox->SetVisibility(hasLuaScriptableObject ? EVisibility::Visible : EVisibility::Collapsed);
	this->LoadedFilesVBox->SetEnabled(hasLuaScriptableObject);
}

FReply SUnrealLuaObjectInspector::NotifyNewScriptValueButtonClicked()
{
	UObject* obj = this->WatchedObject.Get();
	if (obj)
	{
		this->NotifyRequestEditLuaScriptValue("");
	}
	return FReply::Handled();
}


void SUnrealLuaObjectInspector::SelectMainObject(UObject* mainObject)
{
	UObject* oldMainObject = this->SelectedMainObject.Get();
	this->SelectedMainObject = mainObject;
	this->SetWatchedUObject(mainObject);
	if (oldMainObject != this->SelectedMainObject.Get())
	{
		this->SubobjectsBrowser->SetWatchedObject(this->SelectedMainObject.Get());
	}
}

void SUnrealLuaObjectInspector::SetWatchedUObject(UObject* newWatchedUObject)
{
	if (this->WatchedObject.IsValid())
	{
		UObject* obj = this->WatchedObject.Get();
		if (newWatchedUObject == obj)
		{
			return;
		}
		FLuaUObjectItem* item = UnrealLua::UObjectRegistry::TryGetUObjectItem(obj);
		if (item)
		{
			item->OnNumberOfValuesChanged.RemoveAll(this);			
		}
	}
	this->ClearWatchedData();
	if (::IsValid(newWatchedUObject))
	{
		verify(this->SelectedMainObject.IsValid());
		verify(newWatchedUObject == this->SelectedMainObject || newWatchedUObject->IsInOuter(this->SelectedMainObject.Get()));
		this->WatchedObject = newWatchedUObject;
		this->bHadValidObjectLastTick = true;
		this->SetVisibility(EVisibility::SelfHitTestInvisible);
		//if (newWatchedUObject == this->SelectedMainObject)
		//{
		//	this->WatchedObjectOwnerLabel->SetText(FText::GetEmpty());
		//	this->WatchedObjectOwnerLabel->SetVisibility(EVisibility::Collapsed);
		//}
		//else
		{
			this->WatchedObjectOwnerLabel->SetVisibility(EVisibility::SelfHitTestInvisible);
			this->WatchedObjectOwnerLabel->SetText(FText::AsCultureInvariant(*GetNameSafe(this->SelectedMainObject.Get())));
		}
		this->WatchedObjectLabel->SetText(FText::AsCultureInvariant(*GetNameSafe(newWatchedUObject)));
		this->UpdateOuterObjectSection(newWatchedUObject->GetOuter());
		UObject* obj = this->WatchedObject.Get();
		FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(obj);
		item.OnNumberOfValuesChanged.AddSP(this, &SUnrealLuaObjectInspector::RefreshLuaScriptValueList);
		item.OnLuaScriptApplied.AddSP(this, &SUnrealLuaObjectInspector::NotifyObjectLuaScriptApplied);
	}
	else
	{
		this->WatchedObject = nullptr;
		this->SubobjectsBrowser->SetWatchedObject(nullptr);
		this->bHadValidObjectLastTick = false;
		this->WatchedObjectLabel->SetText(FText::AsCultureInvariant("No watched UObject"));
		this->WatchedObjectOwnerLabel->SetText(FText::GetEmpty());
		this->WatchedObjectOwnerLabel->SetVisibility(EVisibility::Collapsed);
		this->UpdateOuterObjectSection(nullptr);
	}
	this->UpdateLuaScriptableObjectFunctionality();
	this->RebuildLuaScriptValueList();
	this->NotifyObjectLuaScriptApplied(newWatchedUObject);
}

void SUnrealLuaObjectInspector::ClearWatchedData()
{
	auto copy = this->LuaScriptValueEditors;
	for (TWeakPtr<SLuaScriptValueEditor> editor : copy)
	{
		if (editor.IsValid())
		{
			editor.Pin()->Shutdown();
		}
	}
	verify(this->LuaScriptValueEditors.IsEmpty())
	
	this->WatchedObjectLabel->SetText(FText::AsCultureInvariant("No Object Selected"));
	this->UpdateOuterObjectSection(nullptr);
	this->WatchedObject = nullptr;
}


bool SUnrealLuaObjectInspector::IsValid() const
{
	return false;
}

UObject* SUnrealLuaObjectInspector::GetUObject() const
{
	return nullptr;
}

void SUnrealLuaObjectInspector::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	if (this->bHadValidObjectLastTick)
	{
		if (!this->SelectedMainObject.IsValid())
		{
			this->bHadValidObjectLastTick = false;
			this->SelectMainObject(nullptr);
		}
		else if (!this->WatchedObject.IsValid())
		{
			this->bHadValidObjectLastTick = false;
			this->SetWatchedUObject(nullptr);
		}
	}
}

void SUnrealLuaObjectInspector::NotifyInputKeyEvent(const FInputKeyEventArgs& inputEvent)
{
	FKey key = inputEvent.Key;
	EInputEvent eventType = inputEvent.Event;
	
	if (eventType != EInputEvent::IE_Pressed)
	{
		return;
	}
	UGameInstance* gi = this->Session->GetGameInstance();
	UGameViewportClient* viewport = gi->GetGameViewportClient();
	if (key.IsMouseButton() && key == EKeys::LeftMouseButton)
	{
		APlayerController* pc = gi->GetFirstLocalPlayerController();
		if (pc)
		{	FVector2D MousePosition;
			if (viewport->GetMousePosition(MousePosition))
			{
				FHitResult hitResult;
				
				FVector WorldOrigin;
				FVector WorldDirection;
				if (UGameplayStatics::DeprojectScreenToWorld(pc, MousePosition, WorldOrigin, WorldDirection) == true)
				{
					FWidgetPath WidgetsUnderCursor = FSlateApplication::Get().LocateWindowUnderMouse( FSlateApplication::Get().GetCursorPos(), FSlateApplication::Get().GetInteractiveTopLevelWindows() );
					if (WidgetsUnderCursor.IsValid())
					{
						TArray<UUserWidget*> UserWidgetsUnderCursor{}; 
						for (int32 WidgetIndex = 0; WidgetIndex < WidgetsUnderCursor.Widgets.Num(); WidgetIndex++)
						{
							const TSharedPtr<SWidget> CurrentWidget = WidgetsUnderCursor.Widgets[WidgetIndex].Widget;
							if (CurrentWidget.IsValid() && CurrentWidget->GetType() == "SObjectWidget")
							{
								TSharedPtr<SObjectWidget> ObjectWidget = StaticCastSharedPtr<SObjectWidget, SWidget>(CurrentWidget);
								UserWidgetsUnderCursor.Add(ObjectWidget->GetWidgetObject());
							}		
						}
						if (!UserWidgetsUnderCursor.IsEmpty())
						{
							this->SelectMainObject(UserWidgetsUnderCursor[0]);
							this->Session->GetOninputKeyEvent().RemoveAll(this);
						}
					}

					
					FCollisionObjectQueryParams EverythingButGizmos( FCollisionObjectQueryParams::AllObjects );
					FCollisionQueryParams queryParms = FCollisionQueryParams::DefaultQueryParam;
					queryParms.bTraceComplex = false;
					queryParms.AddIgnoredActor(pc);
					queryParms.AddIgnoredActor(pc->GetPawn());
					if (pc->GetWorld()->LineTraceSingleByObjectType(hitResult, WorldOrigin, WorldOrigin + WorldDirection * pc->HitResultTraceDistance, EverythingButGizmos, queryParms))
					//if (pc->GetHitResultAtScreenPosition(MousePosition, EverythingButGizmos, false, hitResult))
					//if (pc->GetHitResultAtScreenPosition(MousePosition, ECollisionChannel::ECC_Visibility, false, hitResult))
					{
						this->SelectMainObject(hitResult.GetActor());
						this->Session->GetOninputKeyEvent().RemoveAll(this);
					}
				}
			}
		}
	}
	else if (key == EKeys::BackSpace)
	{
		this->Session->GetOninputKeyEvent().RemoveAll(this);
	}
}

bool SUnrealLuaObjectInspector::HasLuaScriptableObject() const
{
	return this->WatchedObject.IsValid() ? UUnrealLuaUObjectRegistry::Get()->GetOverrideRegistry().IsClassLuaOverridable(this->WatchedObject->GetClass()) : false;
}


static const FName sLuaScriptValueListElementWidgetTypeName{"SLuaScriptValueListElementWidget"};

void SUnrealLuaObjectInspector::RefreshLuaScriptValueList()
{
	
	TArray<FString> openNames = {};
	FChildren* children = this->LuaValuesListScrollBox->GetChildren();
	children->ForEachWidget([&openNames](SWidget& child)
	{
		if (child.GetType() == sLuaScriptValueListElementWidgetTypeName)
		{
			SLuaScriptValueListElementWidget* luaScriptValueElementWidget = static_cast<SLuaScriptValueListElementWidget*>(&child);
			if (luaScriptValueElementWidget->IsOpen())
			{
				openNames.Emplace(luaScriptValueElementWidget->KeyString);	
			}			
		}
	});
	this->RebuildLuaScriptValueList(openNames);
}

void SUnrealLuaObjectInspector::RebuildLuaScriptValueList(const TArray<FString>& keepOpenKeys)
{
	this->LuaValuesListScrollBox->ClearChildren();
	if (this->WatchedObject.IsValid())
	{
		FLuaUObjectItem* objectItem = UnrealLua::UObjectRegistry::TryGetUObjectItem(this->WatchedObject.Get());
		if (objectItem)
		{
			for (FLuaScriptValue& value : objectItem->GetLuaScriptValues())
			{
				bool startOpen = keepOpenKeys.Contains(value.GetKeyNameString());
				this->LuaValuesListScrollBox->AddSlot()
				[
					SNew(SLuaScriptValueListElementWidget)
					.LuaScriptValue(&value)
					.OnRequestEditValue(this, &SUnrealLuaObjectInspector::NotifyRequestEditLuaScriptValue)
					.OnSelectUObject(this, &SUnrealLuaObjectInspector::NotifySelectUObjectFromLuaScriptValueKey)
					.InitiallyOpen(startOpen)
				]
				.AutoSize()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Top);
			}
		}
	}
}



void SUnrealLuaObjectInspector::NotifyWatcherWindowClosed(const TSharedRef<SWindow>& window)
{
	window->GetOnWindowClosedEvent().Clear();
	this->SetAsWindow(false);
}

void SUnrealLuaObjectInspector::UpdateOuterObjectSection(UObject* newOuter)
{
	if (newOuter)
	{
		this->OuterObjectText->SetText(FText::AsCultureInvariant("Outer: " + GetNameSafe(newOuter)));
		this->SelectParentButton->SetEnabled(true);
		this->ParentHBox->SetVisibility(EVisibility::SelfHitTestInvisible);
	}
	else
	{
		this->OuterObjectText->SetText(FText::AsCultureInvariant("No Outer"));
		this->SelectParentButton->SetEnabled(false);
		this->ParentHBox->SetVisibility(EVisibility::Collapsed);		
	}

}

TSharedPtr<SLuaScriptValueEditor> SUnrealLuaObjectInspector::FindLuaScriptValueEditor(const FString& key)
{
	for (TWeakPtr<SLuaScriptValueEditor>& editor : this->LuaScriptValueEditors)
	{
		if (editor.IsValid())
		{
			TSharedPtr<SLuaScriptValueEditor> luaScriptValueEditor = editor.Pin();
			if (luaScriptValueEditor->Key == key)
			{
				return luaScriptValueEditor;
			}			
		}
	}
	return nullptr;
}

TSharedPtr<SWindow> SUnrealLuaObjectInspector::GetParentWindowIfInWindow()
{
	if (this->Window.IsValid())
	{
		return this->Window.Pin().ToSharedRef(); 
	}
	return nullptr;
}

void SUnrealLuaObjectInspector::NotifyObjectLuaScriptApplied(UObject* object)
{
	
	this->LoadedFilesVBox->ClearChildren();
	
	FString projectFolder = FPaths::ProjectDir();
	auto createButton = [this, &projectFolder](const FLoadedLuaFileInfo& file)
	{
		if (!file.IsValid())
		{
			return;
		}
		FString shortPath = file.FullPathOnDisk.RightChop(projectFolder.Len());
		this->LoadedFilesVBox->AddSlot()
		[
			SNew(SButton)
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant(shortPath))
				.OverflowPolicy(ETextOverflowPolicy::Clip)
			]
			.OnClicked_Lambda([this, path = file.FullPathOnDisk]()
			{
				UUnrealLuaScriptEditorTool* tool = Cast<UUnrealLuaToolsSession>(this->Session.GetObject())->GetTool<UUnrealLuaScriptEditorTool>();
				if (!tool)
				{
					return FReply::Handled();
				}
				TSharedRef<SLuaScriptEditor> editor = tool->GetLuaScriptEditor();
				editor->OpenFile(path);
				return FReply::Handled();
			})
		]	
		.AutoHeight();		
	};
	
	if (this->WatchedObject.IsValid() && this->WatchedObject == object && UUnrealLuaEngineSubsystem::Get()->IsGameSessionActive())
	{
		FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(object);
		ULoadedLuaScriptCollection* coll = item.GetLuaScriptHandle().GetLuaScriptCollection();
		if (coll)
		{
			for (const FLoadedLuaFileInfo& file : coll->FileInfo.MainFileInfo)
			{
				createButton(file);
			}
			for (const FLoadedLuaFileInfo& file : coll->FileInfo.ModFileInfos)
			{
				createButton(file);
			}
		}
	}
}
