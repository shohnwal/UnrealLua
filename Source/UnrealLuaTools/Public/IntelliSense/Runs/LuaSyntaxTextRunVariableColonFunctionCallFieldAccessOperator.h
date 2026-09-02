#pragma once

#include "CoreMinimal.h"
#include "LuaSyntaxTextRun.h"

/**
 * 
 */
class UNREALLUATOOLS_API FLuaSyntaxTextRunVariableColonFunctionCallFieldAccessOperator : public FUnrealLuaSyntaxTextRun
{
public:
	static TSharedPtr<FLuaSyntaxTextRunVariableColonFunctionCallFieldAccessOperator> Create(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText,
		const FTextBlockStyle& InStyle, const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous);

	virtual FString GetRunName() const override { return "Call :";}
	virtual ELuaSyntaxTextRunType GetRunType() const override { return ELuaSyntaxTextRunType::Colon; }
	
	virtual TSharedPtr<FUnrealLuaSyntaxVariable> FindCurrentVariableOrField() const override;
	virtual TSharedPtr<FLuaSyntaxTextRunVariable> FindCurrentVariableRun() override;
	virtual TSharedPtr<FLuaSyntaxTextRunVariable> FindTopOwningVariableRun() override;
	
	FLuaSyntaxTextRunVariableColonFunctionCallFieldAccessOperator(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText,
	                                                const FTextBlockStyle& InStyle, const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous)
		: FUnrealLuaSyntaxTextRun(InRunInfo, InText, InStyle, InRange, previous)
	{
	}
};
