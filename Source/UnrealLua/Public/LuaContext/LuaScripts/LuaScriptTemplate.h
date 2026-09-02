// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "sol/sol.hpp"
#include "UObject/CoreNetTypes.h"
#include "LuaScriptTemplate.generated.h"

struct FLuaScriptInstanceHandle;

UENUM(Flags)
enum class ELuaScriptTemplateFlags : uint8
{
	HasTickFunction,
	StartWithTickEnabled,
	AutoRegisterReplicatedSubobject,
	OverrideInput
};
ENUM_CLASS_FLAGS(ELuaScriptTemplateFlags)

USTRUCT()
struct UNREALLUA_API FLuaScriptAttributes
{
	GENERATED_BODY()
	UPROPERTY()
	bool StartWithTickEnabled = true;
	UPROPERTY()
	bool OverrideInput = false;
	UPROPERTY()
	float ReplicationFrequency = 0.0f;
	UPROPERTY()
	bool AutoRegisterForReplicationInOuter = false;
	UPROPERTY()
	TEnumAsByte<ELifetimeCondition> ObjectReplicationCondition = ELifetimeCondition::COND_None;
};
USTRUCT()
struct UNREALLUA_API FLuaScriptTemplate
{
	GENERATED_BODY()
	FLuaScriptTemplate();
	explicit FLuaScriptTemplate(sol::table& mainScriptTable);
	bool IsLuaScriptValid() const;

	void LockTemplate();
	bool IsValid() const;
	bool ShouldOverrideInput() const;
	bool AutoRegisterReplicatedSubobject() const;
	bool HasTickFunction() const;
	bool StartWithTickEnabled() const;
	ELifetimeCondition GetObjectReplicationCondition() const;
	float GetReplicationFrequency() const;
	sol::table GetSubobjectOverrides() const;
	sol::table GetRepLayoutTable();
	sol::table GetScriptTable();
	//FLuaScriptInstanceHandle CreateInstance() const;
private:
	sol::object GetScriptValueInternal(const sol::object& key);
public:
	sol::object GetSubobjectScriptValue(FName subobjectName, const sol::object& key);

	TMap<FString, FString> LuaScriptToString();
private:
	sol::table LuaTable = {};
	sol::table SubobjectOverrides = {};
	sol::table RepLayoutTable = {};
	
	UPROPERTY(VisibleAnywhere)
	FLuaScriptAttributes ScriptAttributes;
	UPROPERTY(VisibleAnywhere)
	bool bHasTickFunction = false;
	friend struct FLuaScriptInstanceHandle;
};