#include "Session/UnrealLuaToolsSession.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Config/UnrealLuaConfig.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "LuaContext/GameLuaContext.h"
#include "MainMenu/SUnrealLuaMainMenu.h"
#include "Tools/ObjectInspector/SUnrealLuaObjectInspector.h"
#include "Tools/UnrealLuaTool.h"
#include "UIWidgets/SMultiLineEditableTextBoxEx.h"
#include "UIWidgets/Draggable/SUnrealLuaDraggableBoxOverlay.h"
#include "Utility/LuaLogMacros.h"
#include "Widgets/Layout/SConstraintCanvas.h"

void UUnrealLuaToolsSession::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	FWorldDelegates::OnStartGameInstance.AddUObject(this, &UUnrealLuaToolsSession::NotifyGameInstanceStart);
}

void UUnrealLuaToolsSession::NotifyGameInstanceStart(UGameInstance* gameInstance)
{
	if (gameInstance != this->GetGameInstance())
	{
		return;
	}
	FWorldDelegates::OnStartGameInstance.RemoveAll(this);
	
	this->GameLuaContext = UGameLuaContext::Get(this);
	
	UGameViewportClient* viewport = this->GetGameInstance()->GetGameViewportClient();
	verify(IsValid(viewport))
	this->GameViewportClient = viewport;
	
	
	this->DebugKey = UUnrealLuaConfig::GetCachedSettings().UnrealLuaToolsMenuKey;
	
	viewport->OnInputKey().AddUObject(this, &UUnrealLuaToolsSession::NotifyInputKeyEvent);

	this->MainCanvas = SNew(SConstraintCanvas);
	this->GameViewportClient->AddViewportWidgetContent(this->MainCanvas.ToSharedRef());
	
	this->MainMenu = SNew(SUnrealLuaMainMenu)
		.Title("Unreal Lua Tools")
		.InitiallyHidden(true)
		.StartAsWindow(false)
		.ExternalWindowSizingRule(ESizingRule::Autosized)
		.Session(this)
		.GameScreenAnchors(FAnchors{0,0.0f,0.0f,1.1f})
	;
	UUnrealLuaTools* unrealLuaTools = UUnrealLuaTools::Get();
	
	//Create a new set of tools for each game instance
	for (UClass* toolClass : unrealLuaTools->RegisteredTools)
	{
		UUnrealLuaTool* newTool = NewObject<UUnrealLuaTool>(this, toolClass, toolClass->GetFName(), RF_Transient);
		this->Tools.Emplace(toolClass, newTool);
		this->MainMenu->AddTool(newTool);
	}
		
	for (TTuple<TObjectPtr<UClass>, TObjectPtr<UUnrealLuaTool>>& newTool : this->Tools)
	{
		newTool.Value->InitializeTool();
	}
	
	bool requireTick = false;
	for (TTuple<TObjectPtr<UClass>, TObjectPtr<UUnrealLuaTool>> tool : this->Tools)
	{
		if (tool.Value->RequiresTick())
		{
			requireTick = true;
			break;
		}
	}
	if (requireTick)
	{
		this->GameViewportClient->OnTick().AddUObject(this, &UUnrealLuaToolsSession::Tick);
	}
}

void UUnrealLuaToolsSession::Deinitialize()
{
	if (this->CurrentTool)
	{
		this->CurrentTool->DeactivateTool();
	}
	for (auto tool : this->Tools)
	{
		tool.Value->Shutdown();
	}
	this->MainMenu->Shutdown();
	for (auto tool : this->Tools)
	{
		tool.Value->ConditionalBeginDestroy();
	}
	this->Tools.Empty();
	
	Super::Deinitialize();
}

void UUnrealLuaToolsSession::SetActiveTool(UUnrealLuaTool* tool, const FUnrealLuaTooleActivateCallback& preActivateCallback)
{
	this->SetActiveTool(tool->GetClass(), preActivateCallback);
}

void UUnrealLuaToolsSession::SetActiveTool(TSubclassOf<UUnrealLuaTool> toolClass, const FUnrealLuaTooleActivateCallback& preActivateCallback)
{
	if (!toolClass)
	{
		this->DeactivateCurrentToolInternal();
		return;
	}
	
	if (this->CurrentTool)
	{
		this->DeactivateCurrentToolInternal();
	}
	
	TObjectPtr<UUnrealLuaTool> debugTool = this->Tools.FindChecked(toolClass);
	this->CurrentTool = debugTool;
	
	this->CurrentTool->ActivateTool(preActivateCallback); 
}

void UUnrealLuaToolsSession::DeactivateTool(UUnrealLuaTool* toolToDeactivate)
{
	this->DeactivateTool(toolToDeactivate->GetClass());
}

void UUnrealLuaToolsSession::DeactivateTool(TSubclassOf<UUnrealLuaTool> debugToolClass)
{
	if (!debugToolClass)
	{
		this->DeactivateCurrentToolInternal();
		return;
	}
	if (this->CurrentTool)
	{
		if (this->CurrentTool->GetClass() == debugToolClass)
		{
			this->DeactivateCurrentToolInternal();
		}
	}
}

bool UUnrealLuaToolsSession::IsCurrentTool(TSubclassOf<UUnrealLuaTool> debugToolClass) const
{
	return (!debugToolClass && !this->CurrentTool) || (this->CurrentTool && this->CurrentTool->GetClass() == debugToolClass);
}

bool UUnrealLuaToolsSession::IsCurrentTool(const UUnrealLuaTool* querier) const
{
	return querier && this->IsCurrentTool(querier->GetClass());
}

bool UUnrealLuaToolsSession::ToggleTool(TSubclassOf<UUnrealLuaTool> debugToolClass)
{
	if (this->IsCurrentTool(debugToolClass))
	{
		this->DeactivateTool(debugToolClass);
		return false;
	}
	else
	{
		this->SetActiveTool(debugToolClass);
		return true;
	}
}

void UUnrealLuaToolsSession::DeactivateCurrentToolInternal()
{
	if (this->CurrentTool)
	{
		this->CurrentTool->DeactivateTool();
	}
	this->CurrentTool = nullptr;	
}

UGameLuaContext* UUnrealLuaToolsSession::GetLuaContext() const
{
	return this->GameLuaContext;
}

TSharedPtr<FScopedLuaContext> UUnrealLuaToolsSession::GetScopedLuaContext() const
{
	return this->GameLuaContext->GetScopedLuaContextSharedPtr();
}

void UUnrealLuaToolsSession::NotifyInputKeyEvent(const FInputKeyEventArgs& inputEvent)
{
	FKey key = inputEvent.Key;
	EInputEvent eventType = inputEvent.Event;
	
	if (eventType != EInputEvent::IE_Pressed)
	{
		return;
	}
	
	FReply reply = FReply::Unhandled();
	
	this->OnInputKeyEvent.Broadcast(inputEvent);
	
	//if (this->CurrentTool)
	//{
	//	reply = this->CurrentTool->NotifyInputKeyEvent(key, eventType, this->GetGameInstance());
	//}
	////@TODO : Use ULuaConfig::DebugKey
	//if (!reply.IsEventHandled() && key == this->DebugKey)
	//{
	//	this->MainMenu->ToggleVisibility();
	//}
}

void UUnrealLuaToolsSession::Tick(float dt)
{
	if (this->CurrentTool)
	{
		this->CurrentTool->Tick(dt);
	}
}

TSharedPtr<SConstraintCanvas> UUnrealLuaToolsSession::GetCanvas() const
{
	return this->MainCanvas;
}

UUnrealLuaTool* UUnrealLuaToolsSession::GetTool(TSubclassOf<UUnrealLuaTool> toolClass) const
{
	return this->Tools.FindRef(toolClass);
}

void UUnrealLuaToolsSession::UpdateInputMode()
{
	this->InputModeOverriders.RemoveAll([](const TWeakPtr<SWidget>& widget) { return !widget.IsValid();});

	APlayerController* pc = this->GetGameInstance()->GetFirstLocalPlayerController();
	
	if (!pc)
	{
		return;
	}
	
	if (this->InputModeOverriders.IsEmpty())
	{
		UWidgetBlueprintLibrary::SetInputMode_GameOnly(pc);
		pc->SetShowMouseCursor(false);	
		return;
	}
	
	EDockableWindowWidgetInputMode inputMode = EDockableWindowWidgetInputMode::GameAndUI;
	for (TWeakPtr<SGamescreenDockableWindowWidget> widget : this->InputModeOverriders)
	{
		EDockableWindowWidgetInputMode widgetinputMode = widget.Pin()->GetViewportInputMode();
		if (widgetinputMode == EDockableWindowWidgetInputMode::UIOnly)
		{
			inputMode = EDockableWindowWidgetInputMode::UIOnly;
			break;
		}
	}
	
	if (inputMode == EDockableWindowWidgetInputMode::UIOnly)
	{
		UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(pc, nullptr, EMouseLockMode::DoNotLock);
		pc->SetShowMouseCursor(true);
		return;
	}
	else if (inputMode == EDockableWindowWidgetInputMode::GameAndUI)
	{
		UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(pc, nullptr, EMouseLockMode::DoNotLock);
		pc->SetShowMouseCursor(true);
	}
}

void UUnrealLuaToolsSession::AddInputModeOverride(TSharedRef<SGamescreenDockableWindowWidget> widget)
{
	this->InputModeOverriders.AddUnique(widget);
	this->UpdateInputMode();
}

void UUnrealLuaToolsSession::RemoveInputModeOverride(TSharedRef<SGamescreenDockableWindowWidget> widget)
{
	this->InputModeOverriders.Remove(widget);
	this->UpdateInputMode();
}

UGameViewportClient* UUnrealLuaToolsSession::GetViewportClient() const
{
	return this->GameViewportClient.Get();
}

UGameInstance* UUnrealLuaToolsSession::GetGameInstance() const
{
	return this->GetOuterUGameInstance();
}

FOnInputKeySignature& UUnrealLuaToolsSession::GetOninputKeyEvent()
{
	return this->OnInputKeyEvent;
}
