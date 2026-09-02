// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Framework/MultiBox/MultiBoxExtender.h"

/**
 * 
 */

class SLuaScriptEditorTextBox;
class SMultiLineEditableTextBoxEx;
class SLuaScriptBoxSubEditorNewObjectBase;
class SLuaScriptMultiEditorSwitcher;
class SLuaScriptEditorWindow;
struct ILuaScriptEditorContextMenuOwner;
class FMenuBuilder;
class SWidget;
class SLuaScriptEditorWindowBase;

enum ELuaScriptEditorContextOptions
{
	CopyPaste,
	NewStruct,
	NewUObject,
	ScriptTemplate,
	Mixin,
	All = CopyPaste | NewStruct | NewUObject | ScriptTemplate | Mixin,
};

struct UNREALLUATOOLS_API ILuaScriptEditorContextMenuOwner
{
protected:
	~ILuaScriptEditorContextMenuOwner() = default;
public:
	virtual TSharedRef<SMultiLineEditableTextBoxEx> GetEditableTextBox() = 0;;
	virtual void AddNewChildObjectEditor(TSharedRef<SLuaScriptBoxSubEditorNewObjectBase> newObjectEditor) = 0;;
	virtual void NotifyInsertTextFromContextMenu(FString text) = 0;
	virtual void ExtendContextMenu(FMenuBuilder& menuBuilder) = 0;
	virtual ELuaScriptEditorContextOptions GetContextMenuOptions() { return ELuaScriptEditorContextOptions::All; };
};

namespace UnrealLuaTools::ContextMenuBuilder
{
	UNREALLUATOOLS_API TSharedPtr<SWidget> BuildMenu(TSharedRef<SLuaScriptEditorTextBox>);
	UNREALLUATOOLS_API void CreateStructsSubMenu(FMenuBuilder& menuBuilder, TSharedPtr<SLuaScriptEditorTextBox>);
	UNREALLUATOOLS_API void CreateUObjectSubMenu(FMenuBuilder& MenuBuilder, TSharedPtr<SLuaScriptEditorTextBox>);
	UNREALLUATOOLS_API void CreateScriptTemplatesSubMenu(FMenuBuilder& MenuBuilder, TSharedPtr<SLuaScriptEditorTextBox>);
	UNREALLUATOOLS_API void CreateMixinSubMenu(FMenuBuilder& MenuBuilder, TSharedPtr<SLuaScriptEditorTextBox>);
}
