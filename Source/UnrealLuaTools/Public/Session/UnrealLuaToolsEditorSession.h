// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#if WITH_EDITOR
#include "EditorSubsystem.h"
#endif
#include "LuaToolsSession.h"
#include "UnrealLuaToolsEditorSession.generated.h"

#if WITH_EDITOR
/**
 * 
 */
UCLASS()
class UNREALLUATOOLS_API UUnrealLuaToolsEditorSession : public UObject, public ILuaToolsSession
{
	GENERATED_BODY()
public:
	void Initialize();

	virtual UGameViewportClient* GetViewportClient() const override;
	virtual UGameInstance* GetGameInstance() const override;
	virtual TSharedPtr<SConstraintCanvas> GetCanvas() const override;
	virtual FOnInputKeySignature& GetOninputKeyEvent() override;
	virtual TSharedPtr<FScopedLuaContext> GetScopedLuaContext() const override;
	virtual ELuaToolsSessionType GetSessionType() const override;
	virtual void AddInputModeOverride(TSharedRef<SGamescreenDockableWindowWidget> widget) override;
	virtual void RemoveInputModeOverride(TSharedRef<SGamescreenDockableWindowWidget> widget) override;

	TSharedPtr<FScopedLuaContext> LuaContext = {};
};
#endif