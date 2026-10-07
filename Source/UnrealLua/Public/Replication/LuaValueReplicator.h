// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaNetHandle.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Replication/SerializedLuaValue.h"
#include "LuaValueReplicator.generated.h"
/**
 * 
 */

struct FUnrealLuaRepLayoutProperty;
class ULuaObjectReplicator;

UENUM()
enum class ELuaValueChangeOP : uint8
{
	ADD,
	CHANGE,
	REMOVE
};

USTRUCT()
struct UNREALLUA_API FChangedNetLuaValueOp
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere)
	int32 ValueOwnerID = 0;
	UPROPERTY(VisibleAnywhere)
	uint8 RepIndex = 0;
	UPROPERTY(VisibleAnywhere)
	ELuaValueChangeOP Op = ELuaValueChangeOP::CHANGE;
	FUnrealLuaRepLayoutProperty* foundRepProp = nullptr;
	FNetSerializedLuaValue* ReplicatedLuaValue = nullptr;
};

namespace UnrealLua::NetSerialize
{
	extern FScopedLuaContext* CurrentCtx;	
}

struct FStructView;
struct FLuaRepLayout;
class ALuaActor;
class ULuaScriptReplicationComponent;
//struct FReplicatedLuaScript;
class ILuaScriptable;
class ULuaScriptInstance;
struct FLuaScriptInstanceHandle;

USTRUCT(BlueprintType)
struct UNREALLUA_API FLuaValueReplicator : public FFastArraySerializer
{
	GENERATED_BODY()
	FLuaValueReplicator();

	//UObject* GetReplicatorScriptOwner();
	
	void ServerAddReplicatedObject(const FWeakRegisteredLuaNetObjectInfo& repObjectInfo, const FLuaRepLayout& repLayout, ELifetimeCondition repCondition);
	void ServerUnregisterObject(const FRegisteredLuaNetObjectInfo& ToRemove);

	//UPROPERTY(VisibleAnywhere, NotReplicated)
	//TObjectPtr<ULuaObjectReplicator> OuterReplicator;
	
	UPROPERTY(VisibleAnywhere, meta=(ShowOnlyInnerProperties))
	TArray<FNetSerializedLuaValue>	Items;	/** Step 3: You MUST have a TArray named Items of the struct you made in step 1. */

	//Value owners collection on client
	UPROPERTY(NotReplicated, VisibleAnywhere)
	TArray<FNetSerializedLuaValue> ValueOwners;
	
	FNetSerializedLuaValue* FindValueOwnerForOwnerID(int32 ownerID);
	
	//Temporary buffer for changed items events on client
	UPROPERTY(NotReplicated, VisibleAnywhere)
	TArray<FChangedNetLuaValueOp> ChangedValues;

	void InitialReplication();
	/**
	 *	Read replicated Lua values from LuaValue script  
	 *	and serialize changed values as FFastArrayArrayItems
	 */
	void PreReplication();
	bool ServerProcessValues(const double currentServerTime);
	//void ServerProcessValue(const FLuaValue& currentScriptValue, const FUnrealLuaRepLayoutProperty* const repProp);
	void ServerProcessValue(const FLuaValue& currentScriptValue, FNetSerializedLuaValue& netValue);

	/**
	* Called before removing elements and after the elements themselves are notified.  The indices are valid for this function call only!
	*
	* NOTE: intentionally not virtual; invoked via templated code, @see FExampleItemEntry
	*/
	void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize);

	/**
	* Called after adding all new elements and after the elements themselves are notified.  The indices are valid for this function call only!
	*
	* NOTE: intentionally not virtual; invoked via templated code, @see FExampleItemEntry
	*/
	void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);

	/**
	* Called after updating all existing elements with new data and after the elements themselves are notified. The indices are valid for this function call only!
	*
	* NOTE: intentionally not virtual; invoked via templated code, @see FExampleItemEntry
	*/
	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);

	/**
	* Called after all 3 above functions were called on the receiving client.
	*/
	void PostReplicatedReceive(const FPostReplicatedReceiveParameters& Parameters);
	
private:
	void ClientProcessChangedValues();
	
	void UpdateScriptOwnerValueInternal(const FChangedNetLuaValueOp& changedValue, FUnrealLuaRepLayoutProperty* foundRepProp, FLuaUObjectItem& targetObject);
	void UpdateSubobjectPropertyValueInternal(const FChangedNetLuaValueOp& changedValue, FUnrealLuaRepLayoutProperty* foundRepProp, UObject* targetSubobject);
	void CallRepNotifies(const TMap<UObject*, TArray<FChangedNetLuaValueOp*>>& valueOwnerToChangedValuesMap);
	//void ProcessChangedValue(FChangedLuaValueOp& changedValue);
public:
	
	/** Step 4: Copy this, replace example with your names */
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms) {
		return FFastArraySerializer::FastArrayDeltaSerialize(Items, DeltaParms, *this);
	}
	void ResetValues();
private:
	uint32 GetNewOwnerID();
	UPROPERTY(NotReplicated, VisibleAnywhere)
	uint32 OwnerIDCounter = 0;
};

/** Step 5: Copy and paste this struct trait, replacing FExampleArray with your Step 2 struct. */
template<>
struct TStructOpsTypeTraits< FLuaValueReplicator > : TStructOpsTypeTraitsBase2<FLuaValueReplicator>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};