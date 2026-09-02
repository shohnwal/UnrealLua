// Fill out your copyright notice in the Description page of Project Settings.


#include "BlueprintSupport/UnrealLuaGameplayStatics.h"

#include "Templates/SubclassOf.h"
#include "Engine/World.h"
#include "UnrealLua.h"
#include "sol/sol.hpp"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"
#include "ScriptableUObject/LuaUserWidget.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"

void StaticsInitFunc(UObject* obj, FLuaValue& initVal)
{
	
	if(obj && initVal.IsType<FLuaFunctionHandle>())
	{
		sol::protected_function func = initVal.Get<FLuaFunctionHandle>().GetFunction();
		if (func.valid())
		{
			func(obj);		
		}
	}
	else if(obj && initVal.IsType<FLuaTableHandle>())
	{
		sol::table inittbl = initVal.Get<FLuaTableHandle>().GetTable();
		if (inittbl.valid())
		{
			UnrealLua::PropertyHelper::InitializeUObjectFromTable(obj, inittbl);
			sol::object initFunc_o = inittbl["__Init"];
			if(initFunc_o.is<sol::function>())
			{
				sol::protected_function func = initFunc_o.as<sol::function>();
				func(obj);
			}	
		}
	} 
}

AActor* UUnrealLuaGameplayStatics::SpawnActor(UObject* worldContext, TSubclassOf<AActor> actorClass, FLuaValue init, FTransform spawnTransform, AActor* owner, APawn* instigator, ESpawnActorCollisionHandlingMethod spawnmethod)
{
	if(!worldContext || !actorClass)
	{
		return nullptr;
	}
	UWorld* world = worldContext->GetWorld();
	if(!world)
	{
		return nullptr;
	}
	AActor* newActor = world->SpawnActorDeferred<AActor>(actorClass, spawnTransform, owner, instigator, spawnmethod);
	if(newActor)
	{
		StaticsInitFunc(newActor, init);
		newActor->FinishSpawning(spawnTransform, true);
	}

	return newActor;
}

UObject* UUnrealLuaGameplayStatics::NewObject(TSubclassOf<UObject> clazz, FLuaValue init, UObject* outer, FName name, UObject* templat)
{
	if(!clazz)
	{
		return nullptr;
	}
	if(clazz->IsChildOf<AActor>())
	{
		LUA_LOG_WARNING("Can't create actors via NewObject, please use SpawnActor!")
		return nullptr;
	}
	if(outer == nullptr)
	{
		outer = GetTransientPackage();
	}
	UObject* obj = ::NewObject<UObject>(outer, clazz, name, RF_NoFlags, templat);
	
	if(!obj)
	{
		return nullptr;
	}

	StaticsInitFunc(obj, init);
	return obj;
}

bool UUnrealLuaGameplayStatics::RenameObject(UObject* obj, FString newName, UObject* newOuter)
{
	if(!obj)
	{
		return false;
	}
	if(newName.IsEmpty())
	{
		return obj->Rename(nullptr, newOuter);	
	}
	else
	{
		return obj->Rename(*newName, newOuter);
	}
}

UWidget* UUnrealLuaGameplayStatics::CreateWidget(TSubclassOf<UWidget> widgetClass, FLuaValue initializer, UObject* outerObj, const FString luaPathOverride, const FString name)
{
	if(widgetClass == nullptr || !outerObj)
	{
		return nullptr;
	}
	
	LUA_LOG("Trying to create Widget %s", *widgetClass->GetName())
	
	if(outerObj == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("No valid outer given to create widget"));
		return nullptr;
	}
	FName widgetName = *name;
	
	UWidget* newWidget = nullptr;
	UClass* clazz = widgetClass;
	
	if (UUserWidget* outerUserWidget = Cast<UUserWidget>(outerObj))
	{
		if (clazz->IsChildOf(UUserWidget::StaticClass()))
		{
			newWidget = outerUserWidget->WidgetTree->ConstructWidget<UUserWidget>(clazz, widgetName);
		}
		else if (clazz->IsChildOf<UWidget>())
		{
			newWidget = outerUserWidget->WidgetTree->ConstructWidget<UWidget>(clazz, widgetName);
		}
	}
	else if(clazz->IsChildOf(UUserWidget::StaticClass()))
	{
		if(outerObj->IsA<UWidget>())
		{
			newWidget = ::CreateWidget<UUserWidget>(Cast<UWidget>(outerObj), clazz, widgetName);
		}
		else if(outerObj->IsA<APlayerController>())
		{
			newWidget = ::CreateWidget<UUserWidget>(Cast<APlayerController>(outerObj), clazz, widgetName);
		}
		else if(outerObj->IsA<UGameInstance>())
		{
			newWidget = ::CreateWidget<UUserWidget>(Cast<UGameInstance>(outerObj), clazz, widgetName);
		}
		else if (outerObj->IsA<UWorld>())
		{
			newWidget = ::CreateWidget<UUserWidget>(Cast<UWorld>(outerObj), clazz, widgetName);
		}
		
		if(newWidget != nullptr && newWidget->IsA<ULuaUserWidget>() && !luaPathOverride.IsEmpty())
		{
			ULuaUserWidget* luaWidget = Cast<ULuaUserWidget>(newWidget);
			luaWidget->LuaScriptSettings.ScriptPathOverride = luaPathOverride;
			luaWidget->BeginPlayNative();
		}
	}
	return newWidget;
}
