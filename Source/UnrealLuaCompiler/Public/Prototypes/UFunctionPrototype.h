#pragma once
#include "CoreMinimal.h"
#include "PrototypeBase.h"
#include "UPropertyPrototype.h"
#include "sol/sol.hpp"

namespace UnrealLua::Compiler
{
	struct UNREALLUA_API FUnrealLuaCompilerUFunctionPrototype : public IPrototypeBase
	{
		virtual EUnrealLuaCompilerPrototypeType GetPrototypeCategory() override
		{
			return EUnrealLuaCompilerPrototypeType::Function;
		}

		void CheckValidity();
		
		FString ParamString = "";
		TArray<FString> FuncFlags = {};
		TMap<FString, FString> FuncMap = {};
		sol::function Func = sol::nil;
		TArray<FUnrealLuaCompilerUPropertyPrototype> Params = {};
		UFunction* ParentFunc = nullptr;
		
		bool bIsBlueprintCallable:1 = false;
		bool bIsSelfCall:1 = false;
		bool bIsStaticCall:1 = false;
		bool bIsClientRPC:1 = false;
		bool bIsServerRPC:1 = false;
		bool bIsMulticastRPC:1 = false;
		bool bIsReliableRPC:1 = false;
		
		FUnrealLuaCompilerUFunctionPrototype(IStructPrototypeBase& ownerStruct);
		virtual void SetIsError(const FString& reason) override;
	};
}