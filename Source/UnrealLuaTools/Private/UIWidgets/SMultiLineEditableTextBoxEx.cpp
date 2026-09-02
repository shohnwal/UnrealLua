// Fill out your copyright notice in the Description page of Project Settings.


#include "UIWidgets/SMultiLineEditableTextBoxEx.h"
#include "SlateOptMacros.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SMultiLineEditableTextBoxEx::Construct(const FArguments& InArgs)
{
	SMultiLineEditableTextBox::Construct(SMultiLineEditableTextBox::FArguments()
	.Style(InArgs._Style)
	.Marshaller(InArgs._Marshaller)
	.Text(InArgs._Text)
	.HintText(InArgs._HintText)
	.SearchText(InArgs._SearchText)
	.Font(InArgs._Font)
	.ForegroundColor(InArgs._ForegroundColor)
	.ReadOnlyForegroundColor(InArgs._ReadOnlyForegroundColor)
	.FocusedForegroundColor(InArgs._FocusedForegroundColor)
	.Justification(InArgs._Justification)
	.MaximumLength(InArgs._MaximumLength)
	.LineHeightPercentage(InArgs._LineHeightPercentage)
	.IsReadOnly( InArgs._IsReadOnly )
	.AllowMultiLine( InArgs._AllowMultiLine )
	.IsCaretMovedWhenGainFocus ( InArgs._IsCaretMovedWhenGainFocus )
	.SelectAllTextWhenFocused( InArgs._SelectAllTextWhenFocused )
	.ClearTextSelectionOnFocusLoss( InArgs._ClearTextSelectionOnFocusLoss )
	.RevertTextOnEscape( InArgs._RevertTextOnEscape )
	.ClearKeyboardFocusOnCommit( InArgs._ClearKeyboardFocusOnCommit )
	.AllowContextMenu(InArgs._AllowContextMenu)
	.AlwaysShowScrollbars( InArgs._AlwaysShowScrollbars )
	.HScrollBar(InArgs._HScrollBar)
	.VScrollBar(InArgs._VScrollBar)
	.WrapTextAt(InArgs._WrapTextAt)
	.AutoWrapText(InArgs._AutoWrapText)
	.WrappingPolicy(InArgs._WrappingPolicy)
	.SelectAllTextOnCommit( InArgs._SelectAllTextOnCommit )
	.SelectWordOnMouseDoubleClick( InArgs._SelectWordOnMouseDoubleClick )
	.BackgroundColor(InArgs._BackgroundColor)		
	.Padding(InArgs._Padding)
	.Margin(InArgs._Margin)
	.ErrorReporting(InArgs._ErrorReporting)
	.ModiferKeyForNewLine(InArgs._ModiferKeyForNewLine)
	.VirtualKeyboardOptions(InArgs._VirtualKeyboardOptions)
	.VirtualKeyboardTrigger(InArgs._VirtualKeyboardTrigger)
	.VirtualKeyboardDismissAction(InArgs._VirtualKeyboardDismissAction)
	.TextShapingMethod(InArgs._TextShapingMethod)
	.TextFlowDirection(InArgs._TextFlowDirection)
	.OverflowPolicy(InArgs._OverflowPolicy)
	.HScrollBarPadding(InArgs._HScrollBarPadding)
	.VScrollBarPadding(InArgs._VScrollBarPadding)
	//.OnContextMenuOpening(InArgs._OnContextMenuOpening)
	.OnContextMenuOpening(InArgs._OnContextMenuOpening)
	.OnIsTypedCharValid(InArgs._OnIsTypedCharValid)
	.OnTextChanged(InArgs._OnTextChanged)
	.OnTextCommitted(InArgs._OnTextCommitted)
	.OnVerifyTextChanged(InArgs._OnVerifyTextChanged)
	.OnHScrollBarUserScrolled(InArgs._OnHScrollBarUserScrolled)
	.OnVScrollBarUserScrolled(InArgs._OnVScrollBarUserScrolled)
	.OnCursorMoved(InArgs._OnCursorMoved)
	.OnKeyCharHandler(InArgs._OnKeyCharHandler)
	.OnKeyDownHandler_Raw(this, &SMultiLineEditableTextBoxEx::NotifyTextKeyDown)
	.ContextMenuExtender_Raw(this, &SMultiLineEditableTextBoxEx::ExtendContextMenu)
	.CreateSlateTextLayout(InArgs._CreateSlateTextLayout)
	);
	
	this->OnKeyDownHandler = InArgs._OnKeyDownHandler;
	
	UnrealLuaTools::SlateStyles::GetOnStyleChangedDelegate().AddSPLambda(this, [this]()
	{
		this->SetStyle(UnrealLuaTools::SlateStyles::GetEditableTextBoxStyle());
	});
}

FReply SMultiLineEditableTextBoxEx::NotifyTextKeyDown(const FGeometry& geometry, const FKeyEvent& keyEvent)
{
	if (keyEvent.GetKey() == EKeys::Tab)
	{
		if (!keyEvent.GetModifierKeys().AnyModifiersDown())
		{
			this->InsertTextAtCursor(FString{"\t"});
			return FReply::Handled();
		}
		else if (keyEvent.GetModifierKeys().IsShiftDown())
		{
			FTextLocation currentLoc = this->GetCursorLocation();
			int32 line = currentLoc.GetLineIndex();
			int32 offset = currentLoc.GetOffset();
			if (offset > 0)
			{
				FString currentLine{};
				this->GetCurrentTextLine(currentLine);
				if (currentLine[offset] == '\t' || currentLine[offset] == ' ')
				{
					currentLine.RemoveAt(offset);
				}
			}
		}
	}

	if (this->OnKeyDownHandler.IsBound())
	{
		return this->OnKeyDownHandler.Execute(geometry, keyEvent);
	}

	return FReply::Unhandled();
}

TSharedPtr<SMultiLineEditableText> SMultiLineEditableTextBoxEx::GetEditableText() const
{
	return this->EditableText;
}

void SMultiLineEditableTextBoxEx::DeleteSelectedText()
{
	this->EditableText->DeleteSelectedText();
}


void SMultiLineEditableTextBoxEx::NotifyInsertTextFromContextMenu(FString text)
{
	this->InsertTextAtCursor(text);
}

FTextSelection SMultiLineEditableTextBoxEx::GetTextSelection() const
{
	return this->GetEditableText()->GetSelection();
}

TSharedRef<SMultiLineEditableTextBoxEx> SMultiLineEditableTextBoxEx::GetEditableTextBox()
{
	return this->SharedThis(this);	
}

void SMultiLineEditableTextBoxEx::SelectText(const FTextLocation& InSelectionStart, const FTextLocation& InCursorLocation)
{
	this->GetEditableText()->SelectText(InSelectionStart, InCursorLocation);
}

void SMultiLineEditableTextBoxEx::ExtendContextMenu(FMenuBuilder& menuBuilder)
{
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
