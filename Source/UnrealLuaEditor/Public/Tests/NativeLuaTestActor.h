// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/LuaScriptable.h"
#include "NativeLuaTestActor.generated.h"

UCLASS()
class UNREALLUAEDITOR_API ANativeLuaTestActor : public AActor, public ILuaScriptable
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ANativeLuaTestActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION(BlueprintImplementableEvent)
	void BlueprintImplementable();

	UFUNCTION(BlueprintNativeEvent)
	void BlueprintNative();
	virtual void BlueprintNative_Implementation();
};
