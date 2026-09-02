#pragma once
#include "CoreMinimal.h"
#include "PrototypeBase.h"
#include "UObject/ObjectMacros.h"
#include "sol/sol.hpp"

namespace UnrealLua::Compiler
{
	struct IStructPrototypeBase;

	struct UNREALLUA_API FUnrealLuaCompilerUPropertyPrototype : public IPrototypeBase
	{
		void Assign(const sol::object& arg);
		virtual EUnrealLuaCompilerPrototypeType GetPrototypeCategory() override
		{
			return EUnrealLuaCompilerPrototypeType::Property;
		}
		FString PropFlagsString = {};
		TArray<FString> PropFlags = {};
		TArray<FString> MetaFlags = {};
		sol::object AssignedTypeOrValue = sol::nil;
		sol::object PropertyCompilerEvaluatedType = sol::nil;
		sol::object EvaluatedDefaultValue = sol::nil;
		EPropertyFlags PropertyFlags = CPF_None;
		FProperty* CompiledProperty = nullptr;
		int32 LineDefined = INT32_MAX;
		bool bIsStaticProperty = false;
		FString ReplicatedUsingFuncName = "";
		
		void CheckValidity();
		FUnrealLuaCompilerUPropertyPrototype(IPrototypeBase& ownerStruct);
		virtual ~FUnrealLuaCompilerUPropertyPrototype() override = default;
		bool IsStaticProperty();
		virtual void SetIsError(const FString& reason) override;
	};
}