// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"
#include "UObject/Interface.h"
#include "LuaToolsSession.generated.h"

class SGamescreenDockableWindowWidget;
class SConstraintCanvas;
class UGameInstance;
class UGameViewportClient;
struct FScopedLuaContext;
// This class does not need to be modified.
UINTERFACE()
class ULuaToolsSession : public UInterface
{
	GENERATED_BODY()
};


enum class ELuaToolsSessionType : uint8
{
	Game,
	Editor
};
/**
 * 
 */
class UNREALLUATOOLS_API ILuaToolsSession
{
	GENERATED_BODY()

public:
	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
	virtual UGameViewportClient* GetViewportClient() const = 0;
	virtual UGameInstance* GetGameInstance() const = 0;
	virtual TSharedPtr<SConstraintCanvas> GetCanvas() const = 0;
	virtual FOnInputKeySignature& GetOninputKeyEvent() = 0;
	virtual TSharedPtr<FScopedLuaContext> GetScopedLuaContext() const = 0;
	virtual ELuaToolsSessionType GetSessionType() const = 0;
	virtual void AddInputModeOverride(TSharedRef<SGamescreenDockableWindowWidget> widget) = 0;
	virtual void RemoveInputModeOverride(TSharedRef<SGamescreenDockableWindowWidget> widget) = 0;
	UWorld* GetWorld() const;
};
