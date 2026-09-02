
#include "IntelliSense/Runs/LuaSyntaxTextRunVariable.h"

#include "IntelliSense/UnrealLuaSyntaxParserScope.h"

TSharedPtr<FLuaSyntaxTextRunVariable> FLuaSyntaxTextRunVariable::CreateNewVariableRun(const FRunInfo& InRunInfo,
	const TSharedRef<const FString>& InText, const FTextBlockStyle& InStyle, const FTextRange& InRange,
	const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous, const TSharedPtr<FUnrealLuaSyntaxVariable>& variable)
{
	auto newRun = MakeShared<FLuaSyntaxTextRunVariable>(InRunInfo, InText, InStyle, InRange, previous, variable);
	newRun->bIsNewVariableRun = true;
	newRun->bIsNestedVariable = false;
	return newRun;
}

TSharedPtr<FLuaSyntaxTextRunVariable> FLuaSyntaxTextRunVariable::CreateAccessVariableRun(const FRunInfo& InRunInfo,
	const TSharedRef<const FString>& InText, const FTextBlockStyle& InStyle, const FTextRange& InRange,
	const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous, const TSharedPtr<FUnrealLuaSyntaxVariable>& variable)
{
	auto newRun = MakeShared<FLuaSyntaxTextRunVariable>(InRunInfo, InText, InStyle, InRange, previous, variable);
	newRun->bIsNewVariableRun = false;
	newRun->bIsNestedVariable = false;
	return newRun;
}

TSharedPtr<FLuaSyntaxTextRunVariable> FLuaSyntaxTextRunVariable::CreateFuncCallVariableAccessRun(
	const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText, const FTextBlockStyle& InStyle,
	const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous,
	const TSharedPtr<FUnrealLuaSyntaxVariable>& variable)
{
	auto newRun = MakeShared<FLuaSyntaxTextRunVariable>(InRunInfo, InText, InStyle, InRange, previous, variable);
	return newRun;
}

TSharedPtr<FLuaSyntaxTextRunVariable> FLuaSyntaxTextRunVariable::CreateAccessRValueVariableRun(const FRunInfo& InRunInfo,
	const TSharedRef<const FString>& InText, const FTextBlockStyle& style,
	const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous, const TSharedPtr<FUnrealLuaSyntaxVariable>& variable)
{
	auto newRun = MakeShared<FLuaSyntaxTextRunVariable>(InRunInfo, InText, style, InRange, previous, variable);
	return newRun;
}

TSharedPtr<FLuaSyntaxTextRunVariable> FLuaSyntaxTextRunVariable::CreateAccessNestedVariableRun(const FRunInfo& InRunInfo,
                                                                                               const TSharedRef<const FString>& InText, const FString& varName,  const FTextBlockStyle& InStyle, const FTextRange& InRange,
                                                                                               const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous, int32  lineNumber)
{
	verify(previous->IsRunType(ELuaSyntaxTextRunType::Period) || previous->IsRunType(ELuaSyntaxTextRunType::Colon));
	TSharedPtr<FLuaSyntaxTextRunVariable> varRun = previous->FindCurrentVariableRun();
	verify(previous->PreviousRun == varRun);
	
	
	TSharedRef<FUnrealLuaSyntaxVariable> var = varRun->GetVariable()->AccessField(varName, lineNumber);
	
	auto newRun = MakeShared<FLuaSyntaxTextRunVariable>(InRunInfo, InText, InStyle, InRange, previous, var);
	newRun->bIsNewVariableRun = false;
	newRun->bIsNestedVariable = true;

	return newRun;
}

TSharedPtr<FLuaSyntaxTextRunVariable> FLuaSyntaxTextRunVariable::FindCurrentVariableRun()
{
	return this->SharedThis(this);
}

TSharedPtr<FLuaSyntaxTextRunVariable> FLuaSyntaxTextRunVariable::FindTopOwningVariableRun()
{
	if (!this->PreviousRun.IsValid())
	{
		return this->SharedThis(this);
	}
	if (this->IsLocalVariable())
	{
		return this->SharedThis(this);
	}
	auto previous = this->PreviousRun.Pin();
	if (previous->GetRunType() == ELuaSyntaxTextRunType::Period || previous->GetRunType() == ELuaSyntaxTextRunType::Colon)
	{
		return previous->FindTopOwningVariableRun();
	}
	return this->SharedThis(this);
}

FString FLuaSyntaxTextRunVariable::GetVariableName() const
{
	return this->Variable ? this->Variable->VariableName : "UnknownVarName";
}

bool FLuaSyntaxTextRunVariable::IsLocalVariable() const
{
	return this->Variable.IsValid() ? this->Variable->IsLocalVariable() : false;
}

void FLuaSyntaxTextRunVariable::SetVariableType(const FString& varType,int32 lineNumbe)
{
	if (this->bIsNestedVariable)
	{
		return;
	}
	this->Variable->ChangeType(varType);
	this->bIsNewVariableRun = true;
}

bool FLuaSyntaxTextRunVariable::IsGlobalVariable() const
{
	return this->Variable.IsValid() ? this->Variable->IsGlobalVariable() : false;
}
