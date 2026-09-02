#include "Prototypes/UPropertyPrototype.h"
#include "sol/sol.hpp"
#include "UnrealLuaCompiler.h"

void UnrealLua::Compiler::FUnrealLuaCompilerUPropertyPrototype::Assign(const sol::object& arg)
{
	if (this->AssignedTypeOrValue.valid())
	{
		this->SetIsErrorWithArgs("Property %s error: Attempted to assign a new value type when a valid type was already assigned", *this->TypeName.ToString());
		return;
	}
	if (!arg.valid())
	{
		this->SetIsErrorWithArgs("Property %s error: Attempted to assign a nil", *this->TypeName.ToString());
		return;
	}
	this->AssignedTypeOrValue = arg;
}

void UnrealLua::Compiler::FUnrealLuaCompilerUPropertyPrototype::CheckValidity()
{
	if (this->TypeName.IsNone())
	{
		this->SetIsError("Property name is empty");
		return;
	}
	if (!this->AssignedTypeOrValue.valid())
	{
		this->SetIsErrorWithArgs("Property %s error: type is nil", *this->TypeName.ToString());
		return;	
	}
}

UnrealLua::Compiler::FUnrealLuaCompilerUPropertyPrototype::FUnrealLuaCompilerUPropertyPrototype(IPrototypeBase& ownerStruct)
{
	this->OwnerPrototype = &ownerStruct;
	this->Compiler = ownerStruct.Compiler;
}

bool UnrealLua::Compiler::FUnrealLuaCompilerUPropertyPrototype::IsStaticProperty()
{
	return bIsStaticProperty;
}

void UnrealLua::Compiler::FUnrealLuaCompilerUPropertyPrototype::SetIsError(const FString& reason)
{
	IPrototypeBase::SetIsError();
	FString errorMsg = FString::Printf(TEXT("Property %s has errors:\n%s"), *this->GetFullPathString(), *reason);
	this->OwnerPrototype->SetIsError(errorMsg);
}