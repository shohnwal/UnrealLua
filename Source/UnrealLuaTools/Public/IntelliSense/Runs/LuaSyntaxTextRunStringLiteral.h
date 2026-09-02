// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaSyntaxTextRun.h"

/**
 * 
 */

class UNREALLUATOOLS_API FLuaSyntaxTextRunStringLiteralMarker : public FUnrealLuaSyntaxTextRun
{
public:
	static TSharedPtr<FLuaSyntaxTextRunStringLiteralMarker> Create(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText,
		const FTextBlockStyle& InStyle, const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous);
	
	FLuaSyntaxTextRunStringLiteralMarker(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText,
		const FTextBlockStyle& InStyle, const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous)
		: FUnrealLuaSyntaxTextRun(InRunInfo, InText, InStyle, InRange, previous)
	{
	}
	
	virtual FString GetRunName() const override { return "StringParenthesis"; }
	virtual ELuaSyntaxTextRunType GetRunType() const override { return ELuaSyntaxTextRunType::String; }

	FString StringLiteral = "";
};


class UNREALLUATOOLS_API FLuaSyntaxTextRunStringLiteral : public FUnrealLuaSyntaxTextRun
{
public:
	static TSharedPtr<FLuaSyntaxTextRunStringLiteral> Create(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText,
		const FTextBlockStyle& InStyle, const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous, const FString& stringLiteral);
	
	FLuaSyntaxTextRunStringLiteral(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText,
		const FTextBlockStyle& InStyle, const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous, const FString& stringLiteral)
		: FUnrealLuaSyntaxTextRun(InRunInfo, InText, InStyle, InRange, previous)
	, StringLiteral(stringLiteral)
	{
	}
	
	virtual FString GetRunName() const override { return "StringLiteral"; }
	virtual ELuaSyntaxTextRunType GetRunType() const override { return ELuaSyntaxTextRunType::String; }

	FString StringLiteral = "";
};
