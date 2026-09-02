
#include "Replication/LuaObjectReplicator.h"

#include "Replication/LuaScriptReplicationComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"

void ULuaObjectReplicator::PostInitProperties()
{
	this->Replicator.OuterReplicator = this;
	UObject::PostInitProperties();
}

void ULuaObjectReplicator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	UObject::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams params;
	params.bIsPushBased = true;
	params.Condition = COND_InitialOnly;
	DOREPLIFETIME_WITH_PARAMS_FAST(ULuaObjectReplicator, ScriptOwnerInfo, params);
	DOREPLIFETIME_WITH_PARAMS_FAST(ULuaObjectReplicator, ReplicationCondition, params);
	params.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(ULuaObjectReplicator, Replicator, params);
}

void ULuaObjectReplicator::SetScriptOwner(const FRegisteredLuaNetObjectInfo& info)
{
	this->Replicator.OuterReplicator = this;
	this->ScriptOwnerInfo = info;
	MARK_PROPERTY_DIRTY_FROM_NAME(ULuaObjectReplicator, ScriptOwnerInfo, this);
}

void ULuaObjectReplicator::SetReplicationCondition(ELifetimeCondition repCondition)
{
	this->ReplicationCondition = repCondition;
}

ELifetimeCondition ULuaObjectReplicator::GetReplicationCondition() const
{
	return this->ReplicationCondition;
}

void ULuaObjectReplicator::PreReplication()
{
	this->Replicator.PreReplication();
}

void ULuaObjectReplicator::ResetValues()
{
	this->Replicator.ResetValues();
}

void ULuaObjectReplicator::InitialReplication()
{
	this->Replicator.InitialReplication();
}

void ULuaObjectReplicator::PreDestroyFromReplication()
{
	ULuaScriptReplicationComponent* cmp = this->GetReplicationComponent();
	if(cmp)
	{
		cmp->RemoveLuaReplicator(this);
	}
	UObject::PreDestroyFromReplication();
}

UObject* ULuaObjectReplicator::GetReplicatorScriptOwner()
{
	UObject* obj = this->ScriptOwnerInfo.RegisteredObject.Get();
	if(!obj)
	{
		FLuaNetHandle netHandle = this->ScriptOwnerInfo.LuaNetHandle; 
		if(netHandle.IsValid())
		{
			const FRegisteredLuaNetObjectInfo* registeredInfo = this->GetReplicationComponent()->FindReplicatedObjectInfo(netHandle);
			if(registeredInfo)
			{
				obj = registeredInfo->RegisteredObject.Get();
				this->ScriptOwnerInfo.RegisteredObject = obj;
			}
		}
	}
	return obj;
}

ULuaScriptReplicationComponent* ULuaObjectReplicator::GetReplicationComponent() const
{
	return Cast<ULuaScriptReplicationComponent>(this->GetOuter());
}

ENetRole ULuaObjectReplicator::GetOwnerNetRole() const
{
	return this->GetReplicationComponent()->GetOwnerRole();
}

void ULuaObjectReplicator::OnRep_ScriptOwner()
{
	if(this->ScriptOwnerInfo.LuaNetHandle.IsValid())
	{
		ULuaScriptReplicationComponent* cmp = this->GetReplicationComponent();
		verify(IsValid(cmp));
		const FRegisteredLuaNetObjectInfo* registeredInfo = this->GetReplicationComponent()->FindReplicatedObjectInfo(this->ScriptOwnerInfo.LuaNetHandle);
		if(registeredInfo)
		{
			UObject* obj = registeredInfo->RegisteredObject.Get();
			this->ScriptOwnerInfo.RegisteredObject = obj;
		}
	}
}
