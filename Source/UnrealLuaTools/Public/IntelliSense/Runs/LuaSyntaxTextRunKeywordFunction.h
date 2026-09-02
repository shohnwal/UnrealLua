#pragma once
#include "LuaSyntaxTextRun.h"

struct FUnrealLuaSyntaxVariable;

class FLuaSyntaxTextRunKeywordFunction : public FUnrealLuaSyntaxTextRun
{
public:
	static TSharedPtr<FLuaSyntaxTextRunKeywordFunction> Create(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText,
		const FTextBlockStyle& InStyle, const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous);
	
	virtual FString GetRunName() const { return "function"; }
	virtual ELuaSyntaxTextRunType GetRunType() const override { return ELuaSyntaxTextRunType::FunctionKeyword; }
	
	virtual bool IsAssignable() const override { return true; }
	//The variable corresponding to this function run
	
	enum class EFunctionRunLayout
	{
		LocalVarAssignFunction, //local f = function()
		LocalFunctionVar,		//local function f()
		FunctionVar,			//function f()
		VarAssignFunction		//f = function()
	};

	FLuaSyntaxTextRunKeywordFunction(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText,
		const FTextBlockStyle& InStyle, const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous)
		: FUnrealLuaSyntaxTextRun(InRunInfo, InText, InStyle, InRange, previous)
	{
	}
};

inline TSharedPtr<FLuaSyntaxTextRunKeywordFunction> FLuaSyntaxTextRunKeywordFunction::Create(const FRunInfo& InRunInfo,
	const TSharedRef<const FString>& InText, const FTextBlockStyle& InStyle, const FTextRange& InRange,
	const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous)
{
	return MakeShared<FLuaSyntaxTextRunKeywordFunction>(InRunInfo, InText, InStyle, InRange, previous);
}
