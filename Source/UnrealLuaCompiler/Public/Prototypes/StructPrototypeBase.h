// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PrototypeBase.h"
#include "sol/sol.hpp"
#include "LuaCompilerSourceFileInfo.h"
#include "Prototypes/UPropertyPrototype.h"

class UUnrealLuaCompiler;

namespace UnrealLua::Compiler {
	struct FUnrealLuaCompilerUFunctionPrototype;

	struct UNREALLUA_API IStructPrototypeBase : public IPrototypeBase
	{
		virtual ~IStructPrototypeBase() override = default;
		
		virtual void Run(UUnrealLuaCompiler* compiler, const sol::variadic_args& args); 
		virtual void HandleBodyStep(sol::stack_object arg);
		virtual bool AreUFunctionsAllowed() const = 0;
		virtual bool AreUPropertiesAllowed() const = 0;
		virtual bool IsCommitReady() = 0;
		bool ParseBody();
		virtual TArray<FUnrealLuaCompilerUPropertyPrototype>& GetProperties() = 0;
	protected:
		virtual void SetCommitReady() = 0;
		bool ParseProperties(const TArrayView<FLuaCompilerSourceFileLine> linesView);
		virtual TArray<UnrealLua::Compiler::FUnrealLuaCompilerUFunctionPrototype>& GetFunctionPrototypes();
		bool ParseFunctions(const TArrayView<FLuaCompilerSourceFileLine> linesView);
	public:
		virtual void AddProperty(const FUnrealLuaCompilerUPropertyPrototype& protoToAdd) = 0;	
		virtual void AddUFunction(const FUnrealLuaCompilerUFunctionPrototype& protoToAdd) = 0;
		void SetFinishedCompilation();
		bool HasFinishedCompilation() const;
		virtual UField* GetCompiledField() const = 0;
		
		//Source code line UCLASS/USTRUCT was called on
		//NOTE: This is the zero-indexed (Lua-indexed line - 1)
		int32 DefinedLine = -1;
		FString FileName = "";
	private:
		bool CompilationFinished = false;
	};
}
