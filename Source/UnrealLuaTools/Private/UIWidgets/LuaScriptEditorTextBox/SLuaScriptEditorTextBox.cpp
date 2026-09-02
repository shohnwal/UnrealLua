// Fill out your copyright notice in the Description page of Project Settings.


#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptEditorTextBox.h"

#include "SlateOptMacros.h"
#include "UnrealEngine.h"
#include "Framework/Application/SlateApplication.h"
#include "IntelliSense/UnrealLuaSyntaxLayoutMarshaller.h"
#include "IntelliSense/UnrealLuaSyntaxParserScope.h"
#include "IntelliSense/Runs/LuaSyntaxTextRunVariable.h"
#include "LuaContext/ScopedLuaContext.h"
#include "LuaTypes/LuaPrimitives.h"
#include "Styling/StyleColors.h"
#include "UIWidgets/SMultiLineEditableTextBoxEx.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Text/STextBlock.h"
#include "UnrealLua.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "IntelliSense/Runs/LuaSyntaxTextRunVariablePeriodFieldAccessOperator.h"
#include "UIWidgets/SPropertySelectionList.h"
#include "UIWidgets/Draggable/SUnrealLuaDraggableBoxOverlay.h"
#include "UIWidgets/IntelliSense/SLuaIntelliSenseWidget.h"
#include "UIWidgets/LuaScriptEditorTextBox/LuaScriptTextLayout.h"
#include "Widgets/SViewport.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Input/SMenuAnchor.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SLuaScriptEditorTextBox::Construct(const FArguments& InArgs)
{
	FCreateSlateTextLayout createLayoutDelegate;
	createLayoutDelegate.BindSP(this, &SLuaScriptEditorTextBox::RequestTextLayout);
	
	this->Session = InArgs._Session;
	this->OnTextChanged = InArgs._OnTextChanged;
	this->Marshaller = FUnrealLuaSyntaxLayoutMarshaller::Create(InArgs._Session);
	this->Marshaller->OnReport.BindRaw(this, &SLuaScriptEditorTextBox::NotifyMarshallerReport);
	this->OnNewChildEditorCreated = InArgs._OnNewChildEditorCreated;
	this->OnKeyDown = InArgs._OnKeyDownHandler;
	this->ChildSlot
	[
		SNew(SSplitter)
		.Orientation(EOrientation::Orient_Vertical)
		+ SSplitter::Slot()
		.MinSize(200)
		.Value(0.9)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SAssignNew(this->LuaScriptEditableTextBox, SMultiLineEditableTextBoxEx)
					.Text(InArgs._Text)
					.ClearTextSelectionOnFocusLoss(InArgs._ClearTextSelectionOnFocusLoss)
					.Style(UnrealLuaTools::SlateStyles::GetEditableTextBoxStyle())
					.OnTextChanged(this, &SLuaScriptEditorTextBox::NotifyTextChanged)
					.OnKeyDownHandler_Raw(this, &SLuaScriptEditorTextBox::NotifyKeyDown)
					.OnKeyCharHandler_Raw(this, &SLuaScriptEditorTextBox::OnKeyChar)
					.AllowContextMenu(InArgs._AllowContextMenu)
					.OnContextMenuOpening(this, &SLuaScriptEditorTextBox::NotifyRequestContextMenu)
					.Marshaller(this->Marshaller)
					.IsReadOnly(InArgs._IsReadOnly)
					.CreateSlateTextLayout(createLayoutDelegate)
					.OnCursorMoved(this, &SLuaScriptEditorTextBox::NotifyCursorMoved)
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Top)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(TypeInfoBox, STextBlock)
					.Text(FText::AsCultureInvariant("new"))					
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(PrevRunInfoBox, STextBlock)
					.Text(FText::AsCultureInvariant("new"))					
				]
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SAssignNew(IntellisenseOverlay, SUnrealLuaDraggableBoxOverlay)
				.Visibility(EVisibility::SelfHitTestInvisible)
				.IsDraggable(false)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Top)
				.Content()
				[
					SAssignNew(this->IntellisenseMenuAnchor, SMenuAnchor)
					//.Placement(EMenuPlacement::MenuPlacement_MatchBottomLeft)
					.Placement(EMenuPlacement::MenuPlacement_MenuRight)
					.Padding(20)
					.MenuContent(
						SNew(SOverlay)
						//+SOverlay::Slot()
						//[
						//	SNew(SColorBlock)
						//	.Size(FVector2D{200, 100})
						//	.Color(FLinearColor{0.2,0.2,0.2,1})	
						//]
						+SOverlay::Slot()
						[
							SAssignNew(IntelliSenseWidget, SLuaIntelliSenseWidget)
						]
					)
				]
			]
		]
		+ SSplitter::Slot()
		.Value(0.1)
		[
			SAssignNew(ErrorReportSection, SExpandableArea)
			.AreaTitle(FText::AsCultureInvariant("Code analysis"))
			.Padding(2.0f)
			
			.InitiallyCollapsed(false)
			.Visibility(EVisibility::Collapsed)
			.BodyContent()
			[
				SNew(SBox)
				.MaxDesiredHeight(300)
				[
					SAssignNew(ErrorReportScrollBox, SScrollBox)
					.Orientation(EOrientation::Orient_Vertical)								
				]
			]
		]
	];
	this->IntellisenseMenuAnchor->SetIsOpen(false,false);
}


TSharedPtr<SWidget> SLuaScriptEditorTextBox::NotifyRequestContextMenu()
{
	if (this->GetEditableText()->GetEditableText()->IsTextReadOnly())
	{
		return nullptr;
	}
	return UnrealLuaTools::ContextMenuBuilder::BuildMenu(SharedThis(this));
}

void SLuaScriptEditorTextBox::AddNewChildObjectEditor(TSharedRef<SLuaScriptBoxSubEditor> newSubEditor)
{
	this->OnNewChildEditorCreated.ExecuteIfBound(newSubEditor);
}

void SLuaScriptEditorTextBox::NotifyMarshallerReport(const TArray<FLuaSyntaxReportEntry>& report)
{
	if (!this->ErrorReportScrollBox.IsValid())
	{
		return;
	}
	this->ErrorReportScrollBox->ClearChildren();
	if (report.IsEmpty())
	{
		this->ErrorReportSection->SetVisibility(EVisibility::Collapsed);
	}
	else
	{
		this->ErrorReportSection->SetVisibility(EVisibility::SelfHitTestInvisible);
		for (const FLuaSyntaxReportEntry& entry : report)
		{
			this->ErrorReportScrollBox->AddSlot()
			[
				SNew(SButton)
				[
					SNew(STextBlock)
					.Text(FText::AsCultureInvariant(entry.Entry))					
				]
				.ButtonColorAndOpacity( FStyleColors::Transparent)
				.IsFocusable(false)
				.OnClicked_Lambda([this, line = entry.Line, offset = entry.Offset]()
				{
					FTextLocation cursorLoc{line, offset.BeginIndex};
					FTextLocation endLoc{line, offset.EndIndex};
					
					
					FSlateApplication::Get().SetKeyboardFocus(this->LuaScriptEditableTextBox->GetEditableText());
					this->LuaScriptEditableTextBox->ScrollTo(cursorLoc);
					this->LuaScriptEditableTextBox->GoTo(cursorLoc);
					this->LuaScriptEditableTextBox->SelectText(cursorLoc, endLoc);
					return FReply::Handled();
				})
			]
			.AutoSize()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top);
		}
	}
}

void SLuaScriptEditorTextBox::NotifyCursorMoved(const FTextLocation& textLocation)
{
	//UE_LOG(LogTemp, Log, TEXT("SLuaScriptEditorTextBox::NotifyCursorMoved"));
	FVector2D caretLocation = this->TextLayout->GetLocationAt(textLocation, true);
	//const FGeometry& textgeo = this->GetEditableText()->GetEditableText()->GetTickSpaceGeometry();
	//if (this->Session->GetSessionType() == ELuaToolsSessionType::Game)
	//{
	//FVector2D	cursorLocation = caretLocation * (textgeo.Scale != 0.0f ? textgeo.Scale : 1.f);
	//}
	//cursorLocation = textgeo.LocalToAbsolute(cursorLocation);
	//FVector2f menuLocation = textgeo.LocalToAbsolute(caretLocation);
	//FSlateApplication::Get().SetCursorPos(cursorLocation);
	this->UpdateVarText();
	
	FTextLocation loc = this->GetCursorLocation();

	this->IntellisenseOverlay->SetBoxAlignmentOffset(FVector2f{caretLocation}, false);
}

void SLuaScriptEditorTextBox::NotifyTextChanged(const FText& newText)
{
	//UE_LOG(LogTemp, Log, TEXT("SLuaScriptEditorTextBox::NotifyTextChanged"));
	//this->UpdateVarText();
	//this->UpdateVarText();
	this->OnTextChanged.ExecuteIfBound(newText);
}

void SLuaScriptEditorTextBox::UpdateVarText()
{
	//UE_LOG(LogTemp, Log, TEXT("SLuaScriptEditorTextBox::UpdateVarText"));
	
	FTextLocation loc = this->GetCursorLocation();
	TSharedPtr<const IRun> run = this->LuaScriptEditableTextBox->GetRunUnderCursor();
	if (run.IsValid() && run->GetRunInfo().Name == FUnrealLuaSyntaxTextRun::RunInfoName)
	{
		//we have an unreallua run
		TSharedPtr<const FUnrealLuaSyntaxTextRun> syntaxRun = StaticCastSharedPtr<const FUnrealLuaSyntaxTextRun>(run);
		FString runType = syntaxRun->GetRunName();
		if (syntaxRun->IsRunType(ELuaSyntaxTextRunType::Variable))
		{
			TSharedPtr<const FLuaSyntaxTextRunVariable> variableRun = StaticCastSharedPtr<const FLuaSyntaxTextRunVariable>(run);
			if (variableRun->Variable.IsValid())
			{
				FString varName = variableRun->GetVariableName();
				FString varType = variableRun->Variable->GetType();
				if (this->Session.IsValid() && variableRun->Variable->HasVarTypeString())
				{
					const TSharedPtr<FUnrealLuaSyntaxVariable>& var = variableRun->Variable;
					if (!var->IsResolved())
					{
						TSharedPtr<FScopedLuaContext> ctx = this->Session->GetScopedLuaContext();
						var->ResolveType(ctx->GetLuaState());						
					}

					FLuaValue& val = var->GetLuaValue();
					varType = val.GetTypeString();
					
					//this->IntellisenseMenuAnchor->SetIsOpen(true, false)
				}
				
				FString typeName = "Curr RunType: " + runType + ", VarName " + varName + ", VarType: " + varType;	
				this->TypeInfoBox->SetText(FText::AsCultureInvariant(typeName));
			}
			else
			{
				this->TypeInfoBox->SetText(FText::AsCultureInvariant("Curr RunType: " + runType + ", Var:Unassigned"));		
			}
		}
		else
		{
			this->TypeInfoBox->SetText(FText::AsCultureInvariant("Curr RunType: " + runType));	
		}
		
		if (syntaxRun->PreviousRun.IsValid())
		{
			TSharedPtr<FUnrealLuaSyntaxTextRun> previousRun = syntaxRun->PreviousRun.Pin();
			FString prevRunType = previousRun->GetRunName();

			if (previousRun->IsRunType(ELuaSyntaxTextRunType::Variable))
			{
				TSharedPtr<const FLuaSyntaxTextRunVariable> previousVariableRun = StaticCastSharedPtr<const FLuaSyntaxTextRunVariable>(previousRun);
				verify(previousVariableRun.IsValid())
				if (previousVariableRun->Variable.IsValid())
				{

					FString varName = previousVariableRun->GetVariableName();
					FString varType = previousVariableRun->Variable->GetType();
					if (this->Session.IsValid() && previousVariableRun->Variable->HasVarTypeString())
					{
						TSharedPtr<FScopedLuaContext> ctx = this->Session->GetScopedLuaContext();
						sol::object import = ctx->GetLuaState()["UE"][varType];
						FLuaValue val{import};
						std::string varTypeStr = UnrealLua::LuaTypes::TypeInfo::UType(import);
						varType = varTypeStr.c_str();
				
						//this->IntellisenseMenuAnchor->SetIsOpen(true, false)
					}
					FString typeName = "Prev RunType: " + prevRunType + ", VarName " + varName + ", VarType: " + varType;
					this->PrevRunInfoBox->SetText(FText::AsCultureInvariant(typeName));
					return;
				}
				else
				{
					this->PrevRunInfoBox->SetText(FText::AsCultureInvariant("Prev RunType: " + prevRunType + ", Var:Unassigned"));		
				}
			}
			else
			{
				this->PrevRunInfoBox->SetText(FText::AsCultureInvariant("Prev RunType: " + prevRunType));	
			}
		}
		else
		{
			this->PrevRunInfoBox->SetText(FText::AsCultureInvariant("No Prev Run"));		
		}
	}
	else
	{
		this->TypeInfoBox->SetText(FText::AsCultureInvariant("No Curr Run"));
		this->PrevRunInfoBox->SetText(FText::AsCultureInvariant("No Prev Run"));	
	}
}

FReply SLuaScriptEditorTextBox::OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent)
{
	if (InCharacterEvent.GetCharacter() == ':')
	{
		//UE_LOG(LogTemp, Log, TEXT("SLuaScriptEditorTextBox::pressed colon"));
		this->CheckForIntellisenseMenuForUFunctions();
	}
	else if (InCharacterEvent.GetCharacter() == '.')
	{
		//UE_LOG(LogTemp, Log, TEXT("SLuaScriptEditorTextBox::pressed period"));
		this->CheckForIntellisenseMenuForProperties();
	}
	else if (TChar<TCHAR>::IsIdentifier(InCharacterEvent.GetCharacter()))
	{
		this->CheckPreviousForIntellisenseMenuForProperties();
	}
	return FReply::Unhandled();
}

FReply SLuaScriptEditorTextBox::NotifyKeyDown(const FGeometry& geometry, const FKeyEvent& keyEvent)
{
	FReply reply = FReply::Unhandled();
	if (this->OnKeyDown.IsBound())
	{
		reply = this->OnKeyDown.Execute(geometry,keyEvent);
	}
	if (reply.IsEventHandled())
	{
		return reply;
	}
	if (this->IntellisenseMenuAnchor->IsOpen())
	{
		if (keyEvent.GetKey() == EKeys::Up)
		{
			this->MoveIntellisenseSelectionUp();
			return FReply::Handled();
		}
		else if (keyEvent.GetKey() == EKeys::Down)
		{
			this->MoveIntellisenseSelectionDown();
			return FReply::Handled();
		}
		else if (keyEvent.GetKey() == EKeys::Left)
		{
			this->CloseIntellisenseSelection();
			return FReply::Handled();
		}
		else if (keyEvent.GetKey() == EKeys::Right)
		{
			this->CloseIntellisenseSelection();
			return FReply::Handled();
		}
		else if (keyEvent.GetKey() == EKeys::Enter)
		{
			this->ConfirmIntellisenseSelection();
			return FReply::Handled();
		}
		else if (keyEvent.GetKey() == EKeys::Escape)
		{
			this->CloseIntellisenseSelection();
			return FReply::Handled();
		}
		else if (keyEvent.GetKey() == EKeys::BackSpace)
		{
			this->CloseIntellisenseSelection2();
			return FReply::Handled();
		}
	}
	return FReply::Unhandled();
}


void SLuaScriptEditorTextBox::InsertTextAtCursor(const FString& text)
{
	this->LuaScriptEditableTextBox->InsertTextAtCursor(text);
}

bool SLuaScriptEditorTextBox::AnyTextSelected() const
{
	return this->LuaScriptEditableTextBox->AnyTextSelected();
}

TSharedPtr<SMultiLineEditableTextBoxEx> SLuaScriptEditorTextBox::GetEditableText()
{
	return this->LuaScriptEditableTextBox;
}

FTextSelection SLuaScriptEditorTextBox::GetTextSelection() const
{
	return this->LuaScriptEditableTextBox->GetTextSelection();
}

FText SLuaScriptEditorTextBox::GetSelectedText()
{
	return this->LuaScriptEditableTextBox->GetSelectedText();
}

void SLuaScriptEditorTextBox::DeleteSelectedText()
{
	this->LuaScriptEditableTextBox->DeleteSelectedText();
}

FTextLocation SLuaScriptEditorTextBox::GetCursorLocation() const
{
	return this->LuaScriptEditableTextBox->GetCursorLocation();
}

void SLuaScriptEditorTextBox::GoTo(FTextLocation textLocation)
{
	this->LuaScriptEditableTextBox->GoTo(textLocation);
}

void SLuaScriptEditorTextBox::SelectText(FTextLocation cursorLocation, FTextLocation endLocation)
{
	this->LuaScriptEditableTextBox->SelectText(cursorLocation, endLocation);
}

void SLuaScriptEditorTextBox::SetStyle(const FEditableTextBoxStyle* EditableTextBoxStyle)
{
	this->LuaScriptEditableTextBox->SetStyle(EditableTextBoxStyle);
}

FText SLuaScriptEditorTextBox::GetText() const
{
	return this->LuaScriptEditableTextBox->GetText();
}

void SLuaScriptEditorTextBox::GetCurrentTextLine(FString& outTextLine)
{
	this->LuaScriptEditableTextBox->GetCurrentTextLine(outTextLine);
}

void SLuaScriptEditorTextBox::SetText(const FText& newText)
{
	this->LuaScriptEditableTextBox->SetText(newText);
}

void SLuaScriptEditorTextBox::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	if (this->IntellisenseMenuAnchor->IsOpen())
	{
		TSharedPtr<const IRun> run = this->LuaScriptEditableTextBox->GetRunUnderCursor();
		if (run.IsValid() && run->GetRunInfo().Name == FUnrealLuaSyntaxTextRun::RunInfoName)
		{
			//we have an unreallua run
			TSharedPtr<const FUnrealLuaSyntaxTextRun> syntaxRun = StaticCastSharedPtr<const FUnrealLuaSyntaxTextRun>(run);
			if (syntaxRun->IsRunType(ELuaSyntaxTextRunType::Variable))
			{
				TSharedPtr<const FLuaSyntaxTextRunVariable> variableRun = StaticCastSharedPtr<const FLuaSyntaxTextRunVariable>(run);
				if (variableRun->Variable.IsValid())
				{
					if (!this->IntelliSenseWidget->FilterResults(variableRun->Variable->VariableName))
					{
						this->CloseIntellisenseSelection();
					}
				}
			}
		}	
	}
}

void SLuaScriptEditorTextBox::CheckForIntellisenseMenuForProperties()
{
	UE_LOG(LogTemp, Log, TEXT("SLuaScriptEditorTextBox::CheckForIntellisenseMenu from Layout change"));
	TSharedPtr<const IRun> run = this->LuaScriptEditableTextBox->GetRunUnderCursor();
	if (run.IsValid() && run->GetRunInfo().Name == FUnrealLuaSyntaxTextRun::RunInfoName)
	{
		//we have an unreallua run
		TSharedPtr<const FUnrealLuaSyntaxTextRun> syntaxRun = StaticCastSharedPtr<const FUnrealLuaSyntaxTextRun>(run);
		if (syntaxRun.IsValid() && syntaxRun->IsRunType(ELuaSyntaxTextRunType::Variable))
		{
			UE_LOG(LogTemp, Log, TEXT("SLuaScriptEditorTextBox::UpdateVarText should open menu now"));
			TSharedPtr<const FLuaSyntaxTextRunVariable> variableRun = StaticCastSharedPtr<const FLuaSyntaxTextRunVariable>(syntaxRun);
			if (variableRun->CanIndex())
			{
				const TSharedPtr<FUnrealLuaSyntaxVariable>& var = variableRun->Variable;
				
				if (this->Session.IsValid())
				{
					if (!var->IsResolved())
					{
						TSharedPtr<FScopedLuaContext> ctx = this->Session->GetScopedLuaContext();
						var->ResolveType(ctx->GetLuaState());						
					}
					if (!var->IsResolved())
					{
					
					}
					FLuaValue& val = var->GetLuaValue();
					switch (val.GetTypeIndex())
					{
					case LuaValueData::IndexOfType<FLuaUClass>():
						this->ShowIntellisenseMenuWithFieldsFor(val.Get<FLuaUClass>().TryLoadClass());
						break;
	
					case LuaValueData::IndexOfType<FLuaUStruct>():
						this->ShowIntellisenseMenuWithFieldsFor(val.Get<FLuaUStruct>().TryLoad());
						break;
					default:
						break;
					}						
				}
				if (this->IntellisenseMenuAnchor->IsOpen())
				{
					this->IntelliSenseWidget->FilterResults(variableRun->GetVariableName());
				}
			}
		}
		else if (syntaxRun.IsValid() && syntaxRun->IsRunType(ELuaSyntaxTextRunType::Period))
		{
			UE_LOG(LogTemp, Log, TEXT("SLuaScriptEditorTextBox::UpdateVarText should open menu now"));
			TSharedPtr<const FLuaSyntaxTextRunVariable> variableRun = StaticCastSharedPtr<const FLuaSyntaxTextRunVariable>(syntaxRun);
			if (variableRun->CanIndex())
			{
				const TSharedPtr<FUnrealLuaSyntaxVariable>& var = variableRun->Variable;
				
				if (this->Session.IsValid())
				{
					if (!var->IsResolved())
					{
						TSharedPtr<FScopedLuaContext> ctx = this->Session->GetScopedLuaContext();
						var->ResolveType(ctx->GetLuaState());						
					}
					if (!var->IsResolved())
					{
					
					}
					FLuaValue& val = var->GetLuaValue();
					switch (val.GetTypeIndex())
					{
					case LuaValueData::IndexOfType<FLuaUClass>():
						this->ShowIntellisenseMenuWithFieldsFor(val.Get<FLuaUClass>().TryLoadClass());
						break;
	
					case LuaValueData::IndexOfType<FLuaUStruct>():
						this->ShowIntellisenseMenuWithFieldsFor(val.Get<FLuaUStruct>().TryLoad());
						break;
					default:
						break;
					}						
				}
				if (this->IntellisenseMenuAnchor->IsOpen())
				{
					this->IntelliSenseWidget->FilterResults(variableRun->GetVariableName());
				}
			}
		}
	}
}


void SLuaScriptEditorTextBox::CheckPreviousForIntellisenseMenuForProperties()
{
	if (!this->IntellisenseMenuAnchor->IsOpen())
	{
		this->UpdateVarText();
		TSharedPtr<const IRun> run = this->LuaScriptEditableTextBox->GetRunUnderCursor();
		if (run.IsValid() && run->GetRunInfo().Name == FUnrealLuaSyntaxTextRun::RunInfoName)
		{
			//we have an unreallua run
			TSharedPtr<const FUnrealLuaSyntaxTextRun> syntaxRun = StaticCastSharedPtr<const FUnrealLuaSyntaxTextRun>(run);
			if (syntaxRun.IsValid() && syntaxRun->HasPreviousRunType(ELuaSyntaxTextRunType::Period))
			{
				TSharedPtr<const FLuaSyntaxTextRunVariablePeriodFieldAccessOperator> periodRun = syntaxRun->PreviousAs<const FLuaSyntaxTextRunVariablePeriodFieldAccessOperator>();
				if (periodRun->HasPreviousRunType(ELuaSyntaxTextRunType::Variable))
				{
					TSharedPtr<FLuaSyntaxTextRunVariable> variableRun = periodRun->PreviousAs<FLuaSyntaxTextRunVariable>();
					if (variableRun->CanIndex())
					{
						const TSharedPtr<FUnrealLuaSyntaxVariable>& var = variableRun->Variable;
				
						if (this->Session.IsValid())
						{
							if (!var->IsResolved())
							{
								TSharedPtr<FScopedLuaContext> ctx = this->Session->GetScopedLuaContext();
								var->ResolveType(ctx->GetLuaState());						
							}
							if (!var->IsResolved())
							{
					
							}
							FLuaValue& val = var->GetLuaValue();
							switch (val.GetTypeIndex())
							{
							case LuaValueData::IndexOfType<FLuaUClass>():
								this->ShowIntellisenseMenuWithFieldsFor(val.Get<FLuaUClass>().TryLoadClass());
								break;
	
							case LuaValueData::IndexOfType<FLuaUStruct>():
								this->ShowIntellisenseMenuWithFieldsFor(val.Get<FLuaUStruct>().TryLoad());
								break;
							default:
								break;
							}						
						}
					}
				}
			}
		}	
	}
}


void SLuaScriptEditorTextBox::CheckForIntellisenseMenuForUFunctions()
{
	UE_LOG(LogTemp, Log, TEXT("SLuaScriptEditorTextBox::CheckForIntellisenseMenu from Layout change"));
	this->UpdateVarText();
	TSharedPtr<const IRun> run = this->LuaScriptEditableTextBox->GetRunUnderCursor();
	if (run.IsValid() && run->GetRunInfo().Name == FUnrealLuaSyntaxTextRun::RunInfoName)
	{
		//we have an unreallua run
		TSharedPtr<const FUnrealLuaSyntaxTextRun> syntaxRun = StaticCastSharedPtr<const FUnrealLuaSyntaxTextRun>(run);
		if (syntaxRun.IsValid() && syntaxRun->IsRunType(ELuaSyntaxTextRunType::Variable))
		{
			UE_LOG(LogTemp, Log, TEXT("SLuaScriptEditorTextBox::UpdateVarText should open menu now"));
			TSharedPtr<const FLuaSyntaxTextRunVariable> variableRun = StaticCastSharedPtr<const FLuaSyntaxTextRunVariable>(syntaxRun);
			if (variableRun->CanIndex())
			{
				const TSharedPtr<FUnrealLuaSyntaxVariable>& var = variableRun->Variable;
				if (!var->IsResolved())
				{
					if (this->Session.IsValid())
					{
						TSharedPtr<FScopedLuaContext> ctx = this->Session->GetScopedLuaContext();
						var->ResolveType(ctx->GetLuaState());
					}
				}
				FLuaValue& val = var->GetLuaValue();
				if (val.IsType<FLuaUClass>())
				{
					this->ShowIntellisenseMenuWithFunctionsFor(val.Get<FLuaUClass>().TryLoadClass());
				}
			}
		}
	}
}

void SLuaScriptEditorTextBox::ShowIntellisenseMenuWithFieldsFor(UStruct* ustruct)
{
	if (this->IntelliSenseWidget->ShowPropertiesOfStruct(ustruct))
	{
		UE_LOG(LogTemp, Log, TEXT("Open intellisense menu"));
		this->IntellisenseMenuAnchor->SetIsOpen(true, false);
	}
}

void SLuaScriptEditorTextBox::ShowIntellisenseMenuWithFunctionsFor(UClass* uclass)
{
	if (this->IntelliSenseWidget->ShowUFunctionsOfUClass(uclass))
	{
		UE_LOG(LogTemp, Log, TEXT("Open intellisense menu"));
		this->IntellisenseMenuAnchor->SetIsOpen(true, false);
	}
}

void SLuaScriptEditorTextBox::MoveIntellisenseSelectionUp()
{
	if (this->IntellisenseMenuAnchor->IsOpen())
	{
		this->IntelliSenseWidget->MoveSelectionUp();
	}
}

void SLuaScriptEditorTextBox::MoveIntellisenseSelectionDown()
{
	if (this->IntellisenseMenuAnchor->IsOpen())
	{
		this->IntelliSenseWidget->MoveSelectionDown();
	}
}

void SLuaScriptEditorTextBox::ConfirmIntellisenseSelection()
{
	if (this->IntellisenseMenuAnchor->IsOpen())
	{
		FString selected = this->IntelliSenseWidget->GetCurrentlySelectedItem();
		if (!selected.IsEmpty())
		{
			auto run = this->GetEditableText()->GetRunUnderCursor();
			if (run.IsValid() && run->GetRunInfo().Name == FUnrealLuaSyntaxTextRun::RunInfoName)
			{
				//we have an unreallua run
				TSharedPtr<const FUnrealLuaSyntaxTextRun> syntaxRun = StaticCastSharedPtr<const FUnrealLuaSyntaxTextRun>(run);
				if (syntaxRun.IsValid() && syntaxRun->IsRunType(ELuaSyntaxTextRunType::Variable))
				{
					FTextLocation cursorLocation = this->GetEditableText()->GetCursorLocation();
					FTextRange range = run->GetTextRange();
					FTextLocation selectbegin = { cursorLocation.GetLineIndex(), range.BeginIndex};
				
					this->GetEditableText()->GoTo(selectbegin);
				
					FTextLocation selectend = {cursorLocation.GetLineIndex(), range.EndIndex};
					this->GetEditableText()->SelectText(selectbegin, selectend);	
				}
			}
			this->GetEditableText()->InsertTextAtCursor(selected);
			this->CheckForIntellisenseMenuForProperties();
		}
		this->IntellisenseMenuAnchor->SetIsOpen(false);
	}
}

void SLuaScriptEditorTextBox::CloseIntellisenseSelection()
{
	this->IntelliSenseWidget->ShowPropertiesOfStruct(nullptr);
	this->IntellisenseMenuAnchor->SetIsOpen(false);
}

void SLuaScriptEditorTextBox::CloseIntellisenseSelection2()
{
	TSharedPtr<const IRun> run = this->LuaScriptEditableTextBox->GetRunUnderCursor();
	if (run.IsValid() && run->GetRunInfo().Name == FUnrealLuaSyntaxTextRun::RunInfoName)
	{
		//we have an unreallua run
		TSharedPtr<const FUnrealLuaSyntaxTextRun> syntaxRun = StaticCastSharedPtr<const FUnrealLuaSyntaxTextRun>(run);
		if (syntaxRun.IsValid() && syntaxRun->IsRunType(ELuaSyntaxTextRunType::Period))
		{
			this->CloseIntellisenseSelection();
		}
	}
}


TSharedRef<FSlateTextLayout> SLuaScriptEditorTextBox::RequestTextLayout(SWidget* InOwningWidget, const FTextBlockStyle& InDefaultTextStyle)
{
	this->TextLayout = FLuaScriptTextLayout::Create(InOwningWidget, InDefaultTextStyle);
	//this->TextLayout->OnLayoutChanged.BindRaw(this, &SLuaScriptEditorTextBox::CheckForIntellisenseMenu);
	//this->TextLayout->OnLayoutChanged.BindRaw(this, &SLuaScriptEditorTextBox::CheckForIntellisenseMenu);
	return this->TextLayout.ToSharedRef();
}

void SLuaScriptEditorTextBox::NotifySelectProperty(FProperty* prop, bool checked)
{
	this->IntellisenseMenuAnchor->SetIsOpen(false);
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
