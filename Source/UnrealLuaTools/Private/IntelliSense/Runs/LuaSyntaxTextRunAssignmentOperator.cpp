// Fill out your copyright notice in the Description page of Project Settings.


#include "IntelliSense/Runs/LuaSyntaxTextRunAssignmentOperator.h"

#include "IntelliSense/Runs/LuaSyntaxTextRunVariable.h"

TSharedPtr<FLuaSyntaxTextRunAssignmentOperator> FLuaSyntaxTextRunAssignmentOperator::Create(const FRunInfo& InRunInfo,
	const TSharedRef<const FString>& InText, const FTextBlockStyle& InStyle, const FTextRange& InRange,
	const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous)
{
	return MakeShared<FLuaSyntaxTextRunAssignmentOperator>(InRunInfo, InText, InStyle, InRange, previous);
}

FLuaSyntaxTextRunAssignmentOperator::FLuaSyntaxTextRunAssignmentOperator(const FRunInfo& InRunInfo,
	const TSharedRef<const FString>& InText, const FTextBlockStyle& InStyle, const FTextRange& InRange,
	const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous)
		: FUnrealLuaSyntaxTextRun(InRunInfo, InText, InStyle, InRange, previous)
{
	if (!previous.IsValid())
	{
		checkNoEntry()
	}
	else
	{
		this->AssignmentTarget = previous->FindCurrentVariableRun();
	}
}

void FLuaSyntaxTextRunAssignmentOperator::EndLayout()
{
	FUnrealLuaSyntaxTextRun::EndLayout();
}

TSharedPtr<FLuaSyntaxTextRunVariable> FLuaSyntaxTextRunAssignmentOperator::FindCurrentVariableRun()
{
	//For an assignment, the previous run should be the variable 
	return this->PreviousRun.IsValid() ? this->PreviousRun.Pin()->FindCurrentVariableRun() : nullptr;
}
