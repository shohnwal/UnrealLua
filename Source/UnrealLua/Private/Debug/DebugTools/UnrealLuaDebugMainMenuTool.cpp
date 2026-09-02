// Fill out your copyright notice in the Description page of Project Settings.


#include "Debug/DebugTools/UnrealLuaDebugMainMenuTool.h"

void UUnrealLuaDebugMainMenuTool::InitializeTool_Implementation()
{
	Super::InitializeTool_Implementation();
}

void UUnrealLuaDebugMainMenuTool::ActivateTool_Implementation(FInstancedStruct data)
{
	UGameInstance* gi = this->GetGameInstance();

	if (!gi)
	{
		return;
	}
	
	//ULuaScriptValueEditor* editorWidget = CreateWidget<ULuaScriptValueEditor>(gi->GetFirstLocalPlayerController(), this->LuaScriptValueEditorClass, "LuaScriptValueEditor");
	//this->EditorWidget = editorWidget;
	//editorWidget->InitializeLuaScriptEditor(this->LuaScriptValuePtr);
	//editorWidget->AddToViewport(999);
}

void UUnrealLuaDebugMainMenuTool::DeactivateTool_Implementation()
{

}

void UUnrealLuaDebugMainMenuTool::NotifyInputKeyEvent_Implementation(const FKey& Key, EInputEvent EventType, UGameInstance* gameInstance)
{

}

FName UUnrealLuaDebugMainMenuTool::GetToolMainMenuButtonName_Implementation() const
{
	return NAME_None;
	
}
