// Fill out your copyright notice in the Description page of Project Settings.
#include "Replication/SerializedLuaValue.h"
#include "UnrealLua.h"
#include "Components/ActorComponent.h"
#include "Engine/PackageMapClient.h"
#include "GameFramework/Actor.h"
#include "UObject/UObjectThreadContext.h"

bool FNetSerializedLuaValue::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Ar << this->RepLayoutPropertyIndex;
	bOutSuccess = this->LuaValue.NetSerialize(Ar, Map, bOutSuccess);
	return bOutSuccess;
}

bool FSerializedLuaValue::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	UPackageMapClient* pc = Cast<UPackageMapClient>(Map);

	/*
	if(Ar.IsSaving())
	{
		ELifetimeCondition cond = this->ReplicationCondition;
		
		if(cond != COND_None)
		{
			ULuaObjectReplicator* replicator = Cast<ULuaObjectReplicator>(FUObjectThreadContext::Get().GetSerializeContext()->SerializedObject);
			ENetRole role = replicator->GetOwnerNetRole();
			if(cond == COND_SimulatedOnly && role != ROLE_SimulatedProxy)
			{
				
				return true;
			}
			else if(cond == COND_AutonomousOnly && role != ROLE_AutonomousProxy)
			{
				return true;
			}
			else if(cond == COND_OwnerOnly && role == ROLE_Authority)
			{
				return true;
			}	
		}	
	}
	*/
	
	if(!SerializeScriptOwner(Ar))
	{
		LUA_LOG_ERROR("Could not serialize script owner %s", *GetNameSafe(this->ScriptOwner))
		bOutSuccess = false;
		return false;
	}
	
	SerializeKey(Ar);

	//bOutSuccess = this->SerializeLuaValue(Ar, Map, bOutSuccess);
	
	bOutSuccess = this->LuaValue.NetSerialize(Ar, Map, bOutSuccess);
	return bOutSuccess;
}

bool FSerializedLuaValue::SerializeScriptOwner(FArchive& Ar)
{
	if(Ar.IsSaving())
	{
		verify(this->ScriptOwner != nullptr)

		//If this is already an actor, just serialize actor
		if(this->ScriptOwner->IsA<AActor>())
		{
			bool replicated = true;
			Ar.SerializeBits(&replicated, 1);
			Ar << this->ScriptOwner;	
		}
		//otherwise, must have an Actor as a chained outer
		else if(AActor* outerActor = this->ScriptOwner->GetTypedOuter<AActor>())
		{
			UObject* outer = this->ScriptOwner->GetOuter();
			if(outer == outerActor)
			{
				//UObject is directly in Actor
				if(this->ScriptOwner->IsNameStableForNetworking() || outerActor->IsReplicatedSubObjectRegistered(this->ScriptOwner))
				{
					bool replicated = true;
					Ar.SerializeBits(&replicated, 1);
					Ar << this->ScriptOwner;				
				}
				else
				{
					bool replicated = false;
					Ar.SerializeBits(&replicated, 1);
					FName name = this->ScriptOwner->GetFName(); 
					Ar << name;					
				}
			}
			else if(UActorComponent* cmp = Cast<UActorComponent>(this->ScriptOwner))
			{
				//only replicate as ref if it's registered
				if(this->ScriptOwner->IsNameStableForNetworking() || outerActor->IsReplicatedActorComponentRegistered(cmp))
				{
					bool replicated = true;
					Ar.SerializeBits(&replicated, 1);
					Ar << this->ScriptOwner;	
				}
				//otherwise just replicate the name
				else
				{
					bool replicated = false;
					Ar.SerializeBits(&replicated, 1);
					FName name = this->ScriptOwner->GetFName(); 
					Ar << name;					
				}
			}
			else if(UActorComponent* outerCmp = Cast<UActorComponent>(outer))
			{
				if(this->ScriptOwner->IsNameStableForNetworking() || outerCmp->IsReplicatedSubObjectRegistered(this->ScriptOwner))
				{
					bool replicated = true;
					Ar.SerializeBits(&replicated, 1);
					Ar << this->ScriptOwner;
				}
				else
				{
					bool replicated = false;
					Ar.SerializeBits(&replicated, 1);
					FName name = this->ScriptOwner->GetFName(); 
					Ar << name;					
				}
			}
			else
			{
				bool replicated = false;
				Ar.SerializeBits(&replicated, 1);
				FName name = this->ScriptOwner->GetFName(); 
				Ar << name;
			}
		}
		else
		{
			return false;
		}
	}
	else if(Ar.IsLoading())
	{
		bool objectReplicated = false;
		Ar.SerializeBits(&objectReplicated, 1);
		if(objectReplicated)
		{
			Ar << this->ScriptOwner;
		}
		else
		{
			FName subObjName;
			Ar << subObjName;
			AActor* outer = Cast<AActor>(FUObjectThreadContext::Get().GetSerializeContext()->SerializedObject);
			this->ScriptOwner = FindObjectFast<UObject>(outer, subObjName);
		}
	}
	return true;
}

void FSerializedLuaValue::SerializeKey(FArchive& Ar)
{
	if(Ar.IsSaving())
	{
		Ar << this->RepLayoutPropertyIndex;
	}
	else if(Ar.IsLoading())
	{
		Ar << this->RepLayoutPropertyIndex;
	}
}

bool FSerializedLuaValue::SerializeLuaValue(const FArchive& ar, UPackageMap* map, bool bOutSuccess)
{
	return true;
}
/*
sol::object FSerializedLuaValue::GetAsLuaValue()
{
	return this->LuaValue.GetValue();
}
*/
