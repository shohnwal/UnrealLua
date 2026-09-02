// Fill out your copyright notice in the Description page of Project Settings.


#include "UIWidgets/SLuaScriptValueListElementWidget.h"

#include "SlateOptMacros.h"
#include "Components/HorizontalBox.h"
#include "Framework/Application/SlateApplication.h"
#include "LuaValue/LuaScriptValue.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SLuaScriptValueListElementWidget::Construct(const FArguments& InArgs)
{
	this->OnRequestEditValue = InArgs._OnRequestEditValue;
	this->OnSelectUObject = InArgs._OnSelectUObject;
	FLuaScriptValue* val = InArgs._LuaScriptValue;
	verify(val != nullptr)
	bool bInitiallyOpen = InArgs._InitiallyOpen;	
	this->KeyString = val->GetKeyNameString();
	FString valStr = val->GetLuaValue().ToStringForStructBuilderEditor();
	FString typeStr = val->GetLuaValue().GetTypeString();
	FOnLuaScriptValueChangedNativeDelegate del;
	del.BindSP(this, &SLuaScriptValueListElementWidget::NotifyLuaScriptValueChanged);
	val->AddOnValueChangedDelegate(del);
	
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(SButton)
				[
					SAssignNew(KeyLabelText, STextBlock)
					.Text(FText::AsCultureInvariant(this->KeyString))
				]
				.OnClicked(this, &SLuaScriptValueListElementWidget::ToggleContentVisbility)
			]
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)			
		]
		.AutoHeight()
		+ SVerticalBox::Slot()
		[
			SAssignNew(ValueContentBorder, SBorder)
			[
				//[Type  :         <Type>]
				//[Value :         <value>]
				SNew(SButton)
				.OnClicked(this, &SLuaScriptValueListElementWidget::NotifyValueContentButtonPressed)
				.ToolTip(FSlateApplication::Get().MakeToolTip(FText::AsCultureInvariant("Click to edit value")))
				.ButtonColorAndOpacity(FLinearColor{0.2,0.2,0.2,1})
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					[
						SAssignNew(TypeText, STextBlock)
						.Text(FText::AsCultureInvariant(typeStr))
					]
					.AutoHeight()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Left)
					+SVerticalBox::Slot()
					[
						SAssignNew(ValueText, STextBlock)
						.Text(FText::AsCultureInvariant(valStr))						
					]
					.AutoHeight()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Left)
					+SVerticalBox::Slot()
					[
						SAssignNew(SelectUObjectButton, SButton)
						.Text(FText::AsCultureInvariant("Select Object"))
						.OnClicked(this, &SLuaScriptValueListElementWidget::NotifySelectUObjectButtonClicked)
						.Visibility(EVisibility::Collapsed)
					]
					.AutoHeight()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Fill)
				]
			]
			.Visibility(EVisibility::Collapsed)
		]
		.AutoHeight()
	];
	if (bInitiallyOpen)
	{
		(void)this->ToggleContentVisbility();
	}
	
	this->UpdateValueWidgets(val->GetLuaValue());
}

FReply SLuaScriptValueListElementWidget::ToggleContentVisbility()
{
	//this->OnKeyLabelButtonClicked.ExecuteIfBound(this->KeyLabelText->GetText().ToString());
	if (this->ValueContentBorder->GetVisibility() == EVisibility::Collapsed)
	{
		this->ValueContentBorder->SetVisibility(EVisibility::Visible);
	}
	else
	{
		this->ValueContentBorder->SetVisibility(EVisibility::Collapsed);
	}
	return FReply::Handled();
}

FReply SLuaScriptValueListElementWidget::NotifyValueContentButtonPressed()
{
	this->OnRequestEditValue.ExecuteIfBound(this->KeyString);
	return FReply::Handled();
}

FReply SLuaScriptValueListElementWidget::NotifySelectUObjectButtonClicked()
{
	this->OnSelectUObject.ExecuteIfBound(this->KeyString);
	return FReply::Handled();
}

void SLuaScriptValueListElementWidget::NotifyLuaScriptValueChanged(FLuaValue luaValue)
{
	this->UpdateValueWidgets(luaValue);
}

bool SLuaScriptValueListElementWidget::IsOpen() const
{
	return this->ValueContentBorder->GetVisibility() != EVisibility::Collapsed;
}

void SLuaScriptValueListElementWidget::UpdateValueWidgets(const FLuaValue& luaValue)
{
	if (luaValue.IsDead())
	{
		this->SelectUObjectButton->SetVisibility(EVisibility::Collapsed);
		TSharedPtr<SWidget> parent = this->GetParentWidget();
		if (parent)
		{
			StaticCastSharedPtr<SScrollBox>(parent)->RemoveSlot(this->AsShared());
		}
	}
	else
	{
		FString valStr = luaValue.ToStringForStructBuilderEditor();
		FString typeStr = luaValue.GetTypeString();
		this->TypeText->SetText(FText::AsCultureInvariant(typeStr));
		this->ValueText->SetText(FText::AsCultureInvariant(valStr));
		if (luaValue.IsType<TObjectPtr<UObject>>())
		{
			this->SelectUObjectButton->SetVisibility(EVisibility::Visible);
		}
		else
		{
			this->SelectUObjectButton->SetVisibility(EVisibility::Collapsed);
		}
	}
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
