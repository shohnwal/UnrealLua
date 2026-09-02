// Fill out your copyright notice in the Description page of Project Settings.


#include "Tests/NativeLuaTestActor.h"

#include "Utility/LuaLogMacros.h"


// Sets default values
ANativeLuaTestActor::ANativeLuaTestActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ANativeLuaTestActor::BeginPlay()
{
	LUA_LOG("ANativeLuaTestActor::BeginPlay")
	Super::BeginPlay();
	LUA_LOG("ANativeLuaTestActor::BeginPlay calling BlueprintImplementable")
	this->BlueprintImplementable();
	LUA_LOG("ANativeLuaTestActor::BeginPlay calling BlueprintNative")
	this->BlueprintNative();
	LUA_LOG("ANativeLuaTestActor::post BeginPlay")
}

// Called every frame
void ANativeLuaTestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ANativeLuaTestActor::BlueprintNative_Implementation()
{
	LUA_LOG("ANativeLuaTestActor called BlueprintNative_Implementation")
}

