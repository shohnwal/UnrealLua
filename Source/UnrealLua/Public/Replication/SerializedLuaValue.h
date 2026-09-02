// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Engine/NetSerialization.h"
#include "LuaValue/LuaValue.h"
#include "SerializedLuaValue.generated.h"
/**
 * 
 */

struct FLuaValue;
struct FLuaUEnumEntry;
struct FLuaValueReplicator;

typedef TVariant<bool, uint8, int8, uint16, int16, uint32, int32, uint64, int64, double, float, UObject*, UClass*, UScriptStruct*, FLuaScriptStruct, FLuaInstancedStruct, FLuaSharedStruct, FLuaArray, FLuaSet, FLuaMap, FLuaUEnumEntry> LuaNetValueData;

USTRUCT()
struct UNREALLUA_API FNetSerializedLuaValue : public FFastArraySerializerItem
{
	GENERATED_BODY()
	FNetSerializedLuaValue()
		: RepLayoutPropertyIndex(0), LuaValue(nullptr)
	{
	}

	FNetSerializedLuaValue(uint8 repIndex)
		: RepLayoutPropertyIndex(repIndex)
		  , LuaValue(nullptr)
	{
	}

	UPROPERTY(VisibleAnywhere, NotReplicated)
	uint8 RepLayoutPropertyIndex; //RepPropertyIndex determines Property and the Subobject

	UPROPERTY(VisibleAnywhere, NotReplicated)
	FLuaValue LuaValue;
	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
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

/** Step 1: Make your struct inherit from FFastArraySerializerItem */
USTRUCT()
struct UNREALLUA_API FSerializedLuaValue : public FFastArraySerializerItem
{
	GENERATED_BODY()
	FSerializedLuaValue()
		: RepLayoutPropertyIndex(0), ScriptOwner(nullptr), LuaValue(nullptr)
	{
	}

	FSerializedLuaValue(uint8 repIndex, UObject* scriptOwner)
		: RepLayoutPropertyIndex(repIndex), ScriptOwner(scriptOwner)
		  , LuaValue(nullptr)
	{
	}

	//sol::object GetAsLuaValue();
	UPROPERTY(VisibleAnywhere, NotReplicated)
	TEnumAsByte<ELifetimeCondition> ReplicationCondition = COND_None;

	UPROPERTY(VisibleAnywhere, NotReplicated)
	uint8 RepLayoutPropertyIndex; //RepPropertyIndex determines Property and the Subobject
	UPROPERTY(VisibleAnywhere, NotReplicated)
	TObjectPtr<UObject> ScriptOwner; //The replicated UObject with the LuaScript that owns the RepLayout with the ProperyIndex

	UPROPERTY(VisibleAnywhere, NotReplicated)
	FLuaValue LuaValue;
	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
	bool SerializeScriptOwner(FArchive& Ar);
	void SerializeKey(FArchive& Ar);
	bool SerializeLuaValue(const FArchive& ar, UPackageMap* map, bool bOutSuccess);
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
struct TStructOpsTypeTraits<FSerializedLuaValue> : public TStructOpsTypeTraitsBase2<FSerializedLuaValue>
{
	enum
	{
		WithNetSerializer = true,
		//WithIdenticalViaEquality = true
	};
};