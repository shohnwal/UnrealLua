// Fill out your copyright notice in the Description page of Project Settings.


#include "ScriptableUObject//LuaGameMode.h"

FLuaScriptSettings ALuaGameMode::GetLuaScriptSettings_Implementation()
{
	return {};
}

void ALuaGameMode::SetLuaScriptSettings_Implementation(FLuaScriptSettings newSettings)
{
	this->LuaScriptSettings = newSettings;
}
