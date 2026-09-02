
// Fill out your copyright notice in the Description page of Project Settings.

#include "IntelliSense/LuaSyntaxParserState.h"

#include "IntelliSense/LuaSyntaxReport.h"
#include "IntelliSense/UnrealLuaSyntaxParserScope.h"
#include "IntelliSense/Runs/LuaSyntaxTextRun.h"
#include "IntelliSense/Runs/LuaSyntaxTextRunAssignmentOperator.h"
#include "IntelliSense/Runs/LuaSyntaxTextRunVariable.h"
#include "Utility/LuaLogMacros.h"

FLuaSyntaxParserState::FLuaSyntaxParserState()
{
	
}

void FLuaSyntaxParserState::Start()
{
	this->Reset();
	this->GlobalScope = MakeShared<FUnrealLuaSyntaxParserScope>(nullptr, "", 0);
	this->CurrentScope = this->GlobalScope;
	verify(this->CurrentScope->GetScopeLevel() == 0)
}

int32 FLuaSyntaxParserState::GetCurrentLineNumber() const
{
	return this->LineNumber;
}

FTextRange FLuaSyntaxParserState::GetCurrentLineOffSet() const
{
	return this->LineOffset;
}

void FLuaSyntaxParserState::EnterScope(const FString& keyword)
{
	//LUA_LOG("LUASYNTAX : Enter scope %d -> %d, with %s at %d", this->GetCurrentScopeLevel() , this->GetCurrentScopeLevel() + 1, *keyword, this->GetCurrentLineNumber())
	TSharedPtr<FUnrealLuaSyntaxParserScope> parentScope = this->CurrentScope;
	this->CurrentScope = MakeShared<FUnrealLuaSyntaxParserScope>(parentScope, keyword, this->GetCurrentLineNumber());
	parentScope->AddChildScope(this->CurrentScope);
	verify(this->CurrentScope->GetScopeLevel() > 0)
	verify(this->CurrentScope->GetScopeLevel() == parentScope->GetScopeLevel() + 1)
}

bool FLuaSyntaxParserState::LeaveScope(const FString& keyword)
{
	if (!this->CurrentScope.IsValid())
	{
		this->MakeRecord(MAKE_REPORT("Unable to close scope with %s: No scope in use. This is not a syntax error but an app error, please notify the developer!", *keyword));
		return false;
	}
	if (this->CurrentScope->IsGlobalScope())
	{
		this->MakeRecord(MAKE_REPORT("Unable to close scope with %s: Can't close global scope", *keyword));
		return false;
	}
	//this->LastEncounteredVariable = nullptr;
	
	int32 oldScopeLevel = this->GetCurrentScopeLevel();
	
	//LUA_LOG("LUASYNTAX : Leaving scope %d with %s at %d", oldScopeLevel, *keyword, this->GetCurrentLineNumber())
	
	this->CurrentScope->SetLeavingScope(keyword, this->GetCurrentLineNumber());
			
	this->CurrentScope = this->CurrentScope->GetParentScope();
			
	verify(this->CurrentScope.IsValid())
	if (this->CurrentScope == this->GlobalScope)
	{
		//verify(this->GetCurrentScopeLevel() == 0)
	}
	else
	{
		//verify(this->GetCurrentScopeLevel() > 0)
	}
	int32 newScopeLevel = this->GetCurrentScopeLevel();
	verify(newScopeLevel == oldScopeLevel - 1)
	return true;
}

TSharedPtr<FUnrealLuaSyntaxVariable> FLuaSyntaxParserState::FindVar(const FString& varName)
{
	TWeakPtr<FUnrealLuaSyntaxVariable> foundVar = this->CurrentScope->FindVariable(varName, true);
	if (foundVar.IsValid())
	{
		TSharedPtr<FUnrealLuaSyntaxVariable> foundVarptr = foundVar.Pin();
		//if (!this->LastEncounteredTypeName.IsEmpty())
		//{
		//	foundVarptr->ChangeType(this->LastEncounteredTypeName, this->GetCurrentLineNumber());
		//}
		foundVar.Pin()->AddAccessCount();
		//LUA_LOG("LUASYNTAX : found var %s at scope level %d", *varName, this->GetCurrentScopeLevel())
		return foundVar.Pin();
	}
	return nullptr;
}

TSharedPtr<FUnrealLuaSyntaxVariable> FLuaSyntaxParserState::AddGlobalVar(const FString& varName, const FString& type)
{
	return this->GlobalScope->AddVariable(varName, this->GetCurrentLineNumber(), type, false);
}

TSharedPtr<FUnrealLuaSyntaxVariable> FLuaSyntaxParserState::AddNewLocalVar(const FString& varName)
{
	TWeakPtr<FUnrealLuaSyntaxVariable> foundVar = this->CurrentScope->FindVariable(varName, false);
	if (foundVar.IsValid())
	{
		this->MakeRecord(MAKE_REPORT("Redefinition of local variable %s from line %d in same scope", *varName, foundVar.Pin()->DeclaredLineNumber));
		return nullptr;
	}
	//LUA_LOG("LUASYNTAX : Add local var %s at scope level %d", *varName, this->GetCurrentScopeLevel())
	return this->CurrentScope->AddVariable(varName, this->GetCurrentLineNumber(), "", true);
}

TSharedPtr<FUnrealLuaSyntaxVariable> FLuaSyntaxParserState::AccessVarField(const FString& fieldName)
{
	TSharedPtr<FUnrealLuaSyntaxVariable> var = this->FindVariable();
	if (var.IsValid())
	{
		//LUA_LOG("LUASYNTAX : accessing field %s of var %s at scope level %d", *fieldName, *LastEncounteredVariable.Pin()->VariableName, this->GetCurrentScopeLevel())
		var->AccessField(fieldName, this->GetCurrentLineNumber());
		return var;
	}
	else
	{
		this->MakeRecord(MAKE_REPORT("Trying to access field %s: Can not find owning variable", *fieldName));
		return nullptr;
	}
}

TSharedPtr<FUnrealLuaSyntaxVariable> FLuaSyntaxParserState::CreateTemporaryVar(const FString& varName) const
{
	return MakeShared<FUnrealLuaSyntaxVariable>(varName, "", this->GetCurrentLineNumber(), nullptr, nullptr, false);
}

TSharedPtr<FUnrealLuaSyntaxVariable> FLuaSyntaxParserState::ChangeVarType(TSharedPtr<FUnrealLuaSyntaxVariable> var, const FString& newType)
{
	if (!var->HasVarTypeString())
	{
		var->ChangeType(newType);
		return var;
	}
	if (!var->DeclaredScope.IsValid())
	{
		return var;
	}
	FString varName = var->VariableName;
	return var->DeclaredScope.Pin()->AddVariable(varName, this->GetCurrentLineNumber(), newType, var->IsLocalVariable());
}

void FLuaSyntaxParserState::PushParseState(ELuaSyntaxParseState state)
{
	this->ParseStateStack.Emplace(state);
	this->ParseState = state;
}

void FLuaSyntaxParserState::PopParseState()
{
	this->ParseStateStack.Pop();
	verify(!this->ParseStateStack.IsEmpty())
	this->ParseState = this->ParseStateStack.Last();
}

void FLuaSyntaxParserState::SetParseState(ELuaSyntaxParseState state)
{
	this->ParseStateStack.Pop();
	this->PushParseState(state);
	this->ParseState = state;
}

bool FLuaSyntaxParserState::IsParseState(ELuaSyntaxParseState state)
{
	return this->ParseState == state;
}

int32 FLuaSyntaxParserState::GetCurrentScopeLevel() const
{
	verify(this->CurrentScope.IsValid())
	return this->CurrentScope->GetScopeLevel();
}

void FLuaSyntaxParserState::NewLine(int32 lineNumber)
{
	this->LineNumber = lineNumber;
}

void FLuaSyntaxParserState::SetLineOffset(FTextRange offset)
{
	this->LineOffset = offset;
}


void FLuaSyntaxParserState::MakeRecord(const FString& message)
{
	FStringBuilderBase builder;
	builder << message << " at " << this->GetCurrentLineNumber() << " : " << this->GetCurrentLineOffSet().BeginIndex;
	this->Report.Add({builder.ToString(), this->GetCurrentLineNumber(), this->GetCurrentLineOffSet()});
}

TArray<FLuaSyntaxReportEntry> FLuaSyntaxParserState::GetReport()
{
	return this->Report;	
}

void FLuaSyntaxParserState::EndParse()
{
	if (!this->GlobalScope.IsValid())
	{
		this->MakeRecord(MAKE_REPORT("Did not have any global scope at end of file! Did you use 'end' too many times?"));
	}
	if (this->CurrentScope != this->GlobalScope)
	{
		this->MakeRecord(MAKE_REPORT("Scope from %s at %d is still open at the end of file", *this->CurrentScope->GetScopeOpenKeyword(), this->CurrentScope->GetScopeOpenLineNumber()));
	}
}

TSharedPtr<FUnrealLuaSyntaxVariable> FLuaSyntaxParserState::FindVariable()
{
	if (!this->PreviousLineRun.IsValid())
	{
		this->MakeRecord(MAKE_REPORT("Attempt to index field but could not find owning variable"));
		return nullptr;
	}
	return this->PreviousLineRun->FindCurrentVariableOrField();
}


//If the previous run of "run" is an assignment, 
void FLuaSyntaxParserState::HandleSetAssignedVariableType(const TSharedPtr<FUnrealLuaSyntaxTextRun>& run, const FString& type, int32 lineNumber)
{
	TSharedPtr<FUnrealLuaSyntaxTextRun> previousRun = run->PreviousRun.Pin();
	if (previousRun && previousRun->IsRunType(ELuaSyntaxTextRunType::Assignment))
	{
		TSharedPtr<FLuaSyntaxTextRunAssignmentOperator> assignmentOP = StaticCastSharedPtr<FLuaSyntaxTextRunAssignmentOperator>(previousRun);
		TSharedPtr<FLuaSyntaxTextRunVariable> assignmentTargetRun = assignmentOP->AssignmentTarget.Pin();
		
		//can only change type of non-nested variable
		if (assignmentTargetRun.IsValid() && !assignmentTargetRun->bIsNestedVariable)
		{
			TSharedPtr<FUnrealLuaSyntaxVariable> var = assignmentTargetRun->Variable;
			assignmentTargetRun->Variable = this->ChangeVarType(var, type);
		}
	}	
}

void FLuaSyntaxParserState::Reset()
{
	
	this->GlobalScope = nullptr;
	this->CurrentScope = nullptr;
	this->LineNumber = -1;
	this->PreviousLineRun = nullptr;
	this->LineOffset = {0,0};
	this->ParseState = ELuaSyntaxParseState::None;
	this->Report = {};
	this->SavedVariableType = nullptr;
	this->ParseState = ELuaSyntaxParseState::None;
	this->ParseStateStack.Empty();
	this->PushParseState(ELuaSyntaxParseState::None);
	this->FuncParamStash = {};
}

bool FLuaSyntaxParserState::AddFuncParam(const FString& varName, const FString& varType, bool bIsReturn)
{
	auto found = this->FuncParamStash.FindByPredicate([&varName, bIsReturn](const FLuaSyntaxFunctionParam& item) -> bool
	{
		return varName == item.VarName || (bIsReturn && item.bIsReturnParam);
	});
	if (found)
	{
		this->MakeRecord(MAKE_REPORT("Duplicate function param descriptor %s", *varName));
		return false;
	}
	this->FuncParamStash.Add({varName, varType, bIsReturn});
	return true;
}

FLuaSyntaxFunctionParam* FLuaSyntaxParserState::GetStashedFuncParam(const FString& paramName)
{
	return this->FuncParamStash.FindByPredicate([&paramName](const FLuaSyntaxFunctionParam& item) -> bool { return paramName == item.VarName; });
}

void FLuaSyntaxParserState::ClearStashedFuncParams()
{
	this->FuncParamStash.Empty();
}

void FLuaSyntaxParserState::SetSavedVariableType(TSharedPtr<FLuaSyntaxTextRunVariableType> variableTypeRun)
{
	this->SavedVariableType = variableTypeRun;
}

TSharedPtr<FLuaSyntaxTextRunVariableType> FLuaSyntaxParserState::GetSavedVariableType()
{
	return this->SavedVariableType;
}

TSharedPtr<FUnrealLuaSyntaxTextRun> FLuaSyntaxParserState::GetPreviousLineRun()
{
	return this->PreviousLineRun;
}

void FLuaSyntaxParserState::SetPreviousLineRun(TSharedPtr<FUnrealLuaSyntaxTextRun> previousLineRun)
{
	this->PreviousLineRun = previousLineRun;
}
