// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystem/UnrealLuaTools.h"
#if WITH_EDITOR
#include "Editor.h"
#endif
#include "Session/UnrealLuaToolsSession.h"
#include "Blueprint/UserWidget.h"
#include "Components/Viewport.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Interface/LuaScriptable.h"
#include "Misc/CoreDelegates.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Session/UnrealLuaToolsEditorSession.h"
#include "Tools/ObjectInspector/SUnrealLuaObjectInspector.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"
#include "Subsystems/SubsystemCollection.h"
#include "Tools/UnrealLuaTool.h"
#include "ToolWidgets/SLuaNewDefaultScriptEditorWindow.h"
#include "Tools/LuaScriptEditor/SLuaScriptEditor.h"
#include "Tools/ObjectInspector/SLuaScriptValueEditor.h"
#include "Utility/WindowUIUtility.h"
#include "Widgets/Notifications/SNotificationList.h"

namespace UnrealLua
{
	UUnrealLuaTools* GUnreaLLuaToolsEngineSubsystem = nullptr;
}



UUnrealLuaTools* UUnrealLuaTools::Get()
{
	return UnrealLua::GUnreaLLuaToolsEngineSubsystem;
}

void UUnrealLuaTools::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UUnrealLuaEngineSubsystem>();
	UnrealLua::GUnreaLLuaToolsEngineSubsystem = this;
	FCoreDelegates::OnAllModuleLoadingPhasesComplete.AddUObject(this, &UUnrealLuaTools::NotifyAllModulesLoaded);
	
}

void UUnrealLuaTools::Deinitialize()
{
	UnrealLua::GUnreaLLuaToolsEngineSubsystem = nullptr;
	Super::Deinitialize();
}

void UUnrealLuaTools::NotifyAllModulesLoaded()
{
	//All modules are loaded, now go look for any debug tools
	this->RegisteredTools.Empty();
	UClass* baseToolClass = UUnrealLuaTool::StaticClass();
	for (TObjectIterator<UClass> classIt; classIt; ++classIt)
	{
		if (classIt->IsChildOf(baseToolClass) && *classIt != baseToolClass)
		{
			this->RegisteredTools.Emplace(*classIt);
		}
	}
	
	this->RegisteredTools.Sort([](const UClass& a, const UClass& b)
	{
		return a.GetDefaultObject<UUnrealLuaTool>()->GetToolMainMenuSortOrder() < b.GetDefaultObject<UUnrealLuaTool>()->GetToolMainMenuSortOrder(); 
	});
#if WITH_EDITOR
	this->EditorSession = NewObject<UUnrealLuaToolsEditorSession>(this);
	this->EditorSession->Initialize();
#endif
}

#if WITH_EDITOR
TSharedPtr<SLuaScriptEditor> UUnrealLuaTools::OpenLuaScriptEditorInUnrealEditor()
{
	if (this->UnrealEditorLuaScriptEditor.IsValid())
	{
		TSharedPtr<SLuaScriptEditor> editor = this->UnrealEditorLuaScriptEditor.Pin(); 
		TSharedPtr<SWindow> window = editor->TryGetParentWindow();
		if (window)
		{
			window->FlashWindow();
			window->BringToFront();
		}
		return editor;
	}
	
	this->UnrealEditorLuaScriptEditor = SNew(SLuaScriptEditor)
		.ExternalWindowSize(FVector2D{0.5f,0.5f})
		.ExternalWindowPosition(FVector2D{0.5f,0.5f})
		.Session(this->EditorSession)
		.Title("Lua Script Editor")
		.InitiallyHidden(false)
		.StartAsWindow(true)
	;
	return this->UnrealEditorLuaScriptEditor.Pin();
}

void UUnrealLuaTools::ShowLuaScriptEditorCreateDefaultScript(UClass* uclass)
{
	//if (this->UnrealEditorNewDefaultScriptWindow.IsValid())
	//{
	//	TSharedPtr<SLuaNewDefaultScriptEditorWindow> editor = this->UnrealEditorNewDefaultScriptWindow.Pin(); 
	//	verify(!editor->Session.IsValid())
	//	TSharedPtr<SWindow> window = editor->TryGetParentWindow();
	//	if (window)
	//	{
	//		window->FlashWindow();
	//		window->BringToFront();
	//	}
	//	return;
	//}
	if (uclass)
	{
		//this->OpenLuaScriptEditorInUnrealEditor();
		
		FTimerHandle handle;
		GEditor->GetTimerManager()->SetTimer(handle, [this, uclass]()
		{
			this->OpenLuaScriptEditorInUnrealEditor()->OpenNewTabCreateDefaultScript(uclass);
		}, 0.5f, false, -1);
	}
	else
	{
		FString popupMessage = "UClass not valid";
		FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, FText::FromString(popupMessage));
		LUA_LOG_ERROR("%s", *popupMessage)
	}
}
#endif

void UUnrealLuaTools::ShowModalWindow(TSharedRef<SWindow> window)
{
	UnrealLuaTools::ShowWindowUtility::MakeModalWindow(window);
}
