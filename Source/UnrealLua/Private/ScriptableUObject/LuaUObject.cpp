// Fill out your copyright notice in the Description page of Project Settings.


#include "ScriptableUObject/LuaUObject.h"

FLuaScriptSettings ULuaUObject::GetLuaScriptSettings_Implementation()
{
	return this->LuaScriptSettings;
}

void ULuaUObject::SetLuaScriptSettings_Implementation(FLuaScriptSettings newSettings)
{
	this->LuaScriptSettings = newSettings;
}