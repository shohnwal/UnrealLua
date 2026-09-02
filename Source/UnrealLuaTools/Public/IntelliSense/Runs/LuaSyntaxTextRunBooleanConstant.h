// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaSyntaxTextRun.h"

/**
 * 
 */
class UNREALLUATOOLS_API FLuaSyntaxTextRunBooleanConstant : public FUnrealLuaSyntaxTextRun
{
	
public:
	static TSharedPtr<FLuaSyntaxTextRunBooleanConstant> Create(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText, const FTextBlockStyle& InStyle, const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous);
	FLuaSyntaxTextRunBooleanConstant(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText, const FTextBlockStyle& InStyle, const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous)
		: FUnrealLuaSyntaxTextRun(InRunInfo, InText, InStyle, InRange, previous)
	{
	}
	
	virtual ELuaSyntaxTextRunType GetRunType() const override { return ELuaSyntaxTextRunType::Boolean; }
	virtual FString GetRunName() const override { return "Boolean";	} 
};
