// Fill out your copyright notice in the Description page of Project Settings.


#include "Tools/LuaScriptEditor/UnrealLuaScriptEditorTool.h"

#include "Session/UnrealLuaToolsSession.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/StyleColors.h"
#include "Tools/LuaScriptEditor/SLuaScriptEditor.h"

void UUnrealLuaScriptEditorTool::InitializeTool()
{
	Super::InitializeTool();
	
	this->CreateLuaScriptEditor();
}

void UUnrealLuaScriptEditorTool::CreateLuaScriptEditor()
{
	if (this->UnrealEditorLuaScriptEditor.IsValid())
	{
		return;
	}
	TSharedRef<SLuaScriptEditor> scriptEditorWindow = SNew(SLuaScriptEditor)
	.GameScreenAnchors(FAnchors{0.2,0.1,0.8,0.9})
	.GameScreenAlignment(FVector2D{0.5f})
	.ExternalWindowSize(FVector2D{0.5f,0.5f})
	.ExternalWindowPosition(FVector2D{0.5f,0.5f})
	.Session(this->SessionInfo.Get())
	.Title("Lua Script Editor")
	.InitiallyHidden(true)
	.StartAsWindow(false);
	
	this->UnrealEditorLuaScriptEditor = scriptEditorWindow;
}

TSharedRef<SLuaScriptEditor> UUnrealLuaScriptEditorTool::GetLuaScriptEditor()
{
	this->CreateLuaScriptEditor();
	return this->UnrealEditorLuaScriptEditor.ToSharedRef();
}

void UUnrealLuaScriptEditorTool::NotifyMainMenuButtonClicked_Implementation()
{
	this->UnrealEditorLuaScriptEditor->ToggleVisibility();
}

void UUnrealLuaScriptEditorTool::ActivateTool(const FUnrealLuaTooleActivateCallback& callback)
{
	Super::ActivateTool(callback);
}

void UUnrealLuaScriptEditorTool::DeactivateTool()
{
	Super::DeactivateTool();
}

void UUnrealLuaScriptEditorTool::Shutdown()
{
	if (this->UnrealEditorLuaScriptEditor.IsValid())
	{
		this->UnrealEditorLuaScriptEditor->Shutdown();
	}
	this->UnrealEditorLuaScriptEditor = nullptr;
	Super::Shutdown();
}

void UUnrealLuaScriptEditorTool::BeginDestroy()
{
	Super::BeginDestroy();
}

void UUnrealLuaScriptEditorTool::NotifyCloseWindowButtonClicked(const TSharedRef<SWindow>& window)
{
	this->UnrealEditorLuaScriptEditor->ToggleVisibility();
}

int32 UUnrealLuaScriptEditorTool::GetToolMainMenuSortOrder() const
{
	return 1;
}

FString UUnrealLuaScriptEditorTool::GetToolMainMenuButtonLabel() const
{
	return "Lua Script Editor";
}
