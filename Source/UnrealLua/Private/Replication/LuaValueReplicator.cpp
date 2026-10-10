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

#include <ranges>

#include "Replication/LuaScriptReplicationComponent.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"
#include "Utility/UnrealLuaHash.h"

FLuaValueReplicator::FLuaValueReplicator()
{
}

void FLuaValueReplicator::ServerAddReplicatedObject(const FWeakRegisteredLuaNetObjectInfo& repObjectInfo, const FLuaRepLayout& repLayout, ELifetimeCondition repCondition)
{
	//On server, create new value owner ID
	const int32 ownerID = this->GetNewOwnerID();
	
	//Add entry for rep layout owner first
	FNetSerializedLuaValue& ownerEntry = this->Items.Emplace_GetRef(ownerID, FLuaRepLayout::ReplayoutOwnerIndex);
	ownerEntry.LuaValue.Emplace<FRegisteredLuaNetObjectInfo>(FRegisteredLuaNetObjectInfo{repObjectInfo.RegisteredObject.Get(), repObjectInfo.LuaNetHandle});

	//All following entries are values belonging to that owner
	//Emplace empty entries for each replicated value
	for (const FUnrealLuaRepLayoutProperty& repProperty : repLayout.RepLayoutProperties)
	{
		verify(repProperty.RepLayoutPropertyIndex != FLuaRepLayout::ReplayoutOwnerIndex);
		if (repProperty.Condition == repCondition)
		{
			this->Items.Emplace(ownerID, repProperty.RepLayoutPropertyIndex);
		}
	}
}

void FLuaValueReplicator::ServerUnregisterObject(const FRegisteredLuaNetObjectInfo& repObjectInfo)
{
	FNetSerializedLuaValue* found = this->Items.FindByPredicate([&repObjectInfo](const FNetSerializedLuaValue& item)
	{
		return item.IsReplicatedObjectEntry() && item.LuaValue.Get<FRegisteredLuaNetObjectInfo>() == repObjectInfo;
	});
	if (found)
	{
		int32 valueOwnerID = found->ValueOwnerID;
		this->Items.RemoveAll([valueOwnerID](const FNetSerializedLuaValue& item)
		{
			return item.ValueOwnerID == valueOwnerID;
		});
		this->MarkArrayDirty();
	}
}

FNetSerializedLuaValue* FLuaValueReplicator::FindValueOwnerForOwnerID(int32 ownerID)
{
	return this->ValueOwners.FindByPredicate([ownerID](FNetSerializedLuaValue& item)
	{
		return ownerID == item.ValueOwnerID;
	});
}


void FLuaValueReplicator::InitialReplication()
{
	this->ChangedValues.Reserve(this->Items.Num());
	for(FNetSerializedLuaValue& item : this->Items)
	{
		//Sort new items either in value owners array or into changed values
		if (item.IsReplicatedObjectEntry())
		{
			this->ValueOwners.AddUnique(item);
		}
		else
		{
			this->ChangedValues.Emplace(item.ValueOwnerID, item.RepLayoutPropertyIndex, ELuaValueChangeOP::ADD);
		}
	}	
	//for(FNetSerializedLuaValue& item : Items)
	//{
	//	this->ChangedValues.Emplace(item.ValueOwnerID, item.RepLayoutPropertyIndex, ELuaValueChangeOP::ADD);
	//}
	this->ClientProcessChangedValues();
}

void FLuaValueReplicator::PreReplication()
{
	UObject* replicatorComponent = this->OwningObject;
	if(!replicatorComponent)
	{
		return;
	}
	UWorld* world = replicatorComponent->GetWorld();
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

bool FLuaValueReplicator::ServerProcessValues(const double currentServerTime)
{
	ULuaScriptReplicationComponent* cmp = Cast<ULuaScriptReplicationComponent>(this->OwningObject);
	
	if(!cmp->GetIsReplicated())
	{
		//Component not replicated -> don't bother
		return false;
	}
	
	if (this->Items.IsEmpty())
	{
		return true;
	}
	bool useMultithreadedReplication = UUnrealLuaConfig::IsMultithreadReplicationEnabled();
	
	FNetSerializedLuaValue* serializedLuaScriptOwnerEntry = nullptr;
	FLuaRepLayout* repLayout = nullptr;
	FLuaUObjectItem* uobjectItemPtr = nullptr;
	UObject* scriptOwner = nullptr;
	UClass* scriptOwnerClass = nullptr;
	bool bCheckSubobjects = false;

	//On the server, due to how FLuaValueReplicator::ServerAddReplicatedObject adds new objects,
	//the items are always properly ordered:
	//[owner A],[item A1],[item A2],[item A3],[owner B],[item B1],[item B2],[owner C],[item C1]...
	//So we can just do a normal loop throughout the entire array
	for (int32 index = 0; index < this->Items.Num(); ++index)
	{
		FNetSerializedLuaValue& currentItem = this->Items[index];
		
		if (currentItem.IsReplicatedObjectEntry())
		{
			//new script owner entrx -> get new context items
			serializedLuaScriptOwnerEntry = nullptr;
			uobjectItemPtr = nullptr;
			repLayout = nullptr;
			scriptOwner = nullptr;
			scriptOwnerClass = nullptr;
			bCheckSubobjects = false;
			
			UObject* scriptOwnerTemp = currentItem.ResolveUObject(cmp);
			if (!scriptOwnerTemp)
			{
				continue;
			}
			FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(scriptOwnerTemp);
			
			if (!item.IsNetDirty())
			{
				continue;
			}
			repLayout = item.ScriptHandle.GetRepLayout();
			if(!repLayout)
			{
				continue;
			}
			
			verify(repLayout->ReplicationFrequency >= 0.0f)
			
			if(currentServerTime >= item.ScriptHandle.GetNextSubobjectReplicationTime())
			{
				//Enough time has passed for Subobject replication
				bCheckSubobjects = true;
				item.GetLuaScriptHandle().SetNextSubobjectReplicationTime(currentServerTime + repLayout->ReplicationFrequency);
			}
			
			item.ClearNetDirty();
			scriptOwner = scriptOwnerTemp;
			uobjectItemPtr = &item;
			serializedLuaScriptOwnerEntry = &currentItem;
			scriptOwnerClass = scriptOwner->GetClass();
			continue;
		}
		else
		{
			if (serializedLuaScriptOwnerEntry == nullptr)
			{
				continue;
			}
			verify(serializedLuaScriptOwnerEntry != nullptr);
			verify(repLayout != nullptr);
			verify(uobjectItemPtr != nullptr);
			verify(scriptOwner != nullptr);
			verify(scriptOwnerClass != nullptr);
			verify(serializedLuaScriptOwnerEntry->ValueOwnerID == currentItem.ValueOwnerID)
			
			//we have a lua value item with an owner

			const FUnrealLuaRepLayoutProperty* repPropInfo = repLayout->GetRepPropertyForRepIndex(currentItem.RepLayoutPropertyIndex);
			
			if (repPropInfo)
			{
				//SubObject == NAME_None means checking Lua values / FProperties of the current script owner
				if (repPropInfo->SubObject == NAME_None)
				{
					FLuaScriptValue* val = uobjectItemPtr->GetLuaScriptValue(*repPropInfo->StringKey);
					if(val)
					{
						verify(val->IsNetProperty());
						if(val->IsType<FPropertyReferenceWrapper>())
						{
							//need to process it without dirty, since this value might have changed
							//via Blueprint/C++, in which case no dirty bit is set
							this->ServerProcessValue(val->GetLuaValue(), currentItem);
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
							this->ServerProcessValue(val->GetLuaValue(), currentItem);	
						}
						else
						{
							//can't be replicated -> nil
							this->ServerProcessValue(nullptr, currentItem);
						}
					}
					else
					{
						//no valid value -> is nil
						FLuaValue currentScriptValue{nullptr};
						this->ServerProcessValue(currentScriptValue, currentItem);
					}	
				}
				//SubObject != NAME_None means we are looking for an FObjectProperty in the ScriptOwner
				else
				{
					//But only if enough time has elapsed since the last check
					if(!bCheckSubobjects)
					{
						continue;
					}
					FName subObjectPropertyName = repPropInfo->Property;
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

								if(maybeItem)
								{
									//Item is already known by UnrealLua, so any FProperty or Lua script value
									//should be reachable via GetLuaScriptValue
									FLuaScriptValue* val = maybeItem->GetLuaScriptValue(*repPropInfo->StringKey);
									if(val)
									{
										this->ServerProcessValue(val->GetLuaValue(), currentItem);
									}
									else
									{
										this->ServerProcessValue(nullptr, currentItem);
									}
									continue;
								}
								else
								{
									//subobject not known by UnrealLua yet
									//->Fall back to looking up the FProperty value directly
									FProperty* subObjectPropToReplicate = subObj->GetClass()->FindPropertyByName(repPropInfo->Property);
									if(subObjectPropToReplicate)
									{
										//found a property to replicate
										this->ServerProcessValue({subObj, subObjectPropToReplicate}, currentItem);
									}
									else
									{
										//no valid Property found in subobjects class-> ignore
										//A UClass-FProperty layout shouldn't change during game, so no need to add or remove items
									}	
								}
							}
							else
							{
								//subobject no longer valid -> Remove all entries for that subobj FObject property
								LUA_LOG("Replicated subobject %s no longer valid, removing all replicated items", *repPropInfo->SubObject.ToString())
								FLuaValue currentScriptValue{nullptr};
								this->ServerProcessValue(currentScriptValue, currentItem);								
							}				
						}
						else
						{
							//Targeted Subobject property is not a FUObjectProperty. this is not supported! 
							LUA_LOG_ERROR("Replicated subobject property %s is not a FUObjectProperty! This is not supported. Assigning nil value.", *repPropInfo->SubObject.ToString())
							FLuaValue currentScriptValue{nullptr};
							this->ServerProcessValue(currentScriptValue, currentItem);								
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
	}
		/*
		
		if (currentItem.IsReplicatedObjectEntry())
		{
			UObject* scriptOwner = currentItem.GetLuaScriptOwner(cmp);
			
			FLuaScriptInstanceHandle& scriptHandle = UnrealLua::UObjectRegistry::GetLuaScriptHandle(scriptOwner);
			FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(scriptOwner);

			FLuaRepLayout* repLayout = scriptHandle.GetRepLayout();
			if(!repLayout)
			{
				continue;
			}	

			bool bCheckSubobjects = false;
			if(currentServerTime >= this->NextSubobjectReplicationTime)
			{
				//Enough time has passed for Subobject replication
				bCheckSubobjects = true;
				this->NextSubobjectReplicationTime = currentServerTime + repLayout->ReplicationFrequency;
			}
			sol::state_view lua = scriptHandle.GetLuaStateView();
			
			
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
		}
	}
	*/
	
	return true;
}
/*
void FLuaValueReplicator::ServerProcessValue(const FLuaValue& currentScriptValue, const FUnrealLuaRepLayoutProperty* const repProp)
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
*/

void FLuaValueReplicator::ServerProcessValue(const FLuaValue& currentScriptValue, FNetSerializedLuaValue& netValue)
{
	//Case : item nil / removed from script -> remove item from replicated values

	if(currentScriptValue.IsNil() || !currentScriptValue.CanBeReplicated())
	{
		netValue.LuaValue.Emplace<sol::nil_t>();
		this->MarkItemDirty(netValue);	
	}
	else
	{
		if(netValue.LuaValue.IsNil() || !currentScriptValue.Equals(netValue.LuaValue))
		{
			//values are different -> update!
			netValue.LuaValue = currentScriptValue.MakeCopy(true, true);
			if(netValue.LuaValue.IsNil())
			{
				netValue.LuaValue.Emplace<sol::nil_t>();
				this->MarkItemDirty(netValue);
			}
			else
			{
				verify(!netValue.LuaValue.IsNil());
				this->MarkItemDirty(netValue);		
			}
		}
	}
}


void FLuaValueReplicator::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
	this->ChangedValues.Reserve(this->ChangedValues.Num() + RemovedIndices.Num());
	for(const int32 index : RemovedIndices)
	{
		FNetSerializedLuaValue& item = Items[index];

		//UnrealLua::HashUtility::PrintLuaValue(sol::nil, "Client will remove value ");
		this->ChangedValues.Emplace(item.ValueOwnerID, item.RepLayoutPropertyIndex, ELuaValueChangeOP::REMOVE);
	}	
}

void FLuaValueReplicator::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	this->ChangedValues.Reserve(this->ChangedValues.Num() + AddedIndices.Num());
	for(const int32 index : AddedIndices)
	{
		//Sort new items either in value owners array or into changed values
		FNetSerializedLuaValue& item = Items[index];
		if (item.IsReplicatedObjectEntry())
		{
			this->ValueOwners.AddUnique(item);
		}
		else
		{
			this->ChangedValues.Emplace(item.ValueOwnerID, item.RepLayoutPropertyIndex, ELuaValueChangeOP::ADD);
		}
	}	
}

void FLuaValueReplicator::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
	this->ChangedValues.Reserve(this->ChangedValues.Num() + ChangedIndices.Num());
	for(const int32 index : ChangedIndices)
    {	
        FNetSerializedLuaValue& item = Items[index];
    	this->ChangedValues.Emplace(item.ValueOwnerID, item.RepLayoutPropertyIndex, ELuaValueChangeOP::CHANGE);
    }	
}


void FLuaValueReplicator::PostReplicatedReceive(const FPostReplicatedReceiveParameters& Parameters)
{
	this->ClientProcessChangedValues();
}

void FLuaValueReplicator::ClientProcessChangedValues()
{
	if(this->ChangedValues.IsEmpty())
	{
		return;
	}
	
	TArray<FChangedNetLuaValueOp> changedValueOps = MoveTemp(this->ChangedValues);
	verify(this->ChangedValues.IsEmpty());
	
	TMap<uint32, UObject*> ownerItemMap;
	
	//Get a list of valid value owners
	for (FNetSerializedLuaValue& item : this->ValueOwners)
	{
		UObject* owner = item.ResolveUObject(Cast<ULuaScriptReplicationComponent>(this->OwningObject));
		if (owner)
		{
			ownerItemMap.Emplace(item.ValueOwnerID, owner);
		}
	}

	TMap<UObject*, TArray<FChangedNetLuaValueOp*>> valueOwnerToChangedValuesMap;
	TArray<FNetSerializedLuaValue*> removedOwners;

	//link up serialized value with op and owner
	for(FChangedNetLuaValueOp& changedValueOp : changedValueOps)
	{
		//Find the serialized value in the items array
		FNetSerializedLuaValue* val = this->Items.FindByPredicate([&changedValueOp](const FNetSerializedLuaValue& item)
		{
			return item.RepLayoutPropertyIndex == changedValueOp.RepIndex && item.ValueOwnerID == changedValueOp.ValueOwnerID;
		});
		//if we found a value
		if(val)
		{
			//link it up
			changedValueOp.ReplicatedLuaValue = val;
			if (val->IsReplicatedObjectEntry())
			{
				//This is some kind of lua value owner
				//Keep track of owners that should get removed
				if (changedValueOp.Op == ELuaValueChangeOP::REMOVE)
				{
					removedOwners.Add(val);
				}
			}
			else
			{
				//This is a lua value belonging to some owner
				
				//Find owner
				UObject* owner = ownerItemMap.FindRef(changedValueOp.ValueOwnerID);
				if (owner)
				{
					TArray<FChangedNetLuaValueOp*>& entry = valueOwnerToChangedValuesMap.FindOrAdd(owner, {});
					entry.Add(&changedValueOp);		
				}
			}
		}			
	}
	
	
	//3. We now have a map of value owners and the chagned values -> Process values
	for (TTuple<UObject*, TArray<FChangedNetLuaValueOp*>>& ownerValuesPair : valueOwnerToChangedValuesMap)
	{
		UObject* valueOwner = ownerValuesPair.Key;
		if(!IsValid(valueOwner))
		{
			//may be ok, since main scriptobject will have NAME_None in its target UObject property
			return;
		}
		
		//Gather needed items for value processing
		UClass* scriptOwnerClass = valueOwner->GetClass();
		FLuaUObjectItem& scriptOwnerItem = UnrealLua::UObjectRegistry::GetUObjectItem(valueOwner);
		FLuaScriptInstanceHandle handle = scriptOwnerItem.GetLuaScriptHandle();
		FLuaRepLayout* repLayout = handle.GetRepLayout();
	
		if(!repLayout)
		{
			//Usually only items with a Replayout should be able to register themselves here
		
			//@TODO : What if no rep layout found? For now, lets just ignore it and let the client have
			//useless values hanging around in the Replicator, as long as they don't enter the actual LuaScript
			//space they won't do any harm. Once an appropriate UObject registers, it will take the replicated values
			return;
		}

		const TArray<FChangedNetLuaValueOp*>& changedValuesInThisOwner = ownerValuesPair.Value;
		
		//Process changed values
		for(FChangedNetLuaValueOp* changedValue : changedValuesInThisOwner)
		{
			FUnrealLuaRepLayoutProperty* foundRepProp = repLayout->GetRepPropertyForRepIndex(changedValue->RepIndex);

			if(foundRepProp)
			{
				changedValue->foundRepProp = foundRepProp;
				FName subObjPropertyName = foundRepProp->SubObject;

				//If subobj property name == NAME_None, treat it as a lua value in the owner
				if(subObjPropertyName == NAME_None)
				{
					this->UpdateScriptOwnerValueInternal(*changedValue, foundRepProp, scriptOwnerItem);
				}
				//Otherwise look for an FProperty with the same name which could have a subobject 
				else
				{
					FProperty* prop = scriptOwnerClass->FindPropertyByName(subObjPropertyName);
					if(FObjectProperty* objProp = CastField<FObjectProperty>(prop))
					{
						UObject* subObj = objProp->GetObjectPropertyValue_InContainer(valueOwner);
						if(IsValid(subObj))
						{
							this->UpdateSubobjectPropertyValueInternal(*changedValue, foundRepProp, subObj);
						}
					}
				}
			}
			else
			{
				//@TODO : What if no rep layout property found? For now, lets just ignore it and let the client have
				//useless values hanging around in the Replicator, as long as they don't enter the actual LuaScript
				//space they won't cause any harm
			}
		}
	}
	
	this->CallRepNotifies(valueOwnerToChangedValuesMap);
	
	for (const FNetSerializedLuaValue* removedowner : removedOwners)
	{
		this->ValueOwners.RemoveSingleSwap(*removedowner);
	}
}


void FLuaValueReplicator::UpdateScriptOwnerValueInternal(const FChangedNetLuaValueOp& changedValue, FUnrealLuaRepLayoutProperty* foundRepProp, FLuaUObjectItem& targetObject)
{
	if(changedValue.Op == ELuaValueChangeOP::CHANGE)
	{
		LUA_LOG("Changing replicated %s for object %s", *foundRepProp->Property.ToString(), *GetNameSafe(targetObject.GetUObject()))
		verify(changedValue.ReplicatedLuaValue != nullptr);
		targetObject.SetScriptValue(foundRepProp->Property, changedValue.ReplicatedLuaValue->LuaValue, false);
	}
	else if(changedValue.Op == ELuaValueChangeOP::ADD)
	{
		LUA_LOG("Adding replicated %s for object %s", *foundRepProp->Property.ToString(), *GetNameSafe(targetObject.GetUObject()))
		verify(changedValue.ReplicatedLuaValue != nullptr);
		targetObject.SetScriptValue(foundRepProp->Property, changedValue.ReplicatedLuaValue->LuaValue, false);
	}
	else if(changedValue.Op == ELuaValueChangeOP::REMOVE)
	{
		LUA_LOG("Removing replicated %s for object %s", *foundRepProp->Property.ToString(), *GetNameSafe(targetObject.GetUObject()))
		verify(changedValue.ReplicatedLuaValue == nullptr);
		targetObject.SetScriptValue(foundRepProp->Property, FLuaValue{nullptr}, false);
	}
}

void FLuaValueReplicator::UpdateSubobjectPropertyValueInternal(const  FChangedNetLuaValueOp& changedValue, FUnrealLuaRepLayoutProperty* foundRepProp, UObject* targetSubobject)
{
	FProperty* prop = targetSubobject->GetClass()->FindPropertyByName(foundRepProp->Property);
	if(prop)
	{
		LUA_LOG("Replicating Property %s in subobject %s", *foundRepProp->Property.ToString(), *GetNameSafe(targetSubobject))

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


void FLuaValueReplicator::CallRepNotifies(const TMap<UObject*, TArray<FChangedNetLuaValueOp*>>& valueOwnerToChangedValuesMap)
{
	for (const TTuple<UObject*, TArray<FChangedNetLuaValueOp*>>& ownerValuesPair : valueOwnerToChangedValuesMap)
	{
		UObject* valueOwner = ownerValuesPair.Key;
		if(!IsValid(valueOwner))
		{
			//may be ok, since main scriptobject will have NAME_None in its target UObject property
			return;
		}
		FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(valueOwner);
		TArray<FLuaUObjectItem*> changedItems{};
		LUA_LOG("Attempting to call OnReps for %s ", *GetNameSafe(valueOwner))
		for(const FChangedNetLuaValueOp* changedValuePtr : ownerValuesPair.Value)
		{
			const FChangedNetLuaValueOp& changedValue = *changedValuePtr;
			if(!changedValue.foundRepProp)
			{
				continue;
			}
			FUnrealLuaRepLayoutProperty* repProp = changedValue.foundRepProp;
			changedItems.AddUnique(&item);
			if(repProp->SubObject == NAME_None)
			{
				LUA_LOG("Attempting to call OnRep %s for %s ", *repProp->OnRep, *repProp->Property.ToString())
				item.BroadcastValue(*changedValue.foundRepProp->Property.ToString());
			}
			else
			{	
				LUA_LOG("Attempting to call OnRep %s for Subobject %s::%s ", *repProp->OnRep, *repProp->SubObject.ToString(), *repProp->Property.ToString())

				sol::function repFunc = item.GetLuaScriptFunction(repProp->OnRep);
				if(repFunc.valid())
				{
					if(changedValue.ReplicatedLuaValue != nullptr)
					{
						if(repProp->PassKeyOnRep)
						{
							UnrealLua::LuaScriptCall::CallLuaFunctionSafe(repFunc, valueOwner, changedValue.foundRepProp->Property, changedValue.ReplicatedLuaValue->LuaValue);	
						}
						else
						{
							UnrealLua::LuaScriptCall::CallLuaFunctionSafe(repFunc, valueOwner, changedValue.ReplicatedLuaValue->LuaValue);
						}
					
					}
					else
					{
						if(repProp->PassKeyOnRep)
						{
							UnrealLua::LuaScriptCall::CallLuaFunctionSafe(repFunc, valueOwner, changedValue.foundRepProp->Property, sol::nil);
						}
						else
						{
							UnrealLua::LuaScriptCall::CallLuaFunctionSafe(repFunc, valueOwner, sol::nil);
						}
					}
					
				}
			}
		}	
	}
}


void FLuaValueReplicator::ResetValues()
{
	this->Items.Empty();
	this->MarkArrayDirty();
	this->ChangedValues.Empty();
}

uint32 FLuaValueReplicator::GetNewOwnerID()
{
	++this->OwnerIDCounter;
	verify(this->OwnerIDCounter != 0);
	return this->OwnerIDCounter;
}
