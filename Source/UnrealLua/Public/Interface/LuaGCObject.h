#pragma once
#include "CoreMinimal.h"

class FGCObject;
class FReferenceCollector;

struct FLuaGCObject;

namespace UnrealLua::GCRegistry
{
	void RegisterObject(FLuaGCObject* obj);
	void UnregisterObject(FLuaGCObject* obj);
	void AddReferencedObjects(FReferenceCollector& Collector);
}

struct UNREALLUA_API ILuaGCObjectCollector
{
	
};

enum class ELuaUsertypeCategory : uint8
{
	None,
	UObjectWrapper,
	ScriptStruct,
	UClass,
	UScriptStruct,
	EnumEntry,
	UEnum,
	InstancedStruct,
	SharedStruct,
	Array,
	Map,
	Set,
	MulticastDelegate,
	SingleDelegate
};

struct UNREALLUA_API FLuaGCObject
{
	FLuaGCObject();

	FLuaGCObject(const FGCObject& Other);


	FLuaGCObject(FGCObject&& Other);
	virtual ~FLuaGCObject();
	
	void RegisterGCObject();
	void UnregisterGCObject();
	
	virtual void AddReferencedObjects(FReferenceCollector& Collector);
	virtual bool HasUObjectReferences() { return false; }
	virtual ELuaUsertypeCategory GetUsertypeCategory() const;

	bool IsRegistered() const
	{
		return HasBeenRegistered;
	}

	void SetRegistered(bool newRegistered)
	{
		HasBeenRegistered = newRegistered;		
	}

	bool HasBeenRegistered = false;
};