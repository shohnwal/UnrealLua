// Fill out your copyright notice in the Description page of Project Settings.
#include "Replication/SerializedLuaValue.h"
#include "UnrealLua.h"
#include "Components/ActorComponent.h"
#include "Engine/PackageMapClient.h"
#include "GameFramework/Actor.h"
#include "LuaContext/LuaScripts/LoadedLuaScriptCollection.h"
#include "Replication/LuaScriptReplicationComponent.h"
#include "UObject/UObjectThreadContext.h"

bool FNetSerializedLuaValue::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Ar << this->ValueOwnerID;
	Ar << this->RepLayoutPropertyIndex;
	bOutSuccess = this->LuaValue.NetSerialize(Ar, Map, bOutSuccess);
	return bOutSuccess;
}

bool FNetSerializedLuaValue::IsReplicatedObjectEntry() const
{
	return this->RepLayoutPropertyIndex == FLuaRepLayout::ReplayoutOwnerIndex;
}

UObject* FNetSerializedLuaValue::ResolveUObject(ULuaScriptReplicationComponent* owningComponent)
{
	if (!this->IsReplicatedObjectEntry())
	{
		return nullptr;
	}
	FRegisteredLuaNetObjectInfo& netObjectInfo = this->LuaValue.GetMutable<FRegisteredLuaNetObjectInfo>();
	if (IsValid(netObjectInfo.RegisteredObject))
	{
		return netObjectInfo.RegisteredObject;
	}
	const FWeakRegisteredLuaNetObjectInfo* info = owningComponent->FindReplicatedObjectInfo(netObjectInfo.LuaNetHandle);
	if (info)
	{
		netObjectInfo.RegisteredObject = info->RegisteredObject.Get(); 
	}
	return netObjectInfo.RegisteredObject;
}