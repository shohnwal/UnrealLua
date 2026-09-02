// Fill out your copyright notice in the Description page of Project Settings.


#include "Prototypes/UScriptStructPrototype.h"

#include "Prototypes/UPropertyPrototype.h"

#include "LuaTypes/LuaPrimitives.h"
#include "sol/sol.hpp"
#include "UnrealLuaCompiler.h"
#include "Prototypes/UFunctionPrototype.h"
#include "Utility/LuaLogMacros.h"


UnrealLua::Compiler::FUnrealLuaCompilerUScriptStructPrototype::FUnrealLuaCompilerUScriptStructPrototype()
{
	this->CurrentStep = EUnrealLuaCompilerUScriptStructCompilerStep::StructName;
}


void UnrealLua::Compiler::FUnrealLuaCompilerUScriptStructPrototype::SetIsError(const FString& reason)
{
	IPrototypeBase::SetIsError();
	this->CurrentStep = EUnrealLuaCompilerUScriptStructCompilerStep::Error;
	FString errorHeader = FString::Printf(TEXT("Unable to compile UnrealLua UScriptStruct %s with parent UScriptStruct %s from file %s. Reason:\n%s"), *this->GetFullPathString(), *GetFullNameSafe(this->ParentStruct), *this->FileName, *reason);
	UUnrealLuaCompiler::SetError(errorHeader);
}

void UnrealLua::Compiler::FUnrealLuaCompilerUScriptStructPrototype::Run(UUnrealLuaCompiler* compiler, const sol::variadic_args& args)
{
	//LUA_LOG("Running ScriptStruct constructor")
	IStructPrototypeBase::Run(compiler, args);
	if (this->GetIsError())
	{
		return;
	}
	
	if (args.size() < 1)
	{
		this->SetIsError("No arguments provided, need at least 1 string argument for struct name");
		return;
	}
	
	HandleScriptStructNameStep(args[0]);
	
	if (this->GetIsError())
	{
		this->SetIsError("ScriptStruct prototype has errors");
		return;
	}
	
	sol::stack_object parent{args.lua_state(), sol::nil};
	
	if (args.size() < 2)
	{
;		parent = args[1];
	}
	
	HandleParentStructStep(parent);
}

void UnrealLua::Compiler::FUnrealLuaCompilerUScriptStructPrototype::__call(const sol::variadic_args& args)
{
	if (this->GetIsError())
	{
		this->SetIsError("UScriptStruct has errors during call!");
		return;
	}
	verify(this->CurrentStep == EUnrealLuaCompilerUScriptStructCompilerStep::Body);
	
	if (args.size() < 1)
	{
		this->SetIsError("No args provided for body, expected table.");
		return;
	}
	HandleBodyStep(args[0]);
}

UnrealLua::Compiler::EUnrealLuaCompilerPrototypeType UnrealLua::Compiler::FUnrealLuaCompilerUScriptStructPrototype::GetPrototypeCategory()
{
	return EUnrealLuaCompilerPrototypeType::ScriptStruct;
}

TArray<UnrealLua::Compiler::FUnrealLuaCompilerUPropertyPrototype>& UnrealLua::Compiler::FUnrealLuaCompilerUScriptStructPrototype::GetPropertyPrototypes()
{
	return this->Properties;
}

UScriptStruct* UnrealLua::Compiler::FUnrealLuaCompilerUScriptStructPrototype::GetParentStruct() const
{
	return this->ParentStruct;
}

bool UnrealLua::Compiler::FUnrealLuaCompilerUScriptStructPrototype::IsCommitReady()
{
	return !this->GetIsError() && this->CurrentStep == EUnrealLuaCompilerUScriptStructCompilerStep::Prototype;
}

void UnrealLua::Compiler::FUnrealLuaCompilerUScriptStructPrototype::SetCommitReady()
{
	verify(!this->GetIsError())
	this->CurrentStep = EUnrealLuaCompilerUScriptStructCompilerStep::Prototype;
}

void UnrealLua::Compiler::FUnrealLuaCompilerUScriptStructPrototype::HandleScriptStructNameStep(const sol::stack_object& arg)
{
	//LUA_LOG("ScriptStruct prototype name step")
	if (this->GetIsError())
	{
		LUA_LOG_ERROR("ScriptStruct prototype has errors, returning")
		return;
	}
	
	if(!arg.valid())
	{
		this->SetIsError("No argument provided for ScriptStruct name, expected string");
		return;
	}
	
	sol::string_view scriptStructName = arg.as<sol::string_view>();
	if (scriptStructName.empty())
	{
		this->SetIsError("ScriptStruct name string is empty");
		return;
	}
	
	if (!std::isupper(scriptStructName[0]))
	{
		this->SetIsErrorWithArgs("New ScriptStruct name must start with an uppercase letter: %s", scriptStructName.data());
		return;
	}
	
	this->TypeName = scriptStructName.data();
	this->CurrentStep = EUnrealLuaCompilerUScriptStructCompilerStep::ParentStructName;
	
	//LUA_LOG("ScriptStruct prototype name step %s finished", *this->TypeName.ToString())
}

void UnrealLua::Compiler::FUnrealLuaCompilerUScriptStructPrototype::HandleParentStructStep(const sol::stack_object& arg)
{
	if (arg.valid())
	{
		//@TODO : handle valid parent struct
		
	}
	this->CurrentStep = EUnrealLuaCompilerUScriptStructCompilerStep::Body;
}

void UnrealLua::Compiler::FUnrealLuaCompilerUScriptStructPrototype::SetCompiledStruct(UScriptStruct* newScriptStruct)
{
	verify(this->CompiledScriptStruct == nullptr);
	verify(IsValid(newScriptStruct));
	this->CompiledScriptStruct = newScriptStruct;
}


UField* UnrealLua::Compiler::FUnrealLuaCompilerUScriptStructPrototype::GetCompiledField() const
{
	return this->CompiledScriptStruct;
}

UScriptStruct* UnrealLua::Compiler::FUnrealLuaCompilerUScriptStructPrototype::GetCompiledScriptStruct() const
{
	return this->CompiledScriptStruct;
}

void UnrealLua::Compiler::FUnrealLuaCompilerUScriptStructPrototype::AddProperty(const FUnrealLuaCompilerUPropertyPrototype& protoToAdd)
{
	verify(!protoToAdd.GetIsError())
	//LUA_LOG("UScriptStruct prototype %s adding new property prototype %s", *this->TypeName.ToString(), *protoToAdd.GetTypeName().ToString())
	this->Properties.Add(protoToAdd);	
}

void UnrealLua::Compiler::FUnrealLuaCompilerUScriptStructPrototype::AddUFunction(const FUnrealLuaCompilerUFunctionPrototype& protoToAdd)
{
	this->SetIsErrorWithArgs("UFunction %s not allowed in ScriptStruct!", *protoToAdd.GetTypeName().ToString());
}

TArray<UnrealLua::Compiler::FUnrealLuaCompilerUPropertyPrototype>& UnrealLua::Compiler::FUnrealLuaCompilerUScriptStructPrototype::GetProperties()
{
	return this->Properties;
}
