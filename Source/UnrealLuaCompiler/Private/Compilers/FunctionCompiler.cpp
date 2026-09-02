// Fill out your copyright notice in the Description page of Project Settings.


#include "Compilers/FunctionCompiler.h"

#include "Reflection/PropertyHelper.h"
#include "UnrealLuaCompiler.h"
#include "UnrealLuaCompilerConstants.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UnrealOverrides/UnrealLuaCompiledUFunction.h"
#include "Compilers/PropertyCompiler.h"
#include "Prototypes/UFunctionPrototype.h"

UUnrealLuaCompiledUFunction* CompileUFunction(UClass* uClass, UnrealLua::Compiler::FUnrealLuaCompilerUFunctionPrototype& funcProto)
{
	verify(funcProto.Func.valid())
	verify(!funcProto.GetIsError())
		
	FName funcName = funcProto.TypeName;
	
	bool bIsBlueprintCallable = true;
	bool isStaticFunction = false;
	bool bIsRPC = false;
	bool bIsServerRPC = false;
	bool bIsClientRPC = false;
	bool bIsMulticastRPC = false;
	bool bIsReliableRPC = false;
	
	for (auto& flagstr : funcProto.FuncFlags)
	{
		FStringView flag = flagstr; 
		flag.TrimStartAndEndInline();
		if (flag.Equals(UnrealLua::CompilerConstants::FUNCFLAG_BlueprintCallable, ESearchCase::IgnoreCase))
		{
			bIsBlueprintCallable = true;
		}
		else if (flag.Equals(UnrealLua::CompilerConstants::FUNCFLAG_NotBlueprintCallable, ESearchCase::IgnoreCase))
		{
			bIsBlueprintCallable = false;	
		}
		else if (flag.Equals(UnrealLua::CompilerConstants::FUNCFLAG_Static, ESearchCase::IgnoreCase))
		{
			isStaticFunction = true;
		}
		else if (flag.Equals(UnrealLua::CompilerConstants::FUNCFLAG_ClientRPC, ESearchCase::IgnoreCase))
		{
			bIsRPC = true;
			bIsClientRPC = true;
		}
		else if (flag.Equals(UnrealLua::CompilerConstants::FUNCFLAG_ServerRPC, ESearchCase::IgnoreCase))
		{
			bIsRPC = true;
			bIsServerRPC = true;
		}
		else if (flag.Equals(UnrealLua::CompilerConstants::FUNCFLAG_MulticastRPC, ESearchCase::IgnoreCase))
		{
			bIsRPC = true;
			bIsMulticastRPC = true;
		}
		else if (flag.Equals(UnrealLua::CompilerConstants::FUNCFLAG_ReliableRPC, ESearchCase::IgnoreCase))
		{
			bIsReliableRPC = true;			
		}
		else
		{
			funcProto.SetIsError("Unknown function flag : " + FString(flag));
			return nullptr;
		}
	}
	
	
	funcProto.bIsBlueprintCallable = bIsBlueprintCallable;

	if (funcProto.bIsSelfCall && isStaticFunction)
	{
		funcProto.SetIsError("Function has self parameter but is marked as static!");
		return nullptr;
	}
	else if (!funcProto.bIsSelfCall && !isStaticFunction)
	{
		funcProto.SetIsError("Non-static function is missing self parameter.");
		return nullptr;
	}
	else if (isStaticFunction)
	{
		verify(!funcProto.bIsSelfCall)
		funcProto.bIsStaticCall = true;
	}
	else
	{
		verify(funcProto.bIsSelfCall)
		funcProto.bIsSelfCall = false;
	}
			
	if (bIsRPC)
	{
		if ((bIsServerRPC && bIsClientRPC) || (bIsClientRPC && bIsMulticastRPC) || (bIsServerRPC && bIsMulticastRPC))
		{
			funcProto.SetIsError("Conflicting RPC parameters.");
			return nullptr;
		}
		funcProto.bIsClientRPC = bIsClientRPC;
		funcProto.bIsServerRPC = bIsServerRPC;
		funcProto.bIsMulticastRPC = bIsMulticastRPC;
		funcProto.bIsReliableRPC = bIsReliableRPC;
	}
	
	//we should have already verified field conflicts by now
	
	EFunctionFlags flags = EFunctionFlags::FUNC_None;
	
	//The function should not be in the current class
	verify(!uClass->FindFunctionByName(funcName, EIncludeSuperFlag::ExcludeSuper));
	//but there might be a fitting function in super class or interfaces
	UFunction* parentFunc = uClass->FindFunctionByName(funcName, EIncludeSuperFlag::IncludeSuper);
	
	UUnrealLuaCompiledUFunction* newFunc = nullptr;
	if (parentFunc)
	{
		if (parentFunc->HasAnyFunctionFlags(FUNC_Final))
		{
			funcProto.SetIsError("Attempted to override non-overridable parent UFunction " + parentFunc->GetName());
			return nullptr;
		}
		flags |= parentFunc->FunctionFlags & FUNC_FuncInherit;
	}
	else
	{
		flags |= FUNC_Public;
		flags |= FUNC_BlueprintEvent;
	}
	flags |= FUNC_Native;
	/*
	if (parentFunc)
	{
		flags |= parentFunc->FunctionFlags & FUNC_FuncInherit;
		
		FObjectDuplicationParameters params{parentFunc, uClass};
		params.ApplyFlags = parentFunc->GetFlags();
		params.DestClass = UUnrealLuaCompiledFunction::StaticClass();
		params.DestName = parentFunc->GetFName();
		newFunc = CastChecked<UUnrealLuaCompiledFunction>(StaticDuplicateObjectEx(params));
		
		verify(parentFunc->IsSignatureCompatibleWith(newFunc));
	}
	else
	*/
	{
		for (const UnrealLua::Compiler::FUnrealLuaCompilerUPropertyPrototype& param : funcProto.Params)
		{
			if (EnumHasAnyFlags(param.PropertyFlags, CPF_OutParm))
			{
				flags |= FUNC_HasOutParms;
				break;
			}
		}
		if (funcProto.bIsBlueprintCallable)
		{
			flags |= FUNC_BlueprintCallable;
		}
		if (funcProto.bIsServerRPC)
		{
			verify(!funcProto.bIsClientRPC && !funcProto.bIsMulticastRPC)
			flags |= FUNC_Net;
			flags |= FUNC_NetServer;
		}
		else if (funcProto.bIsClientRPC)
		{
			verify(!funcProto.bIsServerRPC && !funcProto.bIsMulticastRPC)
			flags |= FUNC_Net;
			flags |= FUNC_NetClient;
		}
		else if (funcProto.bIsMulticastRPC)
		{
			verify(!funcProto.bIsServerRPC && !funcProto.bIsClientRPC)
			flags |= FUNC_Net;
			flags |= FUNC_NetMulticast;
		}
		
		if (flags & FUNC_Net)
		{
			if (funcProto.bIsReliableRPC)
			{
				flags |= FUNC_NetReliable;
			}
		}
		
		if (funcProto.bIsStaticCall)
		{
			verify(!funcProto.bIsSelfCall)
			flags |= FUNC_Static;
			flags |= FUNC_Final;
		}
		
		newFunc = NewObject<UUnrealLuaCompiledUFunction>(uClass, funcName, RF_Public | RF_MarkAsNative);
		
		newFunc->NumParms = 0; //+
		newFunc->ChildProperties = nullptr; //+
		newFunc->Children = nullptr; //sollt immer null sein, da wir nu FProperties verwenden
		newFunc->ParmsSize = 0; //+
		newFunc->PropertiesSize = 0; //+
		newFunc->MinAlignment = 0; //+
		newFunc->FirstPropertyToInit = nullptr; //+
		newFunc->DestructorLink = nullptr; //+
		newFunc->PropertyLink = nullptr; //+
#if WITH_EDITOR
		newFunc->PropertyWrappers = {}; //+
#endif 
		newFunc->ReturnValueOffset = UINT16_MAX; //+
		newFunc->EventGraphCallOffset = 0; //
		newFunc->EventGraphFunction = nullptr; //
		
	
		
		TArray<FProperty*> funcParamProperties = {};
		TArray<FProperty*> staticProperties = {};

		if (!UnrealLua::Compiler::CompileProperties(funcProto.Params, funcParamProperties, staticProperties))
		{
			if (newFunc)
			{
				newFunc->ConditionalBeginDestroy();
			}
			return nullptr;
		}
		
		for (FProperty* prop : funcParamProperties)
		{
			//prop->SetFlags(RF_Transient);
			prop->Owner = newFunc;
			newFunc->AddCppProperty(prop);
		}
		newFunc->Next = uClass->Children;
		uClass->Children = newFunc;
	
	}
	
	verify(IsValid(newFunc));
	
	newFunc->FunctionFlags |= flags;
	
	if (!uClass->HasAllClassFlags(EClassFlags::CLASS_Interface))
	{
		newFunc->SetLuaBytecode(funcProto.Func.dump());
	}
	
	newFunc->Script = {};
	
	
	uClass->AddFunctionToFunctionMap(newFunc, funcName);

	newFunc->Bind();
	newFunc->StaticLink(true);
	
	
	if(parentFunc)
	{
		if (!parentFunc->IsSignatureCompatibleWith(newFunc))
		{
			FString incompatibilityReason = "";
			bool compatible = UnrealLua::Compiler::ExamineUFunctionSignatureCompatibility(newFunc, parentFunc, incompatibilityReason);
			verify(!compatible);
			UUnrealLuaCompiler::SetIsErrorWithArgs(TEXT("Compiled UFunction {0}\n\n({1})\n\nhas signature incompatible with parent UFunction {2}\n\n({3})\n\nReason:\n{4}!"), *GetNameSafe(newFunc), *GetFullNameSafe(newFunc), *GetNameSafe(parentFunc), *GetFullNameSafe(parentFunc), *incompatibilityReason);
			newFunc->ConditionalBeginDestroy();
			return nullptr;
		}
		else
		{
			newFunc->SetSuperStruct(parentFunc);
		}
	}
	
	newFunc->AddToCluster(uClass);
	newFunc->Initialize();
	return newFunc;
}

bool UnrealLua::Compiler::CompileUFunctions(UClass* owningClass, TArray<FUnrealLuaCompilerUFunctionPrototype>& prototypes)
{
	for (FUnrealLuaCompilerUFunctionPrototype& funcProto : prototypes)
	{
		UUnrealLuaCompiledUFunction* newFunc = CompileUFunction(owningClass, funcProto);
		if (!newFunc)
		{
			return false;
		}
	}
	return true;
}


bool UnrealLua::Compiler::ExamineUFunctionSignatureCompatibility(UFunction* func1, UFunction* func2, FString& outMismaches)
{
	const uint64 IgnoreFlags = UFunction::GetDefaultIgnoredSignatureCompatibilityFlags();

	// Early out if they're exactly the same function
	if (func1 == func2)
	{
		return true;
	}

	FString func1Name = func1->GetName();
	FString func2Name = func2->GetName();
	// Run thru the parameter property chains to compare each property
	TFieldIterator<FProperty> IteratorA(func1);
	TFieldIterator<FProperty> IteratorB(func2);

	while (IteratorA && (IteratorA->PropertyFlags & CPF_Parm))
	{
		if (IteratorB && (IteratorB->PropertyFlags & CPF_Parm))
		{
			// Compare the two properties to make sure their types are identical
			// Note: currently this requires both to be strictly identical and wouldn't allow functions that differ only by how derived a class is,
			// which might be desirable when binding delegates, assuming there is directionality in the SignatureIsCompatibleWith call
			FProperty* PropA = *IteratorA;
			FProperty* PropB = *IteratorB;

			if (!FStructUtils::ArePropertiesTheSame(PropA, PropB, false))
			{
				// Type mismatch between an argument of A and B
				FString err = FString::Format(TEXT("Properties {0}::{1} and {2}::{3} are of different type"), {*func1->GetName(), *PropA->GetName(), *func2->GetName(), *PropB->GetName()});
				outMismaches += err + "\n";
				++IteratorA;
				++IteratorB;
				continue;
			}
			// Check the flags as well
			
			
			const uint64 PropertyMash = PropA->PropertyFlags ^ PropB->PropertyFlags;
			if((PropertyMash & ~IgnoreFlags) != 0)
			{
				// Flag mismatch between an argument of A and B
				FString flagsStr1 = UnrealLua::PropertyHelper::GetPropertyFlagsString(PropA->PropertyFlags);
				FString flagsStr2 = UnrealLua::PropertyHelper::GetPropertyFlagsString(PropB->PropertyFlags);
				FString err = FString::Format(TEXT("Properties {0}::{1} and {2}::{3} have different flags:! {4} and {5} have val {6}"), {*func1->GetName(), *PropA->GetName(), *func2->GetName(), *PropB->GetName(), *flagsStr1, *flagsStr2, PropertyMash});
				outMismaches += err + "\n";
				++IteratorA;
				++IteratorB;
				continue;
			}
		}
		else
		{
			// B ran out of arguments before A did
			FString err = FString::Format(TEXT("Function {0} has more arguments than {1}"), {*func1->GetName(),*func2->GetName()});
			outMismaches += err + "\n";
			return false;
		}
		++IteratorA;
		++IteratorB;
	}

	// They matched all the way thru A's properties, but it could still be a mismatch if B has remaining parameters
	bool func2HasMoreParms = !(IteratorB && (IteratorB->PropertyFlags & CPF_Parm));
	if (func2HasMoreParms)
	{
		FString err = FString::Format(TEXT("Function {0} has more arguments than {1}"), {*func2->GetName(), *func1->GetName()});
		outMismaches += err + "\n";
		return false;
	}
	return !func2HasMoreParms;
}
