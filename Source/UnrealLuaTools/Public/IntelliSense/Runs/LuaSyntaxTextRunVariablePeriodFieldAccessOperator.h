// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaSyntaxTextRun.h"

/**
 * 
 */
class UNREALLUATOOLS_API FLuaSyntaxTextRunVariablePeriodFieldAccessOperator : public FUnrealLuaSyntaxTextRun
{
public:
	static TSharedPtr<FLuaSyntaxTextRunVariablePeriodFieldAccessOperator> Create(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText,
		const FTextBlockStyle& InStyle, const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous);

	virtual FString GetRunName() const override { return "Access .";}
	virtual ELuaSyntaxTextRunType GetRunType() const override { return ELuaSyntaxTextRunType::Period; }
	
	virtual TSharedPtr<FUnrealLuaSyntaxVariable> FindCurrentVariableOrField() const override;
	virtual TSharedPtr<FLuaSyntaxTextRunVariable> FindTopOwningVariableRun() override;
	virtual TSharedPtr<FLuaSyntaxTextRunVariable> FindCurrentVariableRun() override;
	
	
	FLuaSyntaxTextRunVariablePeriodFieldAccessOperator(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText,
		const FTextBlockStyle& InStyle, const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous)
		: FUnrealLuaSyntaxTextRun(InRunInfo, InText, InStyle, InRange, previous)
	{
	}
};
