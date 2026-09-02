// Fill out your copyright notice in the Description page of Project Settings.


#include "Tools/ObjectInspector/UnrealLuaObjectInspectorTool.h"

#include "UnrealEngine.h"
#include "Session/UnrealLuaToolsSession.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Tools/ObjectInspector/SLuaScriptValueEditor.h"
#include "Tools/ObjectInspector/SUnrealLuaObjectInspector.h"
#include "UIWidgets/UnrealLuaUserWidget.h"
#include "UObjectRegistry/LuaUObjectItem.h"

void UUnrealLuaObjectInSpectorTool::InitializeTool()
{
	Super::InitializeTool();
	this->CursorWidget = CreateWidget<UUnrealLuaUserWidget>(this->GetSession()->GetGameInstance());
	this->CursorImage = this->CursorWidget->WidgetTree->ConstructWidget<UImage>();
	this->CursorWidget->WidgetTree->RootWidget = this->CursorImage;
	this->CursorImage->SetVisibility(ESlateVisibility::Collapsed);
	//this->CursorImage->SetDesiredSizeOverride(FVector2D(100,100));
	FSlateBrush brush = this->CursorImage->GetBrush();
	brush.DrawAs = ESlateBrushDrawType::RoundedBox;
	brush.SetImageSize(FVector2D(100.f,100.f));
	this->CursorImage->SetBrush(brush);
	
	//this->CursorWidget->SetDesiredSizeInViewport(this->CursorImage->GetDesiredSize());
	this->CursorWidget->AddToViewport(500);
	this->CursorWidget->SetAlignmentInViewport(FVector2D(0.5));
	this->CursorWidget->SetPositionInViewport(FVector2D(100.f,100.f));
	
	this->ObjectWatcherWidget = SNew(SUnrealLuaObjectInspector)
		.OnRequestActorSelection_Lambda([this]{ this->SetToolActive(true);})
		.Visibility(EVisibility::Collapsed)
		.Session(this->SessionInfo.Get())
		.HostCanvas(this->SessionInfo->MainCanvas)
		.Title("Lua Object Watcher")
		.ExternalWindowAnchors(FAnchors{0.f,0.f,1.f,1.0f})
		.GameScreenAnchors(FAnchors(1.f,0.f,1.f,1.0f))
		.GameScreenAlignment(FVector2D(1.f,1.f))
		.ExternalWindowSize(FVector2D{0.3f, 0.5f})
		.ExternalWindowPosition(FVector2D{0.5f, 0.5f})
		.InitiallyHidden(true)
		.StartAsWindow(false)
		;
		
	this->SetWatchedObject(nullptr);
}

void UUnrealLuaObjectInSpectorTool::ActivateTool(const FUnrealLuaTooleActivateCallback& preActivateCallback)
{
	Super::ActivateTool(preActivateCallback);
	this->CursorWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
}

int32 UUnrealLuaObjectInSpectorTool::GetToolMainMenuSortOrder() const
{
	return 2;
}

FString UUnrealLuaObjectInSpectorTool::GetToolMainMenuButtonLabel() const
{
	return "Actor Selector";
}

void UUnrealLuaObjectInSpectorTool::NotifyAddedToMainMenu()
{
	Super::NotifyAddedToMainMenu();
}

FReply UUnrealLuaObjectInSpectorTool::NotifyInputKeyEvent(const FKey& key, EInputEvent eventType, UGameInstance* gameInstance)
{
	if (key.IsMouseButton() && key == EKeys::LeftMouseButton && eventType == EInputEvent::IE_Pressed)
	{
		APlayerController* pc = gameInstance->GetFirstLocalPlayerController();
		if (pc)
		{	FVector2D MousePosition;
			if (this->SessionInfo->GameViewportClient->GetMousePosition(MousePosition))
			{
				FHitResult hitResult;
				if (pc->GetHitResultAtScreenPosition(MousePosition, ECollisionChannel::ECC_Visibility, false, hitResult))
				{
					this->SetWatchedObject(hitResult.GetActor());
				}
			}
		}
		this->SetToolActive(false);
		return FReply::Handled();
	}
	else if (key == EKeys::BackSpace)
	{
		this->SetToolActive(false);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void UUnrealLuaObjectInSpectorTool::DeactivateTool()
{
	Super::DeactivateTool();
	this->CursorWidget->SetVisibility(ESlateVisibility::Collapsed);
}

void UUnrealLuaObjectInSpectorTool::Shutdown()
{
	this->CursorImage->RemoveFromParent();
	this->CursorWidget->RemoveFromParent();
	if (this->ObjectWatcherWidget.IsValid())
	{
		this->ObjectWatcherWidget->Shutdown();
	}
	this->ObjectWatcherWidget = nullptr;
	Super::Shutdown();
}

void UUnrealLuaObjectInSpectorTool::NotifyMainMenuButtonClicked_Implementation()
{
	this->ObjectWatcherWidget->ToggleVisibility();
	if (this->ObjectWatcherWidget->GetVisibility() == EVisibility::Collapsed)
	{
		this->DeactivateTool();
	}
}

void UUnrealLuaObjectInSpectorTool::Tick(float dt)
{
	FVector2D mousePos = UWidgetLayoutLibrary::GetMousePositionOnViewport(this->SessionInfo->GetGameInstance());
	this->CursorWidget->SetPositionInViewport(mousePos, false);
}

bool UUnrealLuaObjectInSpectorTool::RequiresTick_Implementation()
{
	return true;
}

void UUnrealLuaObjectInSpectorTool::SetWatchedObject(UObject* watchedObject)
{
	this->ObjectWatcherWidget->SetWatchedUObject(watchedObject);
}
