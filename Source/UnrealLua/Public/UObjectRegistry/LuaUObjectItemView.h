// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaUObjectItemView.generated.h"

struct FLuaUObjectItem;
/**
 * 
 */
USTRUCT(BlueprintType)
struct UNREALLUA_API FLuaUObjectItemView
{
	GENERATED_BODY()
	
	FLuaUObjectItemView()
		:LuaUObjectItem(nullptr)
	{}
	FLuaUObjectItemView(FLuaUObjectItem* item) 
		: LuaUObjectItem(item) 
	{}
	
	FLuaUObjectItem* operator->() const { return this->LuaUObjectItem; }
	bool IsValid() const { return this->LuaUObjectItem != nullptr; }
	bool operator==(const FLuaUObjectItemView& other) const
	{
		return this->LuaUObjectItem == other.LuaUObjectItem;
	}

	FLuaUObjectItem* LuaUObjectItem = nullptr;
};
