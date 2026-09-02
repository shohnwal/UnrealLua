#include "SlateOptMacros.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IntelliSense/UnrealLuaSyntaxLayoutMarshaller.h"
#include "Styling/StyleColors.h"
#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptBoxSubEditorImportPrompt.h"
#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptBoxSubEditorNewObjectBase.h"
#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptEditorTextBox.h"
#include "UIWidgets/SLuaScriptMultiEditorSwitcher.h"
#include "UIWidgets/SMultiLineEditableTextBoxEx.h"
#include "Utility/LuaLogMacros.h"
#include "Utility/WidgetStyles.h"
#include "Utility/WindowUIUtility.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

#define LOCTEXT_NAMESPACE "UnrealLuaTools"
BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SLuaScriptMultiEditorSwitcher::Construct(const FArguments& InArgs)
{
	this->OnCommitToTopLevelTextBox = InArgs._OnCommitToTopLevelTextBox;
	this->Session = InArgs._Session;
	this->ChildSlot
	[
		SAssignNew(TextInputToNewObjectEditorSwitcher, SWidgetSwitcher)
		+ SWidgetSwitcher::Slot()
		[
			SAssignNew(MainContentGridPanel, SGridPanel)
		]
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)			
	]
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill);
	
	SAssignNew(this->ScriptEditorTextBox, SLuaScriptEditorTextBox)
	.Session(InArgs._Session)
	.Text(FText::AsCultureInvariant(InArgs._TextContent))
	.ClearTextSelectionOnFocusLoss(false)
	.Style(UnrealLuaTools::SlateStyles::GetEditableTextBoxStyle())
	.OnTextChanged(this, &SLuaScriptMultiEditorSwitcher::NotifyTextChanged)
	.OnKeyDownHandler(InArgs._OnKeyDownHandler)
	.AllowContextMenu(true)
	.OnNewChildEditorCreated(this, &SLuaScriptMultiEditorSwitcher::AddNewChildObjectEditor);
	
	if (!InArgs._DeferEdtiableTextBlockSlotAssignment)
	{
		this->MainContentGridPanel->AddSlot(0,0)
		[
			this->ScriptEditorTextBox.ToSharedRef()			
		]
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill);
		
		this->MainContentGridPanel->SetColumnFill(0,1);
		this->MainContentGridPanel->SetRowFill(0,1);
	}
}

void SLuaScriptMultiEditorSwitcher::NotifyTextChanged(const FText& text)
{
	this->OnTextContentChanged.ExecuteIfBound(text);
}

void SLuaScriptMultiEditorSwitcher::AddNewChildObjectEditor(TSharedRef<SLuaScriptBoxSubEditor> newEditorObject)
{
	newEditorObject->OnRequestAddNewChildEditor.BindRaw(this, &SLuaScriptMultiEditorSwitcher::AddNewChildObjectEditor);
	newEditorObject->OnCommit.BindRaw(this, &SLuaScriptMultiEditorSwitcher::NotifyCommitedBySubEditor);
	newEditorObject->OnCancelEdit.BindRaw(this, &SLuaScriptMultiEditorSwitcher::NotifyObjectEditorCancelEdit);
	this->ActiveObjectCreatorStack.Emplace(newEditorObject);
	this->TextInputToNewObjectEditorSwitcher->AddSlot()[
		newEditorObject	
	]
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill);;
	
	this->TextInputToNewObjectEditorSwitcher->SetActiveWidget(newEditorObject);
	TSharedPtr<SLuaScriptEditorTextBox> textbox = newEditorObject->GetLuaScriptEditorTextBox();
	if (textbox)
	{
		FSlateApplication::Get().SetKeyboardFocus(textbox->GetEditableText());
	}
}

void SLuaScriptMultiEditorSwitcher::NotifyCommitedBySubEditor(const FSubEditorCommitData& data)
{
	if (this->ActiveObjectCreatorStack.IsEmpty())
	{
		return;
	}
	TSharedRef<SLuaScriptBoxSubEditor> popped = this->ActiveObjectCreatorStack.Pop();
	this->TextInputToNewObjectEditorSwitcher->RemoveSlot(popped);
	if (this->ActiveObjectCreatorStack.IsEmpty())
	{
		this->TextInputToNewObjectEditorSwitcher->SetActiveWidget(this->MainContentGridPanel.ToSharedRef());
		ScriptEditorTextBox->InsertTextAtCursor(data.CommitText);
		this->OnCommitToTopLevelTextBox.ExecuteIfBound({data.CommittedStruct, data.CommitText});
	}
	else
	{
		TSharedRef<SLuaScriptBoxSubEditor> newLast = this->ActiveObjectCreatorStack.Last();
		this->TextInputToNewObjectEditorSwitcher->SetActiveWidget(newLast);
		newLast->NotifyCommitFromSubEditor(data);
	}	
}

void SLuaScriptMultiEditorSwitcher::InsertTextAtCursorLocation(FString text, bool withMetaSelection)
{
	if (this->ActiveObjectCreatorStack.IsEmpty())
	{
		if (withMetaSelection)
		{
			int32 dollarLoc;
			bool foundDollar = text.FindChar('$', dollarLoc);
			int32 first;
			int32 last;
			bool foundFirst = text.FindChar('<', first);
			bool foundLast  = text.FindLastChar('>', last);
			verify(foundFirst)
			verify(foundLast)
			verify(first < last)
			
			//remove meta tags
			text.RemoveAt(first);
			if (foundDollar)
			{
				text.RemoveAt(dollarLoc - 1);
				text.RemoveAt(last-2);
			}
			else
			{
				text.RemoveAt(last-1);
			}
			
			int32 beginning = first;
			int32 end = foundDollar ? last - 2 : last - 1;
			
			FString selectedText{};
			
			FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
			
			//Copy out selected text, if any
			if (this->ScriptEditorTextBox->AnyTextSelected() && foundDollar)
			{
				LUA_LOG("Has selection and dollar")
				FTextSelection selection = this->ScriptEditorTextBox->GetTextSelection();
				if (selection.LocationA.GetLineIndex() == selection.LocationB.GetLineIndex())
				{
					LUA_LOG("same line")
					selectedText = this->ScriptEditorTextBox->GetSelectedText().ToString();
					this->ScriptEditorTextBox->DeleteSelectedText();					
				}
			}

			LUA_LOG("copied text is %s", *selectedText)
			//insert text at cursor
			FTextLocation cursorLocation = this->ScriptEditorTextBox->GetCursorLocation();
			this->ScriptEditorTextBox->InsertTextAtCursor(text);
			
			
			//Cursor now is at where '<' was
			FTextLocation beginLocation{cursorLocation.GetLineIndex(), cursorLocation.GetOffset() + beginning};
			if (!selectedText.IsEmpty())
			{
				LUA_LOG("copying selected text back in")
				this->ScriptEditorTextBox->GoTo(beginLocation);
				this->ScriptEditorTextBox->InsertTextAtCursor(selectedText);
			}
			FTextLocation endLocation{cursorLocation.GetLineIndex(), cursorLocation.GetOffset() + end + selectedText.Len()};
			this->ScriptEditorTextBox->SelectText(beginLocation, endLocation);	
			
			//this->MainContentEditableTextBox->GetEditableText()->InsertRunAtCursor()
		}
		else
		{
			this->ScriptEditorTextBox->InsertTextAtCursor(text);
		}
		FSlateApplication::Get().SetKeyboardFocus(this->ScriptEditorTextBox->GetEditableText());
	}
	else
	{
		TSharedRef<SLuaScriptBoxSubEditor> currentEditor = this->ActiveObjectCreatorStack.Last();
		currentEditor->InsertTextAtCursor(text);
	}
}

void SLuaScriptMultiEditorSwitcher::InsertWrappedTextAtCursorSelection(const FString& insertLeftFromSelection, const FString& insertRightFromSelection)
{
	if (this->ActiveObjectCreatorStack.IsEmpty())
	{
		if (this->ScriptEditorTextBox->AnyTextSelected())
		{
			FTextSelection selection = this->ScriptEditorTextBox->GetTextSelection();
			FTextLocation beginning = selection.GetBeginning();
			FTextLocation end = selection.GetEnd();
			
			this->ScriptEditorTextBox->GoTo(end);
			this->ScriptEditorTextBox->InsertTextAtCursor(insertRightFromSelection);
			
			this->ScriptEditorTextBox->GoTo(beginning);
			this->ScriptEditorTextBox->InsertTextAtCursor(insertLeftFromSelection);
			
		}
		else
		{
			this->ScriptEditorTextBox->InsertTextAtCursor(insertLeftFromSelection + insertRightFromSelection);	
		}
	}
	else
	{
		TSharedRef<SLuaScriptBoxSubEditor> currentEditor = this->ActiveObjectCreatorStack.Last();
		if (currentEditor->GetLuaScriptEditorTextBox()->AnyTextSelected())
		{
			FTextSelection selection = currentEditor->GetLuaScriptEditorTextBox()->GetTextSelection();
			FTextLocation beginning = selection.GetBeginning();
			FTextLocation end = selection.GetEnd();
			
			currentEditor->GetLuaScriptEditorTextBox()->GoTo(end);
			currentEditor->GetLuaScriptEditorTextBox()->InsertTextAtCursor(insertRightFromSelection);
			
			currentEditor->GetLuaScriptEditorTextBox()->GoTo(beginning);
			currentEditor->GetLuaScriptEditorTextBox()->InsertTextAtCursor(insertLeftFromSelection);
		}
		else
		{
			currentEditor->InsertTextAtCursor(insertLeftFromSelection + insertRightFromSelection);	
		}
	}
}

void SLuaScriptMultiEditorSwitcher::NotifyObjectEditorCancelEdit(TSharedRef<SWidget> item)
{
	if (this->ActiveObjectCreatorStack.IsEmpty())
	{
		return;
	}
	if (item != ActiveObjectCreatorStack.Last())
	{
		return;
	}
	TSharedRef<SLuaScriptBoxSubEditor> popped = this->ActiveObjectCreatorStack.Pop();
	this->TextInputToNewObjectEditorSwitcher->RemoveSlot(popped);
	if (!this->ActiveObjectCreatorStack.IsEmpty())
	{
		TSharedRef<SLuaScriptBoxSubEditor> newLast = this->ActiveObjectCreatorStack.Last();
		this->TextInputToNewObjectEditorSwitcher->SetActiveWidget(newLast);
	}
	else
	{
		this->TextInputToNewObjectEditorSwitcher->SetActiveWidget(this->MainContentGridPanel.ToSharedRef());
	}
}

TSharedPtr<SMultiLineEditableTextBoxEx> SLuaScriptMultiEditorSwitcher::GetEditableText()
{
	return this->ScriptEditorTextBox->GetEditableText();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
#undef LOCTEXT_NAMESPACE