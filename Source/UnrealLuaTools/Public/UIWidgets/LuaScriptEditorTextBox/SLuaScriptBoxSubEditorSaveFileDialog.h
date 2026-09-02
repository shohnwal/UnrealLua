// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SLuaScriptBoxSubEditor.h"

/**
 * 
 */
class UNREALLUATOOLS_API SLuaScriptBoxSubEditorSaveFileDialog : public SLuaScriptBoxSubEditor
{
public:
	SLATE_BEGIN_ARGS(SLuaScriptBoxSubEditorSaveFileDialog)
	{
	}
	SLATE_EVENT(FSubEditorCommitDelegate, OnSubEditorCommit)
	SLATE_EVENT(FSimpleWidgetDelegate, OnCancelEdit)
SLATE_END_ARGS()
};
