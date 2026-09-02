// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
class UUnrealLuaCompiler;

namespace UnrealLua::Compiler {
	enum class EUnrealLuaCompilerPrototypeType : uint8
	{
		Invalid			= 0,
		Enum			= 1 << 0,
		Class			= 1 << 1,
		ScriptStruct	= 1 << 2,
		Property 		= 1 << 3,
		Function		= 1 << 4,
		Interface		= 1 << 5
	};
	ENUM_CLASS_FLAGS(EUnrealLuaCompilerPrototypeType)

	struct UNREALLUA_API IPrototypeBase
	{
		virtual ~IPrototypeBase() = default;
		virtual EUnrealLuaCompilerPrototypeType GetPrototypeCategory() = 0;
		virtual void SetIsError(const FString& reason) = 0;
		void SetIsError();
		bool GetIsError() const;
		
		template<typename ...Args>
		void SetIsErrorWithArgs(const FString& reason, Args...);
		
		FString GetTypeNameString() const;
		FName GetTypeName() const;
		
		FString GetFullPathString();
		
		//Only valid on field prototypes (property/function), links to owning class/struct/function
		IPrototypeBase* OwnerPrototype = nullptr;
		
		//If property/function: field name. If class/struct : class/struct name 
		FName TypeName = "";
		bool bIsError = false;
		UUnrealLuaCompiler* Compiler = nullptr;
	};

	template <typename... Args>
	void IPrototypeBase::SetIsErrorWithArgs(const FString& reason, Args... args)
	{
		FString output = FString::Format(*reason, {args...});
		this->SetIsError(output);
	}
}
