// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UFunctionPrototype.h"
#include "StructPrototypeBase.h"
#include "UPropertyPrototype.h"
#include "sol/forward.hpp"
#include "UObject/ObjectPtr.h"
#include "UObject/SoftObjectPtr.h"


class UUnrealLuaCompiler;

namespace UnrealLua::Compiler
{
	enum class EUnrealLuaUTypeClassCompilerStep : uint8
	{
		ClassName,
		ParentClassName,
		Body,
		FinishedPrototype,
		Error
	};

	struct UNREALLUA_API FUnrealLuaCompilerUClassPrototype : public IStructPrototypeBase
	{
		FUnrealLuaCompilerUClassPrototype();

		virtual ~FUnrealLuaCompilerUClassPrototype() override = default;
		virtual void Run(UUnrealLuaCompiler* compiler, const sol::variadic_args& args) override;
		void __call(const sol::variadic_args& arg);
		FUnrealLuaCompilerUClassPrototype* AddInterfaces(sol::variadic_args args);
	private:
		void HandleInterfaceItem(sol::object item);
	public:
		//virtual UClass* Step(UUnrealLuaUTypeCompiler* compiler, UClass* clazz) override;
	
		virtual void SetIsError(const FString& reason) override;
		virtual bool IsPropertyNameTaken(const FString& propName) const;
		virtual void SetCommitReady() override;
		virtual bool IsCommitReady() override;
		void HandleClassNameStep(sol::stack_object arg);
		void HandleParentClassStep(sol::stack_object arg);
		void HandleClassFlags(sol::table& classFlags);

		virtual bool AreUFunctionsAllowed() const override
		{
			return true;
		}
		virtual bool AreUPropertiesAllowed() const override
		{
			return true;
		}
		virtual void AddProperty(const FUnrealLuaCompilerUPropertyPrototype& protoToAdd) override;
		virtual void AddUFunction(const FUnrealLuaCompilerUFunctionPrototype& protoToAdd) override;
		virtual EUnrealLuaCompilerPrototypeType GetPrototypeCategory() override
		{
			return EUnrealLuaCompilerPrototypeType::Class;
		}
		UClass* GetParentClass() const;
		
		virtual TArray<FUnrealLuaCompilerUPropertyPrototype>& GetProperties() override;

		TArray<UnrealLua::Compiler::FUnrealLuaCompilerUPropertyPrototype>& GetPropertyPrototypes();
		virtual TArray<UnrealLua::Compiler::FUnrealLuaCompilerUFunctionPrototype>& GetFunctionPrototypes() override;
		
		virtual UField* GetCompiledField() const override;
		UClass* GetCompiledClass() const;
		void SetCompiledClass(UClass* Class);
		
		FSoftClassPath ParentClassPath = {};
		FUnrealLuaCompilerUClassPrototype* ParentClassPrototype = nullptr;
		TObjectPtr<UClass> CompiledClass = nullptr;
		TObjectPtr<UClass> ParentClass = nullptr;
		
		TArray<FSoftClassPath> InterfacePaths = {};
		TArray<TObjectPtr<UClass>> Interfaces = {};
	
		TMap<FString, FString> Meta = {};
	
		TArray<FUnrealLuaCompilerUPropertyPrototype> Properties = {};
		TArray<FUnrealLuaCompilerUFunctionPrototype> Functions = {};
		EUnrealLuaUTypeClassCompilerStep CurrentStep = EUnrealLuaUTypeClassCompilerStep::ClassName;
	};
}