// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StructPrototypeBase.h"
#include "UPropertyPrototype.h"
#include "UObject/ObjectPtr.h"

/**
 * 
 */
namespace UnrealLua::Compiler
{
	enum class EUnrealLuaCompilerUScriptStructCompilerStep : uint8
	{
		StructName,
		ParentStructName,
		Body,
		Prototype,
		Error
	};

	struct UNREALLUA_API FUnrealLuaCompilerUScriptStructPrototype : public IStructPrototypeBase
	{
		FUnrealLuaCompilerUScriptStructPrototype();
		virtual ~FUnrealLuaCompilerUScriptStructPrototype() override = default;
	
		virtual void SetIsError(const FString& reason) override;
	
		virtual void Run(UUnrealLuaCompiler* compiler, const sol::variadic_args& arg) override;
		void __call(const sol::variadic_args& arg);
		virtual EUnrealLuaCompilerPrototypeType GetPrototypeCategory() override;
		TArray<FUnrealLuaCompilerUPropertyPrototype>& GetPropertyPrototypes();
		UScriptStruct* GetParentStruct() const;
		virtual bool IsCommitReady() override;
	protected:
		virtual void SetCommitReady() override;
	private:
		void HandleScriptStructNameStep(const sol::stack_object& arg);
		void HandleParentStructStep(const sol::stack_object& arg);

	public:
		virtual bool AreUFunctionsAllowed() const override
		{
			return false;
		}
		virtual bool AreUPropertiesAllowed() const override
		{
			return true;
		}
		
		void SetCompiledStruct(UScriptStruct* newScriptStruct);
		virtual UField* GetCompiledField() const override;
		UScriptStruct* GetCompiledScriptStruct() const;
		
		virtual void AddProperty(const FUnrealLuaCompilerUPropertyPrototype& protoToAdd) override;
		virtual void AddUFunction(const FUnrealLuaCompilerUFunctionPrototype& protoToAdd) override;
		virtual TArray<FUnrealLuaCompilerUPropertyPrototype>& GetProperties() override;

	private:
		TObjectPtr<UScriptStruct> CompiledScriptStruct = nullptr;
		TObjectPtr<UScriptStruct> ParentStruct = nullptr;
		TArray<FUnrealLuaCompilerUPropertyPrototype> Properties = {};
		EUnrealLuaCompilerUScriptStructCompilerStep CurrentStep = EUnrealLuaCompilerUScriptStructCompilerStep::StructName;
	};
}