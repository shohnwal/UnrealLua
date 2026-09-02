
#include "Prototypes/UFunctionPrototype.h"

#include "HAL/FileManager.h"
#include "LuaTypes/LuaPrimitives.h"
#include "Misc/FileHelper.h"
#include "UnrealLuaCompiler.h"

void UnrealLua::Compiler::FUnrealLuaCompilerUFunctionPrototype::CheckValidity()
{
	if (this->TypeName.IsNone())
	{
		this->SetIsError("UFunction name is empty");
		return;;
	}
	if (!this->Func.valid())
	{
		this->SetIsError("No valid lua function assigned");
		return;	
	}
	
	/*
	lua_State* L = this->Func.lua_state();
	sol::stack::push(L, this->Func);
	lua_Debug ar;
	lua_getinfo(L, ">Sun", &ar);
	if (ar.nups != 0)
	{
		this->SetIsError(FString::Printf(TEXT("UFunction %s has %d upvalues. Compiled UFunctions should not have any upvalues!"), *this->TypeName.ToString(), ar.nups));
		return;
	}*/
}

UnrealLua::Compiler::FUnrealLuaCompilerUFunctionPrototype::FUnrealLuaCompilerUFunctionPrototype(IStructPrototypeBase& ownerStruct)
{
	this->OwnerPrototype = &ownerStruct;
	this->Compiler = ownerStruct.Compiler;
	verify(this->Compiler != nullptr);
}

void UnrealLua::Compiler::FUnrealLuaCompilerUFunctionPrototype::SetIsError(const FString& reason)
{
	IPrototypeBase::SetIsError();
	FString errorMsg = FString::Printf(TEXT("UFunction %s has errors:\n%s"), *this->GetFullPathString(), *reason);
	this->OwnerPrototype->SetIsError(errorMsg);
}