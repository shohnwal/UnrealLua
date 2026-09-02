// Fill out your copyright notice in the Description page of Project Settings.


#include "Prototypes/EnumPrototype.h"

#include "Prototypes/UPropertyPrototype.h"

#include "Hash/xxhash.h"
#include "LuaContext/ScopedLuaContext.h"
#include "LuaContext/LuaScripts/LoadedLuaScriptResult.h"
#include "LuaTypes/LuaPrimitives.h"
#include "Utility/LuaFileLister.h"
#include "UnrealLuaCompiler.h"
#include "Utility/LuaLogMacros.h"

void UnrealLua::Compiler::FUnrealLuaCompilerUEnumPrototype::Run(UUnrealLuaCompiler* compiler, const sol::variadic_args& args)
{
	IStructPrototypeBase::Run(compiler, args);
	
	int32 numArgs = args.size();
	if (numArgs < 1)
	{
		this->SetIsError("No args provided for UEnum, expected at least a string name.");
		return;
	}
	
	HandleEnumNameStep(args[0]);
	
	if (this->GetIsError())
	{
		LUA_LOG_ERROR("ScriptStruct prototype has errors, returning")
		UUnrealLuaCompiler::SetError();
	}
	
	if (numArgs > 1 && args[1].is<sol::table>())
	{
		sol::table metaTbl = args[1].as<sol::table>();
		HandleEnumMetaStep(metaTbl);
	}
}

void UnrealLua::Compiler::FUnrealLuaCompilerUEnumPrototype::__call(const sol::variadic_args& args)
{
	/* Valid call args :
	 - A table
	 ->	table of strings and integers
	 - A string ending in .lua
	 -> open file and expect a table of strings and integers
	 - A string
	 -> use string as directory path, each .lua file in that tolder becomes one enum entry
	 */
	if (this->GetIsError())
	{
		UUnrealLuaCompiler::SetError();
		return;
	}
	int32 numArgs = args.size();
	if (numArgs < 1)
	{
		this->SetIsError("No args provided for body, expected table.");
		return;
	}
	HandleEnumBodyStep(args[0]);
	
	
	if (this->GetIsError())
	{
		LUA_LOG_ERROR("UEnum prototype has errors, returning")
		return;
	}

	//check for duplicate fields will be done during compilation of FProperties
		
	this->SetCommitReady();
	UUnrealLuaCompiler* compiler = UUnrealLuaCompiler::Get();
	compiler->CommitPrototype(this);
}

void UnrealLua::Compiler::FUnrealLuaCompilerUEnumPrototype::HandleEnumBodyStep(sol::stack_object value)
{
	if (!value.valid())
	{
		this->SetIsError("Received nil as body argument");
		return;
	}
	if (value.is<sol::table>())
	{
		sol::table tbl = value.as<sol::table>();
		this->BuildEnumFromTable(tbl);
	}
	else if (value.is<std::string>())
	{
		sol::string_view strv = value.as<sol::string_view>();
		if(strv.ends_with(".lua"))
		{
			this->BuildEnumFromFileTable(strv);		
		}
		else
		{
			this->BuildEnumFromDirectory(strv);
		}
	}
	else
	{
		std::string type = UnrealLua::LuaTypes::TypeInfo::UTypeInternal(value, true, -1);
		this->SetIsError(FString::Printf(TEXT("Invalid type for UEnum body: %hs"), type.c_str()));
		return;
	}
}

void UnrealLua::Compiler::FUnrealLuaCompilerUEnumPrototype::HandleEnumMetaStep(sol::table& metaTable)
{
	this->DefaultValue = metaTable["Default"].get_or<std::string, std::string>("").c_str();
	this->ExtendEnum = metaTable["Extend"].get_or<bool, bool>(false);
	this->ForeignTargetPackage = metaTable["Package"].get_or<std::string, std::string>("").c_str();
}

void UnrealLua::Compiler::FUnrealLuaCompilerUEnumPrototype::BuildEnumFromTable(const sol::table& tbl)
{
	if (!tbl.valid())
	{
		this->SetIsError("Table invalid for UEnum body");
		return;
	}
	//FScopedLuaContext* ctx = FScopedLuaContext::GetLuaContextFromLuaState(tbl.lua_state());
	//ctx->ModTable("", tbl);
	
	TArray<TPair<FName, int64>> pairs{};
	FString enumNamespace = this->TypeName.ToString() + TEXT("::");
	if (!DefaultValue.IsEmpty())
	{
		pairs.Add({*(enumNamespace + DefaultValue), 0});
	}
	
	int32 tblsize = tbl.size();
	//LUA_LOG("tblsize is %d", tblsize)
	int64 enumValue = 0;
	if (!DefaultValue.IsEmpty())
	{
		enumValue = 1;
	}
	for (int32 index = 1; index <= tblsize; index++)
	{
		std::string_view strv = tbl[index].get_or<std::string_view, std::string_view>("");
		if (strv.empty())
		{
			continue;
		}
		pairs.Emplace(FName{*(enumNamespace + strv.data())}, enumValue);
		enumValue++;
	}
	
	tbl.for_each_stack([&pairs, &enumNamespace](sol::stack_object key, sol::stack_object value_o)
	{
		if (key.get_type() != sol::type::string || value_o.get_type() != sol::type::number)
		{
			return;
		}
		if (!value_o.is<int>())
		{
			return;
		}
		sol::string_view keyStrv = key.as<sol::string_view>();
		if (keyStrv.empty())
		{
			return;
		}
		int64 value = value_o.as<int64>();
		pairs.Emplace(FName{*(enumNamespace + keyStrv.data())}, value);
	});
	
	this->SetEnumPrototypeValues(pairs);
}

void UnrealLua::Compiler::FUnrealLuaCompilerUEnumPrototype::BuildEnumFromDirectory(const sol::string_view directoryPath)
{
	TArray<TPair<FName, int64>> pairs{};
	FString enumNamespace = this->TypeName.ToString() + TEXT("::");
	if (!DefaultValue.IsEmpty())
	{
		pairs.Add({*(enumNamespace + DefaultValue), 0});
	}
	FLuaPath fakePath{};
	ELuaPathFlags searchPaths = ELuaPathFlags::AnyExceptUTypes;
	fakePath.SetupPackagePaths({}, "", searchPaths);
	
	FString subfolderPath = directoryPath.data();
	
	FLuaFileLister lister{fakePath, subfolderPath, false, {"_", "."}};
	for(FString& item : lister.UniqueFileNames)
	{
		FString realStr = item;
		realStr.RemoveFromEnd(".lua");
		uint64 hash = FXxHash64::HashBuffer(*realStr, realStr.NumBytesWithoutNull()).Hash;
		int64 enumValue = *reinterpret_cast<int64*>(&hash);
		pairs.Emplace(enumNamespace + realStr, enumValue);
	}
	this->SetEnumPrototypeValues(pairs);
}

void UnrealLua::Compiler::FUnrealLuaCompilerUEnumPrototype::BuildEnumFromFileTable(const sol::string_view strv)
{
	FScopedLuaContext& ctx = UUnrealLuaCompiler::Get()->GetCompilerScopedLuaStateContext();
	FLoadLuaScriptResult loadResult = ctx.LoadLuaScriptFromDisk(strv.data(), false, true);
	sol::table pairsLoadInfo = loadResult.FinalResult;
	this->BuildEnumFromTable(pairsLoadInfo);
}

void UnrealLua::Compiler::FUnrealLuaCompilerUEnumPrototype::SetIsError(const FString& reason)
{
	IPrototypeBase::SetIsError();
	FString optionalErrorMsg = FString::Printf(TEXT("Unable to compile UnrealLua UEnum %s from file %s\nReason:\n%s"), *this->GetFullPathString(), *this->FileName, *reason);
	UUnrealLuaCompiler::SetError(optionalErrorMsg);
}

UnrealLua::Compiler::EUnrealLuaCompilerPrototypeType UnrealLua::Compiler::FUnrealLuaCompilerUEnumPrototype::GetPrototypeCategory()
{
	return EUnrealLuaCompilerPrototypeType::Enum;
}

UField* UnrealLua::Compiler::FUnrealLuaCompilerUEnumPrototype::GetCompiledField() const
{
	return this->CompiledEnum;
}

UEnum* UnrealLua::Compiler::FUnrealLuaCompilerUEnumPrototype::GetCompiledEnum() const
{
	return this->CompiledEnum;
}

TArray<TPair<FName, int64>> UnrealLua::Compiler::FUnrealLuaCompilerUEnumPrototype::GetEnumValues() const
{
	return this->ElementPrototypes;
}

void UnrealLua::Compiler::FUnrealLuaCompilerUEnumPrototype::SetCompiledEnum(UEnum* uenum)
{
	verify(this->CompiledEnum == nullptr)
	verify(IsValid(uenum));
	this->CompiledEnum = uenum;
}

TArray<UnrealLua::Compiler::FUnrealLuaCompilerUPropertyPrototype>& UnrealLua::Compiler::FUnrealLuaCompilerUEnumPrototype::GetProperties()
{
	checkNoEntry();
	static TArray<UnrealLua::Compiler::FUnrealLuaCompilerUPropertyPrototype> dummy;
	return dummy;
}

bool UnrealLua::Compiler::FUnrealLuaCompilerUEnumPrototype::IsCommitReady()
{
	return !this->GetIsError() && this->bIsCommitReady;
}

void UnrealLua::Compiler::FUnrealLuaCompilerUEnumPrototype::SetCommitReady()
{
	verify(!this->bIsCommitReady);
	verify(!this->GetIsError())
	this->bIsCommitReady = true;
}

void UnrealLua::Compiler::FUnrealLuaCompilerUEnumPrototype::HandleEnumNameStep(sol::stack_object arg)
{//
	//LUA_LOG("UEnum prototype name step")
	if (this->GetIsError())
	{
		LUA_LOG_ERROR("UEnum prototype has errors, returning")
		return;
	}
	
	if(!arg.valid())
	{
		this->SetIsError("No argument provided for UEnum name, expected string");
		return;	
	}
	
	if (arg.get_type() != sol::type::string)
	{
		std::string type = UnrealLua::LuaTypes::TypeInfo::UTypeInternal(arg, true, -1);
		this->SetIsError(FString::Printf(TEXT("Expected UEnum name string, got %hs instead"), type.c_str()));
		return;
	}
	
	sol::string_view scriptStructName = arg.as<sol::string_view>();
	if (scriptStructName.empty())
	{
		this->SetIsError("UEnum name string is empty");
		return;
	}
	
	this->TypeName = scriptStructName.data();
	//LUA_LOG("Enum prototype name step %s finished", *this->TypeName.ToString())
}
void UnrealLua::Compiler::FUnrealLuaCompilerUEnumPrototype::SetEnumPrototypeValues(const TArray<TPair<FName, int64>>& pairs)
{
	this->ElementPrototypes = pairs;
}

void UnrealLua::Compiler::FUnrealLuaCompilerUEnumPrototype::AddProperty(const FUnrealLuaCompilerUPropertyPrototype& protoToAdd)
{
	
}

void UnrealLua::Compiler::FUnrealLuaCompilerUEnumPrototype::AddUFunction(const FUnrealLuaCompilerUFunctionPrototype& protoToAdd)
{
}
