// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Interface/LuaScriptable.h"
#include "LuaGameMode.generated.h"

class UCombat;
struct FCombatResults;
UCLASS(Abstract)
class UNREALLUA_API ALuaGameMode : public AGameMode, public ILuaScriptable
{
	GENERATED_BODY()
public:
	virtual FLuaScriptSettings GetLuaScriptSettings_Implementation() override;

	virtual void SetLuaScriptSettings_Implementation(FLuaScriptSettings newSettings) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLuaScriptSettings LuaScriptSettings;
};

