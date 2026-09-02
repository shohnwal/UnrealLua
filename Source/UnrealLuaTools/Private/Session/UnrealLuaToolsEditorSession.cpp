// Fill out your copyright notice in the Description page of Project Settings.
#if WITH_EDITOR

#include "Session/UnrealLuaToolsEditorSession.h"

#include "LuaContext/ScopedLuaContext.h"
#include "Misc/CoreDelegates.h"

void UUnrealLuaToolsEditorSession::Initialize()
{
	this->LuaContext = MakeShared<FScopedLuaContext>(nullptr, ELuaContextType::LuaScriptEditor, "LuaScriptEditorState");
}

UGameViewportClient* UUnrealLuaToolsEditorSession::GetViewportClient() const
{
	return nullptr;
}

UGameInstance* UUnrealLuaToolsEditorSession::GetGameInstance() const
{
	return nullptr;
}

TSharedPtr<SConstraintCanvas> UUnrealLuaToolsEditorSession::GetCanvas() const
{
	return nullptr;
}

FOnInputKeySignature& UUnrealLuaToolsEditorSession::GetOninputKeyEvent()
{
	static FOnInputKeySignature dummy;
	return dummy;
}

TSharedPtr<FScopedLuaContext> UUnrealLuaToolsEditorSession::GetScopedLuaContext() const
{
	return this->LuaContext;
}

ELuaToolsSessionType UUnrealLuaToolsEditorSession::GetSessionType() const
{
	return ELuaToolsSessionType::Editor;
}

void UUnrealLuaToolsEditorSession::AddInputModeOverride(TSharedRef<SGamescreenDockableWindowWidget> widget)
{
}

void UUnrealLuaToolsEditorSession::RemoveInputModeOverride(TSharedRef<SGamescreenDockableWindowWidget> widget)
{
}

#endif
