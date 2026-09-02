// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Subsystems/LocalPlayerSubsystem.h"
#include "CoreMinimal.h"
#include "LuaContext/LuaContextHelper.h"
#include "LocalPlayerLuaSubsystem.generated.h"

/**
 * 
 */
class ULocalPlayerLuaSubsystem;
enum class ELuaLoadEventType : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLocalPlayerLuaReloadEvent, ULocalPlayerLuaSubsystem*, subsystem, ELuaLoadEventType, luaEventType);
UCLASS(Transient)
class UNREALLUA_API ULocalPlayerLuaSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void PlayerControllerChanged(APlayerController* newPC) override;
	
	UFUNCTION()
	void NotifyEnhancedInputMappingsChanged();
};
