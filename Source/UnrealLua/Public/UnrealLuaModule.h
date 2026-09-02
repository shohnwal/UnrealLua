// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class UNREALLUA_API FUnrealLuaModule : public IModuleInterface
{
public:
	
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual bool SupportsDynamicReloading() override;

	/**
	* Called before the module has been unloaded
	*/
	virtual void PreUnloadCallback() override
	{
		UE_LOG(LogTemp, Warning, TEXT("UnrealLua Module : PreUnloadCallback"));
	}

	/**
	* Called after the module has been reloaded
	*/
	virtual void PostLoadCallback() override
	{
		//checkNoEntry();
		UE_LOG(LogTemp, Warning, TEXT("UnrealLua Module : PostLoadCallback"));
	}
};
