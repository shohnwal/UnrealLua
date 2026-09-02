// Fill out your copyright notice in the Description page of Project Settings.


#include "UObjectRegistry/LuaScriptDynamicDelegateHandler.h"

void ULuaScriptDynamicDelegateHandler::DummyFunc() const
{
}

void ULuaScriptDynamicDelegateHandler::ProcessEvent(UFunction* Function, void* Parms)
{
	if(!this->OnProcessEvent.IsBound())
	{
		return;
	}
	this->OnProcessEvent.Execute(this, Parms);
}