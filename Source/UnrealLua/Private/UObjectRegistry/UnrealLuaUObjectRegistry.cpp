// Fill out your copyright notice in the Description page of Project Settings.
#include "UObjectRegistry/LuaUObjectRegistry.h"
#include "Utility/LuaLogMacros.h"
#include "Engine/Engine.h"
#include "Interface/LuaScriptable.h"
#include "Replication/LuaScriptReplicationComponent.h"
#include "Interface/LuaContext.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"
#include "LuaTypes/LuaEnum.h"
#include "Utility/CPUCycleTimer.h"
#include "UnrealLua.h"
#include "sol/sol.hpp"
#include "Config/UnrealLuaConfig.h"
#include "Config/UnrealLua_CompilerFlags.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Misc/CoreDelegates.h"
#include "UObject/Class.h"
#include "UObject/Object.h"
#include "UObjectRegistry/LuaUObjectItem.h"

namespace UnrealLua::UObjectRegistry
{
	
	static UUnrealLuaUObjectRegistry* GLuaUObjectRegistry = nullptr;
	static TArray<FLuaUObjectItem*> GLuaUObjectItemArray = {};
	static TArray<FLuaUObjectItem*> GUsedLuaUObjectItems = {};
	static FLuaUObjectItem* GActiveLitemsList = nullptr;
	static TArray<FLuaUObjectItemHandle*> GLuaUObjectItemHandles = {};
}


UUnrealLuaUObjectRegistry::UUnrealLuaUObjectRegistry()
{
	if (this->IsTemplate())
	{
		return;
	}
	verify(UnrealLua::UObjectRegistry::GLuaUObjectRegistry == nullptr);
	UnrealLua::UObjectRegistry::GLuaUObjectRegistry = this;
	FCoreDelegates::OnAsyncLoadingFlushUpdate.AddUObject(this, &UUnrealLuaUObjectRegistry::NotifyAsyncLoadingFlushUpdate);
	GUObjectArray.AddUObjectDeleteListener(this);
	GUObjectArray.AddUObjectCreateListener(this);
}

void UUnrealLuaUObjectRegistry::BeginDestroy()
{
	if (!this->IsTemplate())
	{
		verify(UnrealLua::UObjectRegistry::GLuaUObjectRegistry == this);
		UnrealLua::UObjectRegistry::GLuaUObjectRegistry = nullptr;
		
		GUObjectArray.RemoveUObjectDeleteListener(this);
		GUObjectArray.RemoveUObjectCreateListener(this);
	}
	Super::BeginDestroy();
}

UUnrealLuaUObjectRegistry* UUnrealLuaUObjectRegistry::Get()
{
	return UnrealLua::UObjectRegistry::GLuaUObjectRegistry;
}

void UUnrealLuaUObjectRegistry::SetActive(bool bActive)
{
	if(bActive)
	{
		verify(UUnrealLuaEngineSubsystem::IsGameSessionActive())
		LUA_LOG("Activating Lua Registry")
		this->bRegistryIsActive = true;
		verify(GUObjectArray.GetObjectArrayNum() > 0);

		UnrealLua::UObjectRegistry::GLuaUObjectItemArray = {};
		UnrealLua::UObjectRegistry::GUsedLuaUObjectItems = {};
		UnrealLua::UObjectRegistry::GActiveLitemsList = nullptr;
		
		//This must happen before any UObjects are further spawned
		this->UClassOverrideRegistry.ActivateOverrideRegistry();

		//UObject items are lazily linked with their properties when Lua references them the first time
	}
	else
	{
		LUA_LOG("Deactivating Lua Registry")
		verify(!UUnrealLuaEngineSubsystem::IsGameSessionActive())

		//Clear Property and Function mappings and Lua scripts
		for (FLuaUObjectItem* item : UnrealLua::UObjectRegistry::GLuaUObjectItemArray)
		{
			if(item == nullptr)
			{
				continue;
			}
			item->Reset();
			delete item;
		}
		verify(UnrealLua::UObjectRegistry::GUsedLuaUObjectItems.IsEmpty());
		UnrealLua::UObjectRegistry::GLuaUObjectItemArray.Empty();
		verify(UnrealLua::UObjectRegistry::GActiveLitemsList == nullptr);
		
		//Restore overridden UFunctions
		this->UClassOverrideRegistry.ShutdownOverrideRegistry();
		this->bRegistryIsActive = false;
	}
}

bool UUnrealLuaUObjectRegistry::IsActive() const
{
	return this->bRegistryIsActive;
}

FLuaUObjectItem* UUnrealLuaUObjectRegistry::RegisterUObject(const UObject* obj)
{
	int32 index = GUObjectArray.ObjectToIndex(obj);

	if(!UnrealLua::UObjectRegistry::GLuaUObjectItemArray.IsValidIndex(index))
	{
		UnrealLua::UObjectRegistry::GLuaUObjectItemArray.SetNum(GUObjectArray.GetObjectArrayNum(), EAllowShrinking::No);
	}
		
	FLuaUObjectItem* item = UnrealLua::UObjectRegistry::GLuaUObjectItemArray[index];
	if(item == nullptr || item->Object == nullptr)
	{
		if(item == nullptr)
		{
			item = new FLuaUObjectItem(obj); 
			UnrealLua::UObjectRegistry::GLuaUObjectItemArray[index] = item;				
			UnrealLua::UObjectRegistry::GUsedLuaUObjectItems.Add(item);
			this->NumActiveLuaObjects = UnrealLua::UObjectRegistry::GUsedLuaUObjectItems.Num();
		}
		else if(item->Object == nullptr)
		{
			verify(item->VerifyIsClean());
			verify(!UnrealLua::UObjectRegistry::GUsedLuaUObjectItems.Contains(item));
			new(item) FLuaUObjectItem (obj);
			verify(item->LuaRefCount == 0);				
			UnrealLua::UObjectRegistry::GUsedLuaUObjectItems.Add(item);
			this->NumActiveLuaObjects = UnrealLua::UObjectRegistry::GUsedLuaUObjectItems.Num();
		}
		LinkUpRegisteredUObject(item);
		verify(item->PropertyMapping.GetPtr<FUStructPropertyMapping>() != nullptr)
	}
	verify(UnrealLua::UObjectRegistry::GLuaUObjectItemArray[index] == item);
	verify(item->Object == obj);
	return item;
}

FLuaUObjectItem* UUnrealLuaUObjectRegistry::RegisterMetaObject(const UField* obj)
{
	int32 index = GUObjectArray.ObjectToIndex(obj);
	//LUA_LOG("Registering/Getting MetaItem for %s", *GetFullNameSafe(obj))
	if(!UnrealLua::UObjectRegistry::GLuaUObjectItemArray.IsValidIndex(index))
	{
		//UnrealLua::UObjectRegistry::GLuaUObjectWrapperArray.Reserve(GUObjectArray.GetObjectArrayCapacity());
		UnrealLua::UObjectRegistry::GLuaUObjectItemArray.SetNum(GUObjectArray.GetObjectArrayNum(), EAllowShrinking::No);
	}
	verify(UnrealLua::UObjectRegistry::GLuaUObjectItemArray.IsValidIndex(index));
		
	FLuaUObjectItem* item = UnrealLua::UObjectRegistry::GLuaUObjectItemArray[index];
	if(item == nullptr)
	{
		item = new FLuaUObjectItem(obj); 
		UnrealLua::UObjectRegistry::GLuaUObjectItemArray[index] = item;
		item->MarkAsMetaItem();
		UnrealLua::UObjectRegistry::GUsedLuaUObjectItems.Add(item);
		this->NumActiveLuaObjects = UnrealLua::UObjectRegistry::GUsedLuaUObjectItems.Num();
	}
	else if(item->Object == nullptr)
	{
		verify(item->VerifyIsClean());
		verify(!UnrealLua::UObjectRegistry::GUsedLuaUObjectItems.Contains(item));
		new(item) FLuaUObjectItem (obj);
		item->MarkAsMetaItem();
		UnrealLua::UObjectRegistry::GUsedLuaUObjectItems.Add(item);
		this->NumActiveLuaObjects = UnrealLua::UObjectRegistry::GUsedLuaUObjectItems.Num();
	}
	verify(UnrealLua::UObjectRegistry::GLuaUObjectItemArray[index] == item);
	return item;
}

namespace UnrealLua::UObjectRegistry
{
	bool IsValidPropertyForReflection(FProperty* prop)
	{
		return !prop->HasAnyPropertyFlags(CPF_DevelopmentAssets);
	}
	
	void AddBlueprintLibraryToScriptStructMapping(UStruct* metaStruct, FUStructPropertyMapping* ustructPropertyMapping)
	{
		if constexpr(UnrealLua::Compilation::WITH_SCRIPTSTRUCT_FUNCTION_LIBS)
		{
			UScriptStruct* ss = Cast<UScriptStruct>(metaStruct);
			UClass* bpLibraryClass = nullptr;
			if(ss->IsNative())
			{
				FName ssName = ss->GetFName();
				FString ssNameStr = ssName.ToString();
				// /Script/UnrealLua
				FString packagePath = ss->GetStructPathName().GetPackageName().ToString();
				// /Script/UnrealLua.<name>Library
				packagePath += "." + ssNameStr + "Library";
				bpLibraryClass = StaticLoadClass(UBlueprintFunctionLibrary::StaticClass(), ss->GetPackage(), *packagePath, nullptr, ELoadFlags::LOAD_NoWarn);
			}
			else
			{
				FName ssName = ss->GetFName();
				FString ssNameStr = ssName.ToString();

				//BP Script struct package path
				// /Game/FirstPerson/Structs/BPTestStruct
				FPackagePath path = ss->GetPackage()->GetLoadedPath();
				FString packagePath = path.GetPackageName();

				//We are only interested in the folder
				// /Game/FirstPerson/Structs/
				packagePath.RemoveFromEnd(ssNameStr);
				
				//Add the name+Library appended to it to get the potential library path
				// /Game/FirstPerson/Structs/BPTestStructLibrary.BPTestStructLibrary_C
				FString libName = ssNameStr + "Library"; 
				packagePath += libName + "." + libName + "_C";
				
				bpLibraryClass = StaticLoadClass(UBlueprintFunctionLibrary::StaticClass(), ss->GetPackage(), *packagePath, nullptr, ELoadFlags::LOAD_NoWarn);
			}
			
			if(bpLibraryClass && bpLibraryClass->IsChildOf<UBlueprintFunctionLibrary>())
			{
				FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetMetaObjectItem(bpLibraryClass);
				const TSet<FHashedFieldMapping>& bpLibraryMapping = item.PropertyMapping.Get<FUStructPropertyMapping>().PropertyMappings;
				
				//@TODO : This might override property mappings from UScriptStruct
				
				for (const FHashedFieldMapping& funcMapping : bpLibraryMapping)
				{
					if (!ustructPropertyMapping->AddExternalMapping(funcMapping))
					{
						LUA_LOG_WARNING("UScriptStruct field named %s already in mapping, can not add Library function from %s", *funcMapping.GetMappingFName().ToString(), *bpLibraryClass->GetPathName())
						continue;						
					}
				};
				
				//The scriptstruct mapping will keep the BlueprintLibrary alive
				ustructPropertyMapping->HostBlueprintLibrary = bpLibraryClass->GetDefaultObject<UBlueprintFunctionLibrary>();
			}
			else
			{
				FName ssName = ss->GetFName();
				FString ssNameStr = ssName.ToString();
				FString luaStructLibFolder = "StructLib/";
				FString pathStr = luaStructLibFolder + ssNameStr + ".lua";
				
			}
		}
	}

	void CreateMappingForMetaItem(FLuaUObjectItem& metaItem)
	{
		UField* metaField = Cast<UField>(metaItem.Object);
		 
		//LUA_LOG("Lazy building mapping for %s", *GetNameSafe(metaField))
		verify(IsInGameThread());

		//since we're building lazy, this should be all done bby now
		verify(!metaField->HasAnyFlags(EObjectFlags::RF_NeedPostLoad));
		verify(!metaField->HasAnyFlags(EObjectFlags::RF_NeedLoad));
		verify(!metaField->HasAnyFlags(EObjectFlags::RF_NeedInitialization));
		verify(!metaField->HasAnyFlags(EObjectFlags::RF_WillBeLoaded));
		verify(!metaField->HasAnyInternalFlags(UnrealLua::Flags::AsyncObjectFlags));

		verify(!metaItem.PropertyMapping.IsValid());

		if(metaField->IsA<UEnum>())
		{
			//Enums get special mapping
			UEnum* uenum = Cast<UEnum>(metaField);
			metaItem.PropertyMapping.InitializeAs<FLuaUEnumMapping>();
			FLuaUEnumMapping& mapping = metaItem.PropertyMapping.Get<FLuaUEnumMapping>();
			mapping.SetEnum(uenum);
			return;
		}

		//either UClass or UScriptStruct
		UStruct* metaStruct = Cast<UStruct>(metaField);
		
		metaItem.PropertyMapping.InitializeAs<FUStructPropertyMapping>();
		FUStructPropertyMapping* ustructPropertyMapping = metaItem.PropertyMapping.GetPtr<FUStructPropertyMapping>();
		ustructPropertyMapping->OwningField = metaField;

		for(TFieldIterator<FProperty> it(metaStruct, EFieldIterationFlags::IncludeSuper); it; ++it)
		{
			FProperty* prop = *it;
			if(!UnrealLua::UObjectRegistry::IsValidPropertyForReflection(prop))
			{
				continue;
			}
			ustructPropertyMapping->AddProperty(*prop);
		}
		
		if(metaStruct->IsA<UScriptStruct>())
		{
			AddBlueprintLibraryToScriptStructMapping(metaStruct, ustructPropertyMapping);
		}
		else if(metaStruct->IsA<UClass>())
		{
			const UClass* uclass = Cast<UClass>(metaStruct);
			//bool bIsBlueprintFunctionLibrary = uclass->IsChildOf<UBlueprintFunctionLibrary>();
			for(TFieldIterator<UFunction> it(metaStruct, EFieldIterationFlags::IncludeSuper); it; ++it)
			{
				UFunction* func = *it;
				FName funcName = func->GetFName();

				if(funcName == NAME_ExecuteUbergraph)
				{
					continue;
				}
				//uint32 fstrHash = GetTypeHash(funcName.ToString());
				ustructPropertyMapping->AddFunction(*func);
			}
		}
	}
}

FLuaUObjectItem& UUnrealLuaUObjectRegistry::LazyCreateMetaObjectAndBuildMappings(UField* clazz)
{
	FLuaUObjectItem* item = RegisterMetaObject(clazz);
	if(!item->PropertyMapping.IsValid())
	{
		UnrealLua::UObjectRegistry::CreateMappingForMetaItem(*item);	
	}
	return *item;
}

void UUnrealLuaUObjectRegistry::LinkUpRegisteredUObject(FLuaUObjectItem* item)
{
	UObject* obj = item->Object;
	if(!obj->IsA<UField>() && !obj->IsA<UInterface>())
	{
		//Note : This also iterates over UObject instances of trash classes ("SKEL_" and such), but these won't have valid property mappings anyway, so no harm in just copying nullptrs 
		//a metadata (UClass, UScriptStruct, UFunction, etc)
		FLuaUObjectItem& meta = this->GetMetaObjectItem(obj->GetClass());
		verifyf(meta.PropertyMapping.IsValid(), TEXT("Meta obj %s had no property mapping"), *GetNameSafe(obj->GetClass()))
		item->PropertyMapping = meta.PropertyMapping;
	}	
}

void UUnrealLuaUObjectRegistry::UnregisterUObject(UObject* obj)
{
	int32 index = GUObjectArray.ObjectToIndex(obj);
	FLuaUObjectItem* item = TryGetUObjectItem(index);
	if(!item)
	{
		return;
	}
	item->Reset();
}

void UUnrealLuaUObjectRegistry::NotifyUObjectCreated(const UObjectBase* object, int32 index)
{
	UObject* obj = (UObject*)object;

	if(!UUnrealLuaEngineSubsystem::IsGameSessionActive())
	{
		return;
	}
	
	if(!obj->IsTemplate())
	{
		return;
	}
	
	if(!IsInGameThread())
	{
		AsyncTask(ENamedThreads::Type::GameThread, [this, object, index]()
		{
			this->NotifyUObjectCreated(object, index);
		});
		return;
	}
	
	verify(IsInGameThread())
	
	constexpr EInternalObjectFlags AsyncObjectFlags = EInternalObjectFlags_AsyncLoading | /*EInternalObjectFlags::AsyncLoading |*/ EInternalObjectFlags::Async;
	if(obj->HasAnyFlags(EObjectFlags::RF_NeedPostLoad | EObjectFlags::RF_NeedLoad | EObjectFlags::RF_NeedInitialization | RF_WillBeLoaded)
	|| obj->HasAnyInternalFlags(AsyncObjectFlags) || IsInAsyncLoadingThread())
	{
		FScopeLock Lock(&this->CandidatesLock);
		//LUA_LOG("CDO created in async : %s", *GetFullNameSafe(obj))
		this->StillLoadingUObjects.Emplace(obj);
		return;
	}

	//UnrealLua::UObjectRegistry::NotifyUObjectCreated(obj, index);

	if(!IsValid(obj))
	{
		LUA_LOG("UnrealLua::UObjectRegistry::NotifyUObjectCreated : Object not valid! %p %d", obj, index)
		return;
	}

	verify(obj->IsValidLowLevel());
	verify(!obj->HasAnyFlags(EObjectFlags::RF_NeedPostLoad | EObjectFlags::RF_NeedLoad | EObjectFlags::RF_NeedInitialization | RF_WillBeLoaded))
		
	this->UClassOverrideRegistry.TryOverrideObjectClass(obj);
	this->OnNewObjectEvent().Broadcast(obj);
}


void UUnrealLuaUObjectRegistry::NotifyAsyncLoadingFlushUpdate()
{
	FScopeLock Lock(&this->CandidatesLock);
	if(StillLoadingUObjects.IsEmpty())
	{
		return;
	}

	
	if(!UUnrealLuaConfig::IsLuaEnabled() || !UUnrealLuaEngineSubsystem::IsGameSessionActive())
	{
		StillLoadingUObjects.Empty();
		return;
	}
	
	TArray<UObject*> CandidatesTemp{};
	TArray<UObject*> ReadyCandidates{};
	TArray<int> InvalidatedCandidateIndices{};
    
	CandidatesTemp.Append(StillLoadingUObjects);
    
	for (int32 i = CandidatesTemp.Num() - 1; i >= 0; --i)
	{
		UObject* ObjectPtr = CandidatesTemp[i];
		if (!IsValid(ObjectPtr))
		{
			// discard invalid objects
			InvalidatedCandidateIndices.Add(i);
			continue;
		}

		constexpr EInternalObjectFlags AsyncObjectFlags = EInternalObjectFlags_AsyncLoading | /*EInternalObjectFlags::AsyncLoading |*/ EInternalObjectFlags::Async;

		if (ObjectPtr->HasAnyFlags(RF_NeedPostLoad | EObjectFlags::RF_NeedLoad | EObjectFlags::RF_NeedInitialization | EObjectFlags::RF_WillBeLoaded)
			|| ObjectPtr->HasAnyInternalFlags(AsyncObjectFlags))
		{
			// delay bind on next update 
			continue;
		}

		ReadyCandidates.Add(ObjectPtr);
		InvalidatedCandidateIndices.Add(i);
	}

	for (int32 j = 0; j < InvalidatedCandidateIndices.Num(); ++j)
	{
		StillLoadingUObjects.RemoveAt(InvalidatedCandidateIndices[j]);
	}
    
	for (int32 i = 0; i < ReadyCandidates.Num(); ++i)
	{
		UObject* clazz = ReadyCandidates[i];
		//OverriddenClasses.Emplace(clazz->GetFName());
		int32 index = GUObjectArray.ObjectToIndex(clazz);
		this->NotifyUObjectCreated(clazz, index);
	}
    
}

void UUnrealLuaUObjectRegistry::NotifyUObjectDeleted(const UObjectBase* object, int32 Index)
{
	if(!UUnrealLuaEngineSubsystem::IsGameSessionActive())
	{
		return;
	}

	UObject* obj = (UObject*)object;
		
	this->OnRemovedObjectEvent().Broadcast(obj);
	this->UnregisterUObject(obj);
}

void UUnrealLuaUObjectRegistry::NotifyActorDestroyed(AActor* actor)
{
	//At this point, EndPlay has been called on the Actor and all its components
	
	this->OnActorDestroyed.Broadcast(actor);
}

void UUnrealLuaUObjectRegistry::NotifyPawnRestart(APawn* pawn)
{
	if (pawn)
	{
		FLuaUObjectItem& item = this->GetUObjectItem(pawn);
		item.RebuildInput();	
	}
}

void UUnrealLuaUObjectRegistry::NotifyPlayerControllerPossessedPawn(APawn* oldPawn, APawn* newPawn)
{
	if(newPawn && newPawn->Controller)
	{
		AController* controller = newPawn->Controller;
		FLuaUObjectItem& item = this->GetUObjectItem(controller);
		item.RebuildInput();	
	}
	else if(oldPawn && oldPawn->Controller)
	{
		AController* controller = oldPawn->Controller;
		FLuaUObjectItem& item = this->GetUObjectItem(controller);
		item.RebuildInput();
	}
}

FUObjectExistenceEventDelegate& UUnrealLuaUObjectRegistry::OnNewObjectEvent()
{
	return this->OnNewObjectEventDelegate;
}

FUObjectExistenceEventDelegate& UUnrealLuaUObjectRegistry::OnRemovedObjectEvent()
{
	return this->OnRemovedObjectEventDelegate;
}

FLuaUObjectItem& UUnrealLuaUObjectRegistry::GetUObjectItem(const UObject* obj)
{
	verify(IsValid(obj))
	verify(UUnrealLuaEngineSubsystem::IsGameSessionActive());
	return *this->RegisterUObject(obj);
}

FLuaUObjectItem* UUnrealLuaUObjectRegistry::TryGetUObjectItem(int32 index)
{
	return UnrealLua::UObjectRegistry::GLuaUObjectItemArray.IsValidIndex(index) ? UnrealLua::UObjectRegistry::GLuaUObjectItemArray[index] : nullptr;
}

FLuaUObjectItem* UUnrealLuaUObjectRegistry::TryGetUObjectItem(const UObject* obj)
{
	return this->TryGetUObjectItem(GUObjectArray.ObjectToIndex(obj));
}

FLuaUObjectItem& UUnrealLuaUObjectRegistry::GetMetaObjectItem(const UField* obj)
{
	FLuaUObjectItem& item = this->LazyCreateMetaObjectAndBuildMappings(const_cast<UField*>(obj));
	verify(item.PropertyMapping.IsValid());
	return item;
}

UObject* UUnrealLuaUObjectRegistry::GetObject(int32 index)
{
	FLuaUObjectItem* item = this->TryGetUObjectItem(index);
	if(!item)
	{
		return nullptr;
	}
	return item->Object;
}

void UUnrealLuaUObjectRegistry::RemoveUsedItem(FLuaUObjectItem* luaUObjectItem)
{
	UnrealLua::UObjectRegistry::GUsedLuaUObjectItems.RemoveSingleSwap(luaUObjectItem);
	this->NumActiveLuaObjects = UnrealLua::UObjectRegistry::GUsedLuaUObjectItems.Num();
}

void UUnrealLuaUObjectRegistry::CleanUpObjectsForLuaContext(const TScriptInterface<ILuaContext>& ictx)
{
	//Clear Property and Function mappings and Lua scripts
	sol::this_state ctxlua = ictx->GetScopedLuaContext().GetLuaThisState();
	for (FLuaUObjectItem* item : UnrealLua::UObjectRegistry::GLuaUObjectItemArray)
	{
		if(item == nullptr)
		{
			continue;
		}
		item->CleanupForLuaContext(ctxlua);
	}
}

FLuaScriptInstanceHandle& UUnrealLuaUObjectRegistry::GetLuaScriptHandle(UObject* object)
{
	FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(object);
	return item.GetLuaScriptHandle();
}

FLuaScriptInstanceHandle& UUnrealLuaUObjectRegistry::GetLuaScriptHandle(ULuaScriptReplicationComponent* replicator)
{
	return this->GetLuaScriptHandle(replicator->GetOwner());
}

void UUnrealLuaUObjectRegistry::NotifyPostGarbageCollectConditionalBeginDestroy()
{
	for(int32 index = 0; index < UnrealLua::UObjectRegistry::GUsedLuaUObjectItems.Num(); index++)
	{
		FLuaUObjectItemView item = UnrealLua::UObjectRegistry::GUsedLuaUObjectItems[index];
		item->NotifyPostGarbageCollectConditionalBeginDestroy();
	}	
	this->UClassOverrideRegistry.ClearInvalidUClasses();
}

void UUnrealLuaUObjectRegistry::OnUObjectArrayShutdown()
{
	//GLuaUObjectWrapperArray.Empty();
	UnrealLua::UObjectRegistry::GUsedLuaUObjectItems.Empty();
	for(FLuaUObjectItem* item : UnrealLua::UObjectRegistry::GLuaUObjectItemArray)
	{
		if (item != nullptr)
		{
			item->Reset();
			delete item;
		}
	}
	UnrealLua::UObjectRegistry::GLuaUObjectItemArray.Empty();
	verify(UnrealLua::UObjectRegistry::GUsedLuaUObjectItems.IsEmpty());
}

void UUnrealLuaUObjectRegistry::RequestMakeUClassOverridable(UClass* uclass)
{
	this->UClassOverrideRegistry.RequestMakeUClassOverridable(uclass);
}

void UUnrealLuaUObjectRegistry::AddLuaReferencedObjects(FReferenceCollector& Collector)
{
	//LUA_LOG("UUnrealLuaUObjectRegistry::AddReferencedObjects, %d items in container, %d capacity, %d bytes used by num, %d bytes used by allocation", UnrealLua::UObjectRegistry::GLuaUObjectItemArray.Num(), UnrealLua::UObjectRegistry::GLuaUObjectItemArray.Max(), UnrealLua::UObjectRegistry::GLuaUObjectItemArray.NumBytes(), UnrealLua::UObjectRegistry::GLuaUObjectItemArray.GetAllocatedSize())
	int32 collected = 0;
	int32 realEntries = 0;
	//FCPUCycleTimer timer{"UUnrealLuaUObjectRegistry::AddReferencedObjects"};
		
	for (int32 index = 0; index < UnrealLua::UObjectRegistry::GUsedLuaUObjectItems.Num(); index++)
	{
		FLuaUObjectItemView item = UnrealLua::UObjectRegistry::GUsedLuaUObjectItems[index];
		verify(item.IsValid());
		item->AddReferencedObjects(Collector);
	};
}

void UUnrealLuaUObjectRegistry::RefreshOverrideRegistry()
{
	this->UClassOverrideRegistry.InitOverrideRegistry();
}

namespace UnrealLua::UObjectRegistry
{
	FLuaUObjectItem* RegisterMetaObject(const UField* obj)
	{
		return GLuaUObjectRegistry->RegisterMetaObject(obj);
	}
	
	FLuaUObjectItem& LazyCreateMetaObjectAndBuildMappings(UField* clazz)
	{
		return GLuaUObjectRegistry->LazyCreateMetaObjectAndBuildMappings(clazz);
	}

	void LinkUpRegisteredUObject(FLuaUObjectItem* item)
	{
		return GLuaUObjectRegistry->LinkUpRegisteredUObject(item);
	}

	FLuaUObjectItem* RegisterUObject(const UObject* obj)
	{
		return GLuaUObjectRegistry->RegisterUObject(obj);
	}

	void UnregisterUObject(UObject* obj)
	{
		return GLuaUObjectRegistry->UnregisterUObject(obj);
	}

	FUObjectExistenceEventDelegate& OnNewObjectEvent()
	{
		return GLuaUObjectRegistry->OnNewObjectEvent();
	}
	
	FUObjectExistenceEventDelegate& OnRemovedObjectEvent()
	{
		return GLuaUObjectRegistry->OnRemovedObjectEvent();
	}

	void CleanUpObjectsForLuaContext(const TScriptInterface<ILuaContext>& ictx)
	{
		GLuaUObjectRegistry->CleanUpObjectsForLuaContext(ictx);
	}

	FLuaScriptInstanceHandle& GetLuaScriptHandle(UObject* object)
	{
		return GLuaUObjectRegistry->GetLuaScriptHandle(object);
	}
	
	sol::object GetEnumWrapperLuaObject(UEnum* uenum, sol::this_state lua)
	{
		FLuaUObjectItem& item = GLuaUObjectRegistry->GetMetaObjectItem(uenum);
		return item.GetUEnumWrapper(lua);
	}

	sol::object GetEnumValueWrapper(UEnum* uenum, int64 value, sol::this_state lua)
	{
		FLuaUObjectItem& item = GLuaUObjectRegistry->GetMetaObjectItem(uenum);
		return item.GetUEnumValueWrapper(value, lua);
	}

	int PushEnumValueWrapper(TObjectPtr<UEnum> Enum, int64 Val, sol::this_state Lua)
	{
		FLuaUObjectItem& item = GLuaUObjectRegistry->GetMetaObjectItem(Enum);
		return item.PushUEnumValueWrapper(Val, Lua);
	}
	
	FLuaScriptInstanceHandle& GetLuaScriptHandle(ULuaScriptReplicationComponent* replicator)
	{
		return GLuaUObjectRegistry->GetLuaScriptHandle(replicator);
	}

	void NotifyUObjectCreated(UObject* obj, int32 index)
	{
		GLuaUObjectRegistry->NotifyUObjectCreated(obj, index);
	}

	//This only gets called when a UObject gets deleted by GC
	//That means this is not sufficient for unregistering UObjects from their outers,
	//since the GC may happen long after the UObject gets marked for kill
	void NotifyUObjectDeleted(const UObjectBase* Object, int32 Index)
	{
		GLuaUObjectRegistry->NotifyUObjectDeleted(Object, Index);
	}

	void NotifyActorDestroyed(AActor* actor)
	{
		GLuaUObjectRegistry->NotifyActorDestroyed(actor);
	}

	FLuaUObjectItem& GetUObjectItem(const UObject* Object)
	{
		return GLuaUObjectRegistry->GetUObjectItem(Object);
	}
	
	FLuaUObjectItem* TryGetUObjectItem(int32 index)
	{
		return GLuaUObjectRegistry->TryGetUObjectItem(index);
	}

	FLuaUObjectItem* TryGetUObjectItem(const UObject* obj)
	{
		return GLuaUObjectRegistry->TryGetUObjectItem(obj);
	}
	
	FLuaUObjectItem& GetMetaObjectItem(const UField* obj)
	{
		return GLuaUObjectRegistry->GetMetaObjectItem(obj);
	}
	
	UObject* GetObject(int32 index)
	{
		return GLuaUObjectRegistry->GetObject(index);
	}

	FLuaNetHandle GetLuaNetHandleForObject(UObject* object)
	{
		FLuaUObjectItem* item = TryGetUObjectItem(object);
		if(item)
		{
			return item->NetHandle;	
		}
		return FLuaNetHandle();
	}

	int PushUObjectAsLightUserdata(lua_State* luaState, UObject* things)
	{
		return UnrealLua::LightUserdata::PushUObject(luaState, things);
	}

	sol::object GetUObjectAsLightUserdata(lua_State* luaState, UObject* object)
	{
		return UnrealLua::LightUserdata::GetUObjectAsTaggedLightUserdata(object, luaState);
	}

	void RemoveUsedItem(FLuaUObjectItem* luaUObjectItem)
	{
		GLuaUObjectRegistry->RemoveUsedItem(luaUObjectItem);
	}

	void RequestMakeUClassOverridable(UClass* uclass)
	{
		GLuaUObjectRegistry->RequestMakeUClassOverridable(uclass);
	}
	
	FLuaClassOverrideRegistry& GetLuaClassOverrideRegistry()
	{
		return GLuaUObjectRegistry->GetOverrideRegistry();
	}
}






