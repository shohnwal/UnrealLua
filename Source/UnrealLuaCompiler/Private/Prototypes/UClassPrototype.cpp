// Fill out your copyright notice in the Description page of Project Settings.


#include "Prototypes/UClassPrototype.h"

#include "HAL/FileManager.h"
#include "Internationalization/Regex.h"
#include "LuaTypes/LuaPrimitives.h"
#include "LuaTypes/LuaUClass.h"
#include "Misc/FileHelper.h"
#include "UnrealLuaCompiler.h"

UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype::FUnrealLuaCompilerUClassPrototype(): ParentClass(), CurrentStep() 
{
	this->CurrentStep = EUnrealLuaUTypeClassCompilerStep::ClassName;
}

void UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype::Run(UUnrealLuaCompiler* compiler,const sol::variadic_args& args)
{
	IStructPrototypeBase::Run(compiler, args);
	if (this->GetIsError())
	{
		return;
	}
	
	if (args.size() < 2)
	{
		this->SetIsError(FString::Printf(TEXT("No arguments provided, need at least 2 arguments for struct name string and parent reference")));
		return;
	}
	HandleClassNameStep(args[0]);
	
	if (this->GetIsError())
	{
		return;
	}
	
	HandleParentClassStep(args[1]);
	
	if(args.size() < 3)
	{
		return;
	}
	
	if (this->GetIsError())
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
	
	HandleClassFlags(classFlags);
	
	return;
}

void UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype::__call(const sol::variadic_args& args)
{
	if (this->GetIsError())
	{
		UUnrealLuaCompiler::SetError();
	}
	verify(this->CurrentStep == EUnrealLuaUTypeClassCompilerStep::Body);
	
	if (args.size() < 1)
	{
		this->SetIsError("No args provided for body, expected table.");
		return;
	}
	HandleBodyStep(args[0]);
}

UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype* UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype::AddInterfaces(sol::variadic_args args)
{
	if (this->GetIsError())
	{
		return this;
	}
	if (args.size() < 1 )
	{
		return this;
	}

	if (args[0].get_type() == sol::type::table)
	{
		sol::table tbl = args[0];
		int32 tableSize = tbl.size();
		for (int32 index = 1; index <= tableSize; ++index)
		{
			if (this->GetIsError())
			{
				break;
			}
			sol::object item = tbl[index];
			this->HandleInterfaceItem(item);
		}
	}
	else
	{
		for (int32 index = 0; index < args.size(); ++index)
		{
			if (this->GetIsError())
			{
				break;
			}
			sol::object item = args[index];
			this->HandleInterfaceItem(item);
		}
	}
	return this;
}

void UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype::HandleInterfaceItem(sol::object arg)
{
	
	FSoftClassPath interfaceClass{};
	if (arg.is<FLuaUClass>())
	{
		const FLuaUClass& fuclass = arg.as<FLuaUClass>();
		if(!fuclass.Valid())
		{
			this->SetIsError("Lua-provided LuaUClass reference for class interface is not valid");
			return;
		}
		interfaceClass = fuclass.GetSoftClassPath();
	}
	else if (arg.get_type() == sol::type::string)
	{
		sol::string_view strv = arg.as<sol::string_view>();
		if(strv.empty())
		{
			this->SetIsError("Interface class string is empty");
			return;
		}
		if (strv.starts_with("/"))
		{
			//Could be a class path
			FSoftClassPath path{strv.data()};
			if (path.IsValid())
			{
				interfaceClass = path;
			}
			else
			{
				this->SetIsError(FString::Printf(TEXT("Interface class string %hs is not a valid path"), strv.data()));
				return;
			}
		}
		else
		{
			//Could be globally accessible constructor via import registry
			sol::state_view lua{arg.lua_state()};
			sol::object maybeUClass = lua["UE"][arg];
			if (!maybeUClass.valid())
			{
				std::string typeStr = UnrealLua::LuaTypes::TypeInfo::UTypeInternal(maybeUClass, true, -1);
				this->SetIsError(FString::Printf(TEXT("Interface class object %hs is not a valid path"), typeStr.data()));
				return;
			}
			else if (maybeUClass.get_type() == sol::type::userdata && maybeUClass.is<FLuaUClass>())
			{
				const FLuaUClass& fuclass = maybeUClass.as<FLuaUClass>();
				if(!fuclass.Valid())
				{
					this->SetIsError("Lua-provided LuaUClass reference for class interface is not valid");
					return;
				}
				FSoftClassPath path = fuclass.GetSoftClassPath();
				if (path.IsValid())
				{
					interfaceClass = path;
				}
			}
			else
			{
				std::string typeStr = UnrealLua::LuaTypes::TypeInfo::UTypeInternal(maybeUClass, true, -1);
				this->SetIsError(FString::Printf(TEXT("Interface class object %hs is not a valid path"), typeStr.data()));
				return;
			}
		}
	}
	if (!interfaceClass.IsValid())
	{
		this->SetIsError(FString::Printf(TEXT("Interface class path %s is not valid"), *interfaceClass.GetAssetPathString()));
		return;
	}
	this->InterfacePaths.Add(interfaceClass);
}

void UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype::SetIsError(const FString& reason)
{
	this->CurrentStep = EUnrealLuaUTypeClassCompilerStep::Error;
	IPrototypeBase::SetIsError();
	FString errorHeader = FString::Printf(TEXT("Unable to compile UnrealLua UClass %s with parent UClass %s\n from file %s\nReason:\n%s"), *this->GetFullPathString(), *GetFullNameSafe(this->ParentClass), *this->FileName, *reason);
	this->Compiler->SetError(errorHeader);
}

bool UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype::IsPropertyNameTaken(const FString& propName) const
{
	const bool propExists = this->Properties.ContainsByPredicate([&propName](const FUnrealLuaCompilerUPropertyPrototype& item)
    {
    	return propName == item.GetTypeName();
    });
	
    const bool funcExists = this->Functions.ContainsByPredicate([&propName](const FUnrealLuaCompilerUFunctionPrototype& item)
    {
    	return propName == item.GetTypeName();
    });
    return propExists || funcExists;
}

void UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype::SetCommitReady()
{
	verify(!this->GetIsError())
	this->CurrentStep = EUnrealLuaUTypeClassCompilerStep::FinishedPrototype;
}

bool UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype::IsCommitReady()
{
	return !this->GetIsError() && this->CurrentStep == EUnrealLuaUTypeClassCompilerStep::FinishedPrototype;
}

void UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype::HandleClassNameStep(sol::stack_object arg)
{
	if (!arg.valid())
	{
		this->SetIsError("Expected class name string");
		return;
	}
	UUnrealLuaCompiler* compiler = UUnrealLuaCompiler::Get();
	const FString& fileName = compiler->CurrentlyRunFilePath;
	
	if (arg.get_type() != sol::type::string)
	{
		this->SetIsError("Expected string for ClassName");
		return;
	}
	
	sol::string_view classname = arg.as<sol::string_view>();
	if (classname.empty())
	{
		this->SetIsError("Class name string is empty");
		return;
	}
	
	FString className = classname.data();
	
	if (!std::isupper(classname[0]))
	{
		this->SetIsError(FString::Printf(TEXT("New class name %s must start with an uppercase letter"), *className));
		return;
	}
	
	this->TypeName = *className;
	this->CurrentStep = EUnrealLuaUTypeClassCompilerStep::ParentClassName;
}

void UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype::HandleParentClassStep(sol::stack_object arg)
{
	UUnrealLuaCompiler* compiler = UUnrealLuaCompiler::Get();
	const FString& fileName = compiler->CurrentlyRunFilePath;
	if (!arg.valid())
	{
		this->SetIsError(FString::Printf(TEXT("No parent class given in class from file %s"), *this->FileName));
		return;
	}
	
	FSoftClassPath parentClass{};
	if (arg.is<FLuaUClass>())
	{
		const FLuaUClass& fuclass = arg.as<FLuaUClass>();
		if(!fuclass.Valid())
		{
			this->SetIsError("Lua-provided LuaUClass reference for class parent is not valid");
			return;
		}
		parentClass = fuclass.GetSoftClassPath();
	}
	else if (arg.get_type() == sol::type::string)
	{
		sol::string_view strv = arg.as<sol::string_view>();
		if(strv.empty())
		{
			this->SetIsError("Parent class string is empty");
			return;
		}
		if (strv.starts_with("/"))
		{
			//Could be a class path
			FSoftClassPath path{strv.data()};
			if (path.IsValid())
			{
				parentClass = path;
			}
			else
			{
				this->SetIsError(FString::Printf(TEXT("Parent class string %hs is not a valid path"), strv.data()));
				return;
			}
		}
		else
		{
			//Could be globally accessible constructor via import registry
			sol::state_view lua{arg.lua_state()};
			sol::object maybeUClass = lua["UE"][arg];
			if (maybeUClass.is<FLuaUClass>())
			{
				const FLuaUClass& fuclass = arg.as<FLuaUClass>();
				if(!fuclass.Valid())
				{
					this->SetIsError("Lua-provided LuaUClass reference for class parent is not valid");
					return;
				}
				FSoftClassPath path = fuclass.GetSoftClassPath();
				if (path.IsValid())
				{
					parentClass = path;
				}
			}
		}
	}
	this->ParentClassPath = parentClass;
	if (!parentClass.IsValid())
	{
		this->SetIsError(FString::Printf(TEXT("Parent class path %s is not valid"), *parentClass.GetAssetPathString()));
		return;
	}
	this->CurrentStep = EUnrealLuaUTypeClassCompilerStep::Body;
}

void UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype::HandleClassFlags(sol::table& classFlags)
{
	if (!classFlags.valid())
	{
		return;
	}
	
	if (this->GetIsError())
	{
		return;
	}
	
	classFlags.for_each_stack([this](const sol::stack_object key, sol::stack_object value)
	{
		//@TODO : Check class flags
		if (key.get_type() == sol::type::string)
		{
			
		}
		else if(key.is<int>())
		{
		}
	});
}

void UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype::AddProperty(const FUnrealLuaCompilerUPropertyPrototype& protoToAdd)
{
	verify(!protoToAdd.GetIsError())
	//LUA_LOG("UClass prototype %s adding new property prototype %s", *this->TypeName.ToString(), *protoToAdd.GetTypeName().ToString())
	this->Properties.Add(protoToAdd);	
}

void UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype::AddUFunction(const FUnrealLuaCompilerUFunctionPrototype& protoToAdd)
{
	verify(!protoToAdd.GetIsError())
	//LUA_LOG("UClass prototype %s adding new ufunction prototype %s", *this->TypeName.ToString(), *protoToAdd.GetTypeName().ToString())
	this->Functions.Add(protoToAdd);
}

UClass* UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype::GetParentClass() const
{
	return this->ParentClass;
}

TArray<UnrealLua::Compiler::FUnrealLuaCompilerUPropertyPrototype>& UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype::GetProperties()
{
	return this->Properties;
}

TArray<UnrealLua::Compiler::FUnrealLuaCompilerUPropertyPrototype>& UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype::GetPropertyPrototypes()
{
	return this->Properties;
}

TArray<UnrealLua::Compiler::FUnrealLuaCompilerUFunctionPrototype>& UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype::GetFunctionPrototypes()
{
	return this->Functions;
}

UField* UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype::GetCompiledField() const
{
	return this->CompiledClass;
}

UClass* UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype::GetCompiledClass() const
{
	return this->CompiledClass;
}

void UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype::SetCompiledClass(UClass* Class)
{
	verify(this->CompiledClass == nullptr)
	verify(IsValid(Class))
	this->CompiledClass = Class;
}
