// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "UnrealLuaUserWidget.generated.h"

/**
 * 
 */
UCLASS()
class UNREALLUATOOLS_API UUnrealLuaUserWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	template<typename T>
	requires std::is_base_of_v<UWidget, T>
	T* ConstructSubWidget(FName widgetName = NAME_None);
};

template <typename T> 
requires std::is_base_of_v<UWidget, T>
T* UUnrealLuaUserWidget::ConstructSubWidget(FName widgetName)
{
	return this->WidgetTree->ConstructWidget<T>(T::StaticClass(), widgetName);
}
