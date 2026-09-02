// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaContext/LuaScripts/LuaScriptInstanceHandle.h"

#include "Engine/World.h"
#include "LuaContext/ScopedLuaContext.h"
#include "Replication/LuaScriptReplicationComponent.h"
#include "ScriptableUObject/ReplicatedLuaUObject.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"

/*
bool FWeakLuaScriptInstanceHandle::ProcessEvent(FLuaOverrideCallParams& params) const
{
	return UnrealLua::LuaScriptCall::CallUFunctionOverride(params);	
}
*/

FLuaScriptInstanceHandle::FLuaScriptInstanceHandle()
	: LuaScriptCollection(nullptr)
{}

FLuaScriptInstanceHandle::~FLuaScriptInstanceHandle()
{
	this->Reset();
}

/*
bool FLuaScriptInstanceHandle::ProcessEvent(FLuaOverrideCallParams& params) const
{
	return UnrealLua::LuaScriptCall::CallUFunctionOverride(params);
}
*/

void FLuaScriptInstanceHandle::Reset()
{
	if (this->LuaScriptReloadHandle.IsValid())
	{
		ULoadedLuaScriptCollection* coll = this->GetLuaScriptCollection();
		if (coll)
		{
			coll->OnScriptReload.Remove(this->LuaScriptReloadHandle);
		}		
	}
	this->LuaScriptCollection.Reset();
	this->Owner.Reset();
}

bool FLuaScriptInstanceHandle::IsValid() const
{
	return this->LuaScriptCollection.IsValid();
}

UObject* FLuaScriptInstanceHandle::GetScriptOwner() const
{
	return this->Owner.Get();
}

void FLuaScriptInstanceHandle::SetOwner(UObject* Object)
{
	this->Owner = Object;
}

void FLuaScriptInstanceHandle::NotifyLuaScriptReload(ELuaScriptReloadStage luaScriptReloadStage, FLuaScriptReloadCache* luaScriptReloadCache)
{
	UObject* obj = this->GetScriptOwner();
	if (!obj)
	{
		this->Reset();
		return;
	}
	FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(obj);
	if (luaScriptReloadStage == ELuaScriptReloadStage::PRERELOAD)
	{
		item.NotifyPreLuaScriptCollectionReload(luaScriptReloadCache);	
	}
	else if (luaScriptReloadStage == ELuaScriptReloadStage::CLEARVALUES)
	{
		//set all script values to nil, but keep listeners
		item.RemoveLuaScript(true);
	}
	else if (luaScriptReloadStage == ELuaScriptReloadStage::RELOADSCRIPT)
	{
		item.NotifyPostLuaScriptCollectionReload(luaScriptReloadCache);
	}
}

FLuaScriptInstanceHandle::FLuaScriptInstanceHandle(const FLuaScriptInstanceHandle& other)
	: LuaScriptCollection((other.LuaScriptCollection))
{
}

FLuaScriptInstanceHandle::FLuaScriptInstanceHandle(FLuaScriptInstanceHandle&& instance) noexcept
	: LuaScriptCollection(instance.LuaScriptCollection)
{
	instance.Reset();
}

FLuaScriptInstanceHandle::FLuaScriptInstanceHandle(ULoadedLuaScriptCollection* coll) noexcept
	: LuaScriptCollection(coll)
{
	
}

FLuaRepLayout* FLuaScriptInstanceHandle::GetRepLayout() const
{
	ULoadedLuaScriptCollection* coll = GetLuaScriptCollection();
	if(coll)
	{
		return coll->GetRepLayout();
	}
	return nullptr;
}

bool FLuaScriptInstanceHandle::CanReplicate() const
{
	return this->IsValid() && this->GetLuaScriptCollection()->HasReplicatedProperties();
}

sol::state_view FLuaScriptInstanceHandle::GetLuaStateView() const
{
	return this->IsValid() ? this->GetLuaScriptCollection()->OwningContext->GetLuaState() : nullptr;
}

sol::this_state FLuaScriptInstanceHandle::GetLuaThisState() const
{
	return this->GetLuaScriptCollection()->OwningContext->GetLuaThisState();
}

ULoadedLuaScriptCollection* FLuaScriptInstanceHandle::GetLuaScriptCollection() const
{
	return this->LuaScriptCollection.Get();
}

TMap<FString, FString> FLuaScriptInstanceHandle::LuaScriptToString() const
{
	checkNoEntry();
	return {};
}

void FLuaScriptInstanceHandle::InitializeLuaReplication() const
{
	ULoadedLuaScriptCollection* coll = this->GetLuaScriptCollection();
	if (!coll)
	{
		return;
	}
	UObject* obj = this->GetScriptOwner();
	if (!obj)
	{
		return;
	}
	verify(this->GetLuaScriptCollection() != nullptr);
	FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(obj);
	
	FLuaNetHandle netHandle{};
	if (obj->Implements<ULuaScriptable>())
	{
		netHandle = ILuaScriptable::Execute_GetUniqueLuaNetHandle(obj, 123);
	}
	item.NetHandle = netHandle;
	
	UWorld* world = obj->GetWorld();
	
	if(!world || world->GetNetMode() == ENetMode::NM_Standalone)
	{
		return;
	}

	if(coll->AutoRegisterReplicatedSubobject() && (obj->IsNameStableForNetworking() || obj->IsSupportedForNetworking()))
    {
    	if(!obj->IsA<AActor>() && !obj->IsA<UActorComponent>())
    	{
    		UObject* objectOuter = obj->GetOuter();
    		if(AActor* outerActor = Cast<AActor>(objectOuter))
    		{
    			LUA_LOG("Auto-registering replicated subobject %s in outer %s", *GetFullNameSafe(obj), *GetFullNameSafe(objectOuter))
    			outerActor->AddReplicatedSubObject(obj, coll->GetReplicationCondition());
    			item.bIsRegisteredInOuterForReplication = true;
    		}
    		else if(UActorComponent* outerCmp = Cast<UActorComponent>(objectOuter))
    		{
    			LUA_LOG("Auto-registering replicated subobject %s in outer %s", *GetFullNameSafe(obj), *GetFullNameSafe(objectOuter))
    			outerCmp->AddReplicatedSubObject(obj, coll->GetReplicationCondition());
    			item.bIsRegisteredInOuterForReplication = true;
    		}			
    	}
    }

	FLuaRepLayout* replayout = coll->GetRepLayout();
	if(!replayout && !netHandle.IsValid())
	{
		//Has no rep layout nor any netHandle, so don't bother registering anything Lua-replication-based
		return;
	}

	AActor* repActor = nullptr;

	bool registerInReplicationComponent = false;

	if(AActor* actor = Cast<AActor>(obj))
	{
		repActor = actor;
		registerInReplicationComponent = true;
	}
	else if(UActorComponent* cmp = Cast<UActorComponent>(obj))
	{
		repActor = cmp->GetOwner();
		registerInReplicationComponent = true;
	}
	else if(obj->IsA<UReplicatedLuaUObject>())
	{
		//UReplicatedUObjects will replicate their own FFastArraySerializer
		//When their owning actor's ULuaScriptReplicationComponent
		//Calls PreReplication
		repActor = obj->GetTypedOuter<AActor>();
		registerInReplicationComponent = true;	
	}
	else if(obj->IsFullNameStableForNetworking())
	{
		repActor = obj->GetTypedOuter<AActor>();
		registerInReplicationComponent = true;		
	}
	else if(obj->IsSupportedForNetworking())
	{
		repActor = obj->GetTypedOuter<AActor>();
		registerInReplicationComponent = true;
	}
	else if(netHandle.IsValid())
	{
		repActor = obj->GetTypedOuter<AActor>();
		registerInReplicationComponent = true;
	}

	if(!repActor)
	{
		return;
	}
	
	ULuaScriptReplicationComponent* repcmp = repActor->FindComponentByClass<ULuaScriptReplicationComponent>();
	if(!repcmp)
	{
		repcmp = Cast<ULuaScriptReplicationComponent>(repActor->AddComponentByClass(ULuaScriptReplicationComponent::StaticClass(), false, FTransform::Identity, false));
		verify(repcmp->GetIsReplicated())
		repActor->AddInstanceComponent(repcmp);
	}
	//this->ReplicationComponent = repcmp;
	this->InitRepLayout(item, *replayout);
	item.SetNetDirty();
	if(registerInReplicationComponent)
	{
		repcmp->RegisterLuaScriptableObjectForReplication(item);	
	}
}

const FDelegateHandle& FLuaScriptInstanceHandle::GetLuaScriptReloadDelegateHandle() const
{
	return this->LuaScriptReloadHandle;
}

void FLuaScriptInstanceHandle::SetLuaScriptReloadDelegateHandle(const FDelegateHandle newHandle)
{
	this->LuaScriptReloadHandle = newHandle;
}

void FLuaScriptInstanceHandle::InitRepLayout(FLuaUObjectItem& item, const FLuaRepLayout& repLayout) const
{
	UObject* obj = item.GetUObject();
	
	// Auto-bind OnRep-functions
	for(const FUnrealLuaRepLayoutProperty& rep : repLayout.RepLayoutProperties)
	{
		if(rep.SubObject == NAME_None)
		{
			auto propertyNameStr = StringCast<char>(*rep.Property.ToString());
			FLuaScriptValue* val = item.GetLuaScriptValueOrCreateEmpty(propertyNameStr.Get(), true);
			//mark it as net property so it doesn't get removed even if nil and no subscribers
			val->SetIsNetProperty();
			
			if(rep.OnRep.IsEmpty())
			{
				continue;
			}
			//only use listeners for properties that are in the scriptowners script self

			FName funcName = FName{*rep.OnRep,EFindName::FNAME_Find};
			UFunction* onRepFunc = obj->FindFunction(funcName);
			if(onRepFunc)
			{
				FOnLuaScriptValueChangedDelegate del;
				del.BindUFunction(obj, funcName);
				val->AddOnValueChangedDelegate(del);		
			}
			else
			{
				auto onRepFuncStr = StringCast<char>(*rep.OnRep);
				val->AddOnValueChangedLuaScriptListener(obj, onRepFuncStr.Get());
			}	
		}
		else
		{
			//Named subobjects will not have a scriptproperty assigned to the script owner,
			//because of the possibility of ScriptOwner and Subobject having conflicting 
			//Property/ScriptValue names
			
			//during replication, if it's a subobject UProperty, aka not a property of
			//this scriptowners script/UProerties, we will use a direct call to
			//UnrealLua::LuaScriptCall::CallLuaFunctionSafe in this scriptowners script.
			//This avoids empty or name-conflicting entries in the script values
		}
	}
}

FLuaScriptInstanceHandle& FLuaScriptInstanceHandle::Invalid()
{
	static FLuaScriptInstanceHandle dummy = {};
	dummy.Reset();
	return dummy;
}

FLuaScriptInstanceHandle& FLuaScriptInstanceHandle::operator=(const FLuaScriptInstanceHandle& other)
{
	this->LuaScriptCollection = other.LuaScriptCollection;
	return *this;
}

FLuaScriptInstanceHandle& FLuaScriptInstanceHandle::operator=(FLuaScriptInstanceHandle&& other) noexcept
{
	this->LuaScriptCollection = other.LuaScriptCollection;
	other.Reset();
	return *this;
}