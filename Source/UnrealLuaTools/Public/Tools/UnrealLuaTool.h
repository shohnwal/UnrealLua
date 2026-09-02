// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UnrealLuaToolDelegates.h"
#include "Input/Reply.h"
#include "UObject/Object.h"
#include "UnrealLuaTool.generated.h"

class UUnrealLuaToolsSession;
class UUnrealLuaToolsMainMenu;
class UGameInstance;
enum EInputEvent : int;
struct FKey;
/**
 * 
 */
UCLASS(Within=UnrealLuaToolsSession)
class UNREALLUATOOLS_API UUnrealLuaTool : public UObject
{
	GENERATED_BODY()
public:
	UUnrealLuaTool();
	
	virtual void InitializeTool();
	UFUNCTION(BlueprintImplementableEvent)
	void ReceiveInitializeTool();
	
	UFUNCTION(BlueprintCallable)
	void SetToolActive(bool newActive);
	
	virtual void ActivateTool(const FUnrealLuaTooleActivateCallback& preActivateCallback = {});
	UFUNCTION(BlueprintImplementableEvent)
	void ReceiveActivateTool();
	
	UFUNCTION(BlueprintCallable)
	bool IsActiveTool() const;
	
	virtual void DeactivateTool();
	UFUNCTION(BlueprintImplementableEvent)
	void ReceiveDeactivateTool();
	
	virtual void Shutdown();
	UFUNCTION(BlueprintImplementableEvent)
	void ReceiveShutdown();
	
	virtual EVerticalAlignment GetMainMenuButtonAlignment() const;
	virtual FString GetToolMainMenuButtonLabel() const;
	virtual int32 GetToolMainMenuSortOrder() const;
	virtual void NotifyAddedToMainMenu();
	virtual FReply NotifyInputKeyEvent(const FKey& key, EInputEvent eventType, UGameInstance* gameInstance);
	
	UFUNCTION(BlueprintNativeEvent)
	bool RequiresTick();
	virtual bool RequiresTick_Implementation();
	
	virtual void Tick(float dt);
	UFUNCTION(BlueprintImplementableEvent)
	void ReceiveTick(float dt);
	
	UFUNCTION(BlueprintCallable)
	UUnrealLuaToolsSession* GetSession() const;
	
	UFUNCTION(BlueprintCallable)
	UGameInstance* GetGameInstance() const;
	
	UFUNCTION(BlueprintNativeEvent)
	void NotifyMainMenuButtonClicked();
	virtual void NotifyMainMenuButtonClicked_Implementation() {}
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TWeakObjectPtr<UUnrealLuaToolsSession> SessionInfo = nullptr;
};
