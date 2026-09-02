// Fill out your copyright notice in the Description page of Project Settings.


#include "IntelliSense/Runs/LuaSyntaxTextRunStringLiteral.h"


TSharedPtr<FLuaSyntaxTextRunStringLiteralMarker> FLuaSyntaxTextRunStringLiteralMarker::Create(
	const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText, const FTextBlockStyle& InStyle,
	const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous)
{
	return MakeShared<FLuaSyntaxTextRunStringLiteralMarker>(InRunInfo, InText, InStyle, InRange, previous);
}

TSharedPtr<FLuaSyntaxTextRunStringLiteral> FLuaSyntaxTextRunStringLiteral::Create(const FRunInfo& InRunInfo,
                                                                                  const TSharedRef<const FString>& InText, const FTextBlockStyle& InStyle, const FTextRange& InRange,
                                                                                  const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous, const FString& stringLiteral)
{
	return MakeShared<FLuaSyntaxTextRunStringLiteral>(InRunInfo, InText, InStyle, InRange, previous, stringLiteral);	
}
