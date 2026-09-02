#include "Compilers/CompilerHelper.h"
#include "CoreMinimal.h"
#include "UnrealLuaCompiler.h"
#include "Compilers/ClassCompiler.h"
#include "Compilers/EnumCompiler.h"
#include "Compilers/InterfaceCompiler.h"
#include "Compilers/ScriptStructCompiler.h"
#include "Prototypes/EnumPrototype.h"
#include "Prototypes/InterfacePrototype.h"
#include "Prototypes/UClassPrototype.h"
#include "Prototypes/UScriptStructPrototype.h"

namespace UnrealLua::Compiler
{
	bool CreateSkeletonStruct(UUnrealLuaCompiler* compiler, IStructPrototypeBase* proto)
	{
		UField* createdField = nullptr;
		switch (proto->GetPrototypeCategory())
		{
		case EUnrealLuaCompilerPrototypeType::Enum:
			{
				FUnrealLuaCompilerUEnumPrototype* prototype = static_cast<FUnrealLuaCompilerUEnumPrototype*>(proto);
				createdField = UnrealLua::Compiler::CreateAndFillEnum(*prototype, compiler->UnrealLuaCompilerUTypePackage);
			}
			break;
		case EUnrealLuaCompilerPrototypeType::Class:
			{
				FUnrealLuaCompilerUClassPrototype* prototype = static_cast<FUnrealLuaCompilerUClassPrototype*>(proto);
				createdField = UnrealLua::Compiler::CreateSkeletonClass(*prototype, compiler->UnrealLuaCompilerUTypePackage);
			}
			break;
		case EUnrealLuaCompilerPrototypeType::ScriptStruct:
			{
				FUnrealLuaCompilerUScriptStructPrototype* structProto = static_cast<FUnrealLuaCompilerUScriptStructPrototype*>(proto);
				createdField = UnrealLua::Compiler::CreateSkeletonScriptStruct(*structProto, compiler->UnrealLuaCompilerUTypePackage);
			}
			break;
		case EUnrealLuaCompilerPrototypeType::Interface:
			{
				FUnrealLuaCompilerUInterfacePrototype* interfaceProto = static_cast<FUnrealLuaCompilerUInterfacePrototype*>(proto);
				createdField = UnrealLua::Compiler::CreateSkeletonInterfaceClass(*interfaceProto, compiler->UnrealLuaCompilerUTypePackage);
			}
			break;
		default:
			checkNoEntry();
			break;
		}
		return createdField != nullptr;
	}

	bool CreateSkeletonStructs(UUnrealLuaCompiler* compiler, TArray<IStructPrototypeBase*>& prototypesStillNeedingCompilation)
	{
		while (!prototypesStillNeedingCompilation.IsEmpty())
		{
			bool compiledSomethingThisLoop = false;
			for (TArray<IStructPrototypeBase*>::TIterator it = prototypesStillNeedingCompilation.CreateIterator(); it; ++it)
			{
				IStructPrototypeBase* proto = *it;
				
				if (!CreateSkeletonStruct(compiler, proto))
				{
					if (proto->GetIsError())
					{
						compiler->SetIsErrorWithArgs("Can not comile skeleton struct. Found prototype %s with errors from file %s::%d", *proto->GetTypeNameString(), *proto->FileName, proto->DefinedLine);
						return false;
					}
					continue;
				}
				verify(IsValid(proto->GetCompiledField()))
				compiledSomethingThisLoop = true;
				it.RemoveCurrent();
			}
			if(!compiledSomethingThisLoop)
			{
				//all items left in the array can not be compiled due to missing parents or circular dependency
				
				//@TODO : detailed error report about fields still needing compilation
				checkNoEntry();
				return false;
			}
		}
		return true;
	}

	bool ResolveParentClasses(UUnrealLuaCompiler* compiler, TArray<IStructPrototypeBase*>& prototypesStillNeedingCompilation)
	{
		TArray<FUnrealLuaCompilerUClassPrototype*> classPrototypes{};
		
		//We only want class prototypes to check for parents
		Algo::TransformIf(prototypesStillNeedingCompilation, classPrototypes,
			[](IStructPrototypeBase* proto) 
			{ 
				return proto->GetPrototypeCategory() == UnrealLua::Compiler::EUnrealLuaCompilerPrototypeType::Class; 
			},
			[](IStructPrototypeBase* proto)
			{
				return static_cast<FUnrealLuaCompilerUClassPrototype*>(proto);
			}
		);
		
		for (FUnrealLuaCompilerUClassPrototype* proto : classPrototypes)
		{
			UClass* myClass = proto->GetCompiledClass();
			verify(myClass != nullptr)
			UClass* parentClass = myClass->GetSuperClass();
			verify(parentClass != nullptr)
			UPackage* parentPackage = parentClass->GetPackage();
			verify(parentPackage != nullptr);
			if (parentPackage != compiler->UnrealLuaCompilerUTypePackage)
			{
				//Super class is not a compiled type
				continue;
			}
			bool foundParent = false;
			for (FUnrealLuaCompilerUClassPrototype* potentialParent : classPrototypes)
			{
				if (potentialParent == proto)
				{
					continue;
				}
				if (potentialParent->GetCompiledClass() == parentClass)
				{
					proto->ParentClassPrototype = potentialParent;
					foundParent = true;
					break;
				}
			}
			if (!foundParent)
			{
				compiler->SetIsErrorWithArgs("Unable to find parent prototype for prototype class %s and expected parent class %s", *myClass->GetFullName(), *parentClass->GetFullName());
				return false;
			}
			verify((proto->ParentClassPrototype != nullptr && parentPackage == compiler->UnrealLuaCompilerUTypePackage) || parentPackage != compiler->UnrealLuaCompilerUTypePackage)
		}
		return true;
	}
	
	bool ResolveInterfaceClasses(UUnrealLuaCompiler* compiler, TArray<IStructPrototypeBase*>& prototypesStillNeedingCompilation)
	{
		TArray<FUnrealLuaCompilerUClassPrototype*> classPrototypes{};
		
		//We only want class prototypes to check for parents
		Algo::TransformIf(prototypesStillNeedingCompilation, classPrototypes,
			[](IStructPrototypeBase* proto) 
			{ 
				return proto->GetPrototypeCategory() == UnrealLua::Compiler::EUnrealLuaCompilerPrototypeType::Class; 
			},
			[](IStructPrototypeBase* proto)
			{
				return static_cast<FUnrealLuaCompilerUClassPrototype*>(proto);
			}
		);
		
		for (FUnrealLuaCompilerUClassPrototype* proto : classPrototypes)
		{
			for (FSoftClassPath& path : proto->InterfacePaths)
			{
				UClass* interfaceClass = path.TryLoadClass<UObject>();
				if (!interfaceClass)
				{
					compiler->SetIsErrorWithArgs("Unable to find interface class %s for prototype class %s", *path.ToString(), *proto->GetCompiledClass()->GetFullName());
					return false;					
				}
				else if (interfaceClass->IsChildOf(UInterface::StaticClass()))
				{
					proto->Interfaces.Add(interfaceClass);
				}
				else
				{
					compiler->SetIsErrorWithArgs("%s is not an interface class for prototype class %s", *path.ToString(), *proto->GetCompiledClass()->GetFullName());
					return false;
				}
			}
		}
		return true;
	}
}

bool UnrealLua::Compiler::CompilePrototypes(UUnrealLuaCompiler* compiler)
{
	
	//Step 1: Create skeleton classes so they can reference each other
	
	//Create pointer list of all prototypes
	TArray<IStructPrototypeBase*> prototypesStillNeedingCompilation = {};
	for (TTuple<FString, TUniquePtr<IStructPrototypeBase>>& item : compiler->UnrealLuaUTypePrototypes)
	{
		IStructPrototypeBase* proto = item.Value.Get();
		verify(!proto->HasFinishedCompilation())
		verify(!proto->GetIsError());
		prototypesStillNeedingCompilation.Add(proto);
	}
	TArray<IStructPrototypeBase*> cachedPrototypesArray = prototypesStillNeedingCompilation;
	
	//Create skeleton classes
	if (!CreateSkeletonStructs(compiler, prototypesStillNeedingCompilation))
	{
		return false;
	}
	UnrealLua::Compiler::LoadMissingTypesFromBackups(compiler);
	//All types should be valid now (though still without any properties)
	
	//Fill array up again
	verify(prototypesStillNeedingCompilation.IsEmpty())
	prototypesStillNeedingCompilation = cachedPrototypesArray;
	
	
	//Step 2: Figure out parent-child relationships
	if (!ResolveParentClasses(compiler, prototypesStillNeedingCompilation))
	{
		return false;
	}
	
	//Step 3: Resolve interface classes
	if (!ResolveInterfaceClasses(compiler, prototypesStillNeedingCompilation))
	{
		return false;
	}
	
	//Step 4: Finish compiling
	
	//First, compile script structs, because classes might need them for calculating total size
	for (TTuple<FString, TUniquePtr<IStructPrototypeBase>>& item : compiler->UnrealLuaUTypePrototypes)
	{
		IStructPrototypeBase* proto = item.Value.Get();
		verify(IsValid(proto->GetCompiledField()))
		if (proto->GetPrototypeCategory() == UnrealLua::Compiler::EUnrealLuaCompilerPrototypeType::ScriptStruct)
		{
			FUnrealLuaCompilerUScriptStructPrototype* structProto = static_cast<FUnrealLuaCompilerUScriptStructPrototype*>(proto);
			if (!UnrealLua::Compiler::CompileScriptStructPrototype(*structProto))
			{
				return false;
			}
			verify(IsValid(structProto->GetCompiledField()))
			verify(IsValid(structProto->GetCompiledScriptStruct()))
		}
	}
	
	//Compile UInterfaces
	for (TTuple<FString, TUniquePtr<IStructPrototypeBase>>& item : compiler->UnrealLuaUTypePrototypes)
	{
		IStructPrototypeBase* proto = item.Value.Get();
		verify(IsValid(proto->GetCompiledField()))
		if (proto->GetPrototypeCategory() == UnrealLua::Compiler::EUnrealLuaCompilerPrototypeType::Interface)
		{
			FUnrealLuaCompilerUInterfacePrototype* interfaceProto = static_cast<FUnrealLuaCompilerUInterfacePrototype*>(proto);
			if (!UnrealLua::Compiler::CompileInterfacePrototype(*interfaceProto))
			{
				return false;
			}
			verify(IsValid(interfaceProto->GetCompiledField()))
		}
	}
	
	//Last, compile UClasses
	for (TTuple<FString, TUniquePtr<IStructPrototypeBase>>& item : compiler->UnrealLuaUTypePrototypes)
	{
		IStructPrototypeBase* proto = item.Value.Get();
		if (proto->GetPrototypeCategory() == UnrealLua::Compiler::EUnrealLuaCompilerPrototypeType::Class)
		{
			FUnrealLuaCompilerUClassPrototype* prototype = static_cast<FUnrealLuaCompilerUClassPrototype*>(proto);
			if (!UnrealLua::Compiler::CompileClassPrototype(*prototype))
			{
				return false;
			}
			verify(IsValid(prototype->GetCompiledField()))
			verify(IsValid(prototype->GetCompiledClass()))
		}
	}
	
	return true;
}