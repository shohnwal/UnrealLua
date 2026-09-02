// Fill out your copyright notice in the Description page of Project Settings.


#include "Tests/DerivedNativeLuaTestActor.h"

#include "Utility/LuaLogMacros.h"


// Sets default values
ADerivedNativeLuaTestActor::ADerivedNativeLuaTestActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ADerivedNativeLuaTestActor::BeginPlay()
{
	LUA_LOG("ADerivedNativeLuaTestActor::BeginPlay")
	Super::BeginPlay();
	LUA_LOG("ADerivedNativeLuaTestActor::post BeginPlay")
}

// Called every frame
void ADerivedNativeLuaTestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ADerivedNativeLuaTestActor::BlueprintNative_Implementation()
{
	LUA_LOG("ADerivedNativeLuaTestActor called BlueprintNative_Implementation")
	Super::BlueprintNative_Implementation();
}

