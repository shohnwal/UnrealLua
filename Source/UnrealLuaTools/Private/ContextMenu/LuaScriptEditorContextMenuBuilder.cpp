// Fill out your copyright notice in the Description page of Project Settings.


#include "ContextMenu/LuaScriptEditorContextMenuBuilder.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Textures/SlateIcon.h"
#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptBoxSubEditorNewStruct.h"
#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptBoxSubEditorNewUObject.h"
#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptEditorTextBox.h"
#include "UIWidgets/SMultiTabEdtitableLuaScriptSwitcher.h"
#include "UIWidgets/SMultiLineEditableTextBoxEx.h"
#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptBoxSubEditorImportPrompt.h"
#include "Utility/LuaLogMacros.h"
#include "Utility/LuaScriptTemplates.h"
#define LOCTEXT_NAMESPACE "UnrealLuaTools"

TSharedPtr<SWidget> UnrealLuaTools::ContextMenuBuilder::BuildMenu(TSharedRef<SLuaScriptEditorTextBox> hostWindow)
{
	FSlateIcon DummyIcon(NAME_None, NAME_None);
	
	FMenuBuilder menuBuilder(true, nullptr, nullptr, false, &FCoreStyle::Get(), false );
	
	menuBuilder.BeginSection(NAME_None, {});
	{
		menuBuilder.AddMenuEntry(LOCTEXT("CopyTextLabel", "Copy"),LOCTEXT("CopyTextTip", "Copy selected text to clipboard"), FSlateIcon(),FUIAction(
			FExecuteAction::CreateLambda([hostWindow]()
			{
				FString selectedText = hostWindow->GetSelectedText().ToString();
				FPlatformApplicationMisc::ClipboardCopy(*selectedText);
			} ),
			FCanExecuteAction::CreateLambda([hostWindow]() { return hostWindow->AnyTextSelected(); } )
		));
		
		menuBuilder.AddMenuEntry(LOCTEXT("PasteTextLabel", "Paste"),LOCTEXT("PasteTextTip", "Paste clipboard text to text"), FSlateIcon(),FUIAction(
			FExecuteAction::CreateLambda([hostWindow]()
			{
				FString toPaste;
				FPlatformApplicationMisc::ClipboardPaste(toPaste);
				hostWindow->InsertTextAtCursor(toPaste);
			})
		));
	}
	
	TWeakPtr<SLuaScriptEditorTextBox> ptr = hostWindow.ToSharedPtr();
	menuBuilder.AddMenuEntry(LOCTEXT("StructsLabel", "Struct"), {},  FSlateIcon(),FUIAction(FExecuteAction::CreateLambda([ptr]()
	{
		if (ptr.IsValid())
    	{
    		TSharedRef<SLuaScriptBoxSubEditorNewStruct> newWindow = SNew(SLuaScriptBoxSubEditorNewStruct);
    		ptr.Pin()->AddNewChildObjectEditor(newWindow);
    	}
	})));
	menuBuilder.AddMenuEntry(LOCTEXT("UObjectLabel", "UObject"), {}, FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([ptr]()
	{
		if (ptr.IsValid())
		{
			TSharedRef<SLuaScriptBoxSubEditorNewUObject> newWindow = SNew(SLuaScriptBoxSubEditorNewUObject);
			ptr.Pin()->AddNewChildObjectEditor(newWindow);
		}
	})));
	menuBuilder.AddMenuEntry(LOCTEXT("ImportLabel", "Import"), {}, FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([ptr]()
	{
		if (ptr.IsValid())
		{
			TSharedRef<SLuaScriptBoxSubEditorImportPrompt> newWindow = SNew(SLuaScriptBoxSubEditorImportPrompt);
			ptr.Pin()->AddNewChildObjectEditor(newWindow);
		}
	})));
	menuBuilder.AddMenuEntry(LOCTEXT("Test", "Test"), {}, FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([ptr]()
	{
		if (!ptr.IsValid())
		{
			return;
		}
		// 2. Initialize the Menu Builder
		FMenuBuilder MenuBuilder(true, nullptr);

		// 3. Add Menu Entries
		MenuBuilder.AddMenuEntry(
			FText::FromString("Action 1"),
			FText::FromString("Perform the first action"),
			FSlateIcon(), // Add FSlateIcon if needed
			FExecuteAction::CreateLambda([](){ LUA_LOG("clicked!")})
		);
		
		//MenuBuilder.AddSubMenu(LOCTEXT("ScriptTemplatesLabel", "Template"), {},FNewMenuDelegate::CreateLambda([ptr](FMenuBuilder& menuBuilder)
		MenuBuilder.AddSubMenu(SNew(STextBlock).Text(FText::AsCultureInvariant("meow")), FNewMenuDelegate::CreateLambda([ptr](FMenuBuilder& menuBuilder)
		{
			menuBuilder.BeginSection(NAME_None, {});
				{
					menuBuilder.AddMenuEntry(LOCTEXT("NewScriptAttributesTemplateLabel", "Script Attributes"),LOCTEXT("ScriptAttributesTip", "Add script attributes template.\nScript Attributes define behavior\nof Lua scripts attached to UObjects"), FSlateIcon(),FUIAction(
					FExecuteAction::CreateLambda([ptr]()
					{
						if (ptr.IsValid())
						{
							FString attributes = UnrealLuaTools::ScriptTemplates::MakeLuaScriptAttributesTemplate();
							ptr.Pin()->InsertTextAtCursor(attributes);
						}
					})));
				}
				menuBuilder.EndSection();
		}));

		// 4. Push the menu at the current cursor location
		TSharedPtr<SWidget> MenuWidget = MenuBuilder.MakeWidget();
		FWidgetPath WidgetPath = FWidgetPath();

		FSlateApplication::Get().PushMenu(
			ptr.Pin().ToSharedRef(),
			WidgetPath,
			MenuWidget.ToSharedRef(),
			FSlateApplication::Get().GetCursorPos(),
			FPopupTransitionEffect::ContextMenu
		);
	})));

	menuBuilder.AddSubMenu(LOCTEXT("ScriptTemplatesLabel", "Template"), {},FNewMenuDelegate::CreateLambda([ptr = ptr](FMenuBuilder& menuBuilder)
	{
		if (ptr.IsValid())
		{
			UnrealLuaTools::ContextMenuBuilder::CreateScriptTemplatesSubMenu(menuBuilder, ptr.Pin());
		}
	}));
	menuBuilder.AddSubMenu(LOCTEXT("MixinLabel", "Mixin"), {}, FNewMenuDelegate::CreateLambda([ptr = ptr](FMenuBuilder& menuBuilder)
	{
	if (ptr.IsValid())
	{
		UnrealLuaTools::ContextMenuBuilder::CreateMixinSubMenu(menuBuilder, ptr.Pin());
	}
	}));
	menuBuilder.EndSection();
	
	//hostWindow->ExtendContextMenu(menuBuilder);
	
	return menuBuilder.MakeWidget();
}

void UnrealLuaTools::ContextMenuBuilder::CreateStructsSubMenu(FMenuBuilder& menuBuilder, TSharedPtr<SLuaScriptEditorTextBox> self)
{
	TWeakPtr<SLuaScriptEditorTextBox> ptr = self;
	menuBuilder.BeginSection(NAME_None, {});
	{
		menuBuilder.AddMenuEntry(LOCTEXT("MakeStructLabel", "New Struct"),LOCTEXT("MakeStructTip", "Make a new struct"), FSlateIcon(),FUIAction(
		FExecuteAction::CreateLambda([ptr]()
		{
			if (ptr.IsValid())
			{
				TSharedRef<SLuaScriptBoxSubEditorNewStruct> newWindow = SNew(SLuaScriptBoxSubEditorNewStruct);
				ptr.Pin()->AddNewChildObjectEditor(newWindow);
			}
		})));
	}
	menuBuilder.EndSection();
}

void UnrealLuaTools::ContextMenuBuilder::CreateUObjectSubMenu(FMenuBuilder& menuBuilder, TSharedPtr<SLuaScriptEditorTextBox> self)
{
	TWeakPtr<SLuaScriptEditorTextBox> ptr = self;
	menuBuilder.BeginSection(NAME_None, {});
	{
		menuBuilder.AddMenuEntry(LOCTEXT("NewUObjectLabel", "New UObject / Actor"),LOCTEXT("MakeUObjectTip", "Make a new UObject / Spawn a new Actor"), FSlateIcon(),FUIAction(
		FExecuteAction::CreateLambda([ptr]()
		{
			if (ptr.IsValid())
			{
				TSharedRef<SLuaScriptBoxSubEditorNewUObject> newWindow = SNew(SLuaScriptBoxSubEditorNewUObject);
				ptr.Pin()->AddNewChildObjectEditor(newWindow);
			}
		})));
	}
	menuBuilder.EndSection();
}

void UnrealLuaTools::ContextMenuBuilder::CreateScriptTemplatesSubMenu(FMenuBuilder& menuBuilder, TSharedPtr<SLuaScriptEditorTextBox> self)
{
	TWeakPtr<SLuaScriptEditorTextBox> ptr = self;
	menuBuilder.BeginSection(NAME_None, {});
	{
		menuBuilder.AddMenuEntry(LOCTEXT("NewScriptAttributesTemplateLabel", "Script Attributes"),LOCTEXT("ScriptAttributesTip", "Add script attributes template.\nScript Attributes define behavior\nof Lua scripts attached to UObjects"), FSlateIcon(),FUIAction(
		FExecuteAction::CreateLambda([ptr]()
		{
			if (ptr.IsValid())
			{
				FString attributes = UnrealLuaTools::ScriptTemplates::MakeLuaScriptAttributesTemplate();
				ptr.Pin()->InsertTextAtCursor(attributes);
			}
		})));
	}
	menuBuilder.EndSection();
	menuBuilder.BeginSection(NAME_None, {});
	{
		menuBuilder.AddMenuEntry(LOCTEXT("NewLuaScriptReplicationTemplateLabel", "Script Replication"),LOCTEXT("NewLuaScriptReplicationTemplateTip", "Add script replication template.\nAllows Lua scripts to replicate\nvalues over the network"), FSlateIcon(),FUIAction(
		FExecuteAction::CreateLambda([ptr]()
		{
			if (ptr.IsValid())
			{
				FString replicationLayout = UnrealLuaTools::ScriptTemplates::MakeLuaScriptReplicationTemplate();
				ptr.Pin()->InsertTextAtCursor(replicationLayout);
			}
		})));
	}
	menuBuilder.EndSection();
}

void UnrealLuaTools::ContextMenuBuilder::CreateMixinSubMenu(FMenuBuilder& MenuBuilder, TSharedPtr<SLuaScriptEditorTextBox> LuaScriptEditorWindowBase)
{
}

#undef LOCTEXT_NAMESPACE
