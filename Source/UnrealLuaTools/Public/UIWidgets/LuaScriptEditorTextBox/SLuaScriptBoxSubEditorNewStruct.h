// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptBoxSubEditorNewObjectBase.h"
#include "Utility/UnrealLuaSimpleDelegateSignatures.h"

/**
 * 
 */

class UNREALLUATOOLS_API SLuaScriptBoxSubEditorNewStruct : public SLuaScriptBoxSubEditorNewObjectBase
{
public:
	SLATE_BEGIN_ARGS(SLuaScriptBoxSubEditorNewStruct)
		{
		}
	SLATE_EVENT(FSubEditorCommitDelegate, OnSubEditorCommit)

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	
	virtual void RebuildObjectList() override;
	virtual void SetSelectedStruct(UStruct* selectedStruct) override;
	virtual void SetSelectedStruct(FName selected) override;
	virtual void RebuildTextboxContent() override;
};
