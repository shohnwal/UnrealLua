// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeLuaTestActor.h"
#include "DerivedNativeLuaTestActor.generated.h"

UCLASS()
class UNREALLUAEDITOR_API ADerivedNativeLuaTestActor : public ANativeLuaTestActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ADerivedNativeLuaTestActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	virtual void BlueprintNative_Implementation() override;
	
};
