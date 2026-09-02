// Fill out your copyright notice in the Description page of Project Settings.


#include "ScriptableUObject/LuaPlayerController.h"

FLuaScriptSettings ALuaPlayerController::GetLuaScriptSettings_Implementation()
{
	return LuaScriptSettings;
}

void ALuaPlayerController::SetLuaScriptSettings_Implementation(FLuaScriptSettings newSettings)
{
	this->LuaScriptSettings = newSettings;
}
