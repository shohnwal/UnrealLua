// Fill out your copyright notice in the Description page of Project Settings.


#include "Debug/DebugTools/UnrealLuaDebugActorPickerTool.h"

#include "Debug/UnrealLuaDebug.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"

static FAutoConsoleCommand unrealLuaMemoryCheckConsoleCommand(
	TEXT("unreallua.debug.pickactor"),
	TEXT("Activate debug mode to pick an actor to watch.")
	TEXT("Usage: \"unreallua.debug.pickactor\", then left-click to select actor, right click to deactivate actor picking"),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		UUnrealLuaDebug* debug = UUnrealLuaDebug::Get();
		if (debug)
		{
			//debug->SetActiveTool(UUnrealLuaDebugActorPickerTool::StaticClass(), {});
		}
	})
);


void UUnrealLuaDebugActorPickerTool::InitializeTool_Implementation()
{
	Super::InitializeTool_Implementation();	
}

void UUnrealLuaDebugActorPickerTool::ActivateTool_Implementation(FInstancedStruct data)
{
	
}

void UUnrealLuaDebugActorPickerTool::DeactivateTool_Implementation()
{
	
}

void UUnrealLuaDebugActorPickerTool::NotifyInputKeyEvent_Implementation(const FKey& key, EInputEvent eventType, UGameInstance* gameInstance)
{
	if (!gameInstance)
	{
		return;
	}
	if (eventType == EInputEvent::IE_Pressed)
	{
		if (key == EKeys::LeftMouseButton)
		{
			APlayerController* pc = gameInstance->GetFirstLocalPlayerController();
			if (pc)
			{
				FHitResult hit;
				
				FVector traceStart = pc->PlayerCameraManager->GetCameraLocation();
				FVector cameraForward = pc->PlayerCameraManager->GetActorForwardVector();
				float distance = 1000000.f;
				if (pc->GetWorld()->LineTraceSingleByChannel(hit, traceStart, traceStart + cameraForward * distance, ECollisionChannel::ECC_Visibility))
				{
#if WITH_EDITOR
					UKismetSystemLibrary::DrawDebugPoint(pc, hit.Location, 10, FLinearColor::Green, 3);
#endif
					if(this->IsActorValidForPicker(hit.GetActor()))
					{
						this->UnrealLuaDebug->SetWatchedActor(hit.GetActor());
					}					
				}
				//if (pc->GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, hit))
				{

				}
			}

			this->UnrealLuaDebug->DeactivateTool(gameInstance, this->GetClass());
		}
		else if(key == EKeys::RightMouseButton)
		{
			this->UnrealLuaDebug->SetWatchedActor(nullptr);
			this->DeactivateTool();
			return;
		}
	}
}

bool UUnrealLuaDebugActorPickerTool::IsActorValidForPicker(AActor* actor)
{
	return true;
}
