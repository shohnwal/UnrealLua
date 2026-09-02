// Fill out your copyright notice in the Description page of Project Settings.


#include "ScriptableUObject/LuaCharacter.h"

FLuaScriptSettings ALuaCharacter::GetLuaScriptSettings_Implementation()
{
	return LuaScriptSettings;
}

void ALuaCharacter::SetLuaScriptSettings_Implementation(FLuaScriptSettings newSettings)
{
	this->LuaScriptSettings = newSettings;
}
