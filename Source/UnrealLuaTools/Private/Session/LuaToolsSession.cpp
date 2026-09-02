// Fill out your copyright notice in the Description page of Project Settings.


#include "Session/LuaToolsSession.h"

#include "Engine/GameInstance.h"


// Add default functionality here for any ILuaToolsSession functions that are not pure virtual.
UWorld* ILuaToolsSession::GetWorld() const
{
	UGameInstance* gi = this->GetGameInstance();
	if (!gi)
	{
		return nullptr;
	}
	return gi->GetWorld();
}
