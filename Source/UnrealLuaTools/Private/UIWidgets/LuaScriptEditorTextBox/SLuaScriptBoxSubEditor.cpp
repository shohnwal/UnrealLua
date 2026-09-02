// Fill out your copyright notice in the Description page of Project Settings.


#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptBoxSubEditor.h"

#include "SlateOptMacros.h"
#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptEditorTextBox.h"
#include "Utility/WidgetStyles.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SLuaScriptBoxSubEditor::Construct(const FArguments& InArgs)
{
	
	this->OnCommit = InArgs._OnSubEditorCommit;
	this->OnCancelEdit = InArgs._OnCancelEdit;

}

void SLuaScriptBoxSubEditor::CommmitEditing(const FSubEditorCommitData& commitData)
{
	this->OnCommit.ExecuteIfBound(commitData);
}

void SLuaScriptBoxSubEditor::CancelEditing()
{
	this->OnCancelEdit.ExecuteIfBound(this->SharedThis(this));
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
