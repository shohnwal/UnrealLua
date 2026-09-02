// Fill out your copyright notice in the Description page of Project Settings.


#include "Prototypes/InterfacePrototype.h"

#include "UnrealLuaCompiler.h"
#include "Prototypes/UFunctionPrototype.h"
#include "Utility/LuaLogMacros.h"

UnrealLua::Compiler::FUnrealLuaCompilerUInterfacePrototype::FUnrealLuaCompilerUInterfacePrototype()
{
	this->CurrentStep = EUnrealLuaUTypeInterfaceCompilerStep::InterfaceName;
}

void UnrealLua::Compiler::FUnrealLuaCompilerUInterfacePrototype::Run(UUnrealLuaCompiler* compiler, const sol::variadic_args& args)
{
	IStructPrototypeBase::Run(compiler, args);
	if (this->GetIsError())
	{
		return;
	}
	
	if (args.size() < 1)
	{
		this->SetIsError(FString::Printf(TEXT("No arguments provided, need at least 1 arguments for interface name string")));
		return;
	}
	HandleInterfaceNameStep(args[0]);
	
	if (this->GetIsError())
	{
		return;
	}
	
	if(args.size() < 2)
	{
		return;
	}
	
	//Optional flags and metadata
	sol::object flagsMaybe = args[2];
	
	if (flagsMaybe.get_type() != sol::type::table || !flagsMaybe.is<sol::table>())
	{
		return;
	}
	
	sol::table classFlags = flagsMaybe.as<sol::table>();
	
	HandleInterfaceFlags(classFlags);
	
	return;
}

void UnrealLua::Compiler::FUnrealLuaCompilerUInterfacePrototype::__call(const sol::variadic_args& args)
{
	if (this->GetIsError())
	{
		UUnrealLuaCompiler::SetError();
	}
	verify(this->CurrentStep == EUnrealLuaUTypeInterfaceCompilerStep::Body);
	
	if (args.size() < 1)
	{
		this->SetIsError("No args provided for body, expected table.");
		return;
	}
	HandleBodyStep(args[0]);
}

void UnrealLua::Compiler::FUnrealLuaCompilerUInterfacePrototype::HandleInterfaceNameStep(sol::stack_object arg)
{
	if (!arg.valid())
	{
		this->SetIsError("Expected interface name string");
		return;
	}
	UUnrealLuaCompiler* compiler = UUnrealLuaCompiler::Get();
	const FString& fileName = compiler->CurrentlyRunFilePath;
	
	if (arg.get_type() != sol::type::string)
	{
		this->SetIsError("Expected string for InterfaceName");
		return;
	}
	
	sol::string_view classname = arg.as<sol::string_view>();
	if (classname.empty())
	{
		this->SetIsError("Interface name string is empty");
		return;
	}
	
	FString className = classname.data();
	
	if (!std::isupper(classname[0]))
	{
		this->SetIsError(FString::Printf(TEXT("New Interface name %s must start with an uppercase letter"), *className));
		return;
	}
	
	this->TypeName = *className;
	this->CurrentStep = EUnrealLuaUTypeInterfaceCompilerStep::Body;
}

void UnrealLua::Compiler::FUnrealLuaCompilerUInterfacePrototype::HandleBodyStep(sol::stack_object arg)
{
		if (!arg.valid())
		{
			this->SetIsError("No interface body provided");
			return;
		}
		
		if (arg.get_type() != sol::type::table)
		{
			this->SetIsError("Interface body is not a table");
			return;
		}
		
		sol::table body = arg.as<sol::table>();
		int32 bodySize = body.size();
		
		//create properties for types with missing UProperties/UFunction declarations
		body.for_each_stack([this](sol::stack_object key, sol::stack_object value)
		{
			if (this->GetIsError())
			{
				return;
			}
			verify(value.valid()) // having a table entry with nil would be strange, but check just in case
			verify(key.valid()) // having a table entry with nil would be strange, but check just in case
			
			//The key determines the function name, so it must be a string
			if (key.get_type() != sol::type::string)
			{
				return;
			}
			sol::string_view keyStrv = key.as<sol::string_view>();
			
			if (keyStrv.empty())
			{
				this->SetIsError("Found empty key for property");
				return;
			}
			
			if (value.get_type() == sol::type::function)
			{
				sol::function func = value.as<sol::function>();
				
				FUnrealLuaCompilerUFunctionPrototype funcProto = FUnrealLuaCompilerUFunctionPrototype(*this);
				funcProto.TypeName = keyStrv.data();
				funcProto.Func = func;
				funcProto.FuncFlags = TArray<FString>({TEXT("BlueprintCallable")});
				funcProto.CheckValidity();
				verify(!funcProto.GetIsError())
				this->AddUFunction(funcProto);
			}
		});
		
		if (this->GetIsError())
		{
			LUA_LOG_ERROR("UClass prototype has errors, returning")
			return;
		}

		//check for duplicate fields will be done during compilation of FProperties
		
		this->SetCommitReady();
		UUnrealLuaCompiler* compiler = UUnrealLuaCompiler::Get();
		compiler->CommitPrototype(this);
}

void UnrealLua::Compiler::FUnrealLuaCompilerUInterfacePrototype::SetCommitReady()
{
	verify(!this->GetIsError())
	this->CurrentStep = EUnrealLuaUTypeInterfaceCompilerStep::Commit;
}

bool UnrealLua::Compiler::FUnrealLuaCompilerUInterfacePrototype::IsCommitReady()
{
	return !this->GetIsError() && this->CurrentStep == EUnrealLuaUTypeInterfaceCompilerStep::Commit;
}

void UnrealLua::Compiler::FUnrealLuaCompilerUInterfacePrototype::SetIsError(const FString& reason)
{
	this->CurrentStep = EUnrealLuaUTypeInterfaceCompilerStep::Error;
	FString errorHeader = FString::Printf(TEXT("Unable to compile UnrealLua UInterface %s from file %s\nReason:\n%s"), *this->TypeName.ToString(), *this->FileName, *reason);
	UUnrealLuaCompiler::SetError(errorHeader);
}

bool UnrealLua::Compiler::FUnrealLuaCompilerUInterfacePrototype::IsPropertyNameTaken(const FString& propName) const
{
	return false;
}

void UnrealLua::Compiler::FUnrealLuaCompilerUInterfacePrototype::HandleInterfaceFlags(sol::table& classFlags)
{
}

void UnrealLua::Compiler::FUnrealLuaCompilerUInterfacePrototype::AddUFunction(const FUnrealLuaCompilerUFunctionPrototype& protoToAdd)
{
	this->Functions.Add(protoToAdd);
}

TArray<UnrealLua::Compiler::FUnrealLuaCompilerUFunctionPrototype>& UnrealLua::Compiler::FUnrealLuaCompilerUInterfacePrototype::GetFunctionPrototypes()
{
	return this->Functions;
}

UField* UnrealLua::Compiler::FUnrealLuaCompilerUInterfacePrototype::GetCompiledField() const
{
	return this->CompiledInterfaceClass;
}

UClass* UnrealLua::Compiler::FUnrealLuaCompilerUInterfacePrototype::GetCompiledClass() const
{
	return this->CompiledInterfaceClass;
}

void UnrealLua::Compiler::FUnrealLuaCompilerUInterfacePrototype::SetCompiledClass(UClass* Class)
{
	verify(Class->IsChildOf(UInterface::StaticClass()))
	this->CompiledInterfaceClass = Class;
}

TArray<UnrealLua::Compiler::FUnrealLuaCompilerUPropertyPrototype>& UnrealLua::Compiler::FUnrealLuaCompilerUInterfacePrototype::GetProperties()
{
	checkNoEntry();
	static TArray<UnrealLua::Compiler::FUnrealLuaCompilerUPropertyPrototype> dummyArray;
	return dummyArray;
}

void UnrealLua::Compiler::FUnrealLuaCompilerUInterfacePrototype::AddProperty(const FUnrealLuaCompilerUPropertyPrototype& protoToAdd)
{
	checkNoEntry();
}
