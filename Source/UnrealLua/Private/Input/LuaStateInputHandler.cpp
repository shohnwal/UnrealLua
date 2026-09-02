// Fill out your copyright notice in the Description page of Project Settings.


#include "Input/LuaStateInputHandler.h"

#include "GameFramework/Actor.h"
#include "UnrealLua.h"
#include "sol/sol.hpp"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SceneViewport.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"

FUnrealLuaInputKeyBinding& FUnrealLuaInputKeyBindingCollection::FindOrAddBinding(const FKey& key, const FString& id)
{
	for (auto& binding : this->Bindings)
	{
		if (binding.Key == key && binding.BindingID == id)
		{
			return binding;
		}
	}
	return this->Bindings.Emplace_GetRef(0, id, key);
}

FUnrealLuaInputKeyBinding* FUnrealLuaInputKeyBindingCollection::FindBinding(const FString& bindingID)
{	
	for (auto& binding : this->Bindings)
	{
		if (binding.BindingID == bindingID)
		{
			return &binding;
		}
	}
	return nullptr;
}

uint64 FUnrealLuaInputKeyBindingCollection::UnbindBinding(const FString& bindingID)
{
	uint64 handle = 0;
	for (TArray<FUnrealLuaInputKeyBinding>::TIterator it =  this->Bindings.CreateIterator(); it; ++it)
	{
		if (it->BindingID == bindingID)
		{
			handle = it->BindingHandle;
			it.RemoveCurrent();
			break;
		}
	}
	return handle;
}

bool FUnrealLuaInputKeyBindingCollection::UnbindBinding(uint64 handle)
{
	for (TArray<FUnrealLuaInputKeyBinding>::TIterator it =  this->Bindings.CreateIterator(); it; ++it)
	{
		if (it->BindingHandle == handle)
		{
			handle = it->BindingHandle;
			it.RemoveCurrent();
			return true;
		}
	}
	return false;
}

bool FUnrealLuaInputKeyBindingCollection::IsEmpty()
{
	return Bindings.IsEmpty();
}

ULuaStateInputHandler::ULuaStateInputHandler()
{
	if(this->HasAnyFlags(EObjectFlags::RF_ClassDefaultObject))
	{
		return;
	}
	verify(UUnrealLuaEngineSubsystem::IsGameSessionActive())
	
	this->TriggerEventEnum = FindObjectChecked<UEnum>(nullptr, TEXT("/Script/EnhancedInput.ETriggerEvent"));
	this->InputEventEnum = FindObjectChecked<UEnum>(nullptr, TEXT("/Script/Engine.EInputEvent"));
}

void ULuaStateInputHandler::BeginDestroy()
{
	if (!this->IsTemplate())
	{
		LUA_LOG("ULuaStateInputHandler::BeginDestroy")
	}
	Super::BeginDestroy();
}

uint64 ULuaStateInputHandler::BindKeyEvent(const FKey keyCombination, const FLuaFunctionHandle& function, const FString& bindingID)
{
	FKey key = keyCombination;
	
	TArray<FKey> modifierKeys {};

	if (!key.IsValid())
	{
		if (key.GetFName() == NAME_None)
		{
			return 0;
		}
		FString comboKeysString = key.ToString();
		TArray<FString> comboKeysStrings{};
		comboKeysString.ParseIntoArrayWS(comboKeysStrings, TEXT("+"));

		Algo::Transform(comboKeysStrings, modifierKeys, [](const FString& input)
		{
			return FKey(*input);
		});

		if (!modifierKeys.IsEmpty())
		{
			key = modifierKeys.Pop();
		}
	}
	
	if (!key.IsValid() || !function.IsValid())
	{
		return 0;
	}
	
	UWorld* world = this->GetWorld();
	
	UGameViewportClient* viewport = world->GetGameViewport();

	FModifierKeysState modifierKeysState = FSlateApplication::Get().GetModifierKeys();
	
	FUnrealLuaInputKeyBindingCollection& bindingCollection = this->KeyToBindingsMap.FindOrAdd(key);

	FUnrealLuaInputKeyBinding& binding = bindingCollection.FindOrAddBinding(key, bindingID);
	binding.Callback = function;
	
	if (binding.BindingHandle == 0)
	{
		FDelegateHandle delegateHandle = viewport->OnInputKey().AddWeakLambda(this, [this, bindingID](const FInputKeyEventArgs& inputEvent)
		{
			this->NotifyInputKeyEvent(inputEvent, bindingID);
		});
		binding.BindingHandle = std::bit_cast<uint64>(delegateHandle);
	}
	
	verify(binding.BindingHandle != 0);
	
	this->HandleToKeyMap.Add(binding.BindingHandle, key);
	
	return binding.BindingHandle;
}

bool ULuaStateInputHandler::UnbindKeyEventByHandle(uint64 handle)
{
	if (handle == 0)
	{
		return false;
	}
	
	FKey key = {};
	
	if (this->HandleToKeyMap.RemoveAndCopyValue(handle, key))
	{
		FUnrealLuaInputKeyBindingCollection* foundCallbackCollection = this->KeyToBindingsMap.Find(key);
		if (!foundCallbackCollection)
		{
			return false;
		}
		bool removed = foundCallbackCollection->UnbindBinding(handle);
		
		if (foundCallbackCollection->IsEmpty())
		{
			this->KeyToBindingsMap.Remove(key);
		}
	}
	
	UWorld* world = this->GetWorld();
	
	UGameViewportClient* viewport = world->GetGameViewport();

	FDelegateHandle delegateHandle = std::bit_cast<FDelegateHandle>(handle);
	
	return viewport->OnInputKey().Remove(delegateHandle);
}

bool ULuaStateInputHandler::UnbindKeyEvent(const FKey key, const FString& bindingID)
{
	FUnrealLuaInputKeyBindingCollection* foundCallbackCollection = this->KeyToBindingsMap.Find(key);
	if (!foundCallbackCollection)
	{
		return false;
	}
	uint64 handle = foundCallbackCollection->UnbindBinding(bindingID);
	if (handle == 0)
	{
		return false;
	}
	this->HandleToKeyMap.Remove(handle);
	
	UWorld* world = this->GetWorld();
	
	UGameViewportClient* viewport = world->GetGameViewport();
	
	FDelegateHandle delegateHandle = std::bit_cast<FDelegateHandle>(handle);
	
	if (foundCallbackCollection->IsEmpty())
	{
		this->KeyToBindingsMap.Remove(key);
	}
	return viewport->OnInputKey().Remove(delegateHandle);
}

void ULuaStateInputHandler::PressKey(FKey key, bool holdKey)
{
	if (!key.IsValid())
	{
		return;
	}
	LUA_LOG("ULuaStateKeyInputHandler::PressKey : %s pressed", *key.ToString());
	UWorld* world = this->GetWorld();
	UGameViewportClient* viewport = world->GetGameViewport();
	FInputDeviceId dev = IPlatformInputDeviceMapper::Get().GetDefaultInputDevice();
	
	FInputKeyEventArgs args {viewport->GetGameViewport(),dev, key, EInputEvent::IE_Pressed,FPlatformTime::Cycles64()};
	this->GetWorld()->GetGameViewport()->InputKey(args);
	if (!holdKey)
	{
		this->ReleaseKey(key);
	}
}

void ULuaStateInputHandler::ReleaseKey(FKey key)
{
	if (!key.IsValid())
	{
		return;
	}
	LUA_LOG("ULuaStateKeyInputHandler::PressKey : %s released", *key.ToString());
	UWorld* world = this->GetWorld();
	UGameViewportClient* viewport = world->GetGameViewport();
	FInputDeviceId dev = IPlatformInputDeviceMapper::Get().GetDefaultInputDevice();
	
	FInputKeyEventArgs args {viewport->GetGameViewport(),dev, key, EInputEvent::IE_Released,FPlatformTime::Cycles64()};
	this->GetWorld()->GetGameViewport()->InputKey(args);
}


void ULuaStateInputHandler::NotifyInputKeyEvent(const FInputKeyEventArgs& inputEvent, const FString& bindingID)
{
	//inputEvent.Event
	FUnrealLuaInputKeyBindingCollection* foundCallbackCollection = this->KeyToBindingsMap.Find(inputEvent.Key);
	if (!foundCallbackCollection)
	{
		return;
	}
	auto* foundBinding = foundCallbackCollection->FindBinding(bindingID);
	if (!foundBinding)
	{
		return;
	}
	if (!foundBinding->Callback.IsValid())
	{
		return;
	}
	[[maybe_unused]] bool success = foundBinding->Callback.CallFunction({});
}