// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Session/UnrealLuaToolsSession.h"
#include "Utility/UnrealLuaSimpleDelegateSignatures.h"
#include "Widgets/SCompoundWidget.h"

class SVerticalBox;
class SLuaScriptEditorTextBox;
class SLuaScriptBoxSubEditor;
DECLARE_DELEGATE_OneParam(FOnCreateNewChildObjectEditorDelegate, TSharedRef<SLuaScriptBoxSubEditor> newEditor)

/**
 * 
 */
struct FSubEditorCommitData
{
	//Commited struct
	UStruct* CommittedStruct = nullptr;
	
	//Text that was in the editable text box at commit moment
	FString CommitText = "";
};

DECLARE_DELEGATE_OneParam(FSubEditorCommitDelegate, const FSubEditorCommitData&);

/**
 * 
 */
class UNREALLUATOOLS_API SLuaScriptBoxSubEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLuaScriptBoxSubEditor)
		{
		}
		SLATE_EVENT(FSubEditorCommitDelegate, OnSubEditorCommit)
		SLATE_EVENT(FOnCreateNewChildObjectEditorDelegate, OnAddNewChildEditor)
		SLATE_EVENT(FSimpleWidgetDelegate, OnCancelEdit)
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	
	virtual void CommmitEditing(const FSubEditorCommitData& commitData);
	virtual void CancelEditing();
	
	virtual void InsertTextAtCursor(const FString& Text) = 0;
	virtual void NotifyCommitFromSubEditor(const FSubEditorCommitData& data) = 0;
	
	virtual TSharedPtr<SLuaScriptEditorTextBox> GetLuaScriptEditorTextBox() = 0; 

	FSimpleWidgetDelegate OnCancelEdit = {};
	FSubEditorCommitDelegate OnCommit = {};
	FOnCreateNewChildObjectEditorDelegate OnRequestAddNewChildEditor = {};
};
