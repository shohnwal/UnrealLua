#pragma once
#include "ContextMenu/LuaScriptEditorContextMenuBuilder.h"
#include "Framework/SlateDelegates.h"
#include "IntelliSense/LuaSyntaxReport.h"
#include "Widgets/SCompoundWidget.h"

class SLuaScriptEditorTextBox;
class FUnrealLuaSyntaxLayoutMarshaller;
class SGridPanel;
class SWidgetSwitcher;
class SScrollBox;
class SExpandableArea;
struct FSubEditorCommitData;
class SLuaScriptBoxSubEditor;
struct FScopedLuaContext;
struct FReceivedSubEditorCommitData
{
	//Commited struct
	UStruct* CommittedStruct = nullptr;
	
	//Text that was in the editable text box at commit moment
	FString CommitText = "";
};

DECLARE_DELEGATE_OneParam(FReceivedSubEditorCommitDelegate, const FReceivedSubEditorCommitData&);

class UNREALLUATOOLS_API SLuaScriptMultiEditorSwitcher : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLuaScriptMultiEditorSwitcher)
		: _Session(nullptr), _DeferEdtiableTextBlockSlotAssignment(false)
	{
	}
	SLATE_ARGUMENT(TScriptInterface<ILuaToolsSession>, Session)
	/** Delegate to call before a context menu is opened. User returns the menu content or null to the disable context menu */
	SLATE_EVENT(FOnContextMenuOpening, OnContextMenuOpening)
	/** Callback delegate to have first chance handling of the OnKeyDown event */
	SLATE_EVENT(FOnKeyDown, OnKeyDownHandler)
	SLATE_ARGUMENT(FString, TextContent)
	SLATE_ARGUMENT(bool, DeferEdtiableTextBlockSlotAssignment)
	SLATE_EVENT(FReceivedSubEditorCommitDelegate, OnCommitToTopLevelTextBox)
		
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	void NotifyTextChanged(const FText& text);

	
public:	
	void AddNewChildObjectEditor(TSharedRef<SLuaScriptBoxSubEditor> newEditorObject);
	void NotifyCommitedBySubEditor(const FSubEditorCommitData& data);
	void InsertTextAtCursorLocation(FString text, bool withMetaSelection = false);
	void InsertWrappedTextAtCursorSelection(const FString& insertLeftFromSelection, const FString& insertRightFromSelection);
	
	virtual void NotifyObjectEditorCancelEdit(TSharedRef<SWidget> Widget);

	TSharedPtr<SMultiLineEditableTextBoxEx> GetEditableText();
	//virtual void ExtendContextMenu(FMenuBuilder& menuBuilder) override;

	TSharedPtr<SWidgetSwitcher> TextInputToNewObjectEditorSwitcher = {};
	TArray<TSharedRef<SLuaScriptBoxSubEditor>> ActiveObjectCreatorStack = {};

	FOnTextChanged OnTextContentChanged = {};
	TSharedPtr<SLuaScriptEditorTextBox> ScriptEditorTextBox = {};
	TSharedPtr<SGridPanel> MainContentGridPanel = {};
	

	FReceivedSubEditorCommitDelegate OnCommitToTopLevelTextBox;
	TWeakInterfacePtr<ILuaToolsSession> Session = nullptr;
};