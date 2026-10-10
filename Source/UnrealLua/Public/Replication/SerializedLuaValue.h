// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Engine/NetSerialization.h"
#include "LuaValue/LuaValue.h"
#include "SerializedLuaValue.generated.h"
/**
 * 
 */

class ULuaScriptReplicationComponent;
struct FLuaValue;
struct FLuaUEnumEntry;
struct FLuaValueReplicator;

USTRUCT()
struct UNREALLUA_API FNetSerializedLuaValue : public FFastArraySerializerItem
{
	GENERATED_BODY()
	FNetSerializedLuaValue()
		: ValueOwnerID(0), RepLayoutPropertyIndex(0), LuaValue(nullptr)
	{
	}

	FNetSerializedLuaValue(uint32 valueOwnerID, uint8 repIndex)
		: ValueOwnerID(valueOwnerID), RepLayoutPropertyIndex(repIndex), LuaValue(nullptr)
	{
	}

	UPROPERTY(VisibleAnywhere)
	uint32 ValueOwnerID;
	
	UPROPERTY(VisibleAnywhere)
	uint8 RepLayoutPropertyIndex; //RepPropertyIndex determines Property and the Subobject

	UPROPERTY(VisibleAnywhere)
	FLuaValue LuaValue;
	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
	
	bool IsReplicatedObjectEntry() const;
	
	UObject* ResolveUObject(ULuaScriptReplicationComponent* owningComponent);
	
	bool operator==(const FNetSerializedLuaValue& other) const
	{
		return this->ValueOwnerID == other.ValueOwnerID && this->RepLayoutPropertyIndex == other.RepLayoutPropertyIndex && this->LuaValue == other.LuaValue;
	}
	/**
	 * Optional functions you can implement for client side notification of changes to items;
	 * Parameter type can match the type passed as the 2nd template parameter in associated call to FastArrayDeltaSerialize
	 *
	 * NOTE: It is not safe to modify the contents of the array serializer within these functions, nor to rely on the contents of the array
	 * being entirely up-to-date as these functions are called on items individually as they are updated, and so may be called in the middle of a mass update.
	 */
	
	//void PreReplicatedRemove(FLuaValueReplicator& InArraySerializer);
	//void PostReplicatedAdd(FLuaValueReplicator& InArraySerializer);
	//void PostReplicatedChange(FLuaValueReplicator& InArraySerializer);
	
	//void AddReplicatedValueToLua(const struct FReplicatedLuaValuesSerializer& InArraySerializer);
};

template<>
struct TStructOpsTypeTraits<FNetSerializedLuaValue> : public TStructOpsTypeTraitsBase2<FNetSerializedLuaValue>
{
	enum
	{
		WithNetSerializer = true,
		//WithIdenticalViaEquality = true
	};
};