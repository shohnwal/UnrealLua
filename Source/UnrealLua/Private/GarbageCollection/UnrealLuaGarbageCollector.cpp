// Fill out your copyright notice in the Description page of Project Settings.


#include "GarbageCollection/UnrealLuaGarbageCollector.h"

#include "Config/UnrealLuaConfig.h"
#include "Interface/LuaContext.h"
#include "Interface/LuaGCObject.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"
#include "Utility/CPUCycleTimer.h"

namespace UnrealLua::GarbageCollection
{
	static FCriticalSection LuaGCObjectsListLock = {};
}
void FUnrealLuaGarbageCollector::Initialize(UUnrealLuaEngineSubsystem* ss)
{
	this->LuaEngineSubsystem = ss;
	this->LuaGCObjects.Reset(64);
	
	FCoreUObjectDelegates::PostGarbageCollectConditionalBeginDestroy.AddRaw(this, &FUnrealLuaGarbageCollector::NotifyPostGarbageCollectConditionalBeginDestroy);
	FCoreUObjectDelegates::PreGarbageCollectConditionalBeginDestroy.AddRaw(this, &FUnrealLuaGarbageCollector::NotifyPreGarbageCollectConditionalBeginDestroy);
	FCoreUObjectDelegates::GetPreGarbageCollectDelegate().AddRaw(this, &FUnrealLuaGarbageCollector::NotifyPreGarbageCollection);
	FCoreUObjectDelegates::GetPostGarbageCollect().AddRaw(this, &FUnrealLuaGarbageCollector::NotifyPostGarbageCollection);
}

void FUnrealLuaGarbageCollector::PreDeinitialize()
{
	this->ProcessPostDestroyGCInvalidUObjectCollection();
}

void FUnrealLuaGarbageCollector::Deinitialize()
{
	FCoreUObjectDelegates::PostGarbageCollectConditionalBeginDestroy.RemoveAll(this);
	FCoreUObjectDelegates::PreGarbageCollectConditionalBeginDestroy.RemoveAll(this);
	FCoreUObjectDelegates::GetPreGarbageCollectDelegate().RemoveAll(this);
	FCoreUObjectDelegates::GetPostGarbageCollect().RemoveAll(this);
	
}

void FUnrealLuaGarbageCollector::NotifyPreGarbageCollection()
{
}

void FUnrealLuaGarbageCollector::NotifyPreGarbageCollectConditionalBeginDestroy()
{
	//This happens before objects are getting BeginDestroy()-ed during GC.
	//This is the only moment where objects still have their name and their outer
	//Need to immediately take care of invalid FLuaUObjectItems
	//LUA_LOG("UUnrealLuaEngineSubsystem::NotifyPreGarbageCollectConditionalBeginDestroy")
}

void FUnrealLuaGarbageCollector::NotifyPostGarbageCollectConditionalBeginDestroy()
{
	//This happens after objects are getting BeginDestroy()-ed during GC.
	//At this point objects are called "none" and have no outer
	
	if (!this->LuaEngineSubsystem)
	{
		return;
	}
	//FCPUCycleTimer timer{"UUnrealLuaEngineSubsystem::NotifyPostGarbageCollectConditionalBeginDestroy"};
	this->LuaEngineSubsystem->UObjectRegistry->NotifyPostGarbageCollectConditionalBeginDestroy();

	//remove invalid FLuaUObjectWrappers
	this->ProcessPostDestroyGCInvalidUObjectCollection();
}


bool FUnrealLuaGarbageCollector::ProcessPostDestroyGCInvalidUObjectCollection()
{
	if(UUnrealLuaConfig::GetGCMode() == EUnrealLuaGCMode::PostDestroy)
	{
		const TArray<TScriptInterface<ILuaContext>>& luaContextList = this->LuaEngineSubsystem->GetActiveLuaContextListRef();
		for(const TScriptInterface<ILuaContext>& ctx : luaContextList)
		{
			ctx->GetScopedLuaContext().ProcessInvalidUObjectCollection();
		}		
	}
	return true;
}


void FUnrealLuaGarbageCollector::NotifyPostGarbageCollection()
{
	//This happens before NotifyPostGarbageCollection
}

void FUnrealLuaGarbageCollector::NotifyEndFrame(UUnrealLuaEngineSubsystem* ss)
{
	if(UUnrealLuaConfig::GetGCMode() == EUnrealLuaGCMode::Incremental)
	{
		const TArray<TScriptInterface<ILuaContext>>& luaContextList = ss->GetActiveLuaContextListRef();
		for(const TScriptInterface<ILuaContext>& ctx : luaContextList)
		{
			ctx->GetScopedLuaContext().IncrementalProcessInvalidLuaUObjects();
		}		
	}
}

void FUnrealLuaGarbageCollector::RegisterLuaGCObject(FLuaGCObject* gcobj)
{
	
	verify(!gcobj->IsRegistered());
	
	FUnrealLuaGarbageCollector& collector = UUnrealLuaEngineSubsystem::Get()->LuaGarbageCollector;
	
	{
		FScopeLock lock{&UnrealLua::GarbageCollection::LuaGCObjectsListLock};
		collector.LuaGCObjects.AddUnique(gcobj);		
	}
	gcobj->SetRegistered(true);	
	verify(gcobj->IsRegistered());
}

void FUnrealLuaGarbageCollector::UnregisterLuaGCObject(FLuaGCObject* gcobj)
{
	verify(gcobj->IsRegistered());
	
	FUnrealLuaGarbageCollector& collector = UUnrealLuaEngineSubsystem::Get()->LuaGarbageCollector;
	
	{
		FScopeLock lock{&UnrealLua::GarbageCollection::LuaGCObjectsListLock};
		collector.LuaGCObjects.Remove(gcobj);
	}
	gcobj->SetRegistered(false);
	verify(!gcobj->IsRegistered());
}

void FUnrealLuaGarbageCollector::AddReferencedLuaObjects(FReferenceCollector& collector)
{
	//is not necessarily in game thread!

	//ScopedLuaContext are FGCObjects, so they can handle references themselves
	
	{
		//FCPUCycleTimer partimer{FString::Printf(TEXT("UUnrealLuaEngineSubsystem::AddReferencedObjects : Referencing in %d entries"), this->LuaGCObjects.Num())};
		//FScopeLock lock{&this->LuaGCObjectsListLock};
		//ParallelFor(this->LuaGCObjects.Num(), [this, &collector](int32 index)
		FScopeLock lock{&UnrealLua::GarbageCollection::LuaGCObjectsListLock};
		for(int32 index = 0; index < this->LuaGCObjects.Num(); index++)
		{
			FLuaGCObject* obj = this->LuaGCObjects[index];
			verify(obj->IsRegistered());
			obj->AddReferencedObjects(collector);
		}
		//}, true);
	}
	this->LuaEngineSubsystem->UObjectRegistry->AddLuaReferencedObjects(collector);
}
