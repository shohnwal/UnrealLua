// Fill out your copyright notice in the Description page of Project Settings.


#include "SubSystem/LocalPlayerLuaSubsystem.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"

bool ULocalPlayerLuaSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	ULocalPlayer* lp = CastChecked<ULocalPlayer>(Outer);
	UWorld* world = lp->GetWorld(); 
	return world && (world->WorldType == EWorldType::Game || world->WorldType == EWorldType::PIE);
}

void ULocalPlayerLuaSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UEnhancedInputLocalPlayerSubsystem* input = Collection.InitializeDependency<UEnhancedInputLocalPlayerSubsystem>();
	input->ControlMappingsRebuiltDelegate.AddDynamic(this, &ULocalPlayerLuaSubsystem::NotifyEnhancedInputMappingsChanged);
	
	Super::Initialize(Collection);
}

void ULocalPlayerLuaSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void ULocalPlayerLuaSubsystem::PlayerControllerChanged(APlayerController* newPC)
{
	Super::PlayerControllerChanged(newPC);
}

void ULocalPlayerLuaSubsystem::NotifyEnhancedInputMappingsChanged()
{
	APlayerController* pc = this->GetLocalPlayer()->PlayerController;
	if (pc)
	{
		FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(pc);
		item.RebuildInput();	
	}
}
