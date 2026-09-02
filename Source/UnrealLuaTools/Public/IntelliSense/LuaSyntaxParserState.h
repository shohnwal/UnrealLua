// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaSyntaxReport.h"
#include "Runs/LuaSyntaxTextRun.h"

/**
 * 
 */

#define MAKE_REPORT(txt, ...) FString::Printf(TEXT(txt), ##__VA_ARGS__)


class FLuaSyntaxTextRunVariableType;
class FUnrealLuaSyntaxTextRun;
struct FUnrealLuaSyntaxVariable;
struct FUnrealLuaSyntaxParserScope;

enum class ELuaSyntaxParseState : uint8
{
	None,
	LookingForString,
	LookingForCharacter,
	LookingForSingleLineComment,
	LookingForSingleLineAnnotation,
	
	//after a ---@Type, next expected word is the type of the variable
	LookingForSingleLineAnnotationType,
	LookingForMultiLineComment,
		
	LookingForLocalVarName,
	NewFunctionParamList,
	ColonLookingForFunctionName,
	PeriodLookingForFunctionName,
	PeriodLookingForFieldName,
	NewFunctionLookingForName,
	NewFunctionLookingForPeriod,
	NewFunctionLookingForColon,
	

	//expecting a function name after a double colon
	//myVar:<functionname>(...
	NewFunctionLookingForArgsOpenParanthesis,
	
	LookingForFunctionCallArgsOpenParanthesis,
	FunctionCallParamList,
	//we just encountered an assignment operator "=" and expect some kind of value
	//LookingForAssignmentValue,
	LookingForDoubleColon,
	Error,
};



struct FLuaSyntaxFunctionParam
{
	FString VarName = "";
	FString VarType = "";
	bool bIsReturnParam = false;
	const FString& GetVariableType() const { return VarType; }
};



struct UNREALLUATOOLS_API FLuaSyntaxParserState
{
	FLuaSyntaxParserState();
	void Start();
	int32 GetCurrentLineNumber() const;
	FTextRange GetCurrentLineOffSet() const;
	void EnterScope(const FString& keyword);
	bool LeaveScope(const FString& keyword);

	TSharedPtr<FUnrealLuaSyntaxVariable> FindVar(const FString& varName);
	TSharedPtr<FUnrealLuaSyntaxVariable> AddGlobalVar(const FString& varName, const FString& type);
	TSharedPtr<FUnrealLuaSyntaxVariable> AddNewLocalVar(const FString& varName);
	TSharedPtr<FUnrealLuaSyntaxVariable> AccessVarField(const FString& fieldName);
	TSharedPtr<FUnrealLuaSyntaxVariable> CreateTemporaryVar(const FString& varName) const;
	
	TSharedPtr<FUnrealLuaSyntaxVariable> ChangeVarType(TSharedPtr<FUnrealLuaSyntaxVariable>, const FString& newType);
	
	void PushParseState(ELuaSyntaxParseState state);
	void PopParseState();
	void SetParseState(ELuaSyntaxParseState state);
	bool IsParseState(ELuaSyntaxParseState state);
	ELuaSyntaxParseState GetParseState() const { return this->ParseState;}

	int32 GetCurrentScopeLevel() const;
	void NewLine(int32 lineNumber);
	void SetLineOffset(FTextRange offset);

	void MakeRecord(const FString& message);
	
	TArray<FLuaSyntaxReportEntry> GetReport();
	void EndParse();
	
	TSharedPtr<FUnrealLuaSyntaxVariable> FindVariable();
	
	void HandleSetAssignedVariableType(const TSharedPtr<FUnrealLuaSyntaxTextRun>& run, const FString& type, int32 lineNumber);
	
	void Reset();

	bool AddFuncParam(const FString& varName, const FString& varType, bool bIsReturn);
	FLuaSyntaxFunctionParam* GetStashedFuncParam(const FString& paramName);
	void ClearStashedFuncParams();
	
	void SetSavedVariableType(TSharedPtr<FLuaSyntaxTextRunVariableType> variableTypeRun); 
	TSharedPtr<FLuaSyntaxTextRunVariableType> GetSavedVariableType();
	
	TSharedPtr<FUnrealLuaSyntaxTextRun> GetPreviousLineRun();
	void SetPreviousLineRun(TSharedPtr<FUnrealLuaSyntaxTextRun> previousLineRun);

private:
	TArray<FLuaSyntaxFunctionParam> FuncParamStash = {};
	TArray<FLuaSyntaxReportEntry> Report{};
	TArray<ELuaSyntaxParseState> ParseStateStack = {};
	TSharedPtr<FUnrealLuaSyntaxParserScope> GlobalScope = nullptr;
	TSharedPtr<FUnrealLuaSyntaxParserScope> CurrentScope = nullptr;
	TSharedPtr<FUnrealLuaSyntaxTextRun> PreviousLineRun = nullptr;
	TSharedPtr<FLuaSyntaxTextRunVariableType> SavedVariableType = nullptr;
	FTextRange LineOffset = {0,0};
	int32 LineNumber = 0;
	ELuaSyntaxParseState ParseState = ELuaSyntaxParseState::None;
};