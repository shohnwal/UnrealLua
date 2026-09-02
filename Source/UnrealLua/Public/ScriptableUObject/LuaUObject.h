// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Interface/LuaScriptable.h"
#include "LuaUObject.generated.h"

struct FLuaValueReplicator;

UCLASS(Blueprintable)
class UNREALLUA_API ULuaUObject : public UObject, public ILuaScriptable
{
friend class ULuaScriptReplicationComponent;
	
	GENERATED_BODY()
public:
	virtual FLuaScriptSettings GetLuaScriptSettings_Implementation() override;
	virtual void SetLuaScriptSettings_Implementation(FLuaScriptSettings newSettings) override;

	UPROPERTY(EditDefaultsOnly)
	FLuaScriptSettings LuaScriptSettings;
};