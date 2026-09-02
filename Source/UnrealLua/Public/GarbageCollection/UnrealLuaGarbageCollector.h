// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UnrealLuaGarbageCollector.generated.h"

struct FLuaGCObject;
class UUnrealLuaEngineSubsystem;
/**
 * 
 */
USTRUCT()
struct UNREALLUA_API FUnrealLuaGarbageCollector
{
	GENERATED_BODY()
	
	void Initialize(UUnrealLuaEngineSubsystem* ss);
	void PreDeinitialize();
	void Deinitialize();
	void NotifyPreGarbageCollection();
	void NotifyPreGarbageCollectConditionalBeginDestroy();
	void NotifyPostGarbageCollectConditionalBeginDestroy();
	bool ProcessPostDestroyGCInvalidUObjectCollection();
	void NotifyPostGarbageCollection();
	
	void NotifyEndFrame(UUnrealLuaEngineSubsystem* ss);
	
	static void RegisterLuaGCObject(FLuaGCObject* gcobj);
	static void UnregisterLuaGCObject(FLuaGCObject* gcobj);
	
	void AddReferencedLuaObjects(FReferenceCollector& collector);

	UPROPERTY(VisibleAnywhere)
	UUnrealLuaEngineSubsystem* LuaEngineSubsystem = nullptr;
	
	TArray<FLuaGCObject*> LuaGCObjects = {};
};
