// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MainMenu/SUnrealLuaMainMenu.h"
#include "StructUtils/InstancedStruct.h"
#include "Subsystems/EngineSubsystem.h"
#include "Templates/SubclassOf.h"
#include "UnrealLuaTools.generated.h"

class UUnrealLuaToolsEditorSession;
class SLuaNewDefaultScriptEditorWindow;
class SLuaScriptEditor;
class UUnrealLuaToolsSession;
class UUnrealLuaToolsMainMenu;
class UGameViewportClient;
class UUnrealLuaDebugTool;
class UUnrealLuaTool;
class SWindow;
struct FInputKeyEventArgs;
class UGameInstance;
class SLuaScriptEditorWindow;
/**
 * 
 */

/*
 Ideas:
ActorTools
	Teleport current pawn to clicked location
	Teleport watched actor to clicked location
SetLuaScriptValuEditor
	Edit value
	Select value from Actor Property
		-> List all actors, can open submenus of subobbjects and their property lists to copy data from
	Select value from UObject Property
		-> List all objects, can open submenus of subobbjects and their property lists to copy data from
 */


UCLASS()
class UNREALLUATOOLS_API UUnrealLuaTools : public UEngineSubsystem
{
	GENERATED_BODY()
public:
	static UUnrealLuaTools* Get();
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	void NotifyAllModulesLoaded();

#if WITH_EDITOR
	TSharedPtr<SLuaScriptEditor> OpenLuaScriptEditorInUnrealEditor();
	void ShowLuaScriptEditorCreateDefaultScript(UClass* uclass);
#endif
	void ShowModalWindow(TSharedRef<SWindow> window);
public:
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TSet<TObjectPtr<UClass>> RegisteredTools;
	
#if WITH_EDITOR
	TWeakPtr<SLuaNewDefaultScriptEditorWindow> UnrealEditorNewDefaultScriptWindow;
	TWeakPtr<SLuaScriptEditor> UnrealEditorLuaScriptEditor;
#endif
#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UUnrealLuaToolsEditorSession> EditorSession = nullptr;
#endif
};
