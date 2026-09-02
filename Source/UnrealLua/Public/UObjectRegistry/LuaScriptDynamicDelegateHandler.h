// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <string>
#include "sol/sol.hpp"
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LuaScriptDynamicDelegateHandler.generated.h"

class ULuaScriptDynamicDelegateHandler;

DECLARE_DELEGATE_TwoParams(FOnProcessEventDelegate, ULuaScriptDynamicDelegateHandler*, void*);
/**
 * 
 */
UCLASS()
class UNREALLUA_API ULuaScriptDynamicDelegateHandler : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION()
	void DummyFunc() const;
	virtual void ProcessEvent(UFunction* Function, void* Parms) override;

	TWeakObjectPtr<UObject> DelegateOwner = nullptr;
	TWeakObjectPtr<UFunction> DelegateFunction = nullptr;
	UPROPERTY()
	FName FuncName = NAME_None;
	std::string CallbackFuncName = "";
	TArray<sol::object> CallbackArgs = {};

	FOnProcessEventDelegate OnProcessEvent;
};