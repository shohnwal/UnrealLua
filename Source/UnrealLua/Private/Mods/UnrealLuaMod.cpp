// Fill out your copyright notice in the Description page of Project Settings.


#include "Mods/UnrealLuaMod.h"

void UUnrealLuaMod::ModActivated()
{
	this->ReceiveModActivated();
}

void UUnrealLuaMod::ModDeactivated()
{
	this->ReceiveModDeactivated();
}

bool UUnrealLuaMod::IsActive() const
{
	return this->bIsEnabled;
}

void UUnrealLuaMod::SetModEnabled(bool newIsEnabled)
{
	if(newIsEnabled != this->bIsEnabled)
	{
		if(this->ModInfo.bCanEverBeActive)
		{
			this->bIsEnabled = newIsEnabled;
			this->OnModActiveChanged.Broadcast(this, newIsEnabled);
		}
	}
}

void UUnrealLuaMod::NotifyWorldBeginPlay()
{
	this->ReceiveWorldBeginPlay();
}

void UUnrealLuaMod::NotifyWorldEndPlay()
{
	this->ReceiveWorldEndPlay();
}

FLuaScriptSettings UUnrealLuaMod::GetLuaScriptSettings_Implementation()
{
	FLuaScriptSettings settings;
	settings.ScriptPathOverride = this->ModInfo.LuaScriptFilePath;
	return settings;
}
