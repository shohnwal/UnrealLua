#include "Interface/LuaGCObject.h"

#include "Utility/LuaLogMacros.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"

namespace UnrealLua::GCRegistry
{
	TArray<FLuaGCObject*> GCObjects = {};
}

void UnrealLua::GCRegistry::RegisterObject(FLuaGCObject* obj)
{
//	verify(obj->hasBeenRegistered == false);
	GCObjects.AddUnique(obj);
	//obj->hasBeenRegistered = true;
	//LUA_LOG("Registered reference for GCObject %p %s, there are now %d items in the array", obj, *obj->GetName(), GCObjects.Num());
}

void UnrealLua::GCRegistry::UnregisterObject(FLuaGCObject* obj)
{
	//LUA_LOG("Removing reference for GCObject %p %s, there are now %d items in the array", obj, *obj->GetName(), GCObjects.Num());
//	verify(obj->hasBeenRegistered == true);
	//obj->hasBeenRegistered = false;;
	verify(GCObjects.Contains(obj));
	GCObjects.RemoveSingleSwap(obj);
	verify(!GCObjects.Contains(obj));
	verify(GCObjects.Num() >= 0);
}

void UnrealLua::GCRegistry::AddReferencedObjects(FReferenceCollector& Collector)
{
	checkNoEntry();
	unimplemented();
	verify(GCObjects.Num() >= 0);
	for(FLuaGCObject* item : GCObjects)
	{
		//LUA_LOG("Counting reference for GCObject %p %s, there are now %d items in the array", item, *item->GetName(), GCObjects.Num());
		verify(item != nullptr);
		item->AddReferencedObjects(Collector);
	}
}

FLuaGCObject::FLuaGCObject()
{
	RegisterGCObject();
}

FLuaGCObject::FLuaGCObject(const FGCObject& Other)
{
	RegisterGCObject();
}

FLuaGCObject::FLuaGCObject(FGCObject&& Other)
{
	RegisterGCObject();
}

FLuaGCObject::~FLuaGCObject()
{
	UnregisterGCObject();
}

void FLuaGCObject::RegisterGCObject()
{
	if(!this->HasBeenRegistered)
	{
		FUnrealLuaGarbageCollector::RegisterLuaGCObject(this);
	}
	verify(this->HasBeenRegistered);
}

void FLuaGCObject::UnregisterGCObject()
{
	if(this->HasBeenRegistered)
	{
		FUnrealLuaGarbageCollector::UnregisterLuaGCObject(this);
	}
	verify(!this->HasBeenRegistered)
}

void FLuaGCObject::AddReferencedObjects(FReferenceCollector& Collector)
{
	checkNoEntry();
}

ELuaUsertypeCategory FLuaGCObject::GetUsertypeCategory() const
{
	return ELuaUsertypeCategory::None;
}