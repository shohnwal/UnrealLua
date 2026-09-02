// Fill out your copyright notice in the Description page of Project Settings.


#include "UObjectRegistry/LuaUObjectItem.h"
#include <string>
#include "GameFramework/Actor.h"
#include "LuaCallHelpers/LuaScriptOverrideCalls.h"
#include "LuaContext/LuaScripts/LoadedLuaScriptCollection.h"
#include "Replication/LuaScriptReplicationComponent.h"
#include "Engine/World.h"
#include "Reflection/PropertyMapping.h"
#include "SubSystem/UnrealLuaGameWorldSubsystem.h"
#include "UObjectRegistry/LuaScriptDynamicDelegateHandler.h"
#include "UnrealLua.h"
#include "sol/sol.hpp"
#include "Config/UnrealLuaConfig.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Input/LuaUObjectInputOverrides.h"
#include "LuaValue/LuaScriptValue.h"
#include "LuaContext/ScopedLuaContext.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"

FLuaUObjectItemHandle::FLuaUObjectItemHandle(FLuaUObjectItem& item)
	: Item(&item)
{}

void FLuaUObjectItemHandle::Invalidate()
{
	this->Item = nullptr;
}

int FLuaUObjectItemHandle::__index(sol::stack_object key) const
{
	if (!this->IsValid())
	{
		return 0;
	}
	return this->Item->__index(key);
}

int FLuaUObjectItemHandle::__indexAsUEnum(sol::stack_object& key) const
{
	if (!this->IsValid())
	{
		return 0;
	}
	return this->Item->__indexAsUEnum(key);
}

void FLuaUObjectItemHandle::__newindex(sol::stack_object key, sol::stack_object value, lua_State* L) const
{
	if (!this->IsValid())
	{
		return;
	}
	this->Item->__newindex(key, value, L);
}

UObject* FLuaUObjectItemHandle::GetUObject() const
{
	if (!this->IsValid())
	{
		return nullptr;
	}
	return this->Item->Object;
}

FLuaUObjectItem* FLuaUObjectItemHandle::GetUObjctItem() const
{
	return this->IsValid() ? this->Item : nullptr;
}

bool FLuaUObjectItemHandle::IsValid() const
{
	return this->Item != nullptr;
}

int FLuaUObjectItemHandle::__tostring(lua_State* L)
{
	if (!this->IsValid())
	{
		const std::string invalid{"<Invalid>"};
		return sol::stack::push(L, invalid.c_str());		
	}
	UObject* obj = this->GetUObject();
	std::string str = StringCast<char>(*(GetNameSafe(obj))).Get();
	return sol::stack::push(L, str.c_str());
}

//Used by FScopedLuaContext::AddReferencedObjects
//This lets the reference collector know which UObjects are referenced by Lua 
void FLuaUObjectItemHandle::AddReferencedUObject(FReferenceCollector& Collector) const
{
	if (!this->IsValid())
	{
		return;
	}
	Collector.AddReferencedObject(this->Item->Object);
}

FLuaUObjectItem::FLuaUObjectItem()
{
	verify(this->VerifyIsClean());
}

FLuaUObjectItem::FLuaUObjectItem(const UObject* obj)
{
	verify(this->VerifyIsClean());
	
	this->Object = const_cast<UObject*>(obj);
	this->ObjectName = *GetFullNameSafe(obj);
	
	TUniquePtr<FLuaUObjectItemHandle>& handle = this->Handles.Add_GetRef(MakeUnique<FLuaUObjectItemHandle>(*this));
	this->CurrentHandle = handle.Get();
	
	verify(this->CurrentHandle != nullptr);
	verify(this->CurrentHandle->Item == this);
}

//Resets UObject data, but leaves ref counter alone
void FLuaUObjectItem::Reset()
{
	UnrealLua::UObjectRegistry::RemoveUsedItem(this);
	this->RemoveLuaScript();
	//LUA_LOG("Invalidating FLuaUObjectItem %s", *this->ObjectName.ToString())
	this->EmptyAllLuaScriptValues();;
	//this->LuaSelfCache.Reset();
	this->bIsMetaItem = false;
	this->PropertyMapping.Reset();
	this->ScriptHandle.Reset();
	this->TickFunc = {};
	//this->UObjectSerialNumber = 0;
	this->LuaRefCount = 0;
	this->bBlueprintTickEnabled = true;
	this->bLuaTickEnabled = true;
	//this->ObjectName = NAME_None;
	this->Object = nullptr;
	for(TObjectPtr<ULuaScriptDynamicDelegateHandler> handle : this->DelegateHandlers)
	{
		handle.Get()->MarkAsGarbage();
	}
	this->DelegateHandlers.Empty();
	if (this->CurrentHandle)
	{
		this->CurrentHandle->Invalidate();
	}
	for (auto& handle : this->Handles)
	{
		verify(handle.IsValid());
		verify(!handle->IsValid());
	}
	this->CurrentHandle = nullptr;
	this->bLuaTickEnabled = false;
	this->OnLuaScriptApplied.Clear();
	this->OnNumberOfValuesChanged.Clear();
	verify(this->VerifyIsClean());
}

bool FLuaUObjectItem::VerifyIsClean()
{
	verify(this->Object == nullptr);
	verify(this->LuaRefCount == 0);
	//verify(this->UObjectSerialNumber == 0);
	verify(!this->HasAnyLuaScriptValues())
	verify(this->bIsMetaItem == false);
	verify(!this->PropertyMapping.IsValid());
	verify(!this->ScriptHandle.IsValid());
	verify(!this->TickFunc.IsValid());
	verify(!this->bIsRegisteredInOuterForReplication);
	verify(this->bBlueprintTickEnabled);
	verify(!this->bLuaTickEnabled);
	//verify(this->LuaSelfCache.IsEmpty());
	verify(this->DelegateHandlers.IsEmpty())
	verify(this->CurrentHandle == nullptr);
	verify(!this->OnNumberOfValuesChanged.IsBound())
	verify(!this->OnLuaScriptApplied.IsBound())
	//handles stay existing, so any UObject lightuserdata can still look it up to verify its invalid
	for (auto& handle : this->Handles)
	{
		verify(handle.IsValid())
		verify(!handle->IsValid())
	}
	return true;
}

void FLuaUObjectItem::RemoveReplicatedObjectFromOuter()
{
	if(this->bIsRegisteredInOuterForReplication)
	{
		UObject* outer = this->Object->GetOuter();
		if(outer != nullptr)
		{
			if(AActor* outerActor = Cast<AActor>(outer))
			{
				LUA_LOG("Auto-removing replicated subobject %s from outer %s", *GetFullNameSafe(this->Object), *GetFullNameSafe(outer))
				outerActor->RemoveReplicatedSubObject(this->Object);
			}
			else if(UActorComponent* outerCmp = Cast<UActorComponent>(outer))
			{
				LUA_LOG("Auto-removing replicated subobject %s from outer %s", *GetFullNameSafe(this->Object), *GetFullNameSafe(outer))
				outerCmp->RemoveReplicatedSubObject(this->Object);
			}				
		}
		this->bIsRegisteredInOuterForReplication = false;
	}
}

void FLuaUObjectItem::UnregisterFromLuaReplicationComponent()
{
	if(this->Object)
	{
		ULuaScriptReplicationComponent* cmp = this->GetReplicationComponent();
		if(IsValid(cmp))
		{
			cmp->UnregisterFromLuaReplication(*this);
		}
	}
}

void FLuaUObjectItem::CleanupForLuaContext(sol::this_state lua)
{
	UObject* obj = this->Object;
	/*
	UClass* clazz = obj->GetClass();
	UFunction* uberGraphFunc = clazz->FindFunctionByName(NAME_ExecuteUbergraph);
	if(uint8* mem = clazz->GetPersistentUberGraphFrame(obj, uberGraphFunc))
	{
		for (FProperty* Property = uberGraphFunc->PropertyLink; Property; Property = Property->PropertyLinkNext)
		{
			//check for structs containing sol:: objects
			Property->InitializeValue_InContainer(mem);
		}
	}
	*/

	if(this->GetLuaScriptHandle().IsValid())
	{
		sol::this_state mylua = this->GetLuaScriptHandle().GetLuaThisState();
		if(mylua == lua)
		{
			this->TickFunc = {};
			this->ScriptHandle.Reset();
		}	
	}
	else
	{
		this->TickFunc = {};
	}
	
	this->CleanUpLuaScriptValuesForLuaState(lua.lua_state());
	
	if(this->TickFunc.GetFunction().lua_state() == lua)
	{
		this->TickFunc = {};
	}
	//this->LuaSelfCache.CleanUpIfLuaState(lua);
}

//A LuaUObjectItem is scripted if it has a valid ScriptHandle
bool FLuaUObjectItem::IsLuaScripted() const
{
	return this->ScriptHandle.IsValid();
}

void FLuaUObjectItem::RebuildInput()
{
	this->InputOverrides.Reset();
	
	if(!UUnrealLuaConfig::ShouldOverrideInput())
	{
		return;
	}
	
	UObject* obj = this->GetUObject();
	ULoadedLuaScriptCollection* coll = this->GetLuaScriptHandle().GetLuaScriptCollection();
	if(!obj || !coll || !coll->ShouldOverrideInput())
	{
		return;
	}
	
	AActor* actor = Cast<AActor>(obj);
	if(!actor)
	{
		return;
	}
	
	UInputComponent* inputComponent = actor->InputComponent;
	if (!inputComponent)
	{
		return;
	}
	
	bool createInput = false;
	
	if(APawn* pawn = Cast<APawn>(obj))
	{
		if (pawn->IsLocallyControlled())
		{
			createInput = true;
		}
	}
	else if(APlayerController* pcon = Cast<APlayerController>(obj))
	{
		if (pcon->IsLocalPlayerController())
		{
			createInput = true;
		}
	}
	
	if (createInput)
	{
		this->InputOverrides.InitializeAs<FLuaUObjectInputOverrides>(obj, inputComponent);
		this->InputOverrides.GetMutablePtr<FLuaUObjectInputOverrides>()->BindInputFunctions();	
	}
}

void FLuaUObjectItem::RemoveLuaScript(bool bIsReloading)
{
	if (bIsReloading)
	{
		this->ResetNonPropertyWrapperValuesButKeepListeners();
	}
	this->RemoveReplicatedObjectFromOuter();
	this->UnregisterFromLuaReplicationComponent();
	
	if(this->Object)
	{
		UnrealLua::LuaScriptCall::CallLuaFunctionSafeByName(this, UnrealLua::scriptLoading::ScriptEndPlay, this->Object.Get(), bIsReloading);
		
		if(!this->Object->HasAnyFlags(EObjectFlags::RF_ClassDefaultObject))
		{
			UWorld* world = this->Object->GetWorld();
			if(world && world->IsGameWorld())
			{
				UUnrealLuaGameWorldSubsystem* wss = world->GetSubsystem<UUnrealLuaGameWorldSubsystem>();
				if(wss)
				{
					wss->ClearTimersForObject(this->Object);
				}
			}	
		}
		this->InputOverrides.Reset();
	}
	
	if (!bIsReloading)
	{
		this->ScriptHandle = {};
	}
	this->TickFunc = {};
	if (this->Object)
	{
		this->OnLuaScriptApplied.Broadcast(this->Object);
	}
	//this->TickFuncMapping = nullptr;
	//this->NameToLuaScriptFunctionMapping = nullptr;
}

UObject* FLuaUObjectItem::GetUObject() const
{
	return this->Object;
}

UObject* FLuaUObjectItem::GetUObjectVirtual() const
{
	return this->Object;
}


/*
sol::object FLuaUObjectItem::SetFuncDescrLuaScriptValue(const FSetLuaScriptFuncDescrParams& params)
{
	return this->GetLuaScriptHandle().SetFuncDescrLuaScriptValue(params);
}
*/

const FHashedFieldMapping* FLuaUObjectItem::GetPropertyMapping(const uint32 hash)
{
	if(!this->PropertyMapping.IsValid()) [[unlikely]]
	{
		FLuaUObjectItem& meta = UnrealLua::UObjectRegistry::GetUObjectItem(this->Object->GetClass());
		this->PropertyMapping = meta.PropertyMapping;
	}
	return PropertyMapping.Get<FUStructPropertyMapping>().FindMapping(hash);	
}



bool FLuaUObjectItem::ProcessEvent(FLuaOverrideCallParams& params)
{
	//@TODO : what if it's a RPC func? -> FUNC_Net are not overridable yet, so not an issue so far
	params.FuncMapping = this->GetUFunctionOverrideLuaScriptFunction(params.Function->GetFName());
	if(params.FuncMapping == nullptr || !params.FuncMapping->valid())
	{
		return false;
	}
	//params.CallingObjectReference = UnrealLua::LightUserdata::GetUObjectAsTaggedLightUserdata(this->Object, params.FuncMapping->lua_state());//  this->GetUObjectReference(params.FuncMapping->lua_state());
	params.CallingObjectReference = UnrealLua::LightUserdata::GetUObjectAsTaggedLightUserdata(this->CurrentHandle, params.FuncMapping->lua_state());//  this->GetUObjectReference(params.FuncMapping->lua_state());
	return UnrealLua::LuaScriptCall::CallUFunctionOverride(params);
}

bool FLuaUObjectItem::ProcessTickEvent(FLuaOverrideCallParams& params)
{
	if(!this->TickFunc.IsValid())
	{
		return false;
	}
	params.FuncMapping = this->TickFunc.GetFunctionPtr();
	params.CallingObjectReference = UnrealLua::LightUserdata::GetUObjectAsTaggedLightUserdata(this->Object, params.FuncMapping->lua_state());//  this->GetUObjectReference(params.FuncMapping->lua_state());
	return UnrealLua::LuaScriptCall::CallTickUFunctionOverride(params);
}

bool FLuaUObjectItem::ProcessWidgetTickEvent(FLuaOverrideCallParams& params)
{
	if(!this->TickFunc.IsValid())
	{
		return false;
	}
	params.FuncMapping = this->TickFunc.GetFunctionPtr();
	params.CallingObjectReference = UnrealLua::LightUserdata::GetUObjectAsTaggedLightUserdata(this->Object, params.FuncMapping->lua_state());//  this->GetUObjectReference(params.FuncMapping->lua_state());
	return UnrealLua::LuaScriptCall::CallWidgetTickUFunctionOverride(params);
}

FLuaRepLayout* FLuaUObjectItem::GetRepLayout()
{
	FLuaScriptInstanceHandle& handle = this->GetLuaScriptHandle();
	if(!handle.IsValid())
	{
		return nullptr;
	}
	return handle.GetRepLayout();
}

void FLuaUObjectItem::SetLuaScriptHandle(FLuaScriptInstanceHandle& newHandle, bool bIsReload)
{
	UObject* object = this->Object;
	//LUA_LOG("Setting Lua script for %s", *GetPathNameSafe(object))
	verify(IsValid(object))
	verify(newHandle.IsValid())
	
	this->RemoveLuaScript(bIsReload);
	
	//verify(this->ScriptHandle.IsValid() == bIsReload)
	
	//Apply new LuaScript
	this->ScriptHandle = newHandle;
	this->ScriptHandle.SetOwner(this->Object.Get());
	verify(this->ScriptHandle.GetScriptOwner() == object)

	ULoadedLuaScriptCollection* coll = newHandle.GetLuaScriptCollection();
	verify(IsValid(coll));
	
	//Now that the LuaScript is propertly attached, copy values from script template to this new instance
	coll->ApplyLuaScriptTemplateToUObject(*this);
	
	sol::this_state lua = coll->OwningContext->GetLuaThisState();
	if(!object->IsA<AActor>())
	{
		this->ApplyOverridesFromOuterLuaScript(object, lua);	
	}

	//this will clear and rebuild FuncMappings
	this->RebuildInput();
	
	//Register replication
	//will also setup OnRep-Listeners
	this->ScriptHandle.InitializeLuaReplication();
	
	this->OnLuaScriptApplied.Broadcast(object);
	
	//Script BeginPlay on the clean script
	UnrealLua::LuaScriptCall::CallLuaFunctionSafeByName(this, UnrealLua::scriptLoading::ScriptBeginPlay, object, bIsReload);
}

void FLuaUObjectItem::ApplyOverridesFromOuterLuaScript(UObject* object, sol::this_state Lua)
{
	FLuaUObjectItem& myitem = UnrealLua::UObjectRegistry::GetUObjectItem(object);

	UObject* outer = object->GetOuter();
	if(!outer)
	{
		return;
	}
	FLuaUObjectItem* outerItem = UnrealLua::UObjectRegistry::TryGetUObjectItem(outer);

	if (!outerItem)
	{
		return;
	}
	/*
	 LuaScriptable objects can get their script behavior overridden by their outers LuaScript.
	 For example, a UScriptable ActorComponent's script values can be overridden by its ULuaScriptable Actor's Lua script 
	 The table must be named the same as the components name in the
	 - Actors component hierarchy (Blueprint component names)
	 - Names used when creating default subobjects (CreateDefaultSubobject<UMyComponent>("ThisName"))
	 - Names used when creating new subobjects AActor::AddComponentByClass()

		--ActorScript.lua
		local ActorScript = {]
		ActorScript.MovementComponent = {}
		function ActorScript.MovementComponent:ReceiveBeginPlay()
			print("overridden")
		end
		return ActorScript

		--MovementComponentScript.lua
		local MovementComponentScript = {}
		function MovementComponentScript:ReceiveBeginPlay()
			print("original")
		end
		return MovmentComponentScript
	 */

	FLuaScriptInstanceHandle& mainScriptOwnerHandle = outerItem->GetLuaScriptHandle();

	if(!mainScriptOwnerHandle.IsValid())
	{
		//Owner has no lua script, so nothing to override
		return;
	}

	ULoadedLuaScriptCollection* coll = mainScriptOwnerHandle.GetLuaScriptCollection();
	verify(coll != nullptr);

	sol::table overridesTbl = coll->GetSubobjectOverridesForObjectWithName(object->GetName());

	if(overridesTbl.valid())
	{
		//Scriptowner has a valid override table for that subobject
		LUA_LOG("patching in overrides into %s from %s", *GetNameSafe(object), *GetNameSafe(outerItem->Object))
		
		//owner actor can override functions and values, so copy the overridden value over to
		//the ILuaScriptable components script
		overridesTbl.for_each([this, &myitem](sol::object key, sol::object value)
		{
			//"this" is the subobject FLuaScriptInstance that gets its functions overridden
			if(key.get_type() == sol::type::string)
			{
				this->HasAnyLuaScriptOverridesFromOuterUObject = true;
				myitem.SetScriptValue(key, value);
			}
		});
	}
}

void FLuaUObjectItem::NotifyPreLuaScriptCollectionReload(FLuaScriptReloadCache* reloadcache)
{
	sol::object func = this->GetLuaScriptFunction(UnrealLua::scriptLoading::ScriptPreReload);
	UObject* object = this->Object.Get();
	if(func.valid() && func.get_type() == sol::type::function)
	{
		sol::protected_function_result result = UnrealLua::LuaScriptCall::CallLuaFunctionSafe(func.as<sol::function>(), object);
		if(result.valid() && result.return_count() == 1 && result[0].get_type() == sol::type::table)
		{
			reloadcache->AddUObjectData(object, result.get<sol::table>());
		}		
	}
	this->InputOverrides.Reset();
}

void FLuaUObjectItem::NotifyPostLuaScriptCollectionReload(FLuaScriptReloadCache* reloadcache)
{
	//This will also rebuild input
	UnrealLua::UObjectRegistry::LoadLuaScript(this->Object, true);
		
	FLuaScriptInstanceHandle& handle = this->GetLuaScriptHandle();
	if(handle.IsValid())
	{
		//restore potentially saved values from pre-reload
			
		sol::table* savedVals = reloadcache->GetReloadDataForObject(this->Object);
		if(savedVals)
		{
			UObject* obj = this->Object.Get();
			UnrealLua::LuaScriptCall::CallLuaFunctionSafeByName(obj, UnrealLua::scriptLoading::ScriptPostReload, obj, *savedVals);	
		}	
			
		//If this is a ILuaScriptableObject with one or more ILuaScriptable children, need to inform them
		TArray<UObject*> children;
		GetObjectsWithOuter(this->Object, children, false);
		for(UObject* subObj : children)
		{
			//sol::object existing = this->GetLuaScriptValue(subObjkey.Get(), lua).obj;
				
			FLuaScriptValue* sval =  this->GetLuaScriptValue(subObj->GetFName());
			if(sval)
			{
				if(sval->IsType<FLuaTableHandle>())
				{
					FLuaUObjectItem& subObjItem = UnrealLua::UObjectRegistry::GetUObjectItem(subObj);
			
					if(subObjItem.IsLuaScripted())
					{
						//only tell the subobject to reload if either
						//a) main object has an entry for subobject in its script table or
						//b) 
						if(subObjItem.HasAnyLuaScriptOverridesFromOuterUObject)
						{
							//LUA_LOG("UObject %s informing child %s to rebuild Lua script", *GetNameSafe(item.Object), *GetNameSafe(subObj))
							UnrealLua::UObjectRegistry::LoadLuaScript(subObj, true);
						}
					}					
				}
			}
		}
	}
}

void FLuaUObjectItem::SetLuaTickEnabled(bool bSetTickEnabled)
{
	if (!UUnrealLuaConfig::AllowOverrideTick() || !this->Object)
	{
		return;
	}
	UWorld* worldToRegisterIn = nullptr;
	this->bLuaTickEnabled = bSetTickEnabled;
	if(bSetTickEnabled)
	{
		if(AActor* actor = Cast<AActor>(this->Object))
		{
			if (!actor->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) && actor->GetClass()->HasAnyClassFlags(CLASS_Native))
			{
				//Actor will never ReceiveTick on its own, must tick manually
				worldToRegisterIn = actor->GetWorld();
			}
			else
			{
				actor->PrimaryActorTick.bCanEverTick = true;
				actor->SetActorTickEnabled(true);
				actor->PrimaryActorTick.Target = actor;
				actor->PrimaryActorTick.RegisterTickFunction(actor->GetLevel());
				actor->PrimaryActorTick.SetTickFunctionEnable(true);				
			}

		}
		else if (UActorComponent* cmp = Cast<UActorComponent>(this->Object))
		{
			if (!cmp->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) && cmp->GetClass()->HasAnyClassFlags(CLASS_Native))
			{
				//Component will never ReceiveTick on its own, must tick manually
				worldToRegisterIn = cmp->GetWorld();
			}
			else
			{
				cmp->PrimaryComponentTick.bCanEverTick = true;
				cmp->SetComponentTickEnabled(true);
				cmp->PrimaryComponentTick.Target = cmp;
				cmp->PrimaryComponentTick.RegisterTickFunction(cmp->GetOwner()->GetLevel());
				cmp->PrimaryComponentTick.SetTickFunctionEnable(true);				
			}
		}

	}
	else if (!bSetTickEnabled)
	{
		
	}
	
	if (worldToRegisterIn != nullptr)
	{
		if (worldToRegisterIn->IsGameWorld())
		{
			UUnrealLuaGameWorldSubsystem* ss = worldToRegisterIn->GetSubsystem<UUnrealLuaGameWorldSubsystem>();
			if (ss)
			{
				ss->RegisterManualTick(this->Object, bSetTickEnabled);
			}
		}
	}
}

void FLuaUObjectItem::SetBlueprintTickEnabled(bool bEnabled)
{
	this->bBlueprintTickEnabled = bEnabled;
	if(bEnabled && this->Object)
	{
		if(AActor* actor = Cast<AActor>(this->Object))
		{
			actor->PrimaryActorTick.bCanEverTick = true;
			actor->SetActorTickEnabled(true);
			actor->PrimaryActorTick.Target = actor;
			actor->PrimaryActorTick.RegisterTickFunction(actor->GetLevel());
			actor->PrimaryActorTick.SetTickFunctionEnable(true);
		}
	}
}

ULuaScriptReplicationComponent* FLuaUObjectItem::GetReplicationComponent()
{
	UObject* obj = this->Object;
	if(!obj)
	{
		return nullptr;
	}
	AActor* actor = Cast<AActor>(obj);
	if(!actor)
	{
		actor = obj->GetTypedOuter<AActor>();
	}
	if(!actor)
	{
		return nullptr;
	}
	return actor->GetComponentByClass<ULuaScriptReplicationComponent>();
}


void* FLuaUObjectItem::GetOwningContainer()
{
	return this->Object;
}
