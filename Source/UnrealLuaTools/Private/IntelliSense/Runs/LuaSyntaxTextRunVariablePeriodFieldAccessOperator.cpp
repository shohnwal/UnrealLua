// Fill out your copyright notice in the Description page of Project Settings.


#include "IntelliSense/Runs/LuaSyntaxTextRunVariablePeriodFieldAccessOperator.h"

TSharedPtr<FLuaSyntaxTextRunVariablePeriodFieldAccessOperator> FLuaSyntaxTextRunVariablePeriodFieldAccessOperator::Create(
	const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText, const FTextBlockStyle& InStyle,
	const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous)
{
	return MakeShared<FLuaSyntaxTextRunVariablePeriodFieldAccessOperator>(InRunInfo, InText, InStyle, InRange, previous);
}

TSharedPtr<FUnrealLuaSyntaxVariable> FLuaSyntaxTextRunVariablePeriodFieldAccessOperator::FindCurrentVariableOrField() const
{
	return this->PreviousRun.IsValid() ? this->PreviousRun.Pin()->FindCurrentVariableOrField() : nullptr;
}

TSharedPtr<FLuaSyntaxTextRunVariable> FLuaSyntaxTextRunVariablePeriodFieldAccessOperator::FindTopOwningVariableRun()
{
	return this->PreviousRun.IsValid() ? this->PreviousRun.Pin()->FindTopOwningVariableRun() : nullptr;
}

TSharedPtr<FLuaSyntaxTextRunVariable> FLuaSyntaxTextRunVariablePeriodFieldAccessOperator::FindCurrentVariableRun()
{
	return this->PreviousRun.IsValid() ? this->PreviousRun.Pin()->FindCurrentVariableRun() : nullptr;
}
