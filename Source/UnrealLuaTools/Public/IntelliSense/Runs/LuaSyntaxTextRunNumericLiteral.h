// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaSyntaxTextRun.h"

/**
 * 
 */
class UNREALLUATOOLS_API FLuaSyntaxTextRunNumericLiteral : public FUnrealLuaSyntaxTextRun
{
public:
public:
	static TSharedPtr<FLuaSyntaxTextRunNumericLiteral> Create(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText,
		const FTextBlockStyle& InStyle, const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous, const double& literal);
	
	FLuaSyntaxTextRunNumericLiteral(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText,
		const FTextBlockStyle& InStyle, const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous, const double& literal)
		: FUnrealLuaSyntaxTextRun(InRunInfo, InText, InStyle, InRange, previous)
	, NumericLiteral(literal)
	{
	}
	
	virtual FString GetRunName() const override { return "NumberLiteral"; }
	virtual ELuaSyntaxTextRunType GetRunType() const override { return ELuaSyntaxTextRunType::Number; }

	double NumericLiteral = 0.0f;
};
