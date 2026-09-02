// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelScriptActor.h"
#include "Interface/LuaScriptable.h"
#include "LuaLevelScriptActor.generated.h"

/**
 * 
 */
UCLASS()
class UNREALLUA_API ALuaLevelScriptActor : public ALevelScriptActor, public ILuaScriptable
{
	GENERATED_BODY()
public:
	virtual FLuaScriptSettings GetLuaScriptSettings_Implementation() override;

	virtual void SetLuaScriptSettings_Implementation(FLuaScriptSettings newSettings) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLuaScriptSettings LuaScriptSettings;
};
