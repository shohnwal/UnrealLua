// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaSyntaxTextRun.h"

class FLuaSyntaxTextRunVariable;
/**
 * 
 */
class UNREALLUATOOLS_API FLuaSyntaxTextRunAssignmentOperator : public FUnrealLuaSyntaxTextRun
{
public:
	static TSharedPtr<FLuaSyntaxTextRunAssignmentOperator> Create(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText,
							   const FTextBlockStyle& InStyle, const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous);

	FLuaSyntaxTextRunAssignmentOperator(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText,
	                           const FTextBlockStyle& InStyle, const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous);
	
	virtual bool IsAssignable() const override { return false; }
	virtual void EndLayout() override;
	
	virtual FString GetRunName() const { return "="; }
	virtual ELuaSyntaxTextRunType GetRunType() const override { return ELuaSyntaxTextRunType::Assignment; }

	virtual TSharedPtr<FLuaSyntaxTextRunVariable> FindCurrentVariableRun() override;
	TWeakPtr<FLuaSyntaxTextRunVariable> AssignmentTarget = nullptr;
};
