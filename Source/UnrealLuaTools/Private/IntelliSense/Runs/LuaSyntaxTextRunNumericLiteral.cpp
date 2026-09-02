// Fill out your copyright notice in the Description page of Project Settings.


#include "IntelliSense/Runs/LuaSyntaxTextRunNumericLiteral.h"

TSharedPtr<FLuaSyntaxTextRunNumericLiteral> FLuaSyntaxTextRunNumericLiteral::Create(const FRunInfo& InRunInfo,
	const TSharedRef<const FString>& InText, const FTextBlockStyle& InStyle, const FTextRange& InRange,
	const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous, const double& literal)
{
	return MakeShared<FLuaSyntaxTextRunNumericLiteral>(InRunInfo, InText, InStyle, InRange, previous, literal);
}
