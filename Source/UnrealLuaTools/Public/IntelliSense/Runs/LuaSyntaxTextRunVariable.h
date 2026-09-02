#pragma once
#include "LuaSyntaxTextRun.h"
#include "IntelliSense/UnrealLuaSyntaxLayoutMarshaller.h"
#include "Utility/WidgetStyles.h"


enum class ELuaSyntaxVariableRunFlags
{
	New,
	Local,
	Global,
	Temp,
};


enum class ELuaSyntaxVariableAction
{
	Create,
	Access,
	Assignment
};
class UNREALLUATOOLS_API FLuaSyntaxTextRunVariable : public FUnrealLuaSyntaxTextRun
{
public:
	static TSharedPtr<FLuaSyntaxTextRunVariable> CreateNewVariableRun(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText, const FTextBlockStyle& InStyle, const FTextRange& InRange , const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous, const TSharedPtr<FUnrealLuaSyntaxVariable>& variable);
	static TSharedPtr<FLuaSyntaxTextRunVariable> CreateAccessVariableRun(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText, const FTextBlockStyle& InStyle, const FTextRange& InRange , const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous, const TSharedPtr<FUnrealLuaSyntaxVariable>& variable);
	static TSharedPtr<FLuaSyntaxTextRunVariable> CreateAccessNestedVariableRun(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText, const FString& varName, const FTextBlockStyle& InStyle, const FTextRange& InRange , const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous,int32 lineNumber);
	static TSharedPtr<FLuaSyntaxTextRunVariable> CreateFuncCallVariableAccessRun(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText, const FTextBlockStyle& InStyle, const FTextRange& InRange , const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous, const TSharedPtr<FUnrealLuaSyntaxVariable>& variable);
	static TSharedPtr<FLuaSyntaxTextRunVariable> CreateAccessRValueVariableRun(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText, const FTextBlockStyle& style, const FTextRange&
	                                                                           InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous, const TSharedPtr<FUnrealLuaSyntaxVariable>&
	                                                                           variable);
	
	
	FLuaSyntaxTextRunVariable(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText,	const FTextBlockStyle& InStyle, const FTextRange& InRange , const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous, const TSharedPtr<FUnrealLuaSyntaxVariable>& variable)
		: FUnrealLuaSyntaxTextRun(InRunInfo, InText, InStyle, InRange, previous), Variable(variable)
	{}
	virtual FString GetRunName() const override { return "Variable";}
	virtual ELuaSyntaxTextRunType GetRunType() const override { return ELuaSyntaxTextRunType::Variable; }
	virtual TSharedPtr<FLuaSyntaxTextRunVariable> FindCurrentVariableRun() override;
	virtual TSharedPtr<FLuaSyntaxTextRunVariable> FindTopOwningVariableRun() override;
	virtual FString GetVariableName() const;
	
	//Checks whether this variable run can be indexed with a dot or colon
	//Variable must be valid AND not be a new local
	bool CanIndex() const { return this->Variable.IsValid() && !(this->IsLocalVariable() && this->IsNewVariableRun()); }
	TSharedPtr<FUnrealLuaSyntaxVariable> GetVariable() { return Variable; }
	bool IsGlobalVariable() const;
	bool IsLocalVariable() const;
	bool IsNewVariableRun() const { return bIsNewVariableRun; }
	void SetVariableType(const FString& varType, int32 lineNumber);
	TSharedPtr<FUnrealLuaSyntaxVariable> Variable = nullptr;
	
	bool bIsNewVariableRun = false;
	bool bIsNestedVariable = false;
};
