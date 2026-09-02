// Fill out your copyright notice in the Description page of Project Settings.


#include "ScriptableUObject/LuaComponent.h"

FLuaScriptSettings ULuaComponent::GetLuaScriptSettings_Implementation()
{
	return LuaScriptSettings;
}

void ULuaComponent::SetLuaScriptSettings_Implementation(FLuaScriptSettings newSettings)
{
	this->LuaScriptSettings = newSettings;
}
