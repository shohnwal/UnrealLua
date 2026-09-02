// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaContext/LuaScripts/LuaScriptInstanceHandle.h"
#include "Input/LuaUObjectInputOverrides.h"
#include "LuaValue/LuaScriptValue.h"
#include "Replication/LuaNetHandle.h"
#include "LuaValue/LuaScriptValuesContainer.h"
#include "Reflection/LuaFunctionMapping.h"
#include "sol/sol.hpp"
#include "StructUtils/SharedStruct.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/ObjectPtr.h"
#include "UObject/Class.h"
#include "Utility/UnrealLuaHash.h"
#include "UObject/StructOpsTypeTraits.h"
#include "Utility/UnrealLuaSimpleDelegateSignatures.h"
#include "LuaUObjectItem.generated.h"

class ULuaScriptDynamicDelegateHandler;
class ULuaScriptReplicationComponent;
struct FLuaScriptReloadCache;
struct FLuaOverrideCallParams;
struct FLuaRepLayout;

struct UNREALLUA_API FLuaUObjectItemHandle// : public ILuaLightUserdata
{
	FLuaUObjectItemHandle(FLuaUObjectItem& item);
	FLuaUObjectItem* Item = nullptr;
	void Invalidate();
	int __index(sol::stack_object Key) const;
	int __indexAsUEnum(sol::stack_object& key) const;
	void __newindex(sol::stack_object Key, sol::stack_object Value, lua_State* Lua) const;
	UObject* GetUObject() const;
	FLuaUObjectItem* GetUObjctItem() const;
	bool IsValid() const;
	int __tostring(lua_State* Lua);
	void AddReferencedUObject(FReferenceCollector& Collector) const;
};

USTRUCT()
struct UNREALLUA_API FLuaUObjectItem : public FLuaScriptValuesContainer
{
	GENERATED_BODY()
	FLuaUObjectItem();
	FLuaUObjectItem(const UObject* obj);
	//sol::object GetPropertyValue(FName propName, sol::state_view lua );
	void Reset();
	bool VerifyIsClean();
	void RemoveReplicatedObjectFromOuter();
	void UnregisterFromLuaReplicationComponent();
	//void NotifyPreGarbageCollection();
	void NotifyPostGarbageCollectConditionalBeginDestroy();
	void CleanupForLuaContext(sol::this_state lua);
	bool IsLuaScripted() const;
	
	bool ProcessEvent(FLuaOverrideCallParams& params);
	bool ProcessTickEvent(FLuaOverrideCallParams& params);
	bool ProcessWidgetTickEvent(FLuaOverrideCallParams& params);
public:
	FLuaScriptInstanceHandle& GetLuaScriptHandle();
	void RebuildInput();
	void RemoveLuaScript(bool bIsReloading = false);
	UObject* GetUObject() const;
	virtual UObject* GetUObjectVirtual() const override;
	
	virtual const FHashedFieldMapping* GetPropertyMapping(const sol::stack_object& strv) override;
	virtual const FHashedFieldMapping* GetPropertyMapping(const sol::object& strv) override;
	virtual const FHashedFieldMapping* GetPropertyMapping(const sol::string_view& strv) override;
	virtual const FHashedFieldMapping* GetPropertyMapping(const uint32 hash) override;

	void MarkAsMetaItem();
public:
	ULuaScriptDynamicDelegateHandler* GetDelegateHandler(UObject* delegateHost, FName funcName);
	ULuaScriptDynamicDelegateHandler* RemoveDelegate(UObject* delegateOwner, FName subscribedFuncName);

	void NotifyDelegateProcessEvent(ULuaScriptDynamicDelegateHandler* handler, void* params);

protected:
	virtual void UpdateTickFuncMapping(FLuaScriptValue* keyname) override;
public:
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	FLuaRepLayout* GetRepLayout();

	void SetLuaScriptHandle(FLuaScriptInstanceHandle& newHandle, bool bIsReload);
	void ApplyOverridesFromOuterLuaScript(UObject* Object, sol::this_state Lua);
	void NotifyPreLuaScriptCollectionReload(FLuaScriptReloadCache* reloadcache);
	void NotifyPostLuaScriptCollectionReload(FLuaScriptReloadCache* reloadcache);

	//sol::object __index(sol::stack_object& key, sol::this_state lua);
	int __index(sol::stack_object& key);
	int __indexAsUEnum(sol::stack_object key);
	void __newindex(sol::stack_object& key, sol::stack_object& value, sol::this_state lua);
	
	void SetLuaTickEnabled(bool bSetTickEnabled);
	void SetBlueprintTickEnabled(bool bEnabled);

	ULuaScriptReplicationComponent* GetReplicationComponent();
	
	FLuaUObjectItemHandle* CurrentHandle = nullptr;
	
	FLuaFunctionWrapper TickFunc = {};
	
	TArray<TUniquePtr<FLuaUObjectItemHandle>> Handles = {};
	
	//Holds Lua script. Only valid on ILuaScriptable objects that actually have a Lua script loaded
	UPROPERTY(VisibleAnywhere)
	FLuaScriptInstanceHandle ScriptHandle = {};
	
	
	sol::object GetUEnumWrapper(sol::this_state lua);
	sol::object GetUEnumValueWrapper(int64 value, sol::this_state lua);
	int PushUEnumValueWrapper(int64 value, sol::this_state lua);

	//Pointer to property mapping From UClass FLuaUObjectItem func mapping
	UPROPERTY(VisibleAnywhere)
	FSharedStruct PropertyMapping = {};
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UObject> Object = {};
	
	UPROPERTY(VisibleAnywhere)
	TArray<TObjectPtr<ULuaScriptDynamicDelegateHandler>> DelegateHandlers = {};
	
	UPROPERTY(VisibleAnywhere)
	TInstancedStruct<FLuaUObjectInputOverrides> InputOverrides = {};
	
	FCriticalSection Lock = {};
	
	UPROPERTY(VisibleAnywhere)
	FName ObjectName = NAME_None;
	//int32 UObjectSerialNumber = 0;
	UPROPERTY(VisibleAnywhere)
	int32 LuaRefCount = 0;

	UPROPERTY(VisibleAnywhere)
	FLuaNetHandle NetHandle = {};
	
	UPROPERTY(VisibleAnywhere)
	bool bLuaTickEnabled = false;
	UPROPERTY(VisibleAnywhere)
	bool bBlueprintTickEnabled = true;
	UPROPERTY(VisibleAnywhere)
	bool bIsMetaItem = false;
	UPROPERTY(VisibleAnywhere)
	bool bIsRegisteredInOuterForReplication = false;
	UPROPERTY(VisibleAnywhere)
	bool HasAnyLuaScriptOverridesFromOuterUObject = false;

	FObjectMulticastDelegate OnLuaScriptApplied = {};

	bool operator==(const FLuaUObjectItem& other) const
	{
		return this->Object == other.Object ;
	}
	
	friend uint32 GetKeyHash(const FLuaUObjectItem& This)
	{
		return This.Object->GetUniqueID();
	}

	friend uint32 GetTypehash(const FLuaUObjectItem& This)
	{
		return This.Object->GetUniqueID();
	}

	virtual void* GetOwningContainer() override;
};


template<>
struct TStructOpsTypeTraits<FLuaUObjectItem> : public TStructOpsTypeTraitsBase2<FLuaUObjectItem>
{
	enum
	{
		WithCopy = false
	};
};


inline sol::object FLuaUObjectItem::GetUEnumWrapper(sol::this_state lua)
{
	if(!IsValid(this->Object) || !this->Object->IsA<UEnum>())
	{
		return sol::nil;
	}
	sol::object enumObj = UnrealLua::LightUserdata::GetUEnumAsTaggedLightUserdata(Cast<UEnum>(this->Object), lua.lua_state());
	return enumObj;
}

inline sol::object FLuaUObjectItem::GetUEnumValueWrapper(int64 value, sol::this_state lua)
{
	if(!IsValid(this->Object) || !this->Object->IsA<UEnum>())
	{
		return sol::nil;
	}
	FLuaUEnumMapping& mapping = this->PropertyMapping.Get<FLuaUEnumMapping>();
	return mapping.GetEnumEntryLuaObjectByNumberValue(value, lua);
}

inline int FLuaUObjectItem::PushUEnumValueWrapper(int64 value, sol::this_state lua)
{
	if(!IsValid(this->Object) || !this->Object->IsA<UEnum>())
	{
		return 0;
	}
	
	FLuaUEnumMapping& mapping = this->PropertyMapping.Get<FLuaUEnumMapping>();
	return mapping.PushEnumEntryByNumberValue(value, lua);
}

inline FLuaScriptInstanceHandle& FLuaUObjectItem::GetLuaScriptHandle()
{
	return this->ScriptHandle;
} 

inline const FHashedFieldMapping* FLuaUObjectItem::GetPropertyMapping(const sol::string_view& strv)
{
	uint32 hash = UnrealLua::HashUtility::StrCrc32(strv.data());
	return this->GetPropertyMapping(hash);	
}

inline int FLuaUObjectItem::__index(sol::stack_object& key)
{
	//at this point FLuaObjectWrapper already checked object validity via GetItemPtr, no need to check again!	
	return this->PushScriptValue(key);
}


inline int FLuaUObjectItem::__indexAsUEnum(sol::stack_object key)
{
	if(!IsValid(this->Object) || !this->Object->IsA<UEnum>())
	{
		return 0;
	}
	FLuaUEnumMapping* uenum = this->PropertyMapping.GetPtr<FLuaUEnumMapping>();
	return uenum->__index(key);
}


inline void FLuaUObjectItem::__newindex(sol::stack_object& key, sol::stack_object& value, sol::this_state lua)
{
	//at this point FLuaObjectWrapper already checked object validity via GetItemPtr, no need to check again!
	if(this->Object->HasAllFlags(RF_ClassDefaultObject)) [[unlikely]]
	{
		//PROP_LOG_ERROR("Can't modify default object %s", *obj->GetName())
		return;
	}
	this->SetScriptValue(key, value);
}