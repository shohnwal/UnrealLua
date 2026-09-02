// Fill out your copyright notice in the Description page of Project Settings.


#include "IntelliSense/Runs/LuaSyntaxTextRunVariableColonFunctionCallFieldAccessOperator.h"

TSharedPtr<FLuaSyntaxTextRunVariableColonFunctionCallFieldAccessOperator> FLuaSyntaxTextRunVariableColonFunctionCallFieldAccessOperator::Create(
	const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText, const FTextBlockStyle& InStyle,
	const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous)
{
	return MakeShared<FLuaSyntaxTextRunVariableColonFunctionCallFieldAccessOperator>(InRunInfo, InText, InStyle, InRange, previous);
}

TSharedPtr<FUnrealLuaSyntaxVariable> FLuaSyntaxTextRunVariableColonFunctionCallFieldAccessOperator::FindCurrentVariableOrField() const
{
	return this->PreviousRun.IsValid() ? this->PreviousRun.Pin()->FindCurrentVariableOrField() : nullptr;
}

TSharedPtr<FLuaSyntaxTextRunVariable> FLuaSyntaxTextRunVariableColonFunctionCallFieldAccessOperator::FindCurrentVariableRun()
{
	return this->PreviousRun.IsValid() ? this->PreviousRun.Pin()->FindCurrentVariableRun() : nullptr;
}

TSharedPtr<FLuaSyntaxTextRunVariable> FLuaSyntaxTextRunVariableColonFunctionCallFieldAccessOperator::FindTopOwningVariableRun()
{
	return this->PreviousRun.IsValid() ? this->PreviousRun.Pin()->FindCurrentVariableRun() : nullptr;
}
