// Fill out your copyright notice in the Description page of Project Settings.


#include "Tools/UnrealLuaTool.h"

#include "Session/UnrealLuaToolsSession.h"
#include "Engine/GameInstance.h"
#include "Input/Reply.h"

UUnrealLuaTool::UUnrealLuaTool()
{
	if (this->IsTemplate())
	{
		return;
	}
	this->SessionInfo = CastChecked<UUnrealLuaToolsSession>(this->GetOuter());
}

void UUnrealLuaTool::InitializeTool()
{
	this->ReceiveInitializeTool();	
}

void UUnrealLuaTool::SetToolActive(bool bIsActive)
{
	if (bIsActive)
	{
		if (this->IsActiveTool())
		{
			return;
		}
		this->SessionInfo->SetActiveTool(this);
		return;		
	}
	else
	{
		this->SessionInfo->DeactivateTool(this);
	}
}

void UUnrealLuaTool::ActivateTool(const FUnrealLuaTooleActivateCallback& preActivateCallback)
{
	preActivateCallback.ExecuteIfBound(this);
}

bool UUnrealLuaTool::IsActiveTool() const
{
	return this->SessionInfo->IsCurrentTool(this);
}

FString UUnrealLuaTool::GetToolMainMenuButtonLabel() const
{
	return "";
}

int32 UUnrealLuaTool::GetToolMainMenuSortOrder() const
{
	return 0;
}

void UUnrealLuaTool::NotifyAddedToMainMenu()
{
	
}

FReply UUnrealLuaTool::NotifyInputKeyEvent(const FKey& key, EInputEvent eventType, UGameInstance* gameInstance)
{
	return FReply::Unhandled();
}

void UUnrealLuaTool::DeactivateTool()
{
	this->ReceiveDeactivateTool();
}

void UUnrealLuaTool::Shutdown()
{
	this->ReceiveShutdown();
}

EVerticalAlignment UUnrealLuaTool::GetMainMenuButtonAlignment() const
{
	return EVerticalAlignment::VAlign_Top;
}

bool UUnrealLuaTool::RequiresTick_Implementation()
{
	return false;
}

void UUnrealLuaTool::Tick(float dt)
{
	this->ReceiveTick(dt);
}

UUnrealLuaToolsSession* UUnrealLuaTool::GetSession() const
{
	return this->SessionInfo.Get();
}

UGameInstance* UUnrealLuaTool::GetGameInstance() const
{
	return Cast<UGameInstance>(this->GetOuter());
}
