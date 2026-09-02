// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interface/LuaScriptable.h"
#include "LuaCharacter.generated.h"

class ULuaScriptReplicationComponent;
UCLASS()
class UNREALLUA_API ALuaCharacter : public ACharacter, public ILuaScriptable
{
	GENERATED_BODY()
	
public:
	virtual FLuaScriptSettings GetLuaScriptSettings_Implementation() override;

	virtual void SetLuaScriptSettings_Implementation(FLuaScriptSettings newSettings) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLuaScriptSettings LuaScriptSettings;
};
