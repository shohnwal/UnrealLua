// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaUObjectItemView.h"
#include "sol/sol.hpp"
#include "UObject/Object.h"

#include "UnrealOverrides/LuaClassOverrideRegistry.h"
#include "UObject/ScriptInterface.h"
#include "Utility/UnrealLuaSimpleDelegateSignatures.h"
#include "UnrealLuaUObjectRegistry.generated.h"

/**
 * 
 */


class UUnrealLuaOverrideFunctionHostClass;
struct FLuaUObjectItem;
struct FLuaUEnumMapping;
struct FLuaScriptReloadCache;
class ILuaContext;
enum class ELuaScriptReloadStage;
class APlayerController;
struct FLuaScriptInstanceHandle;
class AActor;
class ULuaScriptReplicationComponent;
class UEnhancedInputComponent;
class UInputComponent;
struct FLuaScriptSettings;

DECLARE_MULTICAST_DELEGATE_OneParam(FUObjectExistenceEventDelegate, UObject*);

UCLASS(BlueprintType, Transient, Within=UnrealLuaEngineSubsystem)
class UNREALLUA_API UUnrealLuaUObjectRegistry : public UObject, public FUObjectArray::FUObjectDeleteListener, public FUObjectArray::FUObjectCreateListener
{
	GENERATED_BODY()
public:
	UUnrealLuaUObjectRegistry();
	virtual void BeginDestroy() override;
	UFUNCTION()
	static UUnrealLuaUObjectRegistry* Get();

	void SetActive(bool bActive);
	bool IsActive() const;
	
	FLuaUObjectItem* RegisterUObject(const UObject* obj);
	FLuaUObjectItem* RegisterMetaObject(const UField* obj);
	FLuaUObjectItem& LazyCreateMetaObjectAndBuildMappings(UField* clazz);
	void LinkUpRegisteredUObject(FLuaUObjectItem* item);
	void UnregisterUObject(UObject* obj);
	
	virtual void NotifyUObjectCreated(const UObjectBase* Object, int32 Index) override;
	void NotifyAsyncLoadingFlushUpdate();
	virtual void NotifyUObjectDeleted(const UObjectBase* Object, int32 Index) override;
	virtual void OnUObjectArrayShutdown() override;
	
	void RequestMakeUClassOverridable(UClass* uclass);
	
	void NotifyActorDestroyed(AActor* actor);
	
	UFUNCTION()
	void NotifyPawnRestart(APawn* pawn);
	UFUNCTION()
	void NotifyPlayerControllerPossessedPawn(APawn* oldPawn, APawn* newPawn);
	
	FUObjectExistenceEventDelegate& OnNewObjectEvent();
	FUObjectExistenceEventDelegate& OnRemovedObjectEvent();
	
	FLuaUObjectItem& GetUObjectItem(const UObject* Object);
	FLuaUObjectItem* TryGetUObjectItem(int32 index);
	FLuaUObjectItem* TryGetUObjectItem(const UObject* obj);
	FLuaUObjectItem& GetMetaObjectItem(const UField* obj);
	UObject* GetObject(int32 index);
	
	FLuaScriptInstanceHandle& GetLuaScriptHandle(UObject* object);
	FLuaScriptInstanceHandle& GetLuaScriptHandle(ULuaScriptReplicationComponent* replicator);
	
	void RemoveUsedItem(FLuaUObjectItem* luaUObjectItem);
	void CleanUpObjectsForLuaContext(const TScriptInterface<ILuaContext>& ictx);
	
	void NotifyPostGarbageCollectConditionalBeginDestroy();
	
	void AddLuaReferencedObjects(FReferenceCollector& Collector);
	
	FLuaClassOverrideRegistry& GetOverrideRegistry()
	{
		return this->UClassOverrideRegistry;
	}

	//TArray<FLuaUObjectItem*> LuaUObjectItemArray = {};
	
	//TArray<FLuaUObjectItemView> UsedLuaUObjectItems  = {};
	
	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	int32 NumActiveLuaObjects = 0;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bRegistryIsActive = false;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FLuaClassOverrideRegistry UClassOverrideRegistry = {};
	
	FCriticalSection CandidatesLock = {};
	
	UPROPERTY()
	TArray<TObjectPtr<UObject>> StillLoadingUObjects = {};
	
	FActorMulticastDelegate OnActorDestroyed = {};

	UFUNCTION(CallInEditor)
	void RefreshOverrideRegistry();
	
	FUObjectExistenceEventDelegate OnNewObjectEventDelegate = {};
	FUObjectExistenceEventDelegate OnRemovedObjectEventDelegate = {};
};
