// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaSyntaxTextRun.h"

/**
 * 
 */
class UNREALLUATOOLS_API FLuaSyntaxTextRunVariableType : public FUnrealLuaSyntaxTextRun
{
public:
	static TSharedPtr<FLuaSyntaxTextRunVariableType> Create(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText,
		const FTextBlockStyle& InStyle, const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous, const FString& typeName);
	
	virtual FString GetRunName() const override { return "---@Type";}
	FLuaSyntaxTextRunVariableType(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText,
		const FTextBlockStyle& InStyle, const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous, const FString& typeName)
		: FUnrealLuaSyntaxTextRun(InRunInfo, InText, InStyle, InRange, previous)
	{
		this->VariableType = typeName;
	}

	virtual ELuaSyntaxTextRunType GetRunType() const override { return ELuaSyntaxTextRunType::TypeAnnotation; }
	const FString& GetVariableTypeName() { return this->VariableType; }

	FString VariableType = "";
};
