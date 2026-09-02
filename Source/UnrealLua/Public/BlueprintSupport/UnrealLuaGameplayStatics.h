// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaValue/LuaValue.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UnrealLuaGameplayStatics.generated.h"

UENUM()
enum class ETestEmptyEnum : uint8
{
};

UENUM(BlueprintType)
enum class ETestSingleEnum : uint8
{
	Entry
};

enum class ESpawnActorCollisionHandlingMethod : uint8;
/**
 * 
 */
UCLASS()
class UNREALLUA_API UUnrealLuaGameplayStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION()
	static AActor* SpawnActor(UObject* worldContext, TSubclassOf<AActor> actorClass, FLuaValue init, FTransform spawnTransform, AActor* owner, APawn* instigator, ESpawnActorCollisionHandlingMethod spawnmethod);

	UFUNCTION()
	static UObject* NewObject(TSubclassOf<UObject> clazz, FLuaValue init, UObject* outer, FName name, UObject* templat);
	
	UFUNCTION()
	static bool RenameObject(UObject* obj, FString newName, UObject* newOuter);
	
	UFUNCTION()
	static UWidget* CreateWidget(TSubclassOf<UWidget> widgetClass, FLuaValue initializer, UObject* outerObj, const FString luaPathOverride, const FString name);
};
