// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../SMultiLineEditableTextBoxEx.h"
#include "IntelliSense/LuaSyntaxReport.h"
#include "LuaValue/LuaValue.h"
#include "Session/UnrealLuaToolsSession.h"
#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptBoxSubEditorNewStruct.h"
#include "Widgets/SCompoundWidget.h"

class SLuaIntelliSenseWidget;
class FLuaScriptTextLayout;
class SMenuAnchor;
struct FScopedLuaContext;
class STextBlock;
class SScrollBox;
class SExpandableArea;
class SMultiLineEditableTextBoxEx;

DECLARE_DELEGATE_OneParam(FNewChildEditorCreatedDelegate, TSharedRef<SLuaScriptBoxSubEditor>)
/**
 * 
 */
class UNREALLUATOOLS_API SLuaScriptEditorTextBox : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLuaScriptEditorTextBox)
		: _Session(nullptr)
		{
		}
		SLATE_ARGUMENT(TScriptInterface<ILuaToolsSession>, Session)
		
		/** The styling of the textbox */
		SLATE_STYLE_ARGUMENT(FEditableTextBoxStyle, Style)

		/** Pointer to a style of the text block, which dictates the font, color, and shadow options. */
		SLATE_STYLE_ARGUMENT_DEPRECATED(FTextBlockStyle, TextStyle, 5.2, "TextStyle is deprecated and will be ignored. Please use the TextStyle embedded in FEditableTextBoxStyle Style.")

		/** The marshaller used to get/set the raw text to/from the text layout. */
		SLATE_ARGUMENT(TSharedPtr< ITextLayoutMarshaller >, Marshaller)

		/** Sets the text content for this editable text box widget */
		SLATE_ATTRIBUTE( FText, Text )

		/** Hint text that appears when there is no text in the text box */
		SLATE_ATTRIBUTE( FText, HintText )

		/** Text to search for (a new search is triggered whenever this text changes) */
		SLATE_ATTRIBUTE( FText, SearchText )

		/** Font color and opacity (overrides Style) */
		SLATE_ATTRIBUTE( FSlateFontInfo, Font )

		/** Text color and opacity (overrides Style) */
		SLATE_ATTRIBUTE( FSlateColor, ForegroundColor )
		
		/** Text color and opacity when read-only (overrides Style) */
		SLATE_ATTRIBUTE( FSlateColor, ReadOnlyForegroundColor )

		/** Text color and opacity when this box has keyboard focus (overrides Style) */
		SLATE_ATTRIBUTE(FSlateColor, FocusedForegroundColor)

		/** How the text should be aligned with the margin. */
		SLATE_ATTRIBUTE(ETextJustify::Type, Justification)

		/** Maximum length of the text. Input will refuse text longer than that. */
		SLATE_ATTRIBUTE(int32, MaximumLength)

		/** The amount to scale each lines height by. */
		SLATE_ATTRIBUTE(float, LineHeightPercentage)

		/** Sets whether this text box can actually be modified interactively by the user */
		SLATE_ATTRIBUTE( bool, IsReadOnly )

		/** Whether to allow multi-line text */
		SLATE_ATTRIBUTE(bool, AllowMultiLine)

		/** Workaround as we loose focus when the auto completion closes. */
		SLATE_ATTRIBUTE( bool, IsCaretMovedWhenGainFocus )

		/** Whether to select all text when the user clicks to give focus on the widget */
		SLATE_ATTRIBUTE( bool, SelectAllTextWhenFocused )

		/** Whether to clear text selection when focus is lost */
		SLATE_ATTRIBUTE( bool, ClearTextSelectionOnFocusLoss )

		/** Whether to allow the user to back out of changes when they press the escape key */
		SLATE_ATTRIBUTE( bool, RevertTextOnEscape )

		/** Whether to clear keyboard focus when pressing enter to commit changes */
		SLATE_ATTRIBUTE( bool, ClearKeyboardFocusOnCommit )

		/** Whether the context menu can be opened  */
		SLATE_ATTRIBUTE(bool, AllowContextMenu)

		/** Should we always show the scrollbars (only affects internally created scroll bars) */
		SLATE_ARGUMENT(bool, AlwaysShowScrollbars)

		/** The horizontal scroll bar widget, or null to create one internally */
		SLATE_ARGUMENT( TSharedPtr<SScrollBar>, HScrollBar )

		/** The vertical scroll bar widget, or null to create one internally */
		SLATE_ARGUMENT( TSharedPtr< SScrollBar >, VScrollBar )

		/** Padding around the horizontal scrollbar (overrides Style) */
		SLATE_ATTRIBUTE( FMargin, HScrollBarPadding )

		/** Padding around the vertical scrollbar (overrides Style) */
		SLATE_ATTRIBUTE( FMargin, VScrollBarPadding )

		/** Delegate to call before a context menu is opened. User returns the menu content or null to the disable context menu */
		//SLATE_EVENT(FOnContextMenuOpening, OnContextMenuOpening)

		/**
		 * This is NOT for validating input!
		 * 
		 * Called whenever a character is typed.
		 * Not called for copy, paste, or any other text changes!
		 */
		SLATE_EVENT( FOnIsTypedCharValid, OnIsTypedCharValid )

		/** Callback when the text starts to be edited */
		SLATE_EVENT( FOnBeginTextEdit, OnBeginTextEdit )

		/** Called whenever the text is changed programmatically or interactively by the user */
		SLATE_EVENT( FOnTextChanged, OnTextChanged )

		/** Called whenever the text is committed.  This happens when the user presses enter or the text box loses focus. */
		SLATE_EVENT( FOnTextCommitted, OnTextCommitted )

		/** Called whenever the text is changed programmatically or interactively by the user */
		SLATE_EVENT( FOnVerifyTextChanged, OnVerifyTextChanged )

		/** Called whenever the horizontal scrollbar is moved by the user */
		SLATE_EVENT( FOnUserScrolled, OnHScrollBarUserScrolled )

		/** Called whenever the vertical scrollbar is moved by the user */
		SLATE_EVENT( FOnUserScrolled, OnVScrollBarUserScrolled )

		/** Called when the cursor is moved within the text area */
		SLATE_EVENT( SMultiLineEditableText::FOnCursorMoved, OnCursorMoved )

		/** Callback delegate to have first chance handling of the OnKeyChar event */
		SLATE_EVENT(FOnKeyChar, OnKeyCharHandler)

		/** Callback delegate to have first chance handling of the OnKeyDown event */
		SLATE_EVENT(FOnKeyDown, OnKeyDownHandler)

		/** Menu extender for the right-click context menu */
		//SLATE_EVENT( FMenuExtensionDelegate, ContextMenuExtender )

		/** Delegate used to create text layouts for this widget. If none is provided then FSlateTextLayout will be used. */
		SLATE_EVENT( FCreateSlateTextLayout, CreateSlateTextLayout )

		/** Whether text wraps onto a new line when it's length exceeds this width; if this value is zero or negative, no wrapping occurs. */
		SLATE_ATTRIBUTE( float, WrapTextAt )

		/** Whether to wrap text automatically based on the widget's computed horizontal space.  IMPORTANT: Using automatic wrapping can result
			in visual artifacts, as the the wrapped size will computed be at least one frame late!  Consider using WrapTextAt instead.  The initial 
			desired size will not be clamped.  This works best in cases where the text block's size is not affecting other widget's layout. */
		SLATE_ATTRIBUTE( bool, AutoWrapText )

		/** The wrapping policy to use */
		SLATE_ATTRIBUTE( ETextWrappingPolicy, WrappingPolicy )

		/** Whether to select all text when pressing enter to commit changes */
		SLATE_ATTRIBUTE( bool, SelectAllTextOnCommit )

		/** Whether to select word on mouse double click on the widget */
		SLATE_ATTRIBUTE(bool, SelectWordOnMouseDoubleClick)

		/** The color of the background/border around the editable text (overrides Style) */
		SLATE_ATTRIBUTE( FSlateColor, BackgroundColor )

		/** Padding between the box/border and the text widget inside (overrides Style) */
		SLATE_ATTRIBUTE( FMargin, Padding )

		/** The amount of blank space left around the edges of text area. 
			This is different to Padding because this area is still considered part of the text area, and as such, can still be interacted with */
		SLATE_ATTRIBUTE( FMargin, Margin )

		/** Provide a alternative mechanism for error reporting. */
		SLATE_ARGUMENT( TSharedPtr<class IErrorReportingWidget>, ErrorReporting )

		/** The optional modifier key necessary to create a newline when typing into the editor. */
		SLATE_ARGUMENT( EModifierKey::Type, ModiferKeyForNewLine)

		/** Additional options used by the virtual keyboard summoned by this widget */
		SLATE_ARGUMENT( FVirtualKeyboardOptions, VirtualKeyboardOptions  )

		/** The type of event that will trigger the display of the virtual keyboard */
		SLATE_ATTRIBUTE( EVirtualKeyboardTrigger, VirtualKeyboardTrigger )

		/** The message action to take when the virtual keyboard is dismissed by the user */
		SLATE_ATTRIBUTE( EVirtualKeyboardDismissAction, VirtualKeyboardDismissAction )

		/** Which text shaping method should we use? (unset to use the default returned by GetDefaultTextShapingMethod) */
		SLATE_ARGUMENT( TOptional<ETextShapingMethod>, TextShapingMethod )
		
		/** Which text flow direction should we use? (unset to use the default returned by GetDefaultTextFlowDirection) */
		SLATE_ARGUMENT( TOptional<ETextFlowDirection>, TextFlowDirection )

		/** Determines what happens to text that is clipped and doesn't fit within the allotted area for this widget */
		SLATE_ARGUMENT(TOptional<ETextOverflowPolicy>, OverflowPolicy)

		SLATE_EVENT(FNewChildEditorCreatedDelegate, OnNewChildEditorCreated)

		SLATE_EVENT(FSimpleStringDelegate, OnTextCommitedFromToolEditor)
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	TSharedPtr<SWidget> NotifyRequestContextMenu();
	void AddNewChildObjectEditor(TSharedRef<SLuaScriptBoxSubEditor> newSubEditor);

	void NotifyMarshallerReport(const TArray<FLuaSyntaxReportEntry>& report);
	void NotifyCursorMoved(const FTextLocation& textLocation);
	void NotifyTextChanged(const FText& newText);
	void UpdateVarText();

	virtual FReply OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent) override;
	FReply NotifyKeyDown(const FGeometry& geometry, const FKeyEvent& keyEvent);
	
	void InsertTextAtCursor(const FString& text);
	bool AnyTextSelected() const;
	TSharedPtr<SMultiLineEditableTextBoxEx> GetEditableText();
	FTextSelection GetTextSelection() const;
	FText GetSelectedText();
	void DeleteSelectedText();
	FTextLocation GetCursorLocation() const;
	void GoTo(FTextLocation textLocation);
	void SelectText(FTextLocation cursorLocation, FTextLocation endLocation);
	void SetStyle(const FEditableTextBoxStyle* EditableTextBoxStyle);
	FText GetText() const;
	void GetCurrentTextLine(FString& outTextLine);
	void SetText(const FText& newText);
	
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
private:
	void CheckPreviousForIntellisenseMenuForProperties();
	void CheckForIntellisenseMenuForProperties();
	void CheckForIntellisenseMenuForUFunctions();
	void ShowIntellisenseMenuWithFieldsFor(UStruct* ustruct);
	void ShowIntellisenseMenuWithFunctionsFor(UClass* uclass);
	void MoveIntellisenseSelectionUp();
	void MoveIntellisenseSelectionDown();
	void ConfirmIntellisenseSelection();
	void CloseIntellisenseSelection();
	void CloseIntellisenseSelection2();

public:
	TSharedRef<FSlateTextLayout> RequestTextLayout(SWidget* InOwningWidget, const FTextBlockStyle& InDefaultTextStyle);
	
	TSharedPtr<SMultiLineEditableTextBoxEx> LuaScriptEditableTextBox = {};
	
	FOnTextChanged OnTextChanged = {};
	TSharedPtr<FUnrealLuaSyntaxLayoutMarshaller> Marshaller;
	TSharedPtr<SExpandableArea> ErrorReportSection;
	TSharedPtr<SScrollBox> ErrorReportScrollBox;
	TSharedPtr<STextBlock> PrevRunInfoBox;
	TSharedPtr<STextBlock> TypeInfoBox;
	
	TSharedPtr<SUnrealLuaDraggableBoxOverlay> IntellisenseOverlay = {};
	TSharedPtr<SMenuAnchor> IntellisenseMenuAnchor = {};
	
	FNewChildEditorCreatedDelegate OnNewChildEditorCreated;
	
	FOnKeyDown OnKeyDown = {};
	
	TSharedPtr<FLuaScriptTextLayout> TextLayout = {};
	TWeakInterfacePtr<ILuaToolsSession> Session = {};
	TSharedPtr<SLuaIntelliSenseWidget> IntelliSenseWidget = {};
	void NotifySelectProperty(FProperty* prop, bool checked);
};
