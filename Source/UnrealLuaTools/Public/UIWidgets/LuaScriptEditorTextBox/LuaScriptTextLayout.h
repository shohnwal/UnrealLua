// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Text/SlateTextLayout.h"

/**
 * 
 */
class UNREALLUATOOLS_API FLuaScriptTextLayout : public FSlateTextLayout
{
public:
	FLuaScriptTextLayout(SWidget* InOwner, const FTextBlockStyle& InDefaultTextStyle)
		: FSlateTextLayout(InOwner, InDefaultTextStyle)
	{
	}

	static TSharedRef<FLuaScriptTextLayout> Create(SWidget* InOwner, FTextBlockStyle InDefaultTextStyle);
	
	virtual void UpdateLayout() override;
	
	FSimpleDelegate OnLayoutChanged = {};
};
