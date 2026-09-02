// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


#include "LuaScriptInstanceHandle.h"
#include "sol/sol.hpp"

#include "LoadedLuaScriptResult.h"
#include "LuaScriptTemplate.h"
#include "StructUtils/SharedStruct.h"
#include "UObject/CoreNetTypes.h"
#include "LoadedLuaScriptCollection.generated.h"

struct FLuaUObjectItem;
struct FFileStatData;
//struct FLuaScriptFunctionMap;
struct FLuaStruct;
struct FLoadLuaScriptResult;
enum class ELuaScriptType : uint8;
struct FLuaScriptInstance;
struct FScopedLuaContext;
/**
 * 
 */
class ULuaContext;

USTRUCT(BlueprintType)
struct UNREALLUA_API FUnrealLuaRepLayoutProperty
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere)
	FName Property;
	UPROPERTY(VisibleAnywhere)
	FName SubObject;
	UPROPERTY(VisibleAnywhere)
	FString OnRep;
	//Intentionally not UProperty, will copy FString from Property FName
	FString StringKey = "";
	UPROPERTY(VisibleAnywhere)
	TEnumAsByte<ELifetimeCondition> Condition = COND_None;
	UPROPERTY(VisibleAnywhere)
	uint8 RepLayoutPropertyIndex = 0;
	UPROPERTY(VisibleAnywhere)
	bool PassKeyOnRep = false;
};

USTRUCT(BlueprintType)
struct UNREALLUA_API FUnrealLuaObjectRepLayout
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName SubObjectPropertyName;
	TArray<FUnrealLuaRepLayoutProperty*> ReplicatedProperties;
};

enum class ELuaScriptReloadStage
{
	PRERELOAD,
	CLEARVALUES,
	RELOADSCRIPT,
	POSTRELOAD, 
};

USTRUCT(BlueprintType)
struct UNREALLUA_API FLuaRepLayout
{
	GENERATED_BODY()

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	//TArray<FUnrealLuaRepLayoutProperty> ReplicatedProperties;
	UPROPERTY(VisibleAnywhere)
	TArray<FUnrealLuaRepLayoutProperty> RepLayoutProperties;
	TArray<FUnrealLuaObjectRepLayout, TInlineAllocator<1>> ObjectReplayouts;
	UPROPERTY(VisibleAnywhere)
	float ReplicationFrequency = 0.0f;
	UPROPERTY(VisibleAnywhere)
	bool bAutoRegisterReplicatedSubobject = false;
	UPROPERTY(VisibleAnywhere)
	TEnumAsByte<ELifetimeCondition> ObjectReplicationCondition = COND_None;
	//Collection of all applicable replication conditions for all properties in the Replayout
	//Used to instantiate Lua replicators
	UPROPERTY(VisibleAnywhere)
	TArray<TEnumAsByte<ELifetimeCondition>> PropertyReplicationConditionFlags;
	FUnrealLuaRepLayoutProperty* GetRepPropertyForRepIndex(uint8 index) const;
	FUnrealLuaObjectRepLayout* GetObjectReplayout(FName subObjName);
	void BuildSubObjectMappings();

private:
	FUnrealLuaObjectRepLayout* GetOrCreateObjectReplayout(FName subObj);
};

USTRUCT()
struct UNREALLUA_API FLoadedLuaScriptCollectionFileInfo
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere)
	FString OriginalFileRequestPath = {};
	UPROPERTY(VisibleAnywhere)
	TArray<FLoadedLuaFileInfo> MainFileInfo = {};
	UPROPERTY(VisibleAnywhere)
	TArray<FLoadedLuaFileInfo> ModFileInfos = {};
};

struct UNREALLUA_API FLuaScriptReloadCache
{
	FLuaScriptReloadCache(lua_State* L)
		: Lua(L)
	{}
	void AddUObjectData(UObject* obj, sol::table tbl)
	{
		const uint32 id = obj->GetUniqueID();
		if (!tbl.valid())
		{
			CachedReloadData.Remove(id);
			return;
		}
		CachedReloadData.Emplace(id, tbl);
	}
	
	sol::table* GetReloadDataForObject(UObject* obj)
	{
		const uint32 id = obj->GetUniqueID();
		return CachedReloadData.Find(id);
	}
	sol::this_state Lua;
	private:
	TMap<uint32, sol::table> CachedReloadData = {};
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnLuaScriptReloadDelegate, ELuaScriptReloadStage, FLuaScriptReloadCache*);

USTRUCT()
struct UNREALLUA_API FLoadedLuaScriptFile
{
	GENERATED_BODY()
};
UCLASS(Transient)
class UNREALLUA_API ULoadedLuaScriptCollection : public UObject
{
	GENERATED_BODY()
	
	ULoadedLuaScriptCollection();
public:
	FOnLuaScriptReloadDelegate OnScriptReload = {};
	
	void Initialize(FScopedLuaContext* owningContext, const FName& fileName, FLoadLuaScriptResult& newScriptTemplate);
	virtual void BeginDestroy() override;
	void Reset();
	
	//FLuaScriptInstanceHandle GetSharedLuaScript(UObject* owner = nullptr);
	FLuaScriptInstanceHandle GetInstancedLuaScript(FLuaUObjectItem& scriptOwner);
	void ApplyLuaScriptTemplateToUObject(FLuaUObjectItem& scriptOwner);

	auto GetLuaScriptAsTable(bool bTrackScript = true) -> sol::table;

	bool Reload();
public:
	bool HasReplicatedProperties() const { return this->Replayout.IsValid() && this->Replayout.Get<FLuaRepLayout>().RepLayoutProperties.Num() > 0; }
	sol::table GetSubobjectOverridesForObjectWithName(const FString& name);
	bool ShouldOverrideInput() const;
	bool AutoRegisterReplicatedSubobject() const;
	ELifetimeCondition GetReplicationCondition() const;

	FScopedLuaContext* OwningContext;

	UPROPERTY(VisibleAnywhere)
	FLoadedLuaScriptCollectionFileInfo FileInfo;

	UPROPERTY(VisibleAnywhere)
	FLuaScriptTemplate ScriptTemplate;
	
	FLuaRepLayout* GetRepLayout();
	UPROPERTY(VisibleAnywhere)
	TInstancedStruct<FLuaRepLayout> Replayout;
	
	UPROPERTY(VisibleAnywhere)
	bool bCheckedRepLayout;
};