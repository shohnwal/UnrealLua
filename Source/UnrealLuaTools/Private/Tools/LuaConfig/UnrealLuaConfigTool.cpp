// Fill out your copyright notice in the Description page of Project Settings.


#include "Tools/LuaConfig/UnrealLuaConfigTool.h"
#include "Session/UnrealLuaToolsSession.h"
#include "Tools/LuaConfig/SUnrealLuaConfigEditor.h"

void UUnrealLuaConfigTool::InitializeTool()
{
	Super::InitializeTool();

	TScriptInterface<ILuaToolsSession> session{this->SessionInfo.Get()}; 
	TSharedRef<SUnrealLuaConfigEditor> editor = SNew(SUnrealLuaConfigEditor)
	.Session(session);

	
	this->LuaConfigEditor = editor;
}

void UUnrealLuaConfigTool::Shutdown()
{
	if (this->LuaConfigEditor.IsValid())
	{
		this->LuaConfigEditor->Shutdown();
	}
	this->LuaConfigEditor = nullptr;
	
	Super::Shutdown();
}

EVerticalAlignment UUnrealLuaConfigTool::GetMainMenuButtonAlignment() const
{
	return Super::GetMainMenuButtonAlignment();
}

FString UUnrealLuaConfigTool::GetToolMainMenuButtonLabel() const
{
	return "Config";
}

int32 UUnrealLuaConfigTool::GetToolMainMenuSortOrder() const
{
	return INT32_MAX - 1;
}

void UUnrealLuaConfigTool::NotifyAddedToMainMenu()
{
	Super::NotifyAddedToMainMenu();
}

FReply UUnrealLuaConfigTool::NotifyInputKeyEvent(const FKey& key, EInputEvent eventType, UGameInstance* gameInstance)
{
	return Super::NotifyInputKeyEvent(key, eventType, gameInstance);
}

void UUnrealLuaConfigTool::NotifyMainMenuButtonClicked_Implementation()
{
	this->LuaConfigEditor->ToggleVisibility();
}

