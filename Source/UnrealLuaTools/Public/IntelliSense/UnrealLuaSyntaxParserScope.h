// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaSyntaxParserState.h"
#include "LuaValue/LuaValue.h"

struct FUnrealLuaSyntaxParserScope;

struct UNREALLUATOOLS_API FUnrealLuaSyntaxVariable : public TSharedFromThis<FUnrealLuaSyntaxVariable>
{
	FUnrealLuaSyntaxVariable(const FString& VariableName, const FString& TypeName, int32 DeclaredLineNumber,
		const TWeakPtr<FUnrealLuaSyntaxParserScope>& DeclaredScope,
		const TWeakPtr<FUnrealLuaSyntaxVariable>& ParentVariable, bool isLocalVariable);

	~FUnrealLuaSyntaxVariable();
	TSharedRef<FUnrealLuaSyntaxVariable> AccessField(const FString& fieldName, int32 lineNumber);
	bool IsGlobalVariable() const;
	bool IsLocalVariable() const;
	bool IsFieldVariable() const;
	int32 GetDeclaredLineNumber() const;

	void AddAccessCount()
	{
		this->NumTimesAccessed++;
	}

	void ChangeType(const FString& typeName);
	
	bool ResolveType(const sol::state_view& lua);
	
	const FString& GetType() const;
	bool HasVarTypeString() const { return !this->TypeName.IsEmpty(); }
	bool IsResolved() const;
	FLuaValue& GetLuaValue() { return this->LuaValue;}

	FString VariableName = "";
	FString TypeName = "";
	int32 DeclaredLineNumber = -1;
	TWeakPtr<FUnrealLuaSyntaxParserScope> DeclaredScope = nullptr;
	TWeakPtr<FUnrealLuaSyntaxVariable> ParentVariable = nullptr;
	TArray<TSharedPtr<FUnrealLuaSyntaxVariable>> Fields = {};
	FLuaValue LuaValue = {};
	bool bIsLocalVariable = false;
	int32 NumTimesAccessed = 0;
};

struct UNREALLUATOOLS_API FUnrealLuaSyntaxFieldAccess
{
	FString FieldName = "";
	TWeakPtr<FUnrealLuaSyntaxVariable> BaseVariable = nullptr;
	TWeakPtr<FUnrealLuaSyntaxFieldAccess> ParentField = nullptr;
};

struct UNREALLUATOOLS_API FUnrealLuaSyntaxParserScope : TSharedFromThis<FUnrealLuaSyntaxParserScope>
{
	FUnrealLuaSyntaxParserScope(TSharedPtr<FUnrealLuaSyntaxParserScope> parentScope, const FString& keyword, int32 currentLine);
	~FUnrealLuaSyntaxParserScope();
	int32 GetScopeLevel() const
	{
		return this->ScopeLevel;
	}
		
	TWeakPtr<FUnrealLuaSyntaxVariable> FindVariable(const FString& varName, bool lookInParents);

	TSharedRef<FUnrealLuaSyntaxVariable> AddVariable(const FString& newVar, int32 lineNumber, const FString& variableType, bool isLocalVariable);

	bool IsGlobalScope() const;
	bool IsLocalScope() const;

	TSharedPtr<FUnrealLuaSyntaxParserScope> GetParentScope() const;
	void SetLeavingScope(const FString& keyword, int32 line);
	const FString& GetScopeOpenKeyword() const { return this->ScopeStartKeyword; }
	int32 GetScopeOpenLineNumber() const { return this->ScopeLineBegin; }
	void AddChildScope(const TSharedPtr<FUnrealLuaSyntaxParserScope>& child);

private:
	bool ScopeIsOpen = true;
	int32 ScopeLevel = 0;
	int32 ScopeLineBegin = 0;
	int32 ScopeLineEnd = 0;
	FString ScopeStartKeyword = "";
	FString ScopeEndKeyword = "";
	TArray<TSharedPtr<FUnrealLuaSyntaxVariable>> LocalVars{};
		
	TArray<TSharedPtr<FUnrealLuaSyntaxParserScope>> ChildScopes{};
	TWeakPtr<FUnrealLuaSyntaxParserScope> ParentScope = nullptr;
};
	