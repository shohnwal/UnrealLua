// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaUObject.h"
#include "ReplicatedLuaUObject.generated.h"

/**
 * 
 */
UCLASS()
class UNREALLUA_API UReplicatedLuaUObject : public ULuaUObject
{
	GENERATED_BODY()
public:
	UReplicatedLuaUObject();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual bool IsSupportedForNetworking() const override;
	//Called only on server by ULuaScriptReplicationComponent::PreReplicate
	virtual void PreReplication();

	UPROPERTY(EditDefaultsOnly)
	TEnumAsByte<ELifetimeCondition> LuaValueReplicationCondition;
	
	//UPROPERTY(VisibleAnywhere, Replicated)
	//FLuaValueReplicator LuaValueReplicator;
};
