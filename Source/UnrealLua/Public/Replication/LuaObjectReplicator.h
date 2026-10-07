#pragma once

#include "LuaNetHandle.h"
#include "UObject/Object.h"
#include "LuaValueReplicator.h"
#include "Engine/EngineTypes.h"
#include "LuaObjectReplicator.generated.h"

UCLASS()
class UNREALLUA_API ULuaObjectReplicator : public UObject
{
	GENERATED_BODY()
public:
	virtual void PostInitProperties() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void SetScriptOwner(const FWeakRegisteredLuaNetObjectInfo& info);
	void SetReplicationCondition(ELifetimeCondition repCondition);
	ELifetimeCondition GetReplicationCondition() const;
	void PreReplication();
	void ResetValues();
	void InitialReplication();
	virtual void PreDestroyFromReplication() override;
	UObject* GetReplicatorScriptOwner();

	UPROPERTY(Replicated, VisibleAnywhere)
	TEnumAsByte<ELifetimeCondition> ReplicationCondition;
	ULuaScriptReplicationComponent* GetReplicationComponent() const;
	ENetRole GetOwnerNetRole() const;

	UPROPERTY(ReplicatedUsing=OnRep_ScriptOwner, VisibleAnywhere, meta=(ShowOnlyInnerProperties))
	FWeakRegisteredLuaNetObjectInfo ScriptOwnerInfo;

	UFUNCTION()
	void OnRep_ScriptOwner();
	
	UPROPERTY(Replicated, VisibleAnywhere, meta=(ShowOnlyInnerProperties))
	FLuaValueReplicator Replicator;

	virtual bool IsSupportedForNetworking() const override
	{
		return true;
	}
};
