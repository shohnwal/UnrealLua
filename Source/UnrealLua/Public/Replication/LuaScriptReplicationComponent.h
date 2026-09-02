// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
//#include "Replication/LuaValueReplicator.h"
#include "Replication/LuaNetHandle.h"
#include "LuaScriptReplicationComponent.generated.h"


struct FLuaUObjectItem;
class ULuaObjectReplicator;
class ILuaContext;
struct FLuaBPTable;
class ULuaContext;

UCLASS(BlueprintType, NotBlueprintable, Transient)
class UNREALLUA_API ULuaScriptReplicationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	ULuaScriptReplicationComponent();

	virtual void OnRegister() override;

	virtual void InitializeComponent() override;

	virtual void ReadyForReplication() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


	/** 
	* Allows a component to replicate other subobject on the actor.
	* Must return true if any data gets serialized into the bunch.
	* This method is used only when bReplicateUsingRegisteredSubObjectList is false.
	* Otherwise this function is not called and only the ReplicatedSubObjects list is used.
	*/
	virtual bool ReplicateSubobjects(class UActorChannel *Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags);
	
	/**
	 * Called on the actor right before replication occurs. 
	 * Only called on Server, and for autonomous proxies if recording a Client Replay.
	 */
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;

	virtual void BeginPlay() override;


	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
public:
	void RegisterLuaScriptableObjectForReplication(const FLuaUObjectItem& item);
	void UnregisterFromLuaReplication(const FLuaUObjectItem& item);
	
	const FRegisteredLuaNetObjectInfo* FindReplicatedObjectInfo(UObject* obj) const;
	const FRegisteredLuaNetObjectInfo* FindReplicatedObjectInfo(const FLuaNetHandle handle) const;
	void RemoveLuaReplicator(ULuaObjectReplicator* luaObjectReplicator);
private:
	UPROPERTY(VisibleAnywhere)
	TArray<FRegisteredLuaNetObjectInfo> RegisteredReplicatedObjects = {};

	/*
	UPROPERTY(Replicated, VisibleAnywhere)
	FLuaValueReplicator LuaScriptReplicator;

	FLuaValueReplicator* GetLuaValueReplicator();
	*/

	ULuaObjectReplicator* GetOrCreateReplicatorForObject(const FRegisteredLuaNetObjectInfo& info, ELifetimeCondition repCondition);
	ULuaObjectReplicator* GetReplicatorForObject(UObject* obj, ELifetimeCondition repCondition);
	TArray<ULuaObjectReplicator*> GetAllReplicatorsForObject(UObject* obj);
	void RemoveAllReplicatorsForObject(UObject* obj);

	UPROPERTY(Replicated,VisibleAnywhere)
	TArray<TObjectPtr<ULuaObjectReplicator>> LuaObjectReplicators;
public:
	void LuaRPC(UObject* target, const FString& funcName, const TArray<FLuaValue>& args);

	void LuaRPCBySubobjectName(const FString& target, const FString& funcName, const TArray<FLuaValue>& args);
private:
	/////////////////////////////////////////
	/// RPC by replicated UObject ref
	/// /////////////////////////////////////
	UFUNCTION(NetMulticast, Reliable)
	virtual void MULTICAST_LuaRpc(UObject* target, const FString& funcName, const TArray<FLuaValue>& args);
	virtual void MULTICAST_LuaRpc_Implementation(UObject* target, const FString& funcName, const TArray<FLuaValue>& args);
	UFUNCTION(Client, Reliable)
	virtual void CLIENT_LuaRpc(UObject* target, const FString& funcName, const TArray<FLuaValue>& args);
	virtual void CLIENT_LuaRpc_Implementation(UObject* target, const FString& funcName, const TArray<FLuaValue>& args);
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void SERVER_LuaRpc(UObject* target, const FString& funcName, const TArray<FLuaValue>& args);
	virtual bool SERVER_LuaRpc_Validate(UObject* target, const FString& funcName, const TArray<FLuaValue>& args);
	virtual void SERVER_LuaRpc_Implementation(UObject* target, const FString& funcName, const TArray<FLuaValue>& args);

	/////////////////////////////////////////
	/// RPC by ULuaScriptable unique ID
	/// /////////////////////////////////////
	UFUNCTION(NetMulticast, Reliable)
	virtual void MULTICAST_LuaRpcWithLuaNetHandle(FLuaNetHandle targetID, const FString& funcName, const TArray<FLuaValue>& args);
	virtual void MULTICAST_LuaRpcWithLuaNetHandle_Implementation(FLuaNetHandle targetID, const FString& funcName, const TArray<FLuaValue>& args);
	UFUNCTION(Client, Reliable)
	virtual void CLIENT_LuaRpcWithObjectLuaNetHandle(FLuaNetHandle targetID, const FString& funcName, const TArray<FLuaValue>& args);
	virtual void CLIENT_LuaRpcWithObjectLuaNetHandle_Implementation(FLuaNetHandle targetID, const FString& funcName, const TArray<FLuaValue>& args);
	UFUNCTION(Server, Reliable)
	virtual void SERVER_LuaRpcWithLuaNetHandle(FLuaNetHandle targetID, const FString& funcName, const TArray<FLuaValue>& args);
	virtual bool SERVER_LuaRpcWithLuaNetHandle_Validate(FLuaNetHandle targetID, const FString& funcName, const TArray<FLuaValue>& args);
	virtual void SERVER_LuaRpcWithLuaNetHandle_Implementation(FLuaNetHandle targetID, const FString& funcName, const TArray<FLuaValue>& args);

	
	/////////////////////////////////////////
	/// RPC by objectname
	/// /////////////////////////////////////
	UFUNCTION(NetMulticast, Reliable)
	virtual void MULTICAST_LuaRpcObjectName(const FString& target, const FString& funcName, const TArray<FLuaValue>& args);
	virtual void MULTICAST_LuaRpcObjectName_Implementation(const FString& target, const FString& funcName, const TArray<FLuaValue>& args);
	UFUNCTION(Client, Reliable)
	virtual void CLIENT_LuaRpcObjectName(const FString& target, const FString& funcName, const TArray<FLuaValue>& args);
	virtual void CLIENT_LuaRpcObjectName_Implementation(const FString& target, const FString& funcName, const TArray<FLuaValue>& args);
	UFUNCTION(Server, Reliable)
	virtual void SERVER_LuaRpcObjectName(const FString& target, const FString& funcName, const TArray<FLuaValue>& args);
	virtual bool SERVER_LuaRpcObjectName_Validate(const FString& target, const FString& funcName, const TArray<FLuaValue>& args);
	virtual void SERVER_LuaRpcObjectName_Implementation(const FString& target, const FString& funcName, const TArray<FLuaValue>& args);

	void PerformRpcCallOnTarget(FLuaNetHandle targetID, const FString& funcName, const TArray<FLuaValue>& args);
	void PerformRpcCallOnTarget(UObject* target, const FString& funcName, const TArray<FLuaValue>& args);
};

//DECLARE_FUNCTION(execMakeLuaValue);