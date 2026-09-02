// Fill out your copyright notice in the Description page of Project Settings.


#include "Tools/LuaScriptEditor/SLuaScriptEditor.h"

#include "SlateOptMacros.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "BlueprintSupport/UnrealLuaUtility.h"
#include "Components/VerticalBox.h"
#include "Engine/GameInstance.h"
#include "FileSystem/UnrealLuaFileSystemEntry.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/ITargetDevice.h"
#include "Misc/FileHelper.h"
#include "Session/UnrealLuaToolsSession.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"
#include "Subsystem/UnrealLuaFileSystem.h"
#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptBoxSubEditorNewStruct.h"
#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptBoxSubEditorNewUObject.h"
#include "UIWidgets/SMultiTabEdtitableLuaScriptSwitcher.h"
#include "UIWidgets/FileBrowser/SLuaScriptEditorFileBrowser.h"
#include "Utility/LuaLogMacros.h"
#include "Utility/WidgetStyles.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"

#define LOCTEXT_NAMESPACE "SLuaScriptEditor"
BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FWidgetDragDropOp::OnDragged(const FDragDropEvent& InDragDropEvent)
{
	//this->WidgetToDrag->OnDragUpdate(InDragDropEvent,/* Dropped */ false);
}

void FWidgetDragDropOp::OnDrop(bool bDropWasHandled, const FPointerEvent& InMouseEvent)
{
	//if (TSharedPtr<SLuaScriptEditor> DraggableBox = DraggableBoxWeak.Pin())
	{
		//DraggableBox->OnDragUpdate(InMouseEvent,/* Dropped */ true);
	}
}

void SLuaScriptEditor::Construct(const FArguments& InArgs)
{
	this->Session = InArgs._Session;

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
	.DraggableInGameScreen(true)
	);
	
	this->WindowBodyVBox->AddSlot()
	[
		SAssignNew(TopPartBorder, SBorder)
	]
	.VAlign(VAlign_Top)
	.AutoHeight()
	.Padding(10,10,10,10);
	
	this->WindowBodyVBox->AddSlot()
	[
		SNew(SSplitter)
		.Visibility(EVisibility::Visible)
		.Orientation(EOrientation::Orient_Horizontal)
		+ SSplitter::Slot()
		.MinSize(220)
		.Value(0.1f)
		.SizeRule(SSplitter::FractionOfParent)
		[
			SNew(SBox)
			.Visibility(EVisibility::SelfHitTestInvisible)
			.MinDesiredWidth(220)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SVerticalBox)
				.Visibility(EVisibility::SelfHitTestInvisible)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Center)
				.Padding(0,5,0,5)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					.AutoWidth()
					[
						SNew(SButton)
						.ToolTipText(FText::AsCultureInvariant("File Browser"))
						[
							SNew(STextBlock)
							.Text(FText::AsCultureInvariant("Project Files"))
						]
						.OnClicked_Lambda([this]() { this->FileBrowserAndToolsSwitcher->SetActiveWidgetIndex(0); return FReply::Handled();})
					]
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					.AutoWidth()
					[
						SNew(SButton)
						.ToolTipText(FText::AsCultureInvariant("Script Tools"))
						[
							SNew(STextBlock)
							.Text(FText::AsCultureInvariant("Script Tools"))
						]
						.OnClicked_Lambda([this]() { this->FileBrowserAndToolsSwitcher->SetActiveWidgetIndex(1); return FReply::Handled();})
					]
				]
				+ SVerticalBox::Slot()
				.Padding(0,5,0,5)
				[
					SAssignNew(FileBrowserAndToolsSwitcher, SWidgetSwitcher)
					.Visibility(EVisibility::SelfHitTestInvisible)
					+ SWidgetSwitcher::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						SNew(SVerticalBox)
						.Visibility(EVisibility::SelfHitTestInvisible)
						+ SVerticalBox::Slot()
						.VAlign(VAlign_Top)
						.HAlign(HAlign_Fill)
						.AutoHeight()
						.Padding(2)
						[
							SNew(SBorder)
							[
								SNew(STextBlock)
								.Text(FText::AsCultureInvariant("File Browser"))
							]
							.VAlign(VAlign_Top)
							.HAlign(HAlign_Center)
							.Padding(2)
						]
						+ SVerticalBox::Slot()
						.VAlign(VAlign_Fill)
						.HAlign(HAlign_Fill)
						.Padding(2)
						[
							SAssignNew(FileBrowser, SLuaScriptEditorFileBrowser)
							.Visibility(EVisibility::SelfHitTestInvisible)
							.OnFileDoubleClicked(this, &SLuaScriptEditor::NotifyFileBrowserFileDoubleClicked)
							.OnFileTreeRebuilt(this, &SLuaScriptEditor::NotifyFileTreeRebuilt)
							.OnRequestDefaultScriptFile(this, &SLuaScriptEditor::NotifyRequestCreateDefaultScriptFile)
						]
					]
					+ SWidgetSwitcher::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.VAlign(VAlign_Top)
						.HAlign(HAlign_Fill)
						.Padding(2)
						.AutoHeight()
						[
							SNew(SBorder)
							[
								SNew(STextBlock)
								.Text(FText::AsCultureInvariant("Script Tools"))
							]
							.VAlign(VAlign_Top)
							.HAlign(HAlign_Center)
							.Padding(2)
						]
						+ SVerticalBox::Slot()
						[
							SAssignNew(ToolsCategoryTabs, SHorizontalBox)
						]
						.VAlign(VAlign_Top)
						.HAlign(HAlign_Fill)
						.AutoHeight()
						.Padding(2)
						+ SVerticalBox::Slot()
						[
							SAssignNew(ToolCategorySwitcher, SWidgetSwitcher)
						]
						.AutoHeight()
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Fill)
						.FillContentHeight(1)
						.FillHeight(1)
						.Padding(2)
						+SVerticalBox::Slot()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							[
								SAssignNew(PrintCommentsCheckox, SCheckBox)
								.IsChecked(ECheckBoxState::Checked)
							]
							.HAlign(HAlign_Left)
							.VAlign(VAlign_Center)
							.AutoWidth()
							.Padding(0,0,2,0)
							+ SHorizontalBox::Slot()
							[
								SNew(STextBlock)
								.Text(FText::AsCultureInvariant("Add annotations"))
							]
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.AutoWidth()
						]
						.AutoHeight()
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Bottom)	
					]
				]
			]
		]
		+ SSplitter::Slot()
		.MinSize(250)
		.Value(0.9f)
		[
			SAssignNew(FileTabsSwitcher, SMultiTabEdtitableLuaScriptSwitcher)
			.Session(InArgs._Session)
			.SingleTabOnly(InArgs._SingleFileOnly)
			.Text(FText::GetEmpty())
			.IsReadOnly(false)
			.SingleTabOnly(false)
			.AllowContextMenu(true)
			.OnKeyDownHandler_Raw(this, &SLuaScriptEditor::HandleEditTextBoxKeyDown)			
		]
	];
	
	this->CreateNewObjectsTab();
	this->CreateValuesTab();
	this->CreateUtilityTab();
	
	this->WindowBodyVBox->AddSlot()
	//Hide bottom part for now, as we don't need it 
	[
		SNew(SSpacer)
		.Size(FVector2D(2.f,2.f))
	]
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Bottom)
	.AutoHeight();
	
	this->FileBrowser->RebuildFileTree();

	//always open a tab
	this->FileTabsSwitcher->CreateUnownedTab();
	
	this->TopPartBorder.Get()->SetContent(
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			SNew(SButton)
			.ToolTipText(FText::AsCultureInvariant("Create New File"))
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("New"))
			]
			.OnClicked(this, &SLuaScriptEditor::NotifyNewFileButtonPressed)
		]
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.AutoWidth()
		.Padding(0,0,2,0)
		+ SHorizontalBox::Slot()
		[
			SNew(SButton)
			.ToolTipText(FText::AsCultureInvariant("Open File"))
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("Open"))
			]
			.OnClicked(this, &SLuaScriptEditor::NotifyOpenFileButtonPressed)
		]
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.AutoWidth()
		.Padding(0,0,2,0)
		+ SHorizontalBox::Slot()
		[
			SNew(SButton)
			.ToolTipText(FText::AsCultureInvariant("Save File"))
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("Save"))
			]
			.OnClicked(this, &SLuaScriptEditor::NotifySaveFileButtonPressed)
		]
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.AutoWidth()
		.Padding(0,0,2,0)
		+ SHorizontalBox::Slot()
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("File in game folder: /Content/"))))
			.Justification(ETextJustify::Left)				
		]
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.AutoWidth()
		.Padding(0,0,2,0)
		+ SHorizontalBox::Slot()
		[
			ConstructFilePathText(InArgs)
		]
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		.AutoWidth()
		+ SHorizontalBox::Slot()
		[
			ConstructFileExistsBlock(InArgs)
		]
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		.AutoWidth()
		+ SHorizontalBox::Slot()
		[
			SNew( SButton )
			.ToolTipText( LOCTEXT("LuaScriptReload_ToolTip", "Reload all objects using this Lua script file") )
			.OnClicked( this, &SLuaScriptEditor::NotifyReloadLuaScriptButtonClicked)
			[
				SNew(SImage)
				.Image( FAppStyle::GetBrush(TEXT("AnimEditor.RefreshButton")) )
			]
		]
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		.AutoWidth()
	);
	
	this->ToolCategorySwitcher->SetActiveWidgetIndex(0);
}

EDockableWindowWidgetOnCloseExternalWindowBehavior SLuaScriptEditor::GetOnCloseExternalWindowBehavior() const
{
	if (this->Session->GetSessionType() == ELuaToolsSessionType::Game)
	{
		return EDockableWindowWidgetOnCloseExternalWindowBehavior::RedockOnMainScreen;
	}
	else
	{
		return EDockableWindowWidgetOnCloseExternalWindowBehavior::Remove;
	}
}

EDockableWindowWidgetOnCloseGameScreenWidgetBehavior SLuaScriptEditor::GetOnCloseGameScreenWidgetBehavior() const
{
	return EDockableWindowWidgetOnCloseGameScreenWidgetBehavior::Hide;
}

EDockableWindowWidgetInputMode SLuaScriptEditor::GetViewportInputMode() const
{
	return EDockableWindowWidgetInputMode::UIOnly;
}

bool SLuaScriptEditor::HasAnyTabOpen() const
{
	return this->FileTabsSwitcher->HasAnyTabOpen();
}

TSharedRef<STextBlock> SLuaScriptEditor::ConstructFileExistsBlock(const FArguments& InArgs)
{
	this->FileExistsMessage = SNew(STextBlock)
		.Text(FText::FromString("File already exists!"))
		.ColorAndOpacity(FSlateColor(FLinearColor{1, 1, 0, 1}) )
		.Justification(ETextJustify::Left)
		.Visibility(EVisibility::Hidden);
	return this->FileExistsMessage.ToSharedRef();
}

TSharedRef<SEditableTextBox> SLuaScriptEditor::ConstructFilePathText(const FArguments& InArgs)
{
	this->FilePathText = SNew(SEditableTextBox)
	.AllowContextMenu(true)
	.HintText(FText::FromString(FString::Printf(TEXT("File location (inside game content folder)"))))
	.Text(FText::FromString("Lua/example.lua"))
	.Justification(ETextJustify::Left)
	.OnTextChanged_Lambda([this](const FText& Text)
	{
		this->UpdateFilePath(Text.ToString());
	});
	return this->FilePathText.ToSharedRef();
}


void SLuaScriptEditor::UpdateFilePath(FString newFilePath)
{
	bool updateText = false;
	if (newFilePath.StartsWith("/"))
	{
		newFilePath.RightChopInline(1);
		updateText = true;
	}
	this->FilePath = newFilePath;
	this->UpdateFileExistsMessage();
	if (updateText)
	{
		this->FilePathText->SetText(FText::FromString(newFilePath));
	}
}

void SLuaScriptEditor::UpdateFileExistsMessage()
{
	FString fullPath = FPaths::ProjectContentDir() + this->FilePath; 
	if (IFileManager::Get().FileExists(*fullPath))
	{
		this->FileExistsMessage->SetVisibility(EVisibility::SelfHitTestInvisible);
	}
	else
	{
		this->FileExistsMessage->SetVisibility(EVisibility::Hidden);
	}
}

FReply SLuaScriptEditor::NotifyNewFileButtonPressed() const
{
	this->FileTabsSwitcher->CreateUnownedTab();
	return FReply::Handled();
}

FReply SLuaScriptEditor::NotifyOpenFileButtonPressed()
{
	
	return FReply::Handled();
}

FReply SLuaScriptEditor::NotifySaveFileButtonPressed()
{
	if (!this->HasAnyTabOpen())
	{
		return FReply::Unhandled();
	}
	TSharedPtr<SMultiTabEdtiableLuaScriptSwitcherTab> currentTab = this->FileTabsSwitcher->GetCurrentTab();

	currentTab->SaveFile();
	
	return FReply::Handled();
}

FReply SLuaScriptEditor::NotifyReloadLuaScriptButtonClicked()
{
	if (!this->HasAnyTabOpen())
	{
		return FReply::Handled();
	}
	TSharedPtr<SMultiTabEdtiableLuaScriptSwitcherTab> tab = this->FileTabsSwitcher->GetCurrentTab();
	UUnrealLuaEngineSubsystem::Get()->ReloadScript(tab->FullPath);
	return FReply::Handled();
}

FReply SLuaScriptEditor::AddNewObjectEditorToCurrentTab()
{
	if (this->HasAnyTabOpen())
	{
		TSharedRef<SLuaScriptBoxSubEditorNewUObject> newWindow = SNew(SLuaScriptBoxSubEditorNewUObject);
		this->FileTabsSwitcher->AddNewChildObjectEditor(newWindow);
	}
	return FReply::Handled();
}

FReply SLuaScriptEditor::AddNewStructEditorToCurrentTab()
{
	if (this->HasAnyTabOpen())
	{
		TSharedRef<SLuaScriptBoxSubEditorNewStruct> newWindow = SNew(SLuaScriptBoxSubEditorNewStruct);
		this->FileTabsSwitcher->AddNewChildObjectEditor(newWindow);
	}
	return FReply::Handled();
}

FReply SLuaScriptEditor::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	if (this->HasAnyTabOpen())
	{
		if (KeyEvent.GetKey() == EKeys::S && KeyEvent.IsControlDown() && !KeyEvent.IsAltDown() && !KeyEvent.IsShiftDown())
		{
			return this->NotifySaveFileButtonPressed();
		}
		if (KeyEvent.GetKey() == EKeys::W && KeyEvent.IsControlDown() && !KeyEvent.IsAltDown() && !KeyEvent.IsShiftDown())
		{
			this->FileTabsSwitcher->CloseCurrentTab();
			return FReply::Handled();
		}
	}
	if (KeyEvent.GetKey() == EKeys::N && KeyEvent.IsControlDown() && !KeyEvent.IsAltDown() && !KeyEvent.IsShiftDown())
	{
		return this->NotifyNewFileButtonPressed();
	}
	return FReply::Unhandled();
}

FReply SLuaScriptEditor::HandleEditTextBoxKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	return this->OnKeyDown(Geometry, KeyEvent);
}

void SLuaScriptEditor::OpenFile(const FString& filePath, bool andShow)
{
	TWeakPtr<FUnrealLuaFileSystemEntry> descriptor = UUnrealLuaFileSystem::Get()->FindFileDescriptorForFilePath(*filePath);
	if (descriptor.IsValid())
	{
		this->FileBrowser->ExpandItemChain(descriptor);
		this->NotifyFileBrowserFileDoubleClicked(descriptor.Pin());
		if (andShow)
		{
			this->Show();
		}
	}
}

void SLuaScriptEditor::OpenFiles(const TArray<FString>& filePaths, bool andShow)
{
	for (const FString& filePath : filePaths)
	{
		this->OpenFile(filePath, false);
	}
	if (andShow)
	{
		this->Show();
	}
}

void SLuaScriptEditor::OpenNewTabCreateDefaultScript(UClass* uclass)
{
	UUnrealLuaFileSystem* fileSystem = UUnrealLuaFileSystem::Get();
	//For native classes, this is "DefaultScript/<classname>
	FString path = uclass ? FLuaClassOverrideRegistry::GetDefaultLuaScriptPathForUClass_WithRelativeLuaRootPath(uclass, true) : "";
	
	TWeakPtr<FUnrealLuaFileSystemEntry> descriptor = fileSystem->FindFileDescriptorForFilePath(*path);
	if (descriptor.IsValid())
	{
		LUA_LOG("Valid descriptor")
		this->FileBrowser->ExpandItemChain(descriptor);
		this->NotifyFileBrowserFileDoubleClicked(descriptor.Pin());
		this->Show();
	}
	else
	{
		LUA_LOG("No valid descriptor")
		const FString fullPath = FPaths::ConvertRelativePathToFull(path);
		this->FileTabsSwitcher->CreateNewDefaultLuaScriptTab(fullPath, uclass);
		this->Show();
	}
}

void SLuaScriptEditor::Shutdown()
{
	this->Session = nullptr;
	SGamescreenDockableWindowWidget::Shutdown();
}

void SLuaScriptEditor::AddToolsTabAndContent(FString buttonName, TSharedRef<SScrollBox>& content)
{
	this->ToolCategorySwitcher->AddSlot()
	[
		content
	];
	
	TWeakPtr<SScrollBox> ptr = content;

	this->ToolsCategoryTabs->AddSlot()
	[
		SNew(SButton)
		.IsFocusable(false)
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant(buttonName))
		]
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		.OnClicked_Lambda([this, ptr]()
		{
			if (ptr.IsValid())
			{
				this->ToolCategorySwitcher->SetActiveWidget(ptr.Pin().ToSharedRef());
			}
			return FReply::Handled();
		})
	]
	.Padding(2)
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Top);
}

void SLuaScriptEditor::CreateNewObjectsTab()
{
	TSharedRef<SScrollBox> content =SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SButton)
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("New Object / Spawn Actor"))
			]
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.OnClicked_Lambda([this]()
			{
				this->AddNewObjectEditorToCurrentTab();
				return FReply::Handled();
			})
		]
		+ SScrollBox::Slot()
		[
			SNew(SButton)
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("New Struct"))
			]
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.OnClicked_Lambda([this]()
			{
				this->AddNewStructEditorToCurrentTab();
				return FReply::Handled();
			})
		]
		+ SScrollBox::Slot()
		[
			SNew(SButton)
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("New Widget"))
			]
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
		]
		+ SScrollBox::Slot()
		[
			SNew(SButton)
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("New Array"))
			]
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
		]
		+ SScrollBox::Slot()
		[
			SNew(SButton)
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("New Map"))
			]
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
		]
		+ SScrollBox::Slot()
		[
			SNew(SButton)
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("New Set"))
			]
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
		]
		+ SScrollBox::Slot()
		[
			SNew(SButton)
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("New Enum Entry"))
			]
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
		]
		+ SScrollBox::Slot()
		[
			SNew(SButton)
			.IsFocusable(false)
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("New Delegate"))
			]
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.OnClicked_Lambda([this]()
			{
				this->InsertTextAtCursorLocation("Delegate()");
				return FReply::Handled();
			})
		]
		+ SScrollBox::Slot()
		[
			SNew(SButton)
			.IsFocusable(false)
			.OnClicked_Lambda([this]()
			{
				this->InsertTextAtCursorLocation("MulticastDelegate()");
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("New Multicast Delegate"))
			]
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
		]
		+ SScrollBox::Slot()
		[
			SNew(SButton)
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("New Soft Class Path"))
			]
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
		]
		+ SScrollBox::Slot()
		[
			SNew(SButton)
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("New Soft Struct Path"))
			]
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
		]
		+ SScrollBox::Slot()
		[
			SNew(SButton)
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("New Instanced Struct"))
			]
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
		]
		+ SScrollBox::Slot()
		[
			SNew(SButton)
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("New Shared Struct"))
			]
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
		];
	
	this->AddToolsTabAndContent("New", content);
}

void SLuaScriptEditor::CreateValuesTab()
{
	TSharedRef<SScrollBox> content =  SNew(SScrollBox)
	+ SScrollBox::Slot()
	[
		SNew(SButton)
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant("Add On Value Changed"))
		]
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)						
	]
	+ SScrollBox::Slot()
	[
		SNew(SButton)
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant("Remove On Value Changed"))
		]
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
	]
	+ SScrollBox::Slot()
	[
		SNew(SButton)
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant("Set Timer"))
		]
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)						
	]
	+ SScrollBox::Slot()
	[
		SNew(SButton)
		.IsFocusable(false)
		.OnClicked_Lambda([this]()
		{
			this->InsertTextAtCursorLocation("Delay(\"funcname\", <10>)", true);
			return FReply::Handled();
		})
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant("Delay (Func Name)"))
		]
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
	]
	+ SScrollBox::Slot()
	[
		SNew(SButton)
		.IsFocusable(false)
		.OnClicked_Lambda([this]()
		{
			this->InsertTextAtCursorLocation("Delay(function() end, <10>)", true);
			return FReply::Handled();
		})
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant("Delay (Lua Function)"))
		]
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
	]
	+ SScrollBox::Slot()
	[
		SNew(SButton)
		.IsFocusable(false)
		.OnClicked_Lambda([this]()
		{
			this->InsertTextAtCursorLocation("_SetLuaTickEnabled(<isEnabled>)", true);
			return FReply::Handled();
		})
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant("Set Lua Tick Enabled"))
		]
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
	]
	+ SScrollBox::Slot()
	[
		SNew(SButton)
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant("Set Blueprint TickEnabled"))
		]
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
	];
	
	this->ToolCategorySwitcher->AddSlot()
	[
		content
	];
	
	this->AddToolsTabAndContent("Values", content);
}

void SLuaScriptEditor::CreateUtilityTab()
{
	TSharedRef<SScrollBox> content = SNew(SScrollBox)
	+ SScrollBox::Slot()
	[
		SNew(SButton)
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant("Import Blueprint Class"))
		]
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)						
	]
	+ SScrollBox::Slot()
	[
		SNew(SButton)
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant("Import Blueprint Struct"))
		]
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)						
	]
	+ SScrollBox::Slot()
	[
		SNew(SButton)
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant("Import Blueprint Enum"))
		]
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)						
	]
	+ SScrollBox::Slot()
	[
		SNew(SButton)
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant("Rename Object"))
		]
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
	]
	+ SScrollBox::Slot()
	[
		SNew(SButton)
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant("Is Valid"))
		]
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
	]
	+ SScrollBox::Slot()
	[
		SNew(SButton)
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant("Destroy"))
		]
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
	]
	+ SScrollBox::Slot()
	[
		SNew(SButton)
		.IsFocusable(false)
		.OnClicked_Lambda([this]()
		{
			this->InsertTextAtCursorLocation("utype(<$>)", true);
			return FReply::Handled();
		})
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant("utype"))
		]
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
	]
	+ SScrollBox::Slot()
	[
		SNew(SButton)
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant("is"))
		]
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
	]
	+ SScrollBox::Slot()
	[
		SNew(SButton)
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant("run"))
		]
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
	]
	+ SScrollBox::Slot()
	[
		SNew(SButton)
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant("rpc"))
		]
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
	]
	+ SScrollBox::Slot()
	[
		SNew(SButton)
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant("World"))
		]
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
	];
	
	this->AddToolsTabAndContent("Utility", content);
}

void SLuaScriptEditor::NotifyRequestCreateDefaultScriptFile(TSharedPtr<FUnrealLuaFileSystemEntry> parentFolder)
{
	this->FileTabsSwitcher->CreateNewDefaultLuaScriptTab("", nullptr);
}

void SLuaScriptEditor::NotifyFileBrowserFileDoubleClicked(TSharedPtr<FUnrealLuaFileSystemEntry> treeItemPtr)
{
	if (!treeItemPtr.IsValid())
	{
		return;
	}
	verify(treeItemPtr->IsFile());
	const FString& fileName = treeItemPtr->GetFileSystemName();
	verifyf(fileName.EndsWith(".lua"), TEXT("Expected .lua file, but fullpath file was %s"), *treeItemPtr->GetFullPath());
	int32 foundIndex = INDEX_NONE;
	this->FileTabsSwitcher->CreateTabFromFileHandle(treeItemPtr);		
}

void SLuaScriptEditor::NotifyFileTreeRebuilt(const TArray<TSharedPtr<FUnrealLuaFileSystemEntry>>& treeItems)
{
	this->FileTabsSwitcher->NotifyFileTreeRebuilt(treeItems);
}

void SLuaScriptEditor::InsertTextAtCursorLocation(const FString& insertText, bool withMetaSelection)
{
	this->FileTabsSwitcher->InsertAtCursorLocation(insertText, withMetaSelection);
}

void SLuaScriptEditor::InsertWrappedTextAtCursorSelection(const FString& insertLeftFromSelection, const FString& insertRightFromSelection)
{
	this->FileTabsSwitcher->InsertWrappedTextAtCursorSelection(insertLeftFromSelection, insertRightFromSelection);
}

bool SLuaScriptEditor::HasSettings() const
{
	return true;
}

//FReply SLuaScriptEditor::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
//{
//	if (!this->IsInWindow() && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
//	{
//		// Need to remember where within the box we grabbed. We do this here instead of OnDragDetected because 
//		// our mouse can potentially travel some distance before OnDragDetected fires.
//		ScreenSpaceOffsetOfGrab = MouseEvent.GetScreenSpacePosition() - MyGeometry.GetAbsolutePosition();
//		
//		return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
//	}
//	return FReply::Unhandled();
//}
//
//FReply SLuaScriptEditor::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
//{
//	// This will be called when the drag/drop operation is done
//	FOnDragComplete OnUIDropped = FOnDragComplete::CreateLambda([this] (const FVector2D& ScreenSpacePosition) {
//		this->OnDragComplete.ExecuteIfBound(ScreenSpacePosition);
//		//SetVisibility(EVisibility::SelfHitTestInvisible);
//	});
//	
//	// Create your custom Drag Drop Operation here (e.g., FWidgetDragDropOp)
//	TSharedPtr<FWidgetDragDropOp> DragDropOp = MakeShareable(new FWidgetDragDropOp());
//	DragDropOp->WidgetToDrag = SharedThis(this);
//	DragDropOp->MouseOffset = this->ScreenSpaceOffsetOfGrab;
//	DragDropOp->OnUIDropped = OnUIDropped;
//
//	return FReply::Handled().BeginDragDrop(DragDropOp.ToSharedRef());
//}

//void SLuaScriptEditor::OnDragUpdate(const FPointerEvent& PointerEvent, bool bDropped)
//{
//	const FGeometry& MyGeometry = this->GetTickSpaceGeometry();
//	const FVector2f MouseOffset = (PointerEvent.GetScreenSpacePosition() - InDragInfo.OriginalMousePosition)
//		* (MyGeometry.GetLocalSize() / MyGeometry.GetAbsoluteSize());
//	
//	TSharedRef<SConstraintCanvas> canvas = this->Canvas.Pin().ToSharedRef();
//	TSharedRef<SWidget> me = this->AsShared();
//
//}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
#undef LOCTEXT_NAMESPACE