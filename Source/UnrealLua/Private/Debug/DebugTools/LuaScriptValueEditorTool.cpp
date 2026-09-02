// Fill out your copyright notice in the Description page of Project Settings.


#include "Debug/DebugTools/LuaScriptValueEditorTool.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "BlueprintSupport/UnrealLuaUtility.h"
#include "Debug/UnrealLuaDebug.h"
#include "LuaContext/ScopedLuaContext.h"
#include "LuaValue/LuaScriptValue.h"

void ULuaScriptValueEditorTool::InitializeTool_Implementation()
{
	this->LuaScriptValueEditorClass = ULuaScriptValueEditor::StaticClass();
	Super::InitializeTool_Implementation();
}

void ULuaScriptValueEditorTool::ActivateTool_Implementation(FInstancedStruct args)
{
	FUnrealLuaDebugEditScriptValueToolData& data = args.GetMutable<FUnrealLuaDebugEditScriptValueToolData>();
	this->Context = data.Context;
	this->Data = args;
	this->LuaScriptValuePtr = data.LuaScriptValuePtr;

	FOnLuaScriptValueChangedDelegate del;
	del.BindDynamic(this, &ULuaScriptValueEditorTool::NotifyEditedLuaScriptValueChanged);
	this->LuaScriptValuePtr->AddOnValueChangedDelegate(del);
	
	this->RemoveEditorWidget();
	
	UWorld* world = this->Context->GetWorld();
	UGameInstance* gi = world->GetGameInstance();
	
	ULuaScriptValueEditor* editorWidget = CreateWidget<ULuaScriptValueEditor>(gi->GetFirstLocalPlayerController(), this->LuaScriptValueEditorClass, "LuaScriptValueEditor");
	this->EditorWidget = editorWidget;
	editorWidget->InitializeLuaScriptEditor(this->LuaScriptValuePtr);
	editorWidget->AddToViewport(999);
	
	//UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(gi->GetFirstLocalPlayerController(),this->EditorWidget->EditValueTextBox);
}

void ULuaScriptValueEditorTool::DeactivateTool_Implementation()
{
	UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(this->EditorWidget->GetOwningPlayer());
	this->RemoveEditorWidget();
	
	if (this->LuaScriptValuePtr)
	{
		FOnLuaScriptValueChangedDelegate del;
		del.BindDynamic(this, &ULuaScriptValueEditorTool::NotifyEditedLuaScriptValueChanged);
		this->LuaScriptValuePtr->RemoveOnValueChangedDynamicListener(del);
	}
	this->Data.Reset();
	this->LuaScriptValuePtr = nullptr;
}

void ULuaScriptValueEditorTool::NotifyInputKeyEvent_Implementation(const FKey& Key, EInputEvent EventType, UGameInstance* gameInstance)
{

}

void ULuaScriptValueEditorTool::NotifyEditedLuaScriptValueChanged(FLuaValue newVal)
{
	if (!this->LuaScriptValuePtr)
	{
		return;
	}
	if (this->LuaScriptValuePtr->IsDead())
	{
		this->DeactivateTool();
	}
	else
	{
		this->EditorWidget->UpdateCurrentScriptValue(this->LuaScriptValuePtr->GetLuaValue().ToValueString());
	}
}

bool ULuaScriptValueEditorTool::ProcessAndSetValue(const FString& valueString)
{
	TScriptInterface<ILuaContext> ictx;
	if (!UUnrealLuaUtility::GetLuaContextFromWorldContext(this->Context, ictx))
	{
		return false;
	}
	if (!ictx)
	{
		return false;
	}
	FScopedLuaContext& ctx = ictx->GetScopedLuaContext();
	
	FString processString = valueString;
	if (!processString.StartsWith("return "))
	{
		processString = "return " + processString;
	}
	sol::protected_function_result result = ctx.RunString(*processString, {});
	if(!result.valid())
	{
		return false;
	}
	if (result.return_count() != 1)
	{
		return false;
	}
	sol::object returnResult = result[0];
	ESetLuaValueResult setResult = this->LuaScriptValuePtr->SetScriptValue(returnResult, LuaScriptValuePtr->GetKeyName());
	return EnumHasAllFlags(setResult, ESetLuaValueResult::Success);
}

void ULuaScriptValueEditorTool::RemoveEditorWidget()
{
	if (this->EditorWidget)
	{
		this->EditorWidget->RemoveFromParent();
		this->EditorWidget->ConditionalBeginDestroy();
		this->EditorWidget = nullptr;
	}
}
