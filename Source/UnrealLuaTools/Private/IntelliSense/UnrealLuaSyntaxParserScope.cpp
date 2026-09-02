// Fill out your copyright notice in the Description page of Project Settings.


#include "IntelliSense/UnrealLuaSyntaxParserScope.h"

#include "Utility/LuaLogMacros.h"
#include "UnrealLua.h"

FUnrealLuaSyntaxVariable::FUnrealLuaSyntaxVariable(const FString& VariableName, const FString& TypeName, int32 DeclaredLineNumber, const TWeakPtr<FUnrealLuaSyntaxParserScope>& DeclaredScope,const TWeakPtr<FUnrealLuaSyntaxVariable>& ParentVariable, bool isLocalVariable)
: VariableName(VariableName), TypeName(TypeName), DeclaredLineNumber(DeclaredLineNumber), DeclaredScope(DeclaredScope), ParentVariable(ParentVariable), bIsLocalVariable(isLocalVariable)
{
}

FUnrealLuaSyntaxVariable::~FUnrealLuaSyntaxVariable()
{
	//LUA_LOG("~FUnrealLuaSyntaxVariable %s %d was accessed %d times", *this->VariableName, this->DeclaredLineNumber, this->NumTimesAccessed);
}

TSharedRef<FUnrealLuaSyntaxVariable> FUnrealLuaSyntaxVariable::AccessField(const FString& fieldName, int32 lineNumber)
{
	this->AddAccessCount();
	TSharedPtr<FUnrealLuaSyntaxVariable>* foundField = this->Fields.FindByPredicate(
	[&fieldName](const TSharedPtr<FUnrealLuaSyntaxVariable>& var)
	{
		return var->VariableName.Equals(fieldName);
	});
	if (foundField != nullptr)
	{
		return foundField->ToSharedRef();
	}
	else
	{
		TSharedRef<FUnrealLuaSyntaxVariable> newField = MakeShared<FUnrealLuaSyntaxVariable>(fieldName, "", lineNumber, nullptr, this->AsShared(), false);
		this->Fields.Emplace(newField);
		return newField;
	}
}

bool FUnrealLuaSyntaxVariable::IsGlobalVariable() const
{
	return !this->IsLocalVariable();
}

bool FUnrealLuaSyntaxVariable::IsLocalVariable() const
{
	return this->bIsLocalVariable;
}

bool FUnrealLuaSyntaxVariable::IsFieldVariable() const
{
	return this->ParentVariable.IsValid();
}

int32 FUnrealLuaSyntaxVariable::GetDeclaredLineNumber() const
{
	return this->DeclaredLineNumber;
}

void FUnrealLuaSyntaxVariable::ChangeType(const FString& typeName)
{
	if (!this->HasVarTypeString())
	{
		this->TypeName = typeName;
	}
}

bool FUnrealLuaSyntaxVariable::ResolveType(const sol::state_view& lua)
{
	if (this->IsResolved())
	{
		return true;
	}
	if (this->TypeName.IsEmpty())
	{
		if (this->IsFieldVariable())
		{
			TSharedPtr<FUnrealLuaSyntaxVariable> parentVar = this->ParentVariable.Pin();
			parentVar->ResolveType(lua);
			FLuaValue& val = parentVar->GetLuaValue();
			switch (val.GetTypeIndex())
			{
			case LuaValueData::IndexOfType<FLuaUClass>():
				{
					FLuaUClass luaClass = val.Get<FLuaUClass>();
					UClass* uclass = luaClass.TryLoadClass();
					if (uclass)
					{
						if (FProperty* prop = uclass->FindPropertyByName(*this->VariableName))
						{
							if (FStructProperty* sprop = CastField<FStructProperty>(prop))
							{
								FLuaUStruct uss{sprop->Struct.Get()};
								this->LuaValue.Emplace<FLuaUStruct>(uss);
								return true;
							}
						}
						else if (uclass->FindFunctionByName(*this->VariableName))
						{
							return true;
						}
					}
				}
				break;
		
			case LuaValueData::IndexOfType<FLuaUStruct>():
				{
					const FLuaUStruct& luaStruct = val.Get<FLuaUStruct>();
					UScriptStruct* ss = luaStruct.TryLoad();
					if (ss)
					{
						if (FProperty* prop = ss->FindPropertyByName(*this->VariableName))
						{
							if (FStructProperty* sprop = CastField<FStructProperty>(prop))
							{
								FLuaUStruct uss{sprop->Struct.Get()};
								this->LuaValue.Emplace<FLuaUStruct>(uss);
								return true;
							}
						}
					}
					break;
				}
				default: 
				break;
			}
			this->LuaValue.SetValue(sol::nil);
			return true;
		}
		else
		{
			this->LuaValue.SetValue(sol::nil);
			return true;			
		}

	}
	sol::object import = lua["UE"][this->TypeName];
	this->LuaValue = import;
	return true;
}

const FString& FUnrealLuaSyntaxVariable::GetType() const
{
	return this->TypeName;
}

bool FUnrealLuaSyntaxVariable::IsResolved() const
{
	return this->LuaValue.IsInitialized();
}

FUnrealLuaSyntaxParserScope::FUnrealLuaSyntaxParserScope(TSharedPtr<FUnrealLuaSyntaxParserScope> parentScope,const FString& keyword, int32 currentLine)
: ScopeLineBegin(currentLine), ScopeStartKeyword(keyword), ParentScope(parentScope) 
{
	if (parentScope)
	{
		this->ScopeLevel = parentScope->GetScopeLevel() + 1;
	}
	else
	{
		this->ScopeLevel = 0;
	}
}

FUnrealLuaSyntaxParserScope::~FUnrealLuaSyntaxParserScope()
{
	//LUA_LOG("~FUnrealLuaSyntaxParserScope %s %d", *this->GetScopeOpenKeyword(), this->GetScopeOpenLineNumber())
}

TWeakPtr<FUnrealLuaSyntaxVariable> FUnrealLuaSyntaxParserScope::FindVariable(const FString& varName, bool lookInParents)
{
	//reversed iteration to find newest declared
	for (int32 index = this->LocalVars.Num() - 1; index >= 0; --index)
	{
		const TSharedPtr<FUnrealLuaSyntaxVariable>& var = this->LocalVars[index];
		if (var->VariableName.Equals(varName))
		{
			return var;
		}
	}
	if (lookInParents && ParentScope.IsValid())
	{
		return this->ParentScope.Pin()->FindVariable(varName, true);
	}
	return nullptr;
}

TSharedRef<FUnrealLuaSyntaxVariable> FUnrealLuaSyntaxParserScope::AddVariable(const FString& newVar, int32 lineNumber, const FString& variableType, bool isLocalVariable)
{
	verify(this->ScopeIsOpen)
	auto var = MakeShared<FUnrealLuaSyntaxVariable>(newVar, variableType, lineNumber, this->AsShared(), nullptr, isLocalVariable);
	//LUA_LOG("new var %s at line %d", *newVar, lineNumber)
	this->LocalVars.Add(var);
	return var;
}

bool FUnrealLuaSyntaxParserScope::IsGlobalScope() const
{
	return this->ScopeLevel == 0;
}

bool FUnrealLuaSyntaxParserScope::IsLocalScope() const
{
	return this->ScopeLevel > 0;
}

TSharedPtr<FUnrealLuaSyntaxParserScope> FUnrealLuaSyntaxParserScope::GetParentScope() const
{
	return this->ParentScope.Pin();
}

void FUnrealLuaSyntaxParserScope::SetLeavingScope(const FString& keyword, int32 line)
{
	verify(this->ScopeIsOpen)
	this->ScopeEndKeyword = keyword;
	this->ScopeLineEnd = line;
	this->ScopeIsOpen = false;
}

void FUnrealLuaSyntaxParserScope::AddChildScope(const TSharedPtr<FUnrealLuaSyntaxParserScope>& child)
{
	this->ChildScopes.AddUnique(child);
}
