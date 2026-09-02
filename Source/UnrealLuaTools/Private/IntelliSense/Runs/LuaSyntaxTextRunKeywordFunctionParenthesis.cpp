// Fill out your copyright notice in the Description page of Project Settings.


#include "IntelliSense/Runs/LuaSyntaxTextRunKeywordFunctionParenthesis.h"

FLuaSyntaxTextRunKeywordFunctionParenthesis::FLuaSyntaxTextRunKeywordFunctionParenthesis(const FRunInfo& InRunInfo,
	const TSharedRef<const FString>& InText, const FTextBlockStyle& InStyle, const FTextRange& InRange,
	const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous, bool isOpenParenthesis, bool isFunctionCall)
	: FUnrealLuaSyntaxTextRun(InRunInfo, InText, InStyle, InRange, previous)
	, ParenthesisType(isOpenParenthesis ? EParenthesisType::Open : EParenthesisType::Close)
	, bIsFunctionCall(isFunctionCall)
{
}

TSharedPtr<FLuaSyntaxTextRunKeywordFunctionParenthesis> FLuaSyntaxTextRunKeywordFunctionParenthesis::Create(
	const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText, const FTextBlockStyle& InStyle,
	const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous, bool isOpenParenthesis, bool isFunctionCall)
{
	return MakeShared<FLuaSyntaxTextRunKeywordFunctionParenthesis>(InRunInfo, InText, InStyle, InRange, previous, isOpenParenthesis, isFunctionCall);
}
