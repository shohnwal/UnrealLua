// Fill out your copyright notice in the Description page of Project Settings.


#include "Tools/ObjectInspector/SLuaScriptValueEditor.h"

#include <gsl/pointers>

#include "SlateOptMacros.h"
#include "BlueprintSupport/UnrealLuaUtility.h"
#include "Components/VerticalBox.h"
#include "LuaContext/ScopedLuaContext.h"
#include "LuaValue/LuaScriptValue.h"
#include "UIWidgets/SMultiLineEditableTextBoxEx.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SSeparator.h"
#include "Utility/UnrealVersion.h"
#include "UnrealLua.h"
#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptEditorTextBox.h"
#include "UIWidgets/SFunctionPropertyListSwitcher.h"
#include "UIWidgets/SLuaScriptMultiEditorSwitcher.h"
#include "UIWidgets/SMultiTabEdtitableLuaScriptSwitcher.h"
#include "UnrealOverrides/UnrealLuaOverrideUFunction.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"
#include "Utility/ParseUObjectToLua.h"
BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SLuaScriptValueEditor::Construct(const FArguments& InArgs)
{
	this->ValueOwner = InArgs._ValueOwner;
	this->OnValueCreated = InArgs._OnValueCreated;
	
	SGamescreenDockableWindowWidget::Construct(SGamescreenDockableWindowWidget::FArguments()
	.ExternalWindowAnchors(InArgs._ExternalWindowAnchors)
	.ExternalWindowSize(InArgs._ExternalWindowSize)
	.ExternalWindowPosition(InArgs._ExternalWindowPosition)
	.ExternalWindowSizingRule(InArgs._ExternalWindowSizingRule)
	.Session(InArgs._Session)
	.GameScreenAnchors(InArgs._GameScreenAnchors)
	.BackgroundColor(InArgs._BackgroundColor)
	.Title("")
	.GameScreenAlignment(InArgs._GameScreenAlignment)
	.StartAsWindow(InArgs._StartAsWindow)
	.InitiallyHidden(InArgs._InitiallyHidden)
	.DraggableInGameScreen(true)
	);

	
	TSharedPtr<SVerticalBox> bodyVbodx;
	
	UObject* owner = InArgs._ValueOwner;
	UClass* uclass = owner->GetClass();

	TArray<UFunction*> funcs = SUFunctionSelectionList::CreateDefaultFunctionSelectionList(uclass);

	this->MainContentSizeBox->SetHeightOverride(600);
	this->MainContentSizeBox->SetWidthOverride(800);
	//Owner info
	this->WindowBodyVBox->AddSlot()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
		SNew(STextBlock)
		.Text(FText::AsCultureInvariant("Owner"))		
		]
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		.AutoWidth()
		.Padding(5.f)
		+ SHorizontalBox::Slot()
		[
		SAssignNew(ValueOwnerText, STextBlock)
		.Text(FText::AsCultureInvariant(*GetNameSafe(this->ValueOwner.Get())))		
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
		//Main content(0,0) is a text input field, which gets swapped out with sub editors
		SAssignNew(ContentSwitcher, SLuaScriptMultiEditorSwitcher)
		.Session(InArgs._Session)
		.DeferEdtiableTextBlockSlotAssignment(true)
	]
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	.FillContentHeight(1);
	

	//remove the editable text from the grid panel, we'll be reordering a bit	
	verify(this->ContentSwitcher->MainContentGridPanel->GetChildren()->NumSlot() == 0)
	verify(this->ContentSwitcher->ScriptEditorTextBox->GetParentWidget() == nullptr)
	TSharedPtr<SLuaScriptEditorTextBox> editableTextBox = this->ContentSwitcher->ScriptEditorTextBox;
	verify(editableTextBox.IsValid())

	
	//Grid panel (0,0):                    Grid Panel(1,0)
	//Current Key
	//Current type
	//Current Value                       [Functions][Properties]
	//--------------                          <List>
	//Edit value
	//
	
	auto textboxstyle = &FCoreStyle::Get().GetWidgetStyle< FEditableTextBoxStyle >("NormalEditableTextBox");
	
	//Left upper side(0,0) (current value headline)
	this->ContentSwitcher->MainContentGridPanel->AddSlot(0,0)
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		[
			SAssignNew(CurrentValueSeparator, SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(SSeparator)
				.SeparatorImage(FAppStyle::GetBrush("Menu.Separator"))
				.Orientation(Orient_Horizontal)			
			]
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			.FillContentWidth(1)
			.Padding(5.f)
			+ SHorizontalBox::Slot()
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("Current"))
			]
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			.AutoWidth()
			.Padding(5.f)
			+ SHorizontalBox::Slot()
			[
				SNew(SSeparator)
				.SeparatorImage(FAppStyle::GetBrush("Menu.Separator"))
				.Orientation(Orient_Horizontal)			
			]
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			.FillContentWidth(1)
			.Padding(5.f)			
		]
		.AutoHeight()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Fill)
		+SVerticalBox::Slot()
		[
			SAssignNew(CurrentValueInfoGrid, SGridPanel)
			.FillColumn(1,1)
			+SGridPanel::Slot(0,0)
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("Key"))			
			]
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			.Padding(2,5, 10, 5)
			+ SGridPanel::Slot(1,0)
			[
				SAssignNew(CurrentKeyTextBox, STextBlock)
				.Text(FText::AsCultureInvariant(this->Key))
			]
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Fill)
			+SGridPanel::Slot(0,1)
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("Type"))			
			]
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			.Padding(2,5, 10, 5)
			+ SGridPanel::Slot(1,1)
			[
				SAssignNew(this->CurrentTypeTextBox, STextBlock)
				.Text(FText::AsCultureInvariant("<No Type>"))
			]
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Fill)
			+SGridPanel::Slot(0,2)
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("Value"))			
			]
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			.Padding(2,5, 10, 5)
			+ SGridPanel::Slot(1,2)
			[
				SAssignNew(this->CurrentValueTextBox, STextBlock)
				.Text(FText::AsCultureInvariant("<No Value>"))
			]
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Fill)
		]
		.AutoHeight()
		.Padding(5)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Fill)
		+SVerticalBox::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(SSeparator)
				.SeparatorImage(FAppStyle::GetBrush("Menu.Separator"))
				.Orientation(Orient_Horizontal)			
			]
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			.FillContentWidth(1)
			.Padding(5.f)
			+ SHorizontalBox::Slot()
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("Edit"))
			]
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			.AutoWidth()
			.Padding(5.f)
			+ SHorizontalBox::Slot()
			[
				SNew(SSeparator)
				.SeparatorImage(FAppStyle::GetBrush("Menu.Separator"))
				.Orientation(Orient_Horizontal)			
			]
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			.FillContentWidth(1)
			.Padding(5.f)
		]
		.Padding(5)
		.AutoHeight()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Fill)
		+ SVerticalBox::Slot()
		[
			SAssignNew(TypeRestrictedText, STextBlock)
			.Text(FText::AsCultureInvariant("This value is a FProperty."))
			.Justification(ETextJustify::Center)
		]
		.Padding(5)
		.AutoHeight()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Fill)
		+SVerticalBox::Slot()
		[
			SNew(SGridPanel)
			.FillColumn(1,1)
			.FillRow(1,1)
			+SGridPanel::Slot(0,0)
			[
				SAssignNew(EditKeyTextLabel, STextBlock)
				.Text(FText::AsCultureInvariant("Key"))			
			]
			.VAlign(VAlign_Top)
			.HAlign(HAlign_Left)
			.Padding(2,5, 10, 5)
			+ SGridPanel::Slot(1,0)
			[
				SNew(SOverlay)
				+SOverlay::Slot()
				[
					SNew(SBorder)
					.Content()
					[
						SAssignNew(EditableKeyTextBox, SEditableTextBox)
						.Text(FText::AsCultureInvariant(""))
						.HintText(FText::AsCultureInvariant("<Enter key>"))
						.OnTextChanged(this, &SLuaScriptValueEditor::NotifyEditableKeyTextChanged)
						.OnKeyDownHandler_Raw(this, &SLuaScriptValueEditor::HandleEditKeyTextBoxKeyDown)
					]
				]
				+ SOverlay::Slot()
				.Padding(FMargin(11.0, 5.0, 8.0, 6.0f))
				.VAlign(VAlign_Center)
				[
					SAssignNew(KeyHintText, STextBlock)
					.Text(FText::AsCultureInvariant(""))
					.Visibility(EVisibility::HitTestInvisible)
					.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f, 0.5f))
				]
			]
			.VAlign(VAlign_Top)
			.HAlign(HAlign_Fill)
			+SGridPanel::Slot(0,1)
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("Value"))			
			]
			.VAlign(VAlign_Fill)
			.HAlign(HAlign_Left)
			.Padding(2,5, 10, 5)
			+ SGridPanel::Slot(1,1)
			[
				editableTextBox.ToSharedRef()
			]
			.VAlign(VAlign_Fill)
			.HAlign(HAlign_Fill)
		]
		.Padding(5)
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		//////////////////////////////////////////////
		///  OK / Cancel buttons
		//////////////////////////////////////////////
		+SVerticalBox::Slot()
		[
			SNew(SBorder)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				[
					SNew(SButton)
					[
						SNew(STextBlock)
						.Text(FText::AsCultureInvariant("Ok"))
						.Font(FAppStyle::GetWidgetStyle<FTextBlockStyle>("NormalText").Font)
						.Visibility(EVisibility::SelfHitTestInvisible)
						.Margin(FMargin(2,2,2,2))
					]
					.ButtonStyle(&FButtonStyle::GetDefault())
					.OnClicked_Lambda([this]()
					{
						this->NotifyOkButtonClicked();	
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
						SNew(STextBlock).Text(FText::AsCultureInvariant("Cancel"))
						.Font(FAppStyle::GetWidgetStyle<FTextBlockStyle>("NormalText").Font)
						.Visibility(EVisibility::SelfHitTestInvisible)
						.Margin(FMargin(2,2,2,2))
					]
					.ButtonStyle(&FButtonStyle::GetDefault())
					.OnClicked_Lambda([this]()
					{
						this->NotifyCancelButtonClicked();
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
		.Padding(5)
	]
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill);
	
	//Right side, property list, make it stretch over 
	this->ContentSwitcher->MainContentGridPanel->AddSlot(1,0)
	[
		SAssignNew(this->PropertyFunctionListSwitcher, SFunctionPropertyListSwitcher)
		.TargetStruct(uclass)
		.Functions(funcs)
		.SingleChoiceMode(true)
	]
	.RowSpan(1)
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill);
	
	//this->ContentSwitcher->MainContentGridPanel->SetColumnFill(0, 1);
	this->ContentSwitcher->MainContentGridPanel->ClearFill();
	this->ContentSwitcher->MainContentGridPanel->SetColumnFill(0, 1);
	this->ContentSwitcher->MainContentGridPanel->SetRowFill(0, 1);

	
	
	this->PropertyFunctionListSwitcher->OnFunctionCheckboxChanged.BindSP(this, &SLuaScriptValueEditor::NotifyFunctionListButtonPressed);
	this->PropertyFunctionListSwitcher->OnPropertyCheckboxChanged.BindSP(this, &SLuaScriptValueEditor::NotifyPropertyListButtonPressed);
	
	this->EditValueTextBox = this->ContentSwitcher->ScriptEditorTextBox.ToSharedRef();
	
	
	FLuaScriptValue* val = InArgs._LuaScriptValue;
	if (val)
	{
		this->SetFromLuaScriptValue(*val);
	}
	else
	{
		this->SetFromEmpty();
	}
}



void SLuaScriptValueEditor::NotifyLuaScriptValueChanged(FLuaValue val)
{
	this->SetFromLuaValue(val);
}

void SLuaScriptValueEditor::NotifyEditableKeyTextChanged(const FText& keyText)
{
	this->Key = keyText.ToString();
	this->CurrentKeyTextBox->SetText(keyText);
	if (!this->ValueOwner.IsValid())
	{
		return;
	}
	if (FLuaUObjectItem* item = UnrealLua::UObjectRegistry::TryGetUObjectItem(this->ValueOwner.Get()))
	{
		if (FLuaScriptValue* val = item->GetLuaScriptValue(*this->Key))
		{
			this->SetFromLuaScriptValue(*val);
			return;
		}
	}
	
	TArray<FProperty*> suggestedProperties{};
	TArray<UFunction*> suggestedFunctions{};
	
	if (FProperty* prop = FindPropertyByString(this->Key, suggestedProperties))
	{
		this->PropertyFunctionListSwitcher->ClearAllChecked();
		this->SetFromProperty(prop);
	}
	else if (UFunction* func = FindFunctionByString(this->Key, suggestedFunctions))
	{
		this->PropertyFunctionListSwitcher->ClearAllChecked();
		this->SetFromUFunction(func);
	}
	else
	{
		this->SetFromEmpty();
		if (!suggestedProperties.IsEmpty())
		{

			this->SetSuggestedKey(suggestedProperties[0]->GetName());
			
		}
		else if (!suggestedFunctions.IsEmpty())
		{
			this->SetSuggestedKey(suggestedFunctions[0]->GetName());
		}
	}
}

void SLuaScriptValueEditor::NotifyOkButtonClicked()
{
	if (!this->ValueOwner.IsValid())
	{
		return;
	}
	UObject* owner = this->ValueOwner.Get();
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
	builder << "local args = {...}\n";
	builder << "local self = args[1]\n";
	builder << "local ret = " << this->EditValueTextBox->GetText().ToString() << "\n";
	builder << "return ret";
	
	sol::object arg = sol::make_object(L, owner);
	sol::protected_function_result runResult = ctx.RunString(builder.ToString(), {arg});
	
	if (!runResult.valid())
	{
		return;
	}
	if(runResult.return_count() != 1)
	{
		LUA_LOG_WARNING("Editing Lua Script Values expects exactly 1 result, but got %d", runResult.return_count())
		if (runResult.return_count() == 0)
		{
			return;
		}
	}
	
	//grab the first returned value, discard the rest
	sol::stack_object result {L, -runResult.return_count()};

	//std::string type = UnrealLua::LuaTypes::TypeInfo::UTypeInternal(result, true, -1);

	if (!IsValid(owner))
	{
		return;
	}
	
	FLuaUObjectItem& ownerItem = UnrealLua::UObjectRegistry::GetUObjectItem(owner);
	ownerItem.SetScriptValue(*this->Key, result, true);
	
	FLuaScriptValue* val = ownerItem.GetLuaScriptValue(*this->Key);
	this->SetFromLuaScriptValue(*val);
}

void SLuaScriptValueEditor::NotifyCancelButtonClicked()
{
	this->Shutdown();
}

void SLuaScriptValueEditor::SetFromProperty(FProperty* prop)
{
	this->ClearListenerDelegate();
	this->SetSuggestedKey("");
	//update key
	this->Key = prop->GetName();
	
	//Update restricted text
	this->EditableKeyTextBox->SetText(FText::AsCultureInvariant(this->Key));

	FLuaValue luaValue{this->ValueOwner.Get(), prop};
	
	//Update edit value section
	//becauzse we select a new type, clear out current edit value text box
	FString editValStr = luaValue.ToStringForStructBuilderEditor();
	this->EditValueTextBox->SetText(FText::FromString(editValStr));
	
	//Update current value section
	this->SetFromLuaValue(luaValue);
}

void SLuaScriptValueEditor::SetFromUFunction(UFunction* func)
{
	this->ClearListenerDelegate();
	this->SetSuggestedKey("");
	if (UUnrealLuaOverrideUFunction* overrideFunc = Cast<UUnrealLuaOverrideUFunction>(func))
	{
		func = overrideFunc->Overridden;
	}
	
	//update key
	this->Key = func->GetName();
	this->CurrentKeyTextBox->SetText(FText::AsCultureInvariant(this->Key));
	this->EditableKeyTextBox->SetText(FText::AsCultureInvariant(this->Key));
	
	//Update restricted text
	this->TypeRestrictedText->SetVisibility(EVisibility::SelfHitTestInvisible);
	this->TypeRestrictedText->SetText(FText::AsCultureInvariant("This is a UFunction reference, only lua function allowed"));
	
	//Update current value section
	FString description = UnrealLua::LuaTypes::TypeInfo::UType(func, true).c_str();
	description.Append("\n\n" + UnrealLua::ParseUtility::ParseUFunctionToLuaFunctionAnnontation(func, true, true, true));
	this->CurrentTypeTextBox->SetText(FText::AsCultureInvariant("UFunction, signature:"));
	this->CurrentValueTextBox->SetText(FText::AsCultureInvariant(description));
	
	//Update edit value section
	//because we select a new type, clear out current edit value text box
	FString funcStr = UnrealLua::ParseUtility::ParseUFunctionToLuaFunctionAnnontation(func, false, false, false);
	this->EditValueTextBox->SetText(FText::AsCultureInvariant(funcStr));
}

void SLuaScriptValueEditor::SetFromEmpty()
{
	this->ClearListenerDelegate();
	this->SetSuggestedKey("");
	this->TypeRestrictedText->SetVisibility(EVisibility::Collapsed);
	this->CurrentValueTextBox->SetText(FText::GetEmpty());
	this->CurrentTypeTextBox->SetText(FText::GetEmpty());
}

void SLuaScriptValueEditor::SetFromLuaScriptValue(FLuaScriptValue& val)
{
	this->ClearListenerDelegate();
	this->SetSuggestedKey("");
	//update key
	bool setKeyFromVal = true;
	if (val.IsType<FLuaUFunctionReference>())
	{
		if (const UUnrealLuaOverrideUFunction* overrideFunc = Cast<UUnrealLuaOverrideUFunction>(val.Get<FLuaUFunctionReference>().Func->Func))
		{
			setKeyFromVal = false;
			UFunction* func = overrideFunc->Overridden;
			this->Key = func->GetName();		
		}
	}
	if (setKeyFromVal)
	{
		this->Key = val.GetKeyNameString();
	}
	this->EditableKeyTextBox->SetText(FText::AsCultureInvariant(this->Key));
	
	
	FOnLuaScriptValueChangedNativeDelegate del;
	del.BindSP(this, &SLuaScriptValueEditor::NotifyLuaScriptValueChanged);
	val.AddOnValueChangedDelegate(del);
	
	//Update restricted text && current value section
	this->SetFromLuaValue(val.GetLuaValue());
}

void SLuaScriptValueEditor::SetFromLuaValue(const FLuaValue& luaValue)
{
	this->SetSuggestedKey("");
	if (luaValue.IsDead())
	{
		this->Key = "";
		this->SetFromEmpty();
	}
	else
	{
		if (luaValue.IsType<FPropertyReferenceWrapper>() || luaValue.IsType<FLuaUFunctionReference>())
		{
			this->TypeRestrictedText->SetVisibility(EVisibility::SelfHitTestInvisible);
			FString cppType{};
			if (luaValue.IsType<FPropertyReferenceWrapper>())
			{
				const FPropertyReferenceWrapper& ref = luaValue.Get<FPropertyReferenceWrapper>();
				cppType = UnrealLua::PropertyHelper::GetPropertyTypeName(ref.Prop, true);//luaValue.Get<FPropertyReferenceWrapper>().Prop->GetCPPType();
				this->TypeRestrictedText->SetText(FText::AsCultureInvariant("This is a Property reference, only type allowed:\n" + cppType));
			}
			else if (luaValue.IsType<FLuaUFunctionReference>())
			{
				this->TypeRestrictedText->SetText(FText::AsCultureInvariant("This is a UFunction reference, only type allowed:Lua function or nil"));
			}
		}
		else
		{
			this->TypeRestrictedText->SetVisibility(EVisibility::Collapsed);
		}
		
		FString valStr = luaValue.ToStringForStructBuilderEditor();
		this->CurrentValueTextBox->SetText(FText::AsCultureInvariant(valStr));
		FString typeStr = luaValue.GetTypeString();
		this->CurrentTypeTextBox->SetText(FText::AsCultureInvariant(typeStr));
	}
}

void SLuaScriptValueEditor::NotifyFunctionListButtonPressed(UFunction* function, bool bIsChecked)
{
	UObject* owner = this->ValueOwner.Get();
	if (owner && bIsChecked)
	{
		if (UUnrealLuaOverrideUFunction* overrideFunc = Cast<UUnrealLuaOverrideUFunction>(function))
		{
			function = overrideFunc->Overridden;
		}
		if (FLuaUObjectItem* item = UnrealLua::UObjectRegistry::TryGetUObjectItem(this->ValueOwner.Get()))
		{
			if (FLuaScriptValue* val = item->GetLuaScriptValue(*function->GetName()))
			{
				this->SetFromLuaScriptValue(*val);
				FSlateApplication::Get().SetKeyboardFocus(this->EditValueTextBox->GetEditableText());
				return;
			}
		}
		verify(owner->FindFunctionChecked(function->GetFName()) != nullptr)	
		
		this->SetFromUFunction(function);
		FSlateApplication::Get().SetKeyboardFocus(this->EditValueTextBox->GetEditableText());
	}
	else
	{
		this->SetFromEmpty();
	}
}

void SLuaScriptValueEditor::NotifyPropertyListButtonPressed(FProperty* prop, bool bIsChecked)
{
	UObject* owner = this->ValueOwner.Get();
	if (owner && bIsChecked)
	{
		if (FLuaUObjectItem* item = UnrealLua::UObjectRegistry::TryGetUObjectItem(this->ValueOwner.Get()))
		{
			if (FLuaScriptValue* val = item->GetLuaScriptValue(*prop->GetName()))
			{
				this->SetFromLuaScriptValue(*val);
				FSlateApplication::Get().SetKeyboardFocus(this->EditValueTextBox->GetEditableText());
				return;
			}
		}
		this->SetFromProperty(prop);
		FSlateApplication::Get().SetKeyboardFocus(this->EditValueTextBox->GetEditableText());
	}
	else
	{
		this->SetFromEmpty();
	}
}

void SLuaScriptValueEditor::ClearListenerDelegate()
{
	//Clear delegate we are currently listening to
	UObject* owner = this->ValueOwner.Get();
	if (owner && !this->Key.IsEmpty())
	{
		if (FLuaUObjectItem* item = UnrealLua::UObjectRegistry::TryGetUObjectItem(this->ValueOwner.Get()))
		{
			if (FLuaScriptValue* val = item->GetLuaScriptValue(*this->Key))
			{
				val->RemoveLuaScriptListener(this);
			}
		}
	}
}

FReply SLuaScriptValueEditor::HandleEditKeyTextBoxKeyDown(const FGeometry& geometry, const FKeyEvent& keyEvent)
{
	LUA_LOG("Pressed key %s", *keyEvent.GetKey().ToString());
	if (keyEvent.GetKey() == EKeys::Tab || keyEvent.GetKey() == EKeys::Enter)
	{
		if (!this->KeyHintText->GetText().IsEmpty())
		{
			LUA_LOG("Complete key!");
			this->EditableKeyTextBox->SetText(this->KeyHintText->GetText());
			return FReply::Handled();			
		}
	}
	return FReply::Unhandled();
}

void SLuaScriptValueEditor::SetSuggestedKey(const FString& suggestionKey)
{
	//LUA_LOG("Setting suggested key : %s",*suggestionKey);
	this->KeyHintText->SetText(FText::AsCultureInvariant(suggestionKey));
}

EDockableWindowWidgetOnCloseExternalWindowBehavior SLuaScriptValueEditor::GetOnCloseExternalWindowBehavior() const
{
	return EDockableWindowWidgetOnCloseExternalWindowBehavior::Remove;
}

EDockableWindowWidgetOnCloseGameScreenWidgetBehavior SLuaScriptValueEditor::GetOnCloseGameScreenWidgetBehavior() const
{
	return EDockableWindowWidgetOnCloseGameScreenWidgetBehavior::Remove;
}

FProperty* SLuaScriptValueEditor::FindPropertyByString(FString searchStr, TArray<FProperty*>& outSuggestedProperties) const
{
	if (this->ValueOwner.IsValid())
	{
		UClass* uclass = this->ValueOwner.Get()->GetClass();
		for (FProperty* Property = uclass->PropertyLink; Property != nullptr; Property = Property->PropertyLinkNext)
		{
			FString propname = Property->GetName(); 
			if (propname == searchStr)
			{
				return Property;
			}
			else if (Property->GetName().StartsWith(searchStr))
			{
				outSuggestedProperties.Add(Property);
			}
		}
	}
	return nullptr;
}

UFunction* SLuaScriptValueEditor::FindFunctionByString(const FString& funcNameStr, TArray<UFunction*>& outSuggestedFunctions) const
{
	if (this->ValueOwner.IsValid())
	{
		FName funcName{funcNameStr, FNAME_Find};
		UFunction* foundFunc = nullptr;
		UClass* uclass = this->ValueOwner.Get()->GetClass();
		if (funcName != NAME_None)
		{
			foundFunc = uclass->FindFunctionByName(funcName);
		}
		if (foundFunc != nullptr)
		{
			return foundFunc;
		}
		else
		{
			for (TFieldIterator<UFunction> it(uclass); it; ++it)
			{
				if (it->GetName().StartsWith(funcNameStr))
				{
					outSuggestedFunctions.Add(*it);
				}
			}
		}
	}
	return nullptr;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
