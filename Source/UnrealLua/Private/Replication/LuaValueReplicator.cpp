// Fill out your copyright notice in the Description page of Project Settings.

#include "Replication/LuaValueReplicator.h"

#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "UnrealLua.h"
#include "sol/sol.hpp"
#include "Config/UnrealLuaConfig.h"
#include "Async/ParallelFor.h"
#include "Engine/World.h"
#include "BlueprintSupport/UnrealLuaUtility.h"
#include "LuaContext/LuaScripts/LoadedLuaScriptCollection.h"
#include "Replication/LuaObjectReplicator.h"
//#include "Runtime/Engine/Private/Net/NetSubObjectRegistryGetter.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"
#include "Utility/UnrealLuaHash.h"

FLuaObjectValueReplicator::FLuaObjectValueReplicator(): OuterReplicator(nullptr)
{
}

UObject* FLuaObjectValueReplicator::GetReplicatorScriptOwner()
{
	if (ULuaObjectReplicator* replicator = Cast<ULuaObjectReplicator>(this->OuterReplicator))
	{
		return replicator->GetReplicatorScriptOwner();
	}
	return this->OuterReplicator->GetReplicatorScriptOwner();
}

void FLuaObjectValueReplicator::PreReplication()
{
	UObject* scriptOwner = this->GetReplicatorScriptOwner();
	if(!scriptOwner)
	{
		return;
	}
	UWorld* world = scriptOwner->GetWorld();
	if(!world)
	{
		return;
	}
	if(world->GetNetMode() == ENetMode::NM_Client)
	{
		LUA_LOG_WARNING("FLuaValueReplicator::PreReplication() running on a client. This shouldn't happen. Skipping...")
		return;
	}
	double currentServerTime = world->GetRealTimeSeconds();

	//Actually process values now. Every (sub)object should be valid

	this->ServerProcessValues(currentServerTime);
}

bool FLuaObjectValueReplicator::ServerProcessValues(const double currentServerTime)
{
	UObject* scriptOwner = this->GetReplicatorScriptOwner();
	
	if(!scriptOwner)
	{
		return false;
	}
	if(UActorComponent* cmp = Cast<UActorComponent>(scriptOwner))
	{
		if(!cmp->GetIsReplicated())
		{
			//Component not replicated -> don't bother
			return false;
		}
	}
	FLuaScriptInstanceHandle& scriptHandle = UnrealLua::UObjectRegistry::GetLuaScriptHandle(scriptOwner);
	FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(scriptOwner);

	FLuaRepLayout* repLayout = scriptHandle.GetRepLayout();
	if(!repLayout)
	{
		return false;
	}	

	bool bCheckSubobjects = false;
	if(currentServerTime >= this->NextSubobjectReplicationTime)
	{
		//Enough time has passed for Subobject replication
		bCheckSubobjects = true;
		this->NextSubobjectReplicationTime = currentServerTime + repLayout->ReplicationFrequency;
	}
	sol::state_view lua = scriptHandle.GetLuaStateView();

	bool useMultithreadedReplication = UUnrealLuaConfig::IsMultithreadReplicationEnabled();
	
	UClass* scriptOwnerClass = scriptOwner->GetClass();

	//Go over all replicated object names and try to find the UObjects to replicate
	for(int32 objindex = 0; objindex < repLayout->ObjectReplayouts.Num(); objindex++)
	{
		const FUnrealLuaObjectRepLayout& objectRepLayout = repLayout->ObjectReplayouts[objindex];

		//SubObject == NAME_None is the ScriptOwner currently being examined 
		if(objectRepLayout.SubObjectPropertyName == NAME_None)
		{
			if(!item.IsNetDirty())
			{
				//@TODO : Critical!
				//What if a FPropertyWrapperValue got changed in C++/Blueprint?
				//in that case the item would not have been marked NetDirty
				continue;
			}
			for(int32 propIndex = 0; propIndex < objectRepLayout.ReplicatedProperties.Num(); propIndex++)
			{
				const FUnrealLuaRepLayoutProperty& replicatedProp = *objectRepLayout.ReplicatedProperties[propIndex];
				//No Subobject name given:
				//Replicated Property can be either a UProperty of the Script owning UObject or it's a LuaScript value 

				//Net wrappers for FProperties in the Rep Layout should already have been created during
				//FLuaScriptInstance::InitRepLayout -> GetLuaScriptValueOrCreateEmpty
				//so there is no need to look up the property directly, just access the Lua script
				//value to get the wrapper

				//FLuaScriptValue* val = item.GetLuaScriptValue(*replicatedProp.Property.ToString());
				FLuaScriptValue* val = item.GetLuaScriptValue(*replicatedProp.StringKey);
				if(val)
				{
					verify(val->IsNetProperty());
					if(val->IsType<FPropertyReferenceWrapper>())
					{
						//need to process it without dirty, since this value might have changed
						//via Blueprint/C++, in which case no dirty bit is set
						this->ServerProcessValue(val->GetLuaValue(), &replicatedProp);
						val->ClearNetDirty();
						continue;
					}
					if(!val->IsNetDirty())
					{
						continue;
					}
					val->ClearNetDirty();
					if(val->GetLuaValue().CanBeReplicated())
					{
						this->ServerProcessValue(val->GetLuaValue(), &replicatedProp);	
					}
					else
					{
						//can't be replicated -> nil
						this->ServerProcessValue(nullptr, &replicatedProp);
					}
				}
				else
				{
					//no valid value -> is nil
					FLuaValue currentScriptValue{nullptr};
					this->ServerProcessValue(currentScriptValue, &replicatedProp);
				}
			}
		}
		//SubObject != NAME_None can be any UObject FProperty in the ScriptOwner
		else
		{
			if(!bCheckSubobjects)
			{
				continue;
			}
			FName subObjectPropertyName = objectRepLayout.SubObjectPropertyName;
			FProperty* propContainingSubobject = scriptOwnerClass->FindPropertyByName(subObjectPropertyName);
			if(propContainingSubobject)
			{
				if(FObjectProperty* objectPropContainingSubobject = CastField<FObjectProperty>(propContainingSubobject))
				{
					//Found the subobject property
					
					UObject* subObj = objectPropContainingSubobject->GetObjectPropertyValue_InContainer(scriptOwner);
					if(IsValid(subObj))
					{
						//We have a valid subobject of the Lua Script-owning UObject
						
						//Try to get a UnrealLua representation of the subobject, if one exists
						//We avoid creating an entry just for replication, so maybeItem might fail if
						//this subobject has not been used in Lua yet. In that case we will just get the 
						//property value directly further down below
						FLuaUObjectItem* maybeItem = UnrealLua::UObjectRegistry::TryGetUObjectItem(subObj);

						//examine each replicated property of that subobject
						for(int32 propIndex = 0; propIndex < objectRepLayout.ReplicatedProperties.Num(); ++propIndex)
						{
							const FUnrealLuaRepLayoutProperty& replicatedProp = *objectRepLayout.ReplicatedProperties[propIndex];

							if(maybeItem)
							{
								//Item is already known by UnrealLua, so any FProperty or Lua script value
								//should be reachable via GetLuaScriptValue
								FLuaScriptValue* val = maybeItem->GetLuaScriptValue(*replicatedProp.StringKey);
								if(val)
								{
									this->ServerProcessValue(val->GetLuaValue(), &replicatedProp);
								}
								else
								{
									this->ServerProcessValue(nullptr, &replicatedProp);
								}
								continue;
							}
							else
							{
								//subobject not known by UnrealLua yet
								//->Fall back to looking up the FProperty value directly
								FProperty* subObjectPropToReplicate = subObj->GetClass()->FindPropertyByName(replicatedProp.Property);
								if(subObjectPropToReplicate)
								{
									//found a property to replicate
									this->ServerProcessValue({subObj, subObjectPropToReplicate}, &replicatedProp);
								}
								else
								{
									//no valid Property found in subobjects class-> ignore
									//A UClass-FProperty layout shouldn't change during game, so no need to add or remove items
								}	
							}
						}
					}
					else
					{
						//subobject no longer valid -> Remove all entries for that subobj FObject property
						LUA_LOG("Replicated subobject %s no longer valid, removing all replicated items", *objectRepLayout.SubObjectPropertyName.ToString())
						for(int32 propIndex = 0; propIndex < objectRepLayout.ReplicatedProperties.Num(); ++propIndex)
						{
							const FUnrealLuaRepLayoutProperty& replicatedProp = *objectRepLayout.ReplicatedProperties[propIndex];
							if(replicatedProp.SubObject == subObjectPropertyName)
							{
								FLuaValue currentScriptValue{nullptr};
								this->ServerProcessValue(currentScriptValue, &replicatedProp);								
							}
						}				
					}
				}
				else
				{
					//no valid prop found -> ignore
					//A UClass-FProperty layout shouldn't change during game, so no need to add or remove items
				}
			}
		}
	}
	item.ClearNetDirty();
	return true;
}

void FLuaObjectValueReplicator::ServerProcessValue(const FLuaValue& currentScriptValue, const FUnrealLuaRepLayoutProperty* const repProp)
{
		//Case : item nil / removed from script -> remove item from replicated values

	if(currentScriptValue.IsNil() || !currentScriptValue.CanBeReplicated())
	{
		for(int i = 0; i < this->Items.Num(); i++)
		{
			FNetSerializedLuaValue& entry = this->Items[i];
			if(entry.RepLayoutPropertyIndex == repProp->RepLayoutPropertyIndex)
			{
				//LUA_LOG("Server found replicated property to remove : %s %s %s", *GetNameSafe(scriptOwner), *repProp->SubObject.ToString(), *repProp->Property.ToString())
				this->Items.RemoveAt(i);
				this->MarkArrayDirty();
				return;
			}
		}
	}
	else
	{
		//we have a valid (non-nil) value in current script

		//Either update existing value or add it
		for(int itemIndex = 0; itemIndex < this->Items.Num(); ++itemIndex)
		{
			FNetSerializedLuaValue& currentReplicatedValue = this->Items[itemIndex];
			if(currentReplicatedValue.RepLayoutPropertyIndex == repProp->RepLayoutPropertyIndex)
			{
				//found existing replicated value
				if(currentReplicatedValue.LuaValue.IsNil() || !currentScriptValue.Equals(currentReplicatedValue.LuaValue))
				{
					//values are different -> update!
					currentReplicatedValue.LuaValue = currentScriptValue.MakeCopy(true, true);
					if(currentReplicatedValue.LuaValue.IsNil())
					{
						this->Items.RemoveAt(itemIndex);
						this->MarkArrayDirty();
					}
					else
					{
						verify(!currentReplicatedValue.LuaValue.IsNil());
						this->MarkItemDirty(currentReplicatedValue);		
					}
				}
				return;
			}
		}
		//Add new item
		FNetSerializedLuaValue& newValue = this->Items.Add_GetRef(FNetSerializedLuaValue{repProp->RepLayoutPropertyIndex});
		newValue.LuaValue = currentScriptValue.MakeCopy(true, true);
		//LUA_LOG("Server adding new replicated property : %s %s %s of index type %d", *GetNameSafe(scriptOwner), *repProp->SubObject.ToString(), *repProp->Property.ToString(), newValue.LuaValue.Data.Data.GetIndex())
		verify(!newValue.LuaValue.IsNil());
		this->MarkItemDirty(newValue);
	}
}


void FLuaObjectValueReplicator::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
	this->ChangedValues.Reserve(this->ChangedValues.Num() + RemovedIndices.Num());
	for(const int32 index : RemovedIndices)
	{
		FNetSerializedLuaValue& item = Items[index];

		UnrealLua::HashUtility::PrintLuaValue(sol::nil, "Client will remove value ");
		
		this->ChangedValues.Emplace(ELuaValueChangeOP::REMOVE, item.RepLayoutPropertyIndex);
	}	
}

void FLuaObjectValueReplicator::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	this->ChangedValues.Reserve(this->ChangedValues.Num() + AddedIndices.Num());
	for(const int32 index : AddedIndices)
	{
		FNetSerializedLuaValue& item = Items[index];
		this->ChangedValues.Emplace(ELuaValueChangeOP::ADD, item.RepLayoutPropertyIndex);
	}	
}

void FLuaObjectValueReplicator::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
	this->ChangedValues.Reserve(this->ChangedValues.Num() + ChangedIndices.Num());
	for(const int32 index : ChangedIndices)
    {	
        FNetSerializedLuaValue& item = Items[index];
    	this->ChangedValues.Emplace(ELuaValueChangeOP::CHANGE, item.RepLayoutPropertyIndex);
    }	
}

void FLuaObjectValueReplicator::ClientProcessChangedValues()
{
	if(this->ChangedValues.IsEmpty())
	{
		return;
	}

	TArray<FChangedNetLuaValueOp> changedValues = MoveTemp(this->ChangedValues);
	
	verify(this->ChangedValues.IsEmpty());

	TSet<UObject*> subobjsToProcess{};

	for(FChangedNetLuaValueOp& changedValueOp : changedValues)
	{
		//link up serialized value with op
		FNetSerializedLuaValue* val = this->Items.FindByPredicate([&changedValueOp](const FNetSerializedLuaValue& item)
		{
			return item.RepLayoutPropertyIndex == changedValueOp.RepIndex;
		});
		if(val)
		{
			changedValueOp.ReplicatedLuaValue = val;
		}
	}

	UObject* scriptOwner = this->GetReplicatorScriptOwner();
	if(!IsValid(scriptOwner))
	{
		//may be ok, since main scriptobject will have NAME_None in its target UObject property
		return;
	}
	FLuaScriptInstanceHandle handle = UnrealLua::UObjectRegistry::GetLuaScriptHandle(scriptOwner);

	FLuaRepLayout* repLayout = handle.GetRepLayout();
	
	if(!repLayout)
	{
		//Usually only items with a Replayout should be able to register themselves here
		
		//@TODO : What if no rep layout found? For now, lets just ignore it and let the client have
		//useless values hanging around in the Replicator, as long as they don't enter the actual LuaScript
		//space they won't do any harm. Once an appropriate UObject registers, it will take the replicated values
		return;
	}

	UClass* scriptOwnerClass = scriptOwner->GetClass();

	FLuaUObjectItem& scriptOwnerItem = UnrealLua::UObjectRegistry::GetUObjectItem(scriptOwner);

	//Process changed values
	for(FChangedNetLuaValueOp& changedValue : changedValues)
	{
		FUnrealLuaRepLayoutProperty* foundRepProp = repLayout->GetRepPropertyForRepIndex(changedValue.RepIndex);

		if(foundRepProp)
		{
			changedValue.foundRepProp = foundRepProp;
			FName subObjPropertyName = foundRepProp->SubObject;
			
			if(subObjPropertyName == NAME_None)
			{
				this->UpdateScriptOwnerValueInternal(changedValue, foundRepProp, scriptOwnerItem);
			}
			else
			{
				FProperty* prop = scriptOwnerClass->FindPropertyByName(subObjPropertyName);
				if(FObjectProperty* objProp = CastField<FObjectProperty>(prop))
				{
					UObject* subObj = objProp->GetObjectPropertyValue_InContainer(scriptOwner);
					if(IsValid(subObj))
					{
						this->UpdateSubobjectPropertyValueInternal(changedValue, foundRepProp, subObj);
					}
				}
			}
		}
		else
		{
			//@TODO : What if no rep layout property found? For now, lets just ignore it and let the client have
			//useless values hanging around in the Replicator, as long as they don't enter the actual LuaScript
			//space they won't harm
		}
	}

	this->CallRepNotifies(changedValues);
}

void FLuaObjectValueReplicator::InitialReplication()
{
	for(FNetSerializedLuaValue& item : Items)
	{
		this->ChangedValues.Emplace(ELuaValueChangeOP::ADD, item.RepLayoutPropertyIndex);
	}
	this->ClientProcessChangedValues();
}

void FLuaObjectValueReplicator::UpdateScriptOwnerValueInternal(const FChangedNetLuaValueOp& changedValue, FUnrealLuaRepLayoutProperty* foundRepProp, FLuaUObjectItem& targetObject)
{
	if(changedValue.Op == ELuaValueChangeOP::CHANGE)
	{
		LUA_LOG_WARNING("Changing replicated %s for object %s", *foundRepProp->Property.ToString(), *GetNameSafe(targetObject.GetUObject()))
		verify(changedValue.ReplicatedLuaValue != nullptr);
		targetObject.SetScriptValue(foundRepProp->Property, changedValue.ReplicatedLuaValue->LuaValue, false);
	}
	else if(changedValue.Op == ELuaValueChangeOP::ADD)
	{
		LUA_LOG_WARNING("Adding replicated %s for object %s", *foundRepProp->Property.ToString(), *GetNameSafe(targetObject.GetUObject()))
		verify(changedValue.ReplicatedLuaValue != nullptr);
		targetObject.SetScriptValue(foundRepProp->Property, changedValue.ReplicatedLuaValue->LuaValue, false);
	}
	else if(changedValue.Op == ELuaValueChangeOP::REMOVE)
	{
		LUA_LOG_WARNING("Removing replicated %s for object %s", *foundRepProp->Property.ToString(), *GetNameSafe(targetObject.GetUObject()))
		verify(changedValue.ReplicatedLuaValue == nullptr);
		targetObject.SetScriptValue(foundRepProp->Property, FLuaValue{nullptr}, false);
	}
}

void FLuaObjectValueReplicator::UpdateSubobjectPropertyValueInternal(FChangedNetLuaValueOp& changedValue, FUnrealLuaRepLayoutProperty* foundRepProp, UObject* targetSubobject)
{
	FProperty* prop = targetSubobject->GetClass()->FindPropertyByName(foundRepProp->Property);
	if(prop)
	{
		LUA_LOG_WARNING("Replicating Property %s in subobject %s", *foundRepProp->Property.ToString(), *GetNameSafe(targetSubobject))

		//update value in subobject
		if(changedValue.ReplicatedLuaValue != nullptr)
		{
			//TSetPropertyValueParams params{prop, targetSubobject, 0, changedValue.ReplicatedLuaValue->LuaValue.GetValue(lua)};

			changedValue.ReplicatedLuaValue->LuaValue.WriteValueToPropertyMemoryAddress_WithPropertyTypeCheck(prop, prop->ContainerPtrToValuePtr<void>(targetSubobject));
		}
		else
		{
			//item got removed 
			sol::object nil{sol::nil};
			TSetPropertyValueParams params{prop, targetSubobject, 0, nil};
			UnrealLua::PropertyHelper::SetPropertyValue_InContainer(params);
		}
	}
	else
	{
		//not a valid property in target object
		LUA_LOG_WARNING("Can't replicate Property %s in subobject %s, prop not valid", *foundRepProp->Property.ToString(), *GetNameSafe(targetSubobject))
	}
}


void FLuaObjectValueReplicator::CallRepNotifies(TArray<FChangedNetLuaValueOp>& changedValues)
{
	UObject* owner = this->GetReplicatorScriptOwner();
	if (!owner)
	{
		return;
	}
	TArray<FLuaUObjectItem*> changedItems{};
	LUA_LOG_WARNING("Attempting to call OnReps for %s ", *GetNameSafe(owner))
	for(FChangedNetLuaValueOp& changedValue : changedValues)
	{
		if(!changedValue.foundRepProp)
		{
			continue;
		}
		if(!IsValid(owner))
		{
			continue;
		}

		FUnrealLuaRepLayoutProperty* repProp = changedValue.foundRepProp;
		FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(owner);
		changedItems.AddUnique(&item);
		if(repProp->SubObject == NAME_None)
		{
			LUA_LOG_WARNING("Attempting to call OnRep %s for %s ", *repProp->OnRep, *repProp->Property.ToString())

			
			item.BroadcastValue(*changedValue.foundRepProp->Property.ToString());
		}
		else
		{	
			LUA_LOG_WARNING("Attempting to call OnRep %s for Subobject %s::%s ", *repProp->OnRep, *repProp->SubObject.ToString(), *repProp->Property.ToString())

			sol::function repFunc = item.GetLuaScriptFunction(repProp->OnRep);
			if(repFunc.valid())
			{
				if(changedValue.ReplicatedLuaValue != nullptr)
				{
					if(repProp->PassKeyOnRep)
					{
						UnrealLua::LuaScriptCall::CallLuaFunctionSafe(repFunc, owner, changedValue.foundRepProp->Property, changedValue.ReplicatedLuaValue->LuaValue);	
					}
					else
					{
						UnrealLua::LuaScriptCall::CallLuaFunctionSafe(repFunc, owner, changedValue.ReplicatedLuaValue->LuaValue);
					}
					
				}
				else
				{
					if(repProp->PassKeyOnRep)
					{
						UnrealLua::LuaScriptCall::CallLuaFunctionSafe(repFunc, owner, changedValue.foundRepProp->Property, sol::nil);
					}
					else
					{
						UnrealLua::LuaScriptCall::CallLuaFunctionSafe(repFunc, owner, sol::nil);
					}
				}
					
			}
		}
	}
}

void FLuaObjectValueReplicator::PostReplicatedReceive(const FPostReplicatedReceiveParameters& Parameters)
{
	this->ClientProcessChangedValues();
}

void FLuaObjectValueReplicator::ResetValues()
{
	this->Items.Empty();
	this->MarkArrayDirty();
	this->ChangedValues.Empty();
}