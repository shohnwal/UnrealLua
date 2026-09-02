// Fill out your copyright notice in the Description page of Project Settings.


#include "ScriptableUObject/LuaUserWidget.h"

#include "Utility/LuaLogMacros.h"
#include "Blueprint/WidgetTree.h"
#include "UnrealLua.h"
#include "sol/sol.hpp"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"

void ULuaUserWidget::BeginPlayNative()
{
	if(this->IsDesignTime() || this->bHasBegunPlay)
	{
		return;
	}
	this->bHasBegunPlay = true;

	UnrealLua::UObjectRegistry::LoadLuaScript(this, false);
	
	if(!this->bHasScriptImplementedTick)
	{
		FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(this);
		if(item.TickFunc.IsValid())
		{
			this->bNeedsExplicitLuaTicking = true;
		}
	}
	this->BeginPlay();
	
	//this->TakeWidget();
	
	//this->RebuildWidget();
}

void ULuaUserWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if(this->bNeedsExplicitLuaTicking && this->bLuaTickEnabled)
	{
		FLuaUObjectItem& handle = UnrealLua::UObjectRegistry::GetUObjectItem(this);
		UnrealLua::LuaScriptCall::CallLuaFunctionSafeByName(&handle, "Tick", this, &MyGeometry, InDeltaTime);	
	}	
}

FLuaScriptSettings ULuaUserWidget::GetLuaScriptSettings_Implementation()
{
	return const_cast<ULuaUserWidget*>(this)->LuaScriptSettings;
}

void ULuaUserWidget::SetLuaScriptSettings_Implementation(FLuaScriptSettings newSettings)
{
	this->LuaScriptSettings = newSettings;
}

UWidget* ULuaUserWidget::CreateRootWidget(TSubclassOf<UWidget> widgetClass, FName widgetName)
{
	if(!widgetClass)
	{
		LUA_LOG("Can't create widget : widgetClass is invalid")
		return nullptr;
	}
	UWidget* newWidget = this->WidgetTree->ConstructWidget(widgetClass, widgetName);
	if(this->WidgetTree->RootWidget == nullptr)
	{
		this->WidgetTree->RootWidget = newWidget;
	}
	if(widgetName != NAME_None)
	{
		this->SetContentForSlot(widgetName, newWidget);
	}
	return newWidget;	
}

UWidget* ULuaUserWidget::CreateWidget(TSubclassOf<UWidget> widgetClass, FName widgetName)
{
	if(!widgetClass)
	{
		LUA_LOG("Can't create widget : widgetClass is invalid")
		return nullptr;
	}
	UWidget* newWidget = this->WidgetTree->ConstructWidget(widgetClass, widgetName);
	if(widgetName != NAME_None)
	{
		this->SetContentForSlot(widgetName, newWidget);
	}
	return newWidget;
}

UWidget* ULuaUserWidget::SetRootWidget(UWidget* newRoot)
{
	UWidget* existing = this->WidgetTree->RootWidget;
	this->WidgetTree->RootWidget = newRoot;
	return existing;
}

void ULuaUserWidget::ClearWidgets()
{
	(void)this->SetRootWidget(nullptr);
}

void ULuaUserWidget::SetLuaTickEnabled(bool enabled)
{
	bLuaTickEnabled = enabled;
}

bool ULuaUserWidget::IsOverlappingWidget(const FPointerEvent& pointerEvent, UWidget* widget)
{
	if(!widget)
	{
		return false;
	}
	const FGeometry& geom = widget->GetCachedGeometry();
	return geom.IsUnderLocation(pointerEvent.GetScreenSpacePosition());
}
