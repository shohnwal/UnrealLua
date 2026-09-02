// Fill out your copyright notice in the Description page of Project Settings.


#include "ScriptableUObject/LuaActor.h"

FLuaScriptSettings ALuaActor::GetLuaScriptSettings_Implementation()
{
	return LuaScriptSettings;
}

void ALuaActor::SetLuaScriptSettings_Implementation(FLuaScriptSettings newSettings)
{
	this->LuaScriptSettings = newSettings;
}
