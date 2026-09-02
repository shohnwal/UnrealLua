// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Reflection/PropertyMapping.h"
#include "UObject/Class.h"
#include "UnrealLuaOverrideUFunction.generated.h"

class UUnrealLuaOverrideFunctionHostClass;

UCLASS(Transient)
class UNREALLUA_API UUnrealLuaOverrideUFunction : public UFunction
{
	/**
 * 
 */

	virtual ~UUnrealLuaOverrideUFunction() override;
	GENERATED_BODY()

public:
	//ULuaFunction();
	//ULuaFunction(const FObjectInitializer& ObjectInitializer);
	//UserConstruction call for Actors, loads Lua script for ULuaScriptable Actors
	DECLARE_FUNCTION(execActorUserConstructionScriptLuaCall);

	//LuaScriptable AActor lua scripts get removed via UUnrealLuaGameWorldSubsystem::NotifyActorDestroyed
	//so they can go through their Endplay routine properly before LuaScript gets unloaded

	//Native implementations

	//Default Lua call for a native function
	DECLARE_FUNCTION(execNativeLuaCall);

	//Lua call for a native tick function
	DECLARE_FUNCTION(execNativeTickLuaCall);

	//BeginPlay call for native ULuaScriptable components that hold their own Lua scripts, loads Lua script
	DECLARE_FUNCTION(execNativeLuaScriptableComponentBeginPlayLuaCall);

	//EndPlay call for native ULuaScriptable components that hold their own Lua scripts
	DECLARE_FUNCTION(execNativeLuaScriptableComponentEndPlayLuaCall);

	//Construct call for ULuaScriptable UUSerwidgets, will load Lua script the first time this widget is added to a widget tree/viewport
	DECLARE_FUNCTION(execNativeLuaScriptableUserWidgetConstructLuaCall);

	DECLARE_FUNCTION(execNativeLuaScriptableUserWidgetDestructLuaCall);

	//Blueprint implementations

	//BeginPlay call for Blueprint ULuaScriptable components that hold their own Lua scripts, loads Lua script
	DECLARE_FUNCTION(execBlueprintLuaScriptableComponentBeginPlayLuaCall);
	//Construct call for ULuaScriptable UUSerwidgets, will load Lua script the first time this widget is added to a widget tree/viewport
	DECLARE_FUNCTION(execBlueprintLuaScriptableUserWidgetConstructLuaCall);
	DECLARE_FUNCTION(execBlueprintLuaScriptableUserWidgetDestructLuaCall);
	//EndPlay call for Blueprint ULuaScriptable components that hold their own Lua scripts
	DECLARE_FUNCTION(execBlueprintLuaScriptableComponentEndPlayLuaCall);
	//Default Lua call for a Blueprint function
	DECLARE_FUNCTION(execBlueprintLuaCall);
	//DECLARE_FUNCTION(execBlueprintProcessInternalLuaCall);
	//DECLARE_FUNCTION(execBlueprintLuaProcessInternal);
	//Lua call for a native tick function
	DECLARE_FUNCTION(execBlueprintTickLuaCall);
	DECLARE_FUNCTION(execBlueprintWidgetTickLuaCall);
	//Default Lua call for a Blueprint function in a ULuaScriptableSubobject
	//DECLARE_FUNCTION(execBlueprintSubobjectLuaCall);


	void Initialize();

	virtual void BeginDestroy() override;
	virtual void FinishDestroy() override;

	//	UPROPERTY()
	//	FName ScriptPath;

	virtual void Bind() override;

	UPROPERTY(VisibleAnywhere)
	bool bReplacedExistingFunc = false;
	UPROPERTY(VisibleAnywhere)
	FName OverriddenName = NAME_None;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UFunction> Overridden = nullptr;
	UPROPERTY(VisibleAnywhere)
	FName AssignedPathName = NAME_None;
	UPROPERTY(VisibleAnywhere)
	bool bCallOriginalNativeImplementationFunction = false;
};
