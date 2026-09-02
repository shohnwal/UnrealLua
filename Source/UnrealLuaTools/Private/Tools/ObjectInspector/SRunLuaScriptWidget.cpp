// Fill out your copyright notice in the Description page of Project Settings.


#include "Tools/ObjectInspector/SRunLuaScriptWidget.h"

#include "SlateOptMacros.h"
#include "BlueprintSupport/UnrealLuaUtility.h"
#include "Interface/LuaContext.h"
#include "LuaContext/ScopedLuaContext.h"
#include "UIWidgets/SLuaScriptMultiEditorSwitcher.h"
#include "UIWidgets/SMultiLineEditableTextBoxEx.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Utility/UnrealVersion.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SRunLuaScriptWidget::Construct(const FArguments& InArgs)
{
	SGamescreenDockableWindowWidget::Construct(SGamescreenDockableWindowWidget::FArguments()
	.ExternalWindowAnchors(InArgs._ExternalWindowAnchors)
	.ExternalWindowSize(InArgs._ExternalWindowSize)
	.ExternalWindowPosition(InArgs._ExternalWindowPosition)
	.ExternalWindowSizingRule(InArgs._ExternalWindowSizingRule)
	.Session(InArgs._Session)
	.GameScreenAnchors(InArgs._GameScreenAnchors)
	.BackgroundColor(InArgs._BackgroundColor)
	.Title("Run Lua Script")
	.GameScreenAlignment(InArgs._GameScreenAlignment)
	.StartAsWindow(InArgs._StartAsWindow)
	.InitiallyHidden(InArgs._InitiallyHidden)
	.DraggableInGameScreen(true)
	);
	
	this->MainContentSizeBox->SetHeightOverride(600);
	this->MainContentSizeBox->SetWidthOverride(800);
	
	UObject* selfParamObj = InArgs._SelfParam;
	this->SelfParamObj = selfParamObj;
	FString selfParamName = IsValid(selfParamObj) ? GetNameSafe(selfParamObj) : "nil";
	//Owner info
	this->WindowBodyVBox->AddSlot()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
		SNew(STextBlock)
		.Text(FText::AsCultureInvariant("Self param object"))		
		]
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		.AutoWidth()
		.Padding(5.f)
		+ SHorizontalBox::Slot()
		[
		SAssignNew(SelfParamObjectName, STextBlock)
		.Text(FText::AsCultureInvariant(selfParamName))		
		]
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		.FillContentWidth(1)
		.Padding(5.f)
	]
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Center)
	.AutoHeight()
	.Padding(5.f);
	//Add a body widget switcher 
	this->WindowBodyVBox->AddSlot()
	[
		//Main content(0,0) is a text input field
		SAssignNew(ContentSwitcher, SLuaScriptMultiEditorSwitcher)
		.DeferEdtiableTextBlockSlotAssignment(false)
	]
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	.FillContentHeight(1);
	
	this->WindowBodyVBox->AddSlot()
	[
		SNew(SBorder)
		.Padding(1)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(SButton)
				[
					SNew(STextBlock)
					.Text(FText::AsCultureInvariant("Run"))
					.Font(FAppStyle::GetWidgetStyle<FTextBlockStyle>("NormalText").Font)
					.Visibility(EVisibility::SelfHitTestInvisible)
					.Margin(FMargin(2,2,2,2))
				]
				.ButtonStyle(&FButtonStyle::GetDefault())
				.OnClicked_Lambda([this]()
				{
					this->NotifyRunScriptButtonClicked();	
					return FReply::Handled();;
				})
			]
			.AutoWidth()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.FillWidth(1)
			.Padding(10,10,10,10)
			+ SHorizontalBox::Slot()
			[
				SNew(SButton)
				[
					SNew(STextBlock).Text(FText::AsCultureInvariant("Close"))
					.Font(FAppStyle::GetWidgetStyle<FTextBlockStyle>("NormalText").Font)
					.Visibility(EVisibility::SelfHitTestInvisible)
					.Margin(FMargin(2,2,2,2))
				]
				.ButtonStyle(&FButtonStyle::GetDefault())
				.OnClicked_Lambda([this]()
				{
					this->NotifyCloseButtonClicked();
					return FReply::Handled();
				})
			]
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.AutoWidth()
			.FillWidth(1)
			.Padding(10,10,10,10)	
		]
		.Padding(2)
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
	]
	.AutoHeight()
	.VAlign(VAlign_Bottom)
	.HAlign(HAlign_Fill)
	.Padding(5);
}

void SRunLuaScriptWidget::NotifyRunScriptButtonClicked()
{
	if (!this->SelfParamObj.IsValid())
	{
		return;
	}
	UObject* owner = this->SelfParamObj.Get();
	TScriptInterface<ILuaContext> ictx = UUnrealLuaUtility::GetLuaContext(owner);
	if (!ictx)
	{
		return;
	}
	FScopedLuaContext& ctx = ictx->GetScopedLuaContext();
	
	lua_State* L = ctx.GetLuaState();
	
#if UNREALLUA_UE_VERSION_OLDER_THAN(5,8,0)
	FStringBuilderBase builder;
#else
	TStringBuilder<256> builder;
#endif
	if (IsValid(owner))
	{
		builder << "local args = {...}\n";
		builder << "local self = args[1]\n";		
	}
	builder << this->ContentSwitcher->GetEditableText()->GetText().ToString();
	
	FString scriptToRun = builder.ToString();
	
	LUA_LOG("Running script:\n%s", *scriptToRun);
	
	sol::object arg = sol::make_object(L, owner);
	
	sol::protected_function_result runResult = ctx.RunString(builder.ToString(), {arg});
	
	if (!runResult.valid())
	{
		return;
	}
	LUA_LOG("Ran script!")
}

void SRunLuaScriptWidget::NotifyCloseButtonClicked()
{
	this->Shutdown();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
