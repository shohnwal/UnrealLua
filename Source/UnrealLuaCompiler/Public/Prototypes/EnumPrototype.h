// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StructPrototypeBase.h"

/**
 * 
 */
namespace UnrealLua::Compiler
{
	struct UNREALLUA_API FUnrealLuaCompilerUEnumPrototype : public IStructPrototypeBase
	{
		virtual void Run(UUnrealLuaCompiler* compiler, const sol::variadic_args& args) override;
		void __call(const sol::variadic_args& args);
		void HandleEnumBodyStep(sol::stack_object value);
		void HandleEnumMetaStep(sol::table& metaTable);

		void BuildEnumFromTable(const sol::table& tbl);
		void BuildEnumFromDirectory(const sol::string_view strv);
		void BuildEnumFromFileTable(const sol::string_view strv);
		virtual void SetIsError(const FString& reason) override;
		virtual EUnrealLuaCompilerPrototypeType GetPrototypeCategory() override;
		virtual UField* GetCompiledField() const override;
		UEnum* GetCompiledEnum() const;
		TArray<TPair<FName, int64>> GetEnumValues() const;
		void SetCompiledEnum(UEnum* uenum);
		virtual TArray<FUnrealLuaCompilerUPropertyPrototype>& GetProperties() override;
		
		virtual bool IsCommitReady() override;
	protected:
		virtual void SetCommitReady() override;
	private:
		void HandleEnumNameStep(sol::stack_object Stack_Proxy);
		void SetEnumPrototypeValues(const TArray<TPair<FName, int64>>& pairs);

	public:
		virtual bool AreUFunctionsAllowed() const override { return false; }
		virtual bool AreUPropertiesAllowed() const override { return false; }
		virtual void AddProperty(const FUnrealLuaCompilerUPropertyPrototype& protoToAdd) override;
		virtual void AddUFunction(const FUnrealLuaCompilerUFunctionPrototype& protoToAdd) override;

	private:
		TObjectPtr<UEnum> CompiledEnum = nullptr;
		FString DefaultValue = "";
		FString ForeignTargetPackage = "";
		bool ExtendEnum = false;
		bool bIsCommitReady = false;
		TArray<TPair<FName, int64>> ElementPrototypes = {};
	};
}
