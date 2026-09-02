// Fill out your copyright notice in the Description page of Project Settings.


#include "Replication/LuaScriptReplicationComponent.h"

#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "UnrealLua.h"
#include "Replication/LuaObjectReplicator.h"
#include "Runtime/Engine/Private/Net/NetSubObjectRegistryGetter.h"
#include "ScriptableUObject/ReplicatedLuaUObject.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "sol/sol.hpp"
#include "Config/UnrealLuaConfig.h"
#include "Async/ParallelFor.h"
#include "LuaContext/LuaScripts/LoadedLuaScriptCollection.h"
#include "Net/Core/PushModel/PushModel.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"

// Sets default values for this component's properties
ULuaScriptReplicationComponent::ULuaScriptReplicationComponent()
	: LuaObjectReplicators()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	this->bWantsInitializeComponent = true;
	PrimaryComponentTick.bCanEverTick = false;
	this->SetIsReplicatedByDefault(true);
	this->bReplicateUsingRegisteredSubObjectList = true;
}

void ULuaScriptReplicationComponent::OnRegister()
{
	Super::OnRegister();
}

void ULuaScriptReplicationComponent::InitializeComponent()
{
	Super::InitializeComponent();

	AActor* owner = this->GetOwner();
	TArray<UObject*> subobjects;
	GetObjectsWithOuter(owner, subobjects, true);
	for(UObject* subobject : subobjects)
	{
		FLuaUObjectItem* item = UnrealLua::UObjectRegistry::TryGetUObjectItem(subobject);
		if (item)
		{
			FLuaScriptInstanceHandle& handle = item->GetLuaScriptHandle();
			handle.InitializeLuaReplication();	
		}
	}
}

void ULuaScriptReplicationComponent::ReadyForReplication()
{
	Super::ReadyForReplication();
	if(this->GetOwner()->HasAuthority())
	{
		
	}
}

void ULuaScriptReplicationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams params;
	params.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS(ULuaScriptReplicationComponent, LuaObjectReplicators, params);
}

bool ULuaScriptReplicationComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	/*
	if (Channel->KeyNeedsToReplicate(0, ReplicatedArrayKey) )	// Does the array need to replicate?
	{
		for (int32 idx = 0; idx < ReplicatedSubobjects.Num(); ++idx )
		{
			UMyActorSubobjClass *Obj = ReplicatedSubObjects[idx];
			if (Channel->KeyNeedsToReplicate(1 + idx, Obj->RepKey))
			{								
				WroteSomething |= Channel->ReplicateSubobject<UMyActorSubobjClass>(Obj, *Bunch, *RepFlags);
			}
		}
	}
	*/

	return WroteSomething;
}

/**
 * Called on the actor right before replication occurs. 
 * Only called on Server, and for autonomous proxies if recording a Client Replay.
 */
void ULuaScriptReplicationComponent::PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker)
{
	UActorComponent::PreReplication(ChangedPropertyTracker);

	if(this->LuaObjectReplicators.Num() < 3)
	{
		for(auto& replicator : this->LuaObjectReplicators)
		{
			replicator->PreReplication();
		}
	}
	else
	{
		ParallelFor(this->LuaObjectReplicators.Num(), [this](int32 index)
		{
			auto& replicator = this->LuaObjectReplicators[index];
			replicator->PreReplication();
		});	
	}

	const UE::Net::FSubObjectRegistry& reg = UE::Net::FSubObjectRegistryGetter::GetSubObjects(this->GetOwner());

	for(auto item : reg.GetRegistryList())
	{
		UObject* obj = item.GetSubObject();
		if(UReplicatedLuaUObject* luaObj = Cast<UReplicatedLuaUObject>(obj))
		{
			luaObj->PreReplication();
		}
	}
}

void ULuaScriptReplicationComponent::BeginPlay()
{
	Super::BeginPlay();
	if(!this->GetOwner()->HasAuthority())
	{
		for(TObjectPtr<ULuaObjectReplicator>& replicator : this->LuaObjectReplicators)
		{
			replicator->InitialReplication();
		}
	}
}

void ULuaScriptReplicationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	//this->GetLuaValueReplicator()->ResetValues();
	for(auto& replicator : this->LuaObjectReplicators)
	{
		replicator->ResetValues();
		this->RemoveReplicatedSubObject(replicator);
	}
	this->LuaObjectReplicators.Empty();
	Super::EndPlay(EndPlayReason);
}

void ULuaScriptReplicationComponent::RegisterLuaScriptableObjectForReplication(const FLuaUObjectItem& item)
{
	AActor* ownerActor = this->GetOwner();
	UObject* obj = item.Object;

	FLuaNetHandle netHandle = item.NetHandle;
	if(netHandle.IsValid())
	{
		LUA_LOG("Registering UObject %s with NetHandle %d", *GetNameSafe(obj), netHandle.HandleValue)
	}
	FRegisteredLuaNetObjectInfo info{obj, netHandle};
	this->RegisteredReplicatedObjects.Emplace(info);

	if(ownerActor->GetNetMode() == ENetMode::NM_Client)
	{
		//@TODO : check for existing replicators and apply values?
		//Actually no, this is done in BeginPlay
		return;
	}
	else
	{
		//server will set up Replicators
		
		FLuaRepLayout* repLayout = item.ScriptHandle.GetRepLayout();
		if(repLayout->RepLayoutProperties.IsEmpty())
		{
			return;
		}
		bool canRegisterReplicator = false;
		FString failReason{};

		if(obj == ownerActor)
		{
			//Actors can always register a replicator
			canRegisterReplicator = true;
		}
		else if(netHandle.IsValid())
		{
			//If we have a nethandle we can always register
			canRegisterReplicator = true;
		}
		else if(obj->IsSupportedForNetworking())
		{
			//Objects supported for networking must already be registered
			if(UE::Net::FSubObjectRegistryGetter::IsSubObjectInAnyRegistry(ownerActor, obj))
			{
				canRegisterReplicator = true;	
			}
			else
			{
				failReason = FString::Printf(TEXT("Object not registered as replicated SubObject in %s or any of its components"), *GetFullNameSafe(ownerActor));
			}
		}
		else if(obj->IsFullNameStableForNetworking())
		{
			//Stably named objects can replicated no-prob
			canRegisterReplicator = true;
		}
		else
		{
			failReason = TEXT("Object does not fulfill any criterium for Lua replication");
		}

		if(!canRegisterReplicator)
		{
			LUA_LOG_WARNING("Unable to register UObject %s for Lua replication. Reason: %s", *GetFullNameSafe(obj), *failReason)
			return;
		}
		
		for(auto& repCondition : repLayout->PropertyReplicationConditionFlags)
		{
			ULuaObjectReplicator* replicator = this->GetOrCreateReplicatorForObject(info, repCondition);
		}
	}
}

void ULuaScriptReplicationComponent::UnregisterFromLuaReplication(const FLuaUObjectItem& item)
{
	UObject* obj = item.Object;
	FLuaNetHandle netHandle = item.NetHandle;
	
	FRegisteredLuaNetObjectInfo toRemove{obj, netHandle};
	this->RegisteredReplicatedObjects.RemoveSingleSwap(toRemove);
	
	if (this->GetWorld() && this->GetWorld()->GetNetMode() < ENetMode::NM_Client)
	{
		this->RemoveAllReplicatorsForObject(obj);
	}
}
/*
FLuaValueReplicator* ULuaScriptReplicationComponent::GetLuaValueReplicator()
{
	return &this->LuaScriptReplicator;
}
*/

ULuaObjectReplicator* ULuaScriptReplicationComponent::GetOrCreateReplicatorForObject(const FRegisteredLuaNetObjectInfo& info, ELifetimeCondition repCondition)
{
	verify(this->GetOwner()->HasAuthority());
	UObject* obj = info.RegisteredObject.Get();
	
	if (repCondition == ELifetimeCondition::COND_None)
	{
		
	}
	
	TObjectPtr<ULuaObjectReplicator>* replicatorPtr = this->LuaObjectReplicators.FindByPredicate([obj, repCondition](ULuaObjectReplicator* replicator)
	{
		return replicator->ScriptOwnerInfo.RegisteredObject == obj && replicator->GetReplicationCondition() == repCondition;
	});
	if(replicatorPtr)
	{
		return *replicatorPtr;
	}
	ULuaObjectReplicator* replicator = NewObject<ULuaObjectReplicator>(this);
	replicator->SetReplicationCondition(repCondition);
	replicator->SetScriptOwner(info);
	this->LuaObjectReplicators.Emplace(replicator);
	this->AddReplicatedSubObject(replicator, repCondition);
	MARK_PROPERTY_DIRTY_FROM_NAME(ULuaScriptReplicationComponent, LuaObjectReplicators, this);
	return replicator;
}

ULuaObjectReplicator* ULuaScriptReplicationComponent::GetReplicatorForObject(UObject* obj, ELifetimeCondition repCondition)
{
	TObjectPtr<ULuaObjectReplicator>* replicatorPtr = this->LuaObjectReplicators.FindByPredicate([obj, repCondition](ULuaObjectReplicator* replicator)
	{
		return replicator->ScriptOwnerInfo.RegisteredObject == obj && replicator->GetReplicationCondition() == repCondition;
	});
	if(replicatorPtr)
	{
		return *replicatorPtr;
	}
	return nullptr;
}

TArray<ULuaObjectReplicator*> ULuaScriptReplicationComponent::GetAllReplicatorsForObject(UObject* obj)
{
	TArray<ULuaObjectReplicator*> foundReplicators;
	for(auto& replicator : this->LuaObjectReplicators)
	{
		if(replicator->ScriptOwnerInfo.RegisteredObject == obj)
		{
			foundReplicators.Add(replicator);
		}
	}
	return foundReplicators;
}

void ULuaScriptReplicationComponent::RemoveAllReplicatorsForObject(UObject* obj)
{
	for (TArray<TObjectPtr<ULuaObjectReplicator>>::TIterator it = this->LuaObjectReplicators.CreateIterator(); it; ++it)
	{
		ULuaObjectReplicator* replicator = *it;
		if (replicator->ScriptOwnerInfo.RegisteredObject == obj)
		{
			this->DestroyReplicatedSubObjectOnRemotePeers(replicator);
			replicator->ConditionalBeginDestroy();
			it.RemoveCurrent();
		}
	}
}

void ULuaScriptReplicationComponent::RemoveLuaReplicator(ULuaObjectReplicator* luaObjectReplicator)
{
	this->DestroyReplicatedSubObjectOnRemotePeers(luaObjectReplicator);
	this->LuaObjectReplicators.RemoveSingleSwap(luaObjectReplicator);
	luaObjectReplicator->ConditionalBeginDestroy();
}

void ULuaScriptReplicationComponent::LuaRPC(UObject* target, const FString& funcName, const TArray<FLuaValue>& args)
{
	if(funcName.IsEmpty())
	{
		LUA_LOG_WARNING("Can not call Lua RPC function on UObject %s : Function name is empty", *GetNameSafe(target))
		return;
	}

	AActor* ownerActor = this->GetOwner();
	
	//if target is not the owner actor, make sure target is actually calling the correct RPC component actor
	if(ownerActor != target)
	{
		if(!target->IsInOuter(ownerActor))
		{
			LUA_LOG_WARNING("Can not call Lua RPC function '%s' on UObject %s since it is not in outer %s", *funcName, *GetNameSafe(target), *GetNameSafe(ownerActor))
			return;
		}
	}
	
	bool bDoNormalRPC = false;
	
	if(target == ownerActor)
	{
		bDoNormalRPC = true;
	}
	else if(target->IsSupportedForNetworking())
	{
		if(UE::Net::FSubObjectRegistryGetter::IsSubObjectInAnyRegistry(ownerActor, target))
		{
			bDoNormalRPC = true;	
		}
	}
	else if(target->IsFullNameStableForNetworking())
	{
		bDoNormalRPC = true;
	}

	if(bDoNormalRPC)
	{
		//LUA_LOG_WARNING("Performing normal RPC on target %s in actor %s with LuaFunction %s", *GetNameSafe(target), *GetNameSafe(ownerActor), *funcName)

		if(funcName.StartsWith("MULTICAST_"))
		{
			this->MULTICAST_LuaRpc(target, funcName, args);
		}
		else if(funcName.StartsWith("CLIENT_"))
		{
			this->CLIENT_LuaRpc(target, funcName, args);
		}
		else if(funcName.StartsWith("SERVER_"))
		{
			this->SERVER_LuaRpc(target, funcName, args);
		}		
	}
	else
	{
		FLuaNetHandle netHandle = UnrealLua::UObjectRegistry::GetLuaNetHandleForObject(target);
		if(netHandle.IsValid())
		{
			LUA_LOG("Calling nethandle RPC %s with netHandle %d", *funcName, netHandle.HandleValue)
			if(funcName.StartsWith("MULTICAST_"))
			{
				this->MULTICAST_LuaRpcWithLuaNetHandle(netHandle, funcName, args);
			}
			else if(funcName.StartsWith("CLIENT_"))
			{
				this->CLIENT_LuaRpcWithObjectLuaNetHandle(netHandle, funcName, args);
			}
			else if(funcName.StartsWith("SERVER_"))
			{
				this->SERVER_LuaRpcWithLuaNetHandle(netHandle, funcName, args);
			}	
			return;	
		}
		LUA_LOG_WARNING("Calling name-based RPC on target %s in actor %s with LuaFunction %s", *GetNameSafe(target), *GetNameSafe(ownerActor), *funcName)

		FString strToActor = target->GetPathName(this->GetOwner());
		
		if(funcName.StartsWith("MULTICAST_"))
		{
			this->MULTICAST_LuaRpcObjectName(strToActor, funcName, args);
		}
		else if(funcName.StartsWith("CLIENT_"))
		{
			this->CLIENT_LuaRpcObjectName(strToActor, funcName, args);
		}
		else if(funcName.StartsWith("SERVER_"))
		{
			this->SERVER_LuaRpcObjectName(strToActor, funcName, args);
		}		
	}
}

void ULuaScriptReplicationComponent::LuaRPCBySubobjectName(const FString& targetName, const FString& funcName, const TArray<FLuaValue>& args)
{
	if(funcName.StartsWith("MULTICAST_"))
	{
		this->MULTICAST_LuaRpcObjectName(targetName, funcName, args);
	}
	else if(funcName.StartsWith("CLIENT_"))
	{
		this->CLIENT_LuaRpcObjectName(targetName, funcName, args);
	}
	else if(funcName.StartsWith("SERVER_"))
	{
		this->SERVER_LuaRpcObjectName(targetName, funcName, args);
	}	
}

void ULuaScriptReplicationComponent::MULTICAST_LuaRpc_Implementation(UObject* target, const FString& funcName, const TArray<FLuaValue>& args)
{
	this->PerformRpcCallOnTarget(target,funcName, args);
}

void ULuaScriptReplicationComponent::CLIENT_LuaRpc_Implementation(UObject* target, const FString& funcName, const TArray<FLuaValue>& args)
{
	this->PerformRpcCallOnTarget(target,funcName, args);
}

bool ULuaScriptReplicationComponent::SERVER_LuaRpc_Validate(UObject* target, const FString& funcName, const TArray<FLuaValue>& args)
{
	bool bIgnoreErrors = UUnrealLuaConfig::ShouldIgnoreInvalidServerRPC();
	if(funcName.IsEmpty())
	{
		LUA_LOG_ERROR("LuaRPC: Received empty Lua Server RPC function name for target object '%s'", *GetNameSafe(target))
		if(bIgnoreErrors)
		{
			return true;
		}
		return false;	
	}
	if(!IsValid(target))
	{
		LUA_LOG_ERROR("LuaRPC: Received invalid UObject %s target for SERVER_LuaRpc function '%s'", *GetNameSafe(target), *funcName)
		if(bIgnoreErrors)
		{
			return true;
		}
		return false;	
	}
	else
	{
		AActor* owner = this->GetOwner();
		if(owner != target && target->GetTypedOuter<AActor>() != owner)
		{
			LUA_LOG_ERROR("LuaRPC: Target '%s' for Lua Server RPC function '%s' is not in outer '%s'", *GetNameSafe(target), *funcName, *GetNameSafe(this->GetOwner()))
			if(bIgnoreErrors)
			{
				return true;
			}
			return false;
		}
	}
	FLuaUObjectItem& handle = UnrealLua::UObjectRegistry::GetUObjectItem(target);
	if(!handle.GetLuaScriptFunction(funcName).valid())
	{
		LUA_LOG_ERROR("LuaRPC: SERVER_LuaRpc received non-existing Lua function name '%s' for target object '%s'", *funcName, *GetNameSafe(target))
		if(bIgnoreErrors)
		{
			return true;
		}
		return false;	
	}
	return true;
}

void ULuaScriptReplicationComponent::SERVER_LuaRpc_Implementation(UObject* target, const FString& funcName, const TArray<FLuaValue>& args)
{
	this->PerformRpcCallOnTarget(target, funcName, args);
}

void ULuaScriptReplicationComponent::MULTICAST_LuaRpcWithLuaNetHandle_Implementation(FLuaNetHandle targetID, const FString& funcName, const TArray<FLuaValue>& args)
{
	this->PerformRpcCallOnTarget(targetID, funcName, args);
}

void ULuaScriptReplicationComponent::CLIENT_LuaRpcWithObjectLuaNetHandle_Implementation(FLuaNetHandle targetID, const FString& funcName, const TArray<FLuaValue>& args)
{
	this->PerformRpcCallOnTarget(targetID, funcName, args);
}

bool ULuaScriptReplicationComponent::SERVER_LuaRpcWithLuaNetHandle_Validate(FLuaNetHandle targetID, const FString& funcName, const TArray<FLuaValue>& args)
{
	return targetID.IsValid();
}

void ULuaScriptReplicationComponent::SERVER_LuaRpcWithLuaNetHandle_Implementation(FLuaNetHandle targetID, const FString& funcName, const TArray<FLuaValue>& args)
{
	this->PerformRpcCallOnTarget(targetID, funcName, args);
}

const FRegisteredLuaNetObjectInfo* ULuaScriptReplicationComponent::FindReplicatedObjectInfo(UObject* obj) const
{
	return this->RegisteredReplicatedObjects.FindByPredicate([obj](const FRegisteredLuaNetObjectInfo& item)
	{
		return item.RegisteredObject == obj;
	});
}

const FRegisteredLuaNetObjectInfo* ULuaScriptReplicationComponent::FindReplicatedObjectInfo(const FLuaNetHandle handle) const
{
	return this->RegisteredReplicatedObjects.FindByPredicate([handle](const FRegisteredLuaNetObjectInfo& item)
{
	return item.LuaNetHandle == handle;
});
}

void ULuaScriptReplicationComponent::MULTICAST_LuaRpcObjectName_Implementation(const FString& target, const FString& funcName, const TArray<FLuaValue>& args)
{
	AActor* actor = this->GetOwner();
	LUA_LOG_WARNING("Received named MULTICAST_RPC command on target %s in actor %s with LuaFunction %s", *target, *GetNameSafe(actor), *funcName)
	UObject* obj = FindObject<UObject>(this->GetOwner(), *target);
	if(!obj)
	{
		return;
	}
	this->PerformRpcCallOnTarget(obj, funcName, args);
}

void ULuaScriptReplicationComponent::CLIENT_LuaRpcObjectName_Implementation(const FString& target, const FString& funcName, const TArray<FLuaValue>& args)
{
	AActor* actor = this->GetOwner();
	LUA_LOG("Received named CLIENT_RPC command on target %s in actor %s with LuaFunction %s", *target, *GetNameSafe(actor), *funcName)
	UObject* obj = FindObject<UObject>(this->GetOwner(), *target);
	if(!obj)
	{
		return;
	}
	this->PerformRpcCallOnTarget(obj, funcName, args);
}

bool ULuaScriptReplicationComponent::SERVER_LuaRpcObjectName_Validate(const FString& target, const FString& funcName, const TArray<FLuaValue>& args)
{
	return true;
}

void ULuaScriptReplicationComponent::SERVER_LuaRpcObjectName_Implementation(const FString& target, const FString& funcName, const TArray<FLuaValue>& args)
{
	AActor* owner = this->GetOwner();
	LUA_LOG("Received named SERVER_RPC command on target %s in actor %s with LuaFunction %s", *target, *GetNameSafe(owner), *funcName)
	UObject* obj = FindObject<UObject>(owner, *target);
	if(!obj)
	{
		LUA_LOG_WARNING("Unable to perform named SERVER_RPC command on target %s in actor %s with LuaFunction %s : Object not found in actor", *target, *GetNameSafe(owner), *funcName)
		return;
	}
	this->PerformRpcCallOnTarget(obj, funcName, args);
}

void ULuaScriptReplicationComponent::PerformRpcCallOnTarget(FLuaNetHandle targetID, const FString& funcName, const TArray<FLuaValue>& args)
{
	LUA_LOG_WARNING("Received Nethandle RPC command on handle %d in actor %s with LuaFunction %s", targetID.HandleValue, *GetNameSafe(this->GetOwner()), *funcName)
	const FRegisteredLuaNetObjectInfo* found = this->FindReplicatedObjectInfo(targetID);
	if(!found)
	{
		LUA_LOG_ERROR("Coiuld not find any target object with netID %d", targetID.HandleValue)
		return;
	}
	UObject* target = found->RegisteredObject.Get();
	if(!target)
	{
		LUA_LOG_ERROR("Target object with netID %d is invalid", targetID.HandleValue)
		return;
	}
	this->PerformRpcCallOnTarget(target, funcName, args);
}

void ULuaScriptReplicationComponent::PerformRpcCallOnTarget(UObject* target, const FString& funcName, const TArray<FLuaValue>& args)
{
	AActor* actor = this->GetOwner();
	LUA_LOG_WARNING("Received RPC command on target %s in actor %s with LuaFunction %s", *GetNameSafe(target), *GetNameSafe(actor), *funcName)
	
	if(!IsValid(target) || funcName.IsEmpty())
	{
		LUA_LOG_ERROR("Can't perform RPC script call %s on object %s : Invalid target or funcname is empty", *funcName, *GetNameSafe(target))
		return;
	}
	FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(target);

	//Conditionally try to load Lua script for newly replicated UObjects

	if(!item.ScriptHandle.IsValid())
	{
		UnrealLua::UObjectRegistry::LoadLuaScript(target, true);
	}	
	
	sol::function func = item.GetLuaScriptFunction(funcName);
	
	if(!func.valid())
	{
		LUA_LOG_ERROR("Can't perform RPC script call %s on object %s : no Lua function found" , *funcName, *GetNameSafe(target))
		return;
	}
	UnrealLua::LuaScriptCall::CallLuaFunctionSafe(func, target, sol::as_args(args));
}
