// Fill out your copyright notice in the Description page of Project Settings.


#include "Tools/LuaLog/UnrealLuaErrorLogTool.h"

EVerticalAlignment UUnrealLuaErrorLogTool::GetMainMenuButtonAlignment() const
{
	return EVerticalAlignment::VAlign_Bottom; 
}

int32 UUnrealLuaErrorLogTool::GetToolMainMenuSortOrder() const
{
	return INT32_MAX;
}

FString UUnrealLuaErrorLogTool::GetToolMainMenuButtonLabel() const
{
	return "Error Log";
}
