// Fill out your copyright notice in the Description page of Project Settings.


#include "Tools/LuaStateInspector/UnrealLuaStateInspectorTool.h"

int32 UUnrealLuaStateInspectorTool::GetToolMainMenuSortOrder() const
{
	return 5;
}

FString UUnrealLuaStateInspectorTool::GetToolMainMenuButtonLabel() const
{
	return "Lua State Inspector";
}
