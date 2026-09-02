// Fill out your copyright notice in the Description page of Project Settings.


#include "ScriptableUObject/LuaLevelScriptActor.h"

FLuaScriptSettings ALuaLevelScriptActor::GetLuaScriptSettings_Implementation()
{
	return LuaScriptSettings;
}

void ALuaLevelScriptActor::SetLuaScriptSettings_Implementation(FLuaScriptSettings newSettings)
{
	this->LuaScriptSettings = newSettings;
}
