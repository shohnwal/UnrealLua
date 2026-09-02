// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StructPrototypeBase.h"
#include "UObject/ObjectPtr.h"
class UUnrealLuaCompiler;

namespace UnrealLua::Compiler
{
	enum class EUnrealLuaUTypeInterfaceCompilerStep : uint8
	{
		InterfaceName,
		Body,
		Commit,
		Error
	};

	struct UNREALLUA_API FUnrealLuaCompilerUInterfacePrototype : public IStructPrototypeBase
	{
		FUnrealLuaCompilerUInterfacePrototype();

		virtual ~FUnrealLuaCompilerUInterfacePrototype() override = default;
		virtual void Run(UUnrealLuaCompiler* compiler, const sol::variadic_args& args) override;
		void __call(const sol::variadic_args& arg);
	
		void HandleInterfaceNameStep(sol::stack_object arg);
		virtual void HandleBodyStep(sol::stack_object arg) override;
	protected:
		virtual void SetCommitReady() override;
	public:
		virtual bool IsCommitReady() override;
		//virtual UClass* Step(UUnrealLuaUTypeCompiler* compiler, UClass* clazz) override;
	
		virtual void SetIsError(const FString& reason) override;
		virtual bool IsPropertyNameTaken(const FString& propName) const;
		void HandleInterfaceFlags(sol::table& classFlags);

		virtual bool AreUFunctionsAllowed() const override
		{
			return true;
		}
		virtual bool AreUPropertiesAllowed() const override
		{
			return false;
		}
		virtual void AddUFunction(const FUnrealLuaCompilerUFunctionPrototype& protoToAdd) override;
		virtual EUnrealLuaCompilerPrototypeType GetPrototypeCategory() override
		{
			return EUnrealLuaCompilerPrototypeType::Interface;
		}

		virtual TArray<UnrealLua::Compiler::FUnrealLuaCompilerUFunctionPrototype>& GetFunctionPrototypes() override;
		
		virtual UField* GetCompiledField() const override;
		UClass* GetCompiledClass() const;
		void SetCompiledClass(UClass* Class);
		
		virtual TArray<FUnrealLuaCompilerUPropertyPrototype>& GetProperties() override;
		virtual void AddProperty(const FUnrealLuaCompilerUPropertyPrototype& protoToAdd) override;

		TObjectPtr<UClass> CompiledInterfaceClass = nullptr;
		
		TMap<FString, FString> Meta = {};
	
		TArray<FUnrealLuaCompilerUFunctionPrototype> Functions = {};
		EUnrealLuaUTypeInterfaceCompilerStep CurrentStep = EUnrealLuaUTypeInterfaceCompilerStep::InterfaceName;
	};
}