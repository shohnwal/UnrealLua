// Fill out your copyright notice in the Description page of Project Settings.


#include "Debug/DebugTools/UnrealLuaDebugTool.h"

#include "Debug/UnrealLuaDebug.h"
#include "Engine/GameInstance.h"


UGameInstance* UUnrealLuaDebugTool::GetGameInstance() const
{
	return CastChecked<UGameInstance>(this->GetOuter());
}

void UUnrealLuaDebugTool::Initialize(UUnrealLuaDebug* debug)
{
	this->UnrealLuaDebug = debug;
	this->LoadLuaScript();
	this->InitializeTool();
	this->CreateToolWidgets();
}

void UUnrealLuaDebugTool::InitializeTool_Implementation()
{
}

void UUnrealLuaDebugTool::CreateToolWidgets_Implementation()
{
	
}

// Add default functionality here for any IUnrealLuaDebugTool functions that are not pure virtual.
FName UUnrealLuaDebugTool::GetToolMainMenuButtonName_Implementation() const
{
	return NAME_None;
}

void UUnrealLuaDebugTool::SetActiveTool(TSubclassOf<UUnrealLuaDebugTool> debugToolClass, FInstancedStruct& args)
{
	UGameInstance* gameInstance = GetGameInstance();
	UUnrealLuaDebug* debug = this->UnrealLuaDebug;
	if (gameInstance && debug)
	{
		debug->SetActiveTool(gameInstance, debugToolClass, args);
	}
}

void UUnrealLuaDebugTool::SetToolInactive()
{
	UGameInstance* gameInstance = GetGameInstance();
	UUnrealLuaDebug* debug = this->UnrealLuaDebug;
	if (gameInstance && debug)
	{
		debug->DeactivateTool(gameInstance, this->GetClass());
	}
}

bool UUnrealLuaDebugTool::IsCurrentTool()
{
	UGameInstance* gameInstance = GetGameInstance();
	UUnrealLuaDebug* debug = this->UnrealLuaDebug;
	if (gameInstance && debug)
	{
		return debug->IsCurrentTool(gameInstance, this->GetClass());
	}
	return false;
}

bool UUnrealLuaDebugTool::ToggleTool(FInstancedStruct args)
{
	UGameInstance* gameInstance = GetGameInstance();
	UUnrealLuaDebug* debug = this->UnrealLuaDebug;
	if (gameInstance && debug)
	{
		return debug->ToggleTool(gameInstance, this->GetClass(), args);
	}
	return false;
}
