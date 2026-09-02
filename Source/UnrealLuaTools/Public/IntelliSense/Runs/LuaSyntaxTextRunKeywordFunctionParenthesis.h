// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaSyntaxTextRun.h"


/**
 * 
 */
class UNREALLUATOOLS_API FLuaSyntaxTextRunKeywordFunctionParenthesis : public FUnrealLuaSyntaxTextRun
{
public:
	static TSharedPtr<FLuaSyntaxTextRunKeywordFunctionParenthesis> Create(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText,
		const FTextBlockStyle& InStyle, const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous,
		bool isOpenParenthesis, bool isFunctionCall);
	
	virtual FString GetRunName() const
	{
		if (this->IsFunctionCall())
		{
			return this->IsOpenParenthesis() ? "Call (" : "Call )";
		}
		else
		{
			return this->IsOpenParenthesis() ? "Params (" : "Params )";
		}
	}
	virtual ELuaSyntaxTextRunType GetRunType() const override { return ELuaSyntaxTextRunType::FunctionParamsParenthesis; }
	bool IsOpenParenthesis() const { return this->ParenthesisType == EParenthesisType::Open; }
	bool IsCloseParenthesis() const { return this->ParenthesisType == EParenthesisType::Close; }
	bool IsFunctionCall() const { return this->bIsFunctionCall;}
	FLuaSyntaxTextRunKeywordFunctionParenthesis(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText,
	                                            const FTextBlockStyle& InStyle, const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous,
	                                            bool isOpenParenthesis, bool isFunctionCall);

	enum class EParenthesisType : uint8
	{
		Open,
		Close
	};
protected:
	EParenthesisType ParenthesisType = EParenthesisType::Open;
	bool bIsFunctionCall = false;
};
