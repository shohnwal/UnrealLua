// Fill out your copyright notice in the Description page of Project Settings.


#include "IntelliSense/Runs/LuaSyntaxTextRunBooleanConstant.h"

TSharedPtr<FLuaSyntaxTextRunBooleanConstant> FLuaSyntaxTextRunBooleanConstant::Create(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText, const FTextBlockStyle& InStyle, const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous)
{
	return MakeShared<FLuaSyntaxTextRunBooleanConstant>(InRunInfo, InText, InStyle, InRange, previous);
}
