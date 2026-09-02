// Fill out your copyright notice in the Description page of Project Settings.


#include "IntelliSense/Runs/LuaSyntaxTextRunVariableType.h"

TSharedPtr<FLuaSyntaxTextRunVariableType> FLuaSyntaxTextRunVariableType::Create(const FRunInfo& InRunInfo,
	const TSharedRef<const FString>& InText, const FTextBlockStyle& InStyle, const FTextRange& InRange,
	const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous, const FString& typeName)
{
	return MakeShared<FLuaSyntaxTextRunVariableType>(InRunInfo, InText, InStyle, InRange, previous, typeName);
}
