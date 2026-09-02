// Fill out your copyright notice in the Description page of Project Settings.


#include "ScriptableUObject/ReplicatedLuaUObject.h"

#include "Net/UnrealNetwork.h"


UReplicatedLuaUObject::UReplicatedLuaUObject()
	: LuaValueReplicationCondition(ELifetimeCondition::COND_None)
{
	//this->LuaValueReplicator.ReplicatorOwner = FRegisteredLuaNetObjectInfo{this,0};
}

void UReplicatedLuaUObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	UObject::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Fast Arrays don't use push model, but there's no harm in marking them with it.
	// The flag will just be ignored.
	FDoRepLifetimeParams params;
 
	params.Condition = LuaValueReplicationCondition;
	params.bIsPushBased = true;
	
	//DOREPLIFETIME_WITH_PARAMS_FAST(UReplicatedLuaUObject, LuaValueReplicator, params);
}

bool UReplicatedLuaUObject::IsSupportedForNetworking() const
{
	return true;
}

void UReplicatedLuaUObject::PreReplication()
{
	//this->LuaValueReplicator.PreReplication();
}