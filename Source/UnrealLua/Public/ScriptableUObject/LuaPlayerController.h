// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Interface/LuaScriptable.h"
#include "LuaPlayerController.generated.h"

UCLASS()
class UNREALLUA_API ALuaPlayerController : public APlayerController, public ILuaScriptable
{
	GENERATED_BODY()
public:
	virtual FLuaScriptSettings GetLuaScriptSettings_Implementation() override;

	virtual void SetLuaScriptSettings_Implementation(FLuaScriptSettings newSettings) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLuaScriptSettings LuaScriptSettings;
};
