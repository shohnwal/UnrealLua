// Fill out your copyright notice in the Description page of Project Settings.


#include "UnrealOverrides/LuaClassOverrideRegistry.h"

#include "AssetRegistry/IAssetRegistry.h"
#include "UObject/Class.h"
#include "Utility/LuaLogMacros.h"
#include "Config/UnrealLuaConfig.h"
#include "Blueprint/UserWidget.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "FileSystem/UnrealLuaFileSystemEntry.h"
#include "HAL/FileManager.h"
#include "Interface/LuaScriptable.h"
#include "LuaCallHelpers/LuaScriptOverrideCalls.h"
#include "LuaContext/ScopedLuaContext.h"
#include "Misc/CoreDelegates.h"
#include "UnrealOverrides/UnrealLuaOverrideUFunction.h"
#include "UObject/UObjectIterator.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"
#include "UnrealOverrides/UnrealLuaOverrideFunctionHostClass.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"
#include "StructUtils/SharedStruct.h"
#include "StructUtils/UserDefinedStruct.h"
#include "Subsystem/UnrealLuaFileSystem.h"
#include "UObject/LinkerLoad.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"
//UE5.5 replaced EInternalObjectFlags::AsyncLoading with EInternalObjectFlags_AsyncLoading
//@TODO : Make version-dependent definition via compiler macros

/*
class UObject;
struct FFrame;

typedef void(*NativeFuncPtr)(UObject* context, FFrame& frame, void* RESULT_PARAM);
typedef bool(*NativeFuncPtrB)(UObject* context, FFrame& frame, void* RESULT_PARAM);
std::size_t GetIndex() { return random() * 1024;}

struct FUFunctionOverrideData
{
	void Exec(UObject* context, FFrame& frame, void* RESULT_PARAM)
	{
		if(this->OverridePtr(context, frame, RESULT_PARAM))
		{
			return;
		}
		this->OriginalFunc(context, frame, RESULT_PARAM);
	}
	NativeFuncPtr OriginalFunc = nullptr;
	NativeFuncPtrB OverridePtr = nullptr;
};
static std::vector<FUFunctionOverrideData*> Overrides = {};

void ProcessInternal(UObject* context, FFrame& frame, void* RESULT_PARAM);

void ProcessInternal_LuaOverride(UObject* context, FFrame& frame, void* RESULT_PARAM)
{
	std::size_t index = GetIndex();
	FUFunctionOverrideData* data = Overrides[index];
	data->Exec(context, frame, RESULT_PARAM);
}

static NativeFuncPtr ptr = ProcessInternal;
int main()
{
	FUFunctionOverrideData* data = new FUFunctionOverrideData();
	data->OriginalFunc = ptr;
	ptr = ProcessInternal_LuaOverride;

	return 0;    
}
*/

namespace UnrealLua::OverrideRegistry
{
	static FCriticalSection CandidatesLock = {};
}

static constexpr auto RenameFlags = REN_DontCreateRedirectors | REN_DoNotDirty | REN_ForceNoResetLoaders | REN_NonTransactional;
static FString OverriddenSuffix = TEXT("__Overridden");


namespace UnrealLua::UFunctionOverride
{
	struct FUFunctionOverrideData
	{
		UFunction* Func = nullptr;
		EFunctionFlags originalFunctionFlags = EFunctionFlags::FUNC_None;
		FNativeFuncPtr OriginalFunc = nullptr;
		FNativeFuncPtr OverridePtr = nullptr;
	};
	
	static TMap<int32, FUFunctionOverrideData> Overrides;
	
	static FUFunctionOverrideData dummy;
	void execActorUserConstructionScriptLuaCall(UObject* context, FFrame& Stack, void* RESULT_PARAM)
	{
		
		UFunction* func = Stack.CurrentNativeFunction;
		int32 index = func->GetUniqueID();;
		
		FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(func);
		FUFunctionOverrideData& data = dummy;
				
		if(Stack.Code)
		{
			data.OverridePtr(context, Stack, RESULT_PARAM);
			//originalFunc->Invoke(Context, Stack, RESULT_PARAM);
		}
	
		verify(!Stack.Code || *Stack.Code == EX_EndOfScript)
	}
	
	void execNativeLuaCall(UObject* context, FFrame& Stack, void* RESULT_PARAM)
	{
		verify(!Stack.Code);
		UFunction* func = Stack.CurrentNativeFunction;
		if(UnrealLua::LuaScriptCall::GetSuperCall() == false)
		{
			FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(context);
			FLuaOverrideCallParams params{func, Stack, RESULT_PARAM};
			if(item.ProcessEvent(params))
			{
				//override call handled
				return;
			}
		}
		UnrealLua::LuaScriptCall::SetSuperCall(false);
	
		FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(func);
		FUFunctionOverrideData& data = dummy;
		//originalFunc->Invoke(context, Stack, RESULT_PARAM);
		data.OverridePtr(context, Stack, RESULT_PARAM);

	}
}

void FLuaClassOverrideRegistry::OverrideUFunction_TestDontUse(UFunction* func, UClass* clazz, ELuaScriptableObjectClass objectClass)
{
	checkNoEntry();
	FNativeFuncPtr original = func->GetNativeFunc();
	int32 index = func->GetUniqueID();
	UnrealLua::UFunctionOverride::FUFunctionOverrideData& overrideData = UnrealLua::UFunctionOverride::Overrides.FindOrAdd(index);
	overrideData.OriginalFunc = original;
	overrideData.OverridePtr = UUnrealLuaOverrideUFunction::execBlueprintLuaCall;
	
	FName originalFuncName = func->GetFName();
	bool isActorConstructionScript = objectClass == ELuaScriptableObjectClass::Actor && originalFuncName == UnrealLua::PropertyNames::NAME_UserConstructionScript;
	bool isActorBeginPlay = objectClass == ELuaScriptableObjectClass::Actor && originalFuncName == UnrealLua::PropertyNames::NAME_ReceiveBeginPlay;
	bool bIsComponentConstructionScript = objectClass == ELuaScriptableObjectClass::Component && originalFuncName == UnrealLua::PropertyNames::NAME_ReceiveBeginPlay;
	bool bIsUserWidgetConstruct = objectClass == ELuaScriptableObjectClass::UserWidget && originalFuncName == UnrealLua::PropertyNames::NAME_UserWidgetConstruct;
	
	bool bIsEndPlay = originalFuncName == UnrealLua::PropertyNames::NAME_ReceiveEndPlay || (objectClass == ELuaScriptableObjectClass::UserWidget && originalFuncName == UnrealLua::PropertyNames::NAME_UserWidgetDestruct);
	bool bIsTick = (originalFuncName == UnrealLua::PropertyNames::NAME_ReceiveTick)	|| (objectClass == ELuaScriptableObjectClass::UserWidget && originalFuncName == UnrealLua::PropertyNames::NAME_Tick);
	
	bool isNativeFunc = func->HasAnyFunctionFlags(FUNC_Native);
	
	FNativeFuncPtr funcPtr = nullptr;
	/*
	if(isNativeFunc)
	{
		if(isActorConstructionScript)
		{
			funcPtr = UnrealLua::UFunctionOverride::execActorUserConstructionScriptLuaCall;			
		}
		else if(bIsComponentConstructionScript)
		{
			funcPtr = UnrealLua::UFunctionOverride::execNativeLuaScriptableComponentBeginPlayLuaCall;
		}
		else if(bIsUserWidgetConstruct)
		{
			funcPtr = UnrealLua::UFunctionOverride::execNativeLuaScriptableUserWidgetConstructLuaCall;
		}
		else if(bIsTick)
		{
			funcPtr = UnrealLua::UFunctionOverride::execNativeTickLuaCall; //bIsScriptableMainObject ? execNativeLuaCall : execNativeSubobjectLuaCall;
		}
		else if(bIsEndPlay && objectClass == ELuaScriptableObjectClass::Component)
		{
			//LuaScriptable components get their own call to remove their lua scripts
			funcPtr = UnrealLua::UFunctionOverride::execNativeLuaScriptableComponentEndPlayLuaCall;

			//LuaScriptable AActors on the other hand get removed via ULuaWorldSubSystem::NotifyActorDestroyed
			//so they can go through their Endplay routine properly before LuaScript gets unloaded
		}
		else if(bIsEndPlay && objectClass == ELuaScriptableObjectClass::UserWidget)
		{
			//LuaScriptable components get their own call to remove their lua scripts
			funcPtr = UnrealLua::UFunctionOverride::execNativeLuaScriptableUserWidgetDestructLuaCall;

			//LuaScriptable AActors on the other hand get removed via ULuaWorldSubSystem::NotifyActorDestroyed
			//so they can go through their Endplay routine properly before LuaScript gets unloaded
		}
		else
		{
			funcPtr = UnrealLua::UFunctionOverride::execNativeLuaCall;// : execNativeSubobjectLuaCall;
		}
	}
	else
	{
		if(newFunc->Script.IsEmpty())
		{
			newFunc->FunctionFlags |= FUNC_Native;
		}

		if(isActorConstructionScript)
		{
			funcPtr = UnrealLua::UFunctionOverride::execActorUserConstructionScriptLuaCall;
		}
		else if(bIsComponentConstructionScript)
		{
			funcPtr = UnrealLua::UFunctionOverride::execBlueprintLuaScriptableComponentBeginPlayLuaCall;
		}
		else if(bIsUserWidgetConstruct)
		{
			funcPtr = UnrealLua::UFunctionOverride::execBlueprintLuaScriptableUserWidgetConstructLuaCall;
		}
		else if(bIsEndPlay && objectClass == ELuaScriptableObjectClass::Component)
		{
			//LuaScriptable components get their own call to remove their lua scripts
			funcPtr = UnrealLua::UFunctionOverride::execBlueprintLuaScriptableComponentEndPlayLuaCall;

			//LuaScriptable AActors on the other hand get removed via ULuaWorldSubSystem::NotifyActorDestroyed
			//so they can go through their Endplay routine properly before LuaScript gets unloaded
		}
		else if(bIsEndPlay && objectClass == ELuaScriptableObjectClass::UserWidget)
		{
			//LuaScriptable components get their own call to remove their lua scripts
			funcPtr = UnrealLua::UFunctionOverride::execBlueprintLuaScriptableUserWidgetDestructLuaCall;

			//LuaScriptable AActors on the other hand get removed via ULuaWorldSubSystem::NotifyActorDestroyed
			//so they can go through their Endplay routine properly before LuaScript gets unloaded
		}
		else if(bIsTick)
		{
			funcPtr = UnrealLua::UFunctionOverride::execBlueprintTickLuaCall; //bIsScriptableMainObject ? execBlueprintLuaCall : execBlueprintSubobjectLuaCall;
		}
		else
		{
			funcPtr = UnrealLua::UFunctionOverride::execBlueprintLuaCall; //bIsScriptableMainObject ? execBlueprintLuaCall : execBlueprintSubobjectLuaCall;
		}
	}
	func->SetNativeFunc(funcPtr);
	*/
}

void FLuaClassOverrideRegistry::DisableUFunctionOverriding()
{
	verify(OverrideUFunctionsEnabled);
	OverrideUFunctionsEnabled = false;
}
void FLuaClassOverrideRegistry::EnableUFunctionOverriding()
{
	verify(!OverrideUFunctionsEnabled);
	OverrideUFunctionsEnabled = true;
}

void FLuaClassOverrideRegistry::InitOverrideRegistry()
{
	verify(!UUnrealLuaEngineSubsystem::IsGameSessionActive())
	if (!this->AsyncUpdateHandle.IsValid())
	{
		this->AsyncUpdateHandle = FCoreDelegates::OnAsyncLoadingFlushUpdate.AddRaw(this, &FLuaClassOverrideRegistry::OnAsyncLoadingFlushUpdate);
	}
	this->PrepareOverrideClassPaths();

}

void FLuaClassOverrideRegistry::ActivateOverrideRegistry()
{
	verify(UUnrealLuaEngineSubsystem::IsGameSessionActive())
	this->InitialOverridesAndBuildMappings();
}

void FLuaClassOverrideRegistry::ForceBuildFieldMapping(const UField* metaField)
{
	verify(IsInGameThread());
	verify(!metaField->HasAnyFlags(EObjectFlags::RF_NeedPostLoad));
	verify(!metaField->HasAnyFlags(EObjectFlags::RF_NeedLoad));
	verify(!metaField->HasAnyFlags(EObjectFlags::RF_NeedInitialization));
	verify(!metaField->HasAnyFlags(EObjectFlags::RF_WillBeLoaded));
	verify(!metaField->HasAnyInternalFlags(UnrealLua::Flags::AsyncObjectFlags));

	//getting the metaobjectentry is enough to trigger building of field mappings
	FLuaUObjectItem& metaItem = UnrealLua::UObjectRegistry::GetMetaObjectItem(metaField);
	verify(metaItem.PropertyMapping.IsValid());
}

void FLuaClassOverrideRegistry::HandleLoadingFinishedMetaField(UField* field)
{
	this->TryOverrideObjectClass(field);
	//FLuaUObjectItem& metaItem = UnrealLua::UObjectRegistry::GetMetaObjectItem(field);
	//@TODO : register enum/class/etc in Lua states at global table
}

void FLuaClassOverrideRegistry::OnAsyncLoadingFlushUpdate()
{
	//LUA_LOG("Lua :OnAsyncLoadingFlushUpdate")
	
	FScopeLock Lock(&UnrealLua::OverrideRegistry::CandidatesLock);
	TArray<UField*> CandidatesTemp;
	TArray<UField*> LocalCandidates;
	TArray<int> CandidatesRemovedIndexes;
	
	CandidatesTemp.Append(Candidates);
	
	for (int32 i = CandidatesTemp.Num() - 1; i >= 0; --i)
	{
		UField* ObjectPtr = CandidatesTemp[i];
		if (!IsValid(ObjectPtr))
		{
			// discard invalid objects
			CandidatesRemovedIndexes.Add(i);
			continue;
		}

		if (ObjectPtr->HasAnyFlags(RF_NeedPostLoad | EObjectFlags::RF_NeedLoad | EObjectFlags::RF_NeedInitialization | EObjectFlags::RF_WillBeLoaded)
			|| ObjectPtr->HasAnyInternalFlags(UnrealLua::Flags::AsyncObjectFlags)
			|| ObjectPtr->GetClass()->HasAnyInternalFlags(UnrealLua::Flags::AsyncObjectFlags))
		{
			// delay bind on next update 
			continue;
		}

		LocalCandidates.Add(ObjectPtr);
		CandidatesRemovedIndexes.Add(i);
	}

	for (int32 j = 0; j < CandidatesRemovedIndexes.Num(); ++j)
	{
		Candidates.RemoveAt(CandidatesRemovedIndexes[j]);
	}


	if(!UUnrealLuaConfig::IsLuaEnabled() || !UUnrealLuaEngineSubsystem::IsGameSessionActive())
	{
		Candidates.Empty();
		return;
	}
	
	for (int32 i = 0; i < LocalCandidates.Num(); ++i)
	{
		UField* clazz = LocalCandidates[i];
		LUA_LOG("Post-load attempting to override and build property mapping for %s with flags %lu", *GetNameSafe(clazz), clazz->GetFlags())
		//OverriddenClasses.Emplace(clazz->GetFName());
		HandleLoadingFinishedMetaField(clazz);
	}

}

bool FLuaClassOverrideRegistry::IsClassLuaOverridable(UClass* Class)
{
	if (!IsValid(Class))
	{
		return false;
	}
	if (Class->HasAnyClassFlags(CLASS_NewerVersionExists))
	{
		//LUA_LOG("Class %s is has newer version", *GetNameSafe(Class))
		// filter out recompiled objects
		return false;
	}
	if(Class->IsChildOf(UBlueprintGeneratedClass::StaticClass()))
	{
		//LUA_LOG("Class %s is BlueprintGeneratedClass", *GetNameSafe(Class))
		return false;
	}
	if(Class->IsChildOf(UUnrealLuaOverrideFunctionHostClass::StaticClass()))
	{
		return false;
	}
	UPackage* package = Class->GetPackage();
	const bool bIsEditorOnlyPackage = package->HasAllPackagesFlags(PKG_EditorOnly) || Class->IsEditorOnly();
	const bool bIsUncookedOrDev = package->HasAnyPackageFlags(PKG_UncookedOnly | PKG_Developer);
	if(bIsEditorOnlyPackage)
	{
		return false;
	}
	if(bIsUncookedOrDev)
	{
		return false;
	}
	FString className = Class->GetName();
	if(className.StartsWith("SKEL_") || className.StartsWith("PLACEHOLDER-CLASS_") || className.StartsWith("TRASH_") || className.StartsWith("REINST_"))
	{
		//LUA_LOG("Class %s is a trash class", *GetNameSafe(Class))
		return false;
	}
	if(!Class->ImplementsInterface(ULuaScriptable::StaticClass()) && !this->IsChildOfAnyOverridableClass(Class))
	{
		return false;
	}
	return true;
}

bool FLuaClassOverrideRegistry::HandleStillLoadingField(UField* ss)
{
	if(ss->HasAnyFlags(EObjectFlags::RF_NeedPostLoad | EObjectFlags::RF_NeedLoad | EObjectFlags::RF_NeedInitialization | RF_WillBeLoaded)
		|| ss->HasAnyInternalFlags(UnrealLua::Flags::AsyncObjectFlags) || IsInAsyncLoadingThread())
	{
		//LUA_LOG("Class %s still needs to finish loading", *GetNameSafe(Class))
		FScopeLock Lock(&UnrealLua::OverrideRegistry::CandidatesLock);
		this->Candidates.AddUnique(ss);
		return true;		
	}
	return false;
}

void FLuaClassOverrideRegistry::InitialOverridesAndBuildMappings()
{
	for (UClass* Class : TObjectRange<UClass>(EObjectFlags::RF_NoFlags))
	{
		this->TryOverrideObjectClass(Class);
	}
}

void FLuaClassOverrideRegistry::OverrideUFunction(UFunction* originalFunc, UClass* Class, ELuaScriptableObjectClass objectClass, UUnrealLuaOverrideFunctionHostClass* overrideClass)
{
	//If the UFunction to override is not directly in the target UClass
	//then we shouldnt remove the old one, as it would affect classes other than the current one, some of which might not be LuaScriptable

	if(originalFunc->IsA<UUnrealLuaOverrideUFunction>())
	{
		return;
	}

	verify(IsOverridableUFunction(originalFunc));
	
	UClass* originalFuncClass = originalFunc->GetOuterUClassUnchecked();
	const bool replaceInSameClass = originalFuncClass == Class;
	FName originalFuncName = originalFunc->GetFName();

	if(originalFuncName == EName::ExecuteUbergraph)
	{
		return;
	}

	if (originalFuncName == UnrealLua::PropertyNames::NAME_GetLuaScriptSettings || originalFuncName == UnrealLua::PropertyNames::NAME_SetLuaScriptSettings || originalFuncName == UnrealLua::PropertyNames::NAME_GetUniqueLuaNetHandle)
	{
		return;
	}
	
	bool bIsTick = (originalFuncName == UnrealLua::PropertyNames::NAME_ReceiveTick)	|| (objectClass == ELuaScriptableObjectClass::UserWidget && originalFuncName == UnrealLua::PropertyNames::NAME_Tick);

	if(bIsTick && !UUnrealLuaConfig::AllowOverrideTick())
	{
		return;
	}

	/*
	if(funcName.ToString().EndsWith(OverriddenSuffix))
	{
		//func already overridden by another class ULuaFunction
		return;
	}
	*/

	//LUA_LOG("Overriding UFunction %s::%s with flags %s", *Class->GetName(), *originalFunc->GetName(), *GetUFunctionFlagsAsString(originalFunc))
	//LuaScriptable components use ReceiveBeginpPlay as the event to load their scripts (unfortunately there is no earlier UFUNCTION to hook
	//LuaScriptable actors use UserConstructionScript, which is executed before ACtor::BeginPlay
	bool isActorConstructionScript = objectClass == ELuaScriptableObjectClass::Actor && originalFuncName == UnrealLua::PropertyNames::NAME_UserConstructionScript;
	bool isActorBeginPlay = objectClass == ELuaScriptableObjectClass::Actor && originalFuncName == UnrealLua::PropertyNames::NAME_ReceiveBeginPlay;
	bool bIsComponentConstructionScript = objectClass == ELuaScriptableObjectClass::Component && originalFuncName == UnrealLua::PropertyNames::NAME_ReceiveBeginPlay;
	bool bIsUserWidgetConstruct = objectClass == ELuaScriptableObjectClass::UserWidget && originalFuncName == UnrealLua::PropertyNames::NAME_UserWidgetConstruct;
	
	bool bIsEndPlay = originalFuncName == UnrealLua::PropertyNames::NAME_ReceiveEndPlay || (objectClass == ELuaScriptableObjectClass::UserWidget && originalFuncName == UnrealLua::PropertyNames::NAME_UserWidgetDestruct);
	
	bool isNativeFunc = originalFunc->HasAnyFunctionFlags(FUNC_Native);
	
	//FString oldFuncOverrideName = funcName.ToString() + OverriddenSuffix;
	if (replaceInSameClass)
	{
		//old function is part of the direct UClass we want to put an override in
		//aka we want to replace the existing UFunction in the UClass mappping
		UUnrealLuaOverrideUFunction* existingLuaFunc = Cast<UUnrealLuaOverrideUFunction>(originalFunc);
		if (existingLuaFunc)
		{
			//There is already a ULuaFunction with that name
			existingLuaFunc->Initialize();
			return;
		}
	}
	else
	{
		//old function is in a parent class of the UClass we want to put an override in
		//->We are adding a new UFunction to this class

		//Check if there already exists a ULuaFunction in the class or superclass with that name
		const UFunction* Exists = Class->FindFunctionByName(originalFuncName, EIncludeSuperFlag::IncludeSuper);
		if (Exists && Exists->IsA<UUnrealLuaOverrideUFunction>())
		{
			//If so, then there is no point in adding another child function
			return;
		}
	}

	const EFunctionFlags originalFunctionFlags = originalFunc->FunctionFlags;
	//originalFunc->FunctionFlags &= (~EFunctionFlags::FUNC_Native);

	if(replaceInSameClass)
	{
		//Remove original func association
		Class->RemoveFunctionFromFunctionMap(originalFunc);
		//Rename original func
		//originalFunc->Rename(*oldFuncOverrideName, nullptr, RenameFlags);
	}
	
		
	FString overrideName = originalFuncName.ToString() + "__LuaOverride";
	
	UUnrealLuaOverrideUFunction* newFunc = NewObject<UUnrealLuaOverrideUFunction>(overrideClass, UUnrealLuaOverrideUFunction::StaticClass(), *overrideName, originalFunc->GetFlags() | EObjectFlags::RF_Transient);// CastChecked<ULuaFunction>(StaticDuplicateObjectEx(DuplicationParams));
	newFunc->NumParms = originalFunc->NumParms;
	newFunc->ChildProperties = originalFunc->ChildProperties;
	newFunc->Children = originalFunc->Children;
	newFunc->ParmsSize = originalFunc->ParmsSize;
	newFunc->PropertiesSize = originalFunc->PropertiesSize;
	newFunc->MinAlignment = originalFunc->MinAlignment;
	newFunc->FirstPropertyToInit = originalFunc->FirstPropertyToInit;
	newFunc->DestructorLink = originalFunc->DestructorLink;
	newFunc->FunctionFlags = originalFunctionFlags;
	newFunc->PropertyLink = originalFunc->PropertyLink;
#if WITH_EDITOR
	newFunc->PropertyWrappers = originalFunc->PropertyWrappers;
#endif
	newFunc->ReturnValueOffset = originalFunc->ReturnValueOffset;
	newFunc->EventGraphCallOffset = originalFunc->EventGraphCallOffset;
	newFunc->EventGraphFunction = originalFunc->EventGraphFunction;
	//LUA_LOG("Created ULuaFunction %s %p, propsize %d and parent propsize %d", *GetNameSafe(newFunc), newFunc, newFunc->GetPropertiesSize(), originalFunc->GetPropertiesSize())
	
	//This is to call the _Implementation functions for BlueprintNative events
	bool manuallyCallNativeImplementationFunction = originalFunc->HasAllFunctionFlags(FUNC_BlueprintEvent) 
		&&  originalFuncClass->HasAllClassFlags(EClassFlags::CLASS_Native) 
		//&& originalFuncClass->NativeFunctionLookupTable.ContainsByPredicate([funcName = originalFunc->GetFName()](const FNativeFunctionLookup& item){ return item.Name == funcName;});
		&&originalFunc->GetNativeFunc() != UnrealLua::NativeFunctions::UObject_ProcessInternal;
	
	newFunc->bCallOriginalNativeImplementationFunction = manuallyCallNativeImplementationFunction;
	
	verify(newFunc->GetFName() == overrideName);
	verify(newFunc->ChildProperties == originalFunc->ChildProperties);
	verify(newFunc->Children == originalFunc->Children);
	verify(newFunc->NumParms == originalFunc->NumParms);
	verify(newFunc->ParmsSize == originalFunc->ParmsSize);
	verify(newFunc->PropertiesSize == originalFunc->PropertiesSize);
	newFunc->AssignedPathName = *(GetFullNameSafe(newFunc) + " : " +  *(originalFuncName).ToString());
	
	verify(newFunc->IsSignatureCompatibleWith(originalFunc));
	
	//need to ALWAYS set ULuaFunction to native so it invokes the exec during ProcessLocalFunction in ScriptCore.cpp
	//otherwise functions might stay in Blueprint VM without giving a chance to call the override
	//if(isNativeFunc || !replaceInSameClass)
	//{
		newFunc->FunctionFlags |= FUNC_Native;
	//}
	newFunc->Script = originalFunc->Script;

	Class->AddFunctionToFunctionMap(newFunc, originalFuncName);

	//Link up function into the Children Property chain
	newFunc->Next = Class->Children;
	Class->Children = newFunc;
	
	//newFunc->StaticLink(true);
	
	//no need to bind, we'll set func ptr manually
	//newLuaFunc->Bind();
	FNativeFuncPtr funcPtr{nullptr};

	
	if(isNativeFunc)
	{
		if(isActorConstructionScript)
		{
			funcPtr = UUnrealLuaOverrideUFunction::execActorUserConstructionScriptLuaCall;
		}
		else if(bIsComponentConstructionScript)
		{
			funcPtr = UUnrealLuaOverrideUFunction::execNativeLuaScriptableComponentBeginPlayLuaCall;
		}
		else if(bIsUserWidgetConstruct)
		{
			funcPtr = UUnrealLuaOverrideUFunction::execNativeLuaScriptableUserWidgetConstructLuaCall;
		}
		else if(bIsTick)
		{
			funcPtr = UUnrealLuaOverrideUFunction::execNativeTickLuaCall; //bIsScriptableMainObject ? execNativeLuaCall : execNativeSubobjectLuaCall;
		}
		else if(bIsEndPlay && objectClass == ELuaScriptableObjectClass::Component)
		{
			//LuaScriptable components get their own call to remove their lua scripts
			funcPtr = UUnrealLuaOverrideUFunction::execNativeLuaScriptableComponentEndPlayLuaCall;

			//LuaScriptable AActors on the other hand get removed via ULuaWorldSubSystem::NotifyActorDestroyed
			//so they can go through their Endplay routine properly before LuaScript gets unloaded
		}
		else if(bIsEndPlay && objectClass == ELuaScriptableObjectClass::UserWidget)
		{
			//LuaScriptable components get their own call to remove their lua scripts
			funcPtr = UUnrealLuaOverrideUFunction::execNativeLuaScriptableUserWidgetDestructLuaCall;

			//LuaScriptable AActors on the other hand get removed via ULuaWorldSubSystem::NotifyActorDestroyed
			//so they can go through their Endplay routine properly before LuaScript gets unloaded
		}
		else
		{
			funcPtr = UUnrealLuaOverrideUFunction::execNativeLuaCall;// : execNativeSubobjectLuaCall;
		}
	}
	else
	{
		if(newFunc->Script.IsEmpty())
		{
			newFunc->FunctionFlags |= FUNC_Native;
		}

		if(isActorConstructionScript)
		{
			funcPtr = UUnrealLuaOverrideUFunction::execActorUserConstructionScriptLuaCall;
		}
		else if(bIsComponentConstructionScript)
		{
			funcPtr = UUnrealLuaOverrideUFunction::execBlueprintLuaScriptableComponentBeginPlayLuaCall;
		}
		else if(bIsUserWidgetConstruct)
		{
			funcPtr = UUnrealLuaOverrideUFunction::execBlueprintLuaScriptableUserWidgetConstructLuaCall;
		}
		else if(bIsEndPlay && objectClass == ELuaScriptableObjectClass::Component)
		{
			//LuaScriptable components get their own call to remove their lua scripts
			funcPtr = UUnrealLuaOverrideUFunction::execBlueprintLuaScriptableComponentEndPlayLuaCall;

			//LuaScriptable AActors on the other hand get removed via ULuaWorldSubSystem::NotifyActorDestroyed
			//so they can go through their Endplay routine properly before LuaScript gets unloaded
		}
		else if(bIsEndPlay && objectClass == ELuaScriptableObjectClass::UserWidget)
		{
			//LuaScriptable components get their own call to remove their lua scripts
			funcPtr = UUnrealLuaOverrideUFunction::execBlueprintLuaScriptableUserWidgetDestructLuaCall;

			//LuaScriptable AActors on the other hand get removed via ULuaWorldSubSystem::NotifyActorDestroyed
			//so they can go through their Endplay routine properly before LuaScript gets unloaded
		}
		else if(bIsTick)
		{
			funcPtr = UUnrealLuaOverrideUFunction::execBlueprintTickLuaCall; //bIsScriptableMainObject ? execBlueprintLuaCall : execBlueprintSubobjectLuaCall;
		}
		/*
		else if(isActorBeginPlay)
		{
			funcPtr = execActorBlueprintBeginPlayLuaCall;
		}
		*/
		else
		{
			funcPtr = UUnrealLuaOverrideUFunction::execBlueprintLuaCall; //bIsScriptableMainObject ? execBlueprintLuaCall : execBlueprintSubobjectLuaCall;
		}
	}
	
	verify(funcPtr != nullptr);
	
	newFunc->SetNativeFunc(funcPtr);
/*
#if WITH_METADATA
#if (ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 6) 
FMetaData::CopyMetadata(originalFunc, newFunc);
#else
UMetaData::CopyMetadata(originalFunc, newFunc);
#endif
#endif
*/
	newFunc->bReplacedExistingFunc = replaceInSameClass;
	newFunc->OverriddenName = originalFuncName;
	newFunc->Overridden = originalFunc;
	
	//SetActive
	bool added = !replaceInSameClass;
	
	if(added)
	{
		newFunc->SetSuperStruct(originalFunc);
	}
	else
	{
		newFunc->SetSuperStruct(originalFunc->GetSuperStruct());
		
		newFunc->Children = originalFunc->Children;
		newFunc->ChildProperties = originalFunc->ChildProperties;
		newFunc->PropertyLink = originalFunc->PropertyLink;
	}

	//newLuaFunc->Bind(); //no need, already set native func

	if (Class->IsRooted() || GUObjectArray.IsDisregardForGC(Class))
	{
		newFunc->AddToRoot();
	}
	else
	{
		newFunc->AddToCluster(Class);
	}
	
	newFunc->Initialize();
}



bool FLuaClassOverrideRegistry::IsOverridableUFunction(const UFunction* Function)
{
	constexpr EFunctionFlags disallowedFuncFlags = FUNC_Net | FUNC_Exec | FUNC_EditorOnly | FUNC_NetValidate | FUNC_Final | FUNC_UbergraphFunction;// | FUNC_Native;  
	if(Function->HasAnyFunctionFlags(disallowedFuncFlags))
	{
		return false;
	}

	static constexpr uint32 FlagMask = FUNC_Native | FUNC_Event | FUNC_Net;
	static constexpr uint32 FlagResult = FUNC_Native | FUNC_Event;
	
	bool flagsOk = Function->HasAnyFunctionFlags(FUNC_BlueprintEvent) || (Function->FunctionFlags & FlagMask) == FlagResult;
	if (!flagsOk)
	{
		return false;
	}
	int32 index = INDEX_NONE;
	if (Function->GetName().FindChar(TEXT(' '), index))
	{
		//Lua doesn't support functions with spaces
		return false;
	}
	return true;
}



void FLuaClassOverrideRegistry::ShutdownOverrideRegistry()
{
	FCoreDelegates::OnAsyncLoadingFlushUpdate.Remove(this->AsyncUpdateHandle);

	if (this->OverrideUFunctionsEnabled)
	{
		FScopeLock Lock(&UnrealLua::OverrideRegistry::CandidatesLock);
		this->Candidates.Empty();
		
		this->OverriddenClasses.Empty();
		
		for (auto Class : TObjectRange<UClass>())
		{
			if(Class->IsChildOf<UUnrealLuaOverrideFunctionHostClass>())
			{
				continue;
			}
			Class = Class->GetAuthoritativeClass();
			this->RemoveOverrides(Class);	
		}

		//kill UUnrealLuaOverrideUFunction
		{
			TArray<UObject*> stillAliveLuaFuncs;
			GetObjectsOfClass(UUnrealLuaOverrideUFunction::StaticClass(), stillAliveLuaFuncs, true);
			for(UObject* func : stillAliveLuaFuncs)
			{
				func->ConditionalBeginDestroy();
			}
			stillAliveLuaFuncs.Empty();
			
			verify(stillAliveLuaFuncs.IsEmpty());
			GetObjectsOfClass(UUnrealLuaOverrideUFunction::StaticClass(), stillAliveLuaFuncs, true);
			verify(stillAliveLuaFuncs.IsEmpty());
		}

		//for now, perform a double check
		{
			TArray<UObject*> stillAliveFuncs;
			GetObjectsOfClass(UFunction::StaticClass(), stillAliveFuncs, true);
			for(UObject* funcobj : stillAliveFuncs)
			{
				UFunction* func = Cast<UFunction>(funcobj);
				UClass* uclass = func->GetOwnerClass();
				if(IsValid(uclass))
				{
					verify(!uclass->IsChildOf<UUnrealLuaOverrideFunctionHostClass>());
				}
			}
		}

		//kill UUnrealLuaClasses
	
		for (auto Class : TObjectRange<UUnrealLuaOverrideFunctionHostClass>())
		{
			Class->SetDefaultObject(nullptr);
			Class->ConditionalBeginDestroy();
		}
	}
	
	//make sure we actually removed everything good
	{
		TArray<UObject*> stillAliveClasses;
		GetObjectsOfClass(UUnrealLuaOverrideFunctionHostClass::StaticClass(), stillAliveClasses, true);
		verify(stillAliveClasses.IsEmpty());
	}
	{
			
		TArray<UObject*> stillAliveFuncs;
		GetObjectsOfClass(UUnrealLuaOverrideUFunction::StaticClass(), stillAliveFuncs, true);
		verify(stillAliveFuncs.IsEmpty());
	}
}

void FLuaClassOverrideRegistry::TryOverrideObjectClass(UObject* obj)
{
	if (!obj)
	{
		return;
	}

	//enums and scriptstructs have to build their mappings immediately
	if(UScriptStruct* ss = Cast<UScriptStruct>(obj))
	{
		LUA_LOG_WARNING("UScript struct overriiding %s...................................................", *ss->GetName())
		this->ForceBuildFieldMapping(ss);
		return;
	}
	else if(UEnum* uenum = Cast<UEnum>(obj))
	{
		LUA_LOG_ERROR("UEnum overriiding %s...................................................", *uenum->GetName());
		this->ForceBuildFieldMapping(uenum);
		return;
	}
	
	if (!OverrideUFunctionsEnabled)
	{
		verify(OverriddenClasses.IsEmpty());
		return;
	}
	
	UClass* Class = Cast<UClass>(obj);
	if(!Class)
	{
		if(!obj->HasAllFlags(EObjectFlags::RF_ClassDefaultObject))
		{
			return;
		}
		Class = obj->GetClass();	
	}
	FString className = Class->GetName();
	if(::islower(className[0]))
	{
		//LUA_LOG_ERROR("class name %s is lower, aborting override", *Class->GetFullGroupName(false))
		return;
	}
	
	if(this->OverriddenClasses.Contains(Class))
	{
		//LUA_LOG("Class %s already overridden", *GetNameSafe(Class))
		return;
	}
	
	if(!this->IsClassLuaOverridable(Class))
	{
		//LUA_LOG("Class %s not valid for Lua", *GetNameSafe(Class))
		return;
	}
	
	if(this->HandleStillLoadingField(Class))
	{
		//LUA_LOG("Class %s is still loading", *GetNameSafe(Class))
		return;
	}
	
	this->OverrideClass(Class);
}

void FLuaClassOverrideRegistry::OverrideClass(UClass* clazz)
{
	//LUA_LOG("Attempting to override class %s", *GetNameSafe(clazz))

	verify(OverrideUFunctionsEnabled);
	verify(!clazz->HasAnyFlags(EObjectFlags::RF_NeedPostLoad));
	verify(!clazz->HasAnyFlags(EObjectFlags::RF_NeedLoad));
	verify(!clazz->HasAnyFlags(EObjectFlags::RF_NeedInitialization));
	verify(!clazz->HasAnyFlags(EObjectFlags::RF_WillBeLoaded));
	verify(!clazz->HasAnyInternalFlags(UnrealLua::Flags::AsyncObjectFlags));
	verify(!this->OverriddenClasses.Contains(clazz))
	FString className = GetNameSafe(clazz);
	
	//we need to override from most parent to most derived, so UFunctions are in the correct order
	//build up a chain of super classes
	TryOverrideObjectClass(clazz->GetSuperClass());
	
	LUA_LOG("Overriding class %s", *className)
	
	//Either override info must be explicitly added, or the object should be LuaScriptable
	bool implementsLuaScriptable = clazz->ImplementsInterface(ULuaScriptable::StaticClass());
	FLuaOverrideClassInfo* overrideInfo = this->FindOverrideInfoForClass(clazz);
	verify(overrideInfo != nullptr || implementsLuaScriptable);
	
	UUnrealLuaOverrideFunctionHostClass* overrideClass = UUnrealLuaOverrideFunctionHostClass::Create(clazz);
	
	FLuaOverriddenClassInfo& usedOverrideInfo = this->OverriddenClasses.Emplace(clazz);
	usedOverrideInfo.OverrideHostClass = overrideClass;
	usedOverrideInfo.ClassToOverride = clazz;
	usedOverrideInfo.bImplementsLuaScriptable = implementsLuaScriptable;
	
	if (overrideInfo)
	{
		usedOverrideInfo.ScriptPath = overrideInfo->ScriptPath;
	}
	else
	{
		//must be a ULuaScriptable
		verify(implementsLuaScriptable);
		usedOverrideInfo.ScriptPath = "";
	}
	
	if (!implementsLuaScriptable)
	{
		UFunction* func = clazz->FindFunctionByName(UnrealLua::PropertyNames::NAME_GetLuaScriptSettings);
		if(func && func->NumParms == 1)
		{
			FProperty* returnProperty = func->GetReturnProperty();
			if (FStructProperty* returnprop = CastField<FStructProperty>(returnProperty))
			{
				if (returnprop->Struct == FLuaScriptSettings::StaticStruct())
				{
					usedOverrideInfo.bHasGetLuaScriptSettingsFunction = true;
				}
			}
		}
	}
	////////////////////////////////////
	
	//Generate fallback path to default script lookup via UClass path

	FString defaultAssetScriptFilePath = GetDefaultLuaScriptPathForUClass_LuaFolderRelative(clazz, false);
	
	usedOverrideInfo.DefaultAssetScriptFilePath = defaultAssetScriptFilePath;
	
	///////////////////////////////////
	
	ELuaScriptableObjectClass objectClass = ELuaScriptableObjectClass::UObject;
	if(clazz->IsChildOf(AActor::StaticClass()))
	{
		objectClass = ELuaScriptableObjectClass::Actor;
	}
	else if(clazz->IsChildOf(UActorComponent::StaticClass()))
	{
		objectClass = ELuaScriptableObjectClass::Component;
	}
	else if(clazz->IsChildOf(UUserWidget::StaticClass()))
	{
		objectClass = ELuaScriptableObjectClass::UserWidget;
	}
	//verify(bIsScriptable != bIsScriptableSubobject);
	TMap<FName, UFunction*> foundFuncs;
	for (TFieldIterator<UFunction> it(clazz, EFieldIteratorFlags::IncludeSuper, EFieldIteratorFlags::ExcludeDeprecated, EFieldIteratorFlags::IncludeInterfaces); it; ++it)
	{
		UFunction* func = *it;
		if (!this->IsOverridableUFunction(func))
		{
			continue;
		}
		if(foundFuncs.Contains(func->GetFName()))
		{
			continue;
		}
		foundFuncs.Add(func->GetFName(), func);
	}

	for (int32 i = 0; i < clazz->ClassReps.Num(); ++i)
	{
		FProperty* Property = clazz->ClassReps[i].Property;
		if (!Property->HasAnyPropertyFlags(CPF_RepNotify))
		{
			continue;
		}
		if(foundFuncs.Contains(Property->RepNotifyFunc))
		{
			continue;
		}
		UFunction* Function = clazz->FindFunctionByName(Property->RepNotifyFunc);
		if (!Function)
		{
			continue;
		}
		if (!this->IsOverridableUFunction(Function))
		{
			continue;
		}
		LUA_LOG_WARNING("Adding overridable OnRep-function that did not get detected by UFunction iterator... derp?")
		foundFuncs.Add(Property->RepNotifyFunc, Function);
	}
	for(TTuple<FName, UFunction*>& pair : foundFuncs)
	{
		UFunction* func = pair.Value;
		this->OverrideUFunction(func, clazz, objectClass, overrideClass);
	}
	clazz->ClearFunctionMapsCaches();
	
	this->OnClassOverrideFinished.Broadcast(clazz);
}


/*
void FLuaClassOverrideRegistry::CreateUserConstructionScript(UClass* clazz)
{
	LUA_LOG("Creating construction script for %s", *GetNameSafe(clazz))
	ULuaFunction* constScript = NewObject<ULuaFunction>(clazz, "UserConstructionScript");
	constScript->SetSuperStruct(ULuaFunction::StaticClass());
	constScript->SetNativeFunc(execActorUserConstructionScriptLuaCall);
	clazz->AddFunctionToFunctionMap(constScript, "UserConstructionScript");
	constScript->bReplacedExistingFunc = false;
	constScript->Overridden = nullptr;
	constScript->Next = clazz->Children;
	clazz->Children = constScript;
	constScript->AddToCluster(clazz);
	constScript->Initialize();
	constScript->StaticLink(true);
	clazz->ClearFunctionMapsCaches();
}
*/

void FLuaClassOverrideRegistry::RemoveOverrides(UClass* Class)
{
	if (this->OverriddenClasses.Contains(Class))
	{
		LUA_LOG("Removing Lua overrides for %s", *GetNameSafe(Class))
	}
	TSet<UUnrealLuaOverrideUFunction*> foundFuncs{};
	for(TFieldIterator<UFunction> it(Class, EFieldIteratorFlags::ExcludeSuper); it; ++it)
	{
		UUnrealLuaOverrideUFunction* luaFunc = Cast<UUnrealLuaOverrideUFunction>(*it);
		if (luaFunc)
		{
			foundFuncs.Emplace(luaFunc);
		}
	}
		
	for(UUnrealLuaOverrideUFunction* luaFunc : foundFuncs)
	{
		//verify(luaFunc->GetOuterUClassUnchecked() == Class);

		UFunction* originalFunc = luaFunc->Overridden;

		FName realFuncName = luaFunc->GetFName();
		FString realFuncNameStr = realFuncName.ToString();

		FName overriddenFuncName = originalFunc->GetFName();
		FString overriddenFuncNameStr = overriddenFuncName.ToString();


		//Remove connections of ULuaFunction
		luaFunc->Overridden = nullptr;
		
		//use the original func FName to remove the lua func
		Class->RemoveFunctionFromFunctionMap(originalFunc);
		
		FLinkerLoad::InvalidateExport(luaFunc);
		if(luaFunc->IsRooted())
		{
			luaFunc->RemoveFromRoot();
		}

		luaFunc->Rename(nullptr, GetTransientPackage(), RenameFlags);
		
		if(luaFunc->bReplacedExistingFunc)
		{
			verify(originalFunc->GetOuter() == Class);
			
			//Reparent Overridden UFunction back to original UClass with original name
			//originalFunc->Rename(*realFuncNameStr, Class, RenameFlags);
			
			Class->AddFunctionToFunctionMap(originalFunc, *originalFunc->GetName());
		}
		else
		{
			//ULuaFunction was added new
			verify(originalFunc->GetOuter() != Class);
		}
	}

	while(Class->Children && Class->Children->IsA<UUnrealLuaOverrideUFunction>())
	{
		UUnrealLuaOverrideUFunction* func = Cast<UUnrealLuaOverrideUFunction>(Class->Children);
		Class->Children = func->Next;
	}
	
	for (TFieldIterator<UFunction> it(Class, EFieldIteratorFlags::ExcludeSuper); it; ++it)
	{
		UFunction* func = *it;
		verify(!func->IsA<UUnrealLuaOverrideUFunction>())
	}

	for(UUnrealLuaOverrideUFunction* luaFunc : foundFuncs)
	{
		luaFunc->ConditionalBeginDestroy();
	}

	Class->ClearFunctionMapsCaches();
}

void FLuaClassOverrideRegistry::ClearInvalidUClasses()
{
	for(TMap<UClass*, FLuaOverriddenClassInfo>::TIterator it = this->OverriddenClasses.CreateIterator(); it; ++it)
	{
		UClass* clazz = it->Key;
		if(!IsValid(clazz) || clazz->HasAnyFlags(RF_BeginDestroyed | RF_MirroredGarbage))
		{
			this->RemoveOverrides(clazz);

			if (it->Value.OverrideHostClass != nullptr)
			{
				it->Value.OverrideHostClass->ConditionalBeginDestroy();	
			}
			it.RemoveCurrent();
		}
	}
}

bool FLuaClassOverrideRegistry::IsClassLuaOverridden(UClass* Class)
{
	return this->OverriddenClasses.Contains(Class);
}

FLuaOverriddenClassInfo* FLuaClassOverrideRegistry::GetOverridenClassInfo(UClass* uclass)
{
	return this->OverriddenClasses.Find(uclass);
}

FString FLuaClassOverrideRegistry::GetDefaultLuaScriptPathForUClass_LuaFolderRelative(UClass* uclass, bool addLuaFileExtension)
{
	FString defaultAssetScriptFilePath{};
	if(uclass->HasAnyClassFlags(EClassFlags::CLASS_Native))
	{
		//
		if (UUnrealLuaConfig::Get()->bUsePackagePathForNativeDefaultScripts)
		{
			// native package path "/Script/Package.Classname" gets mapped to "DefaultScript/Script/Package/ClassName"
			// i.e. a class UMyClass in a game module MyGame gets mapped to Default/Script/MyGame/MyClass.lua
			// or an actor class AMyActor gets mapped to DefaultScript/Script/MyGame/MyActor.lua
			FString packagePath = uclass->GetClassPathName().ToString();
		
			//Optionally replace dots of packages with dots, for a shallower folder hierarchy
			if(UUnrealLuaConfig::UsePackagePathAsFolders())
			{
				//Asset   /Script/MyGame.MyActor
				//LuaPath /DefaultScript/Script/MyGame/MyActor.lua
				packagePath.ReplaceCharInline(TEXT('.'), TEXT('/'), ESearchCase::CaseSensitive);
			}
			else
			{
				//Asset   /Script/MyGame.MyActor
				// ->
				//LuaPath /DefaultScript/Script.MyGame.MyActor.lua
				packagePath.ReplaceCharInline(TEXT('/'), TEXT('.'), ESearchCase::CaseSensitive);
				packagePath[0] = TEXT('/');
			}
			defaultAssetScriptFilePath = UnrealLua::scriptLoading::DefaultLuaScriptFolder + packagePath;
		}
		else
		{
			//Native classes have unique names, so just use the name inside DefaultScript/<ClassName>.lua
			defaultAssetScriptFilePath = UnrealLua::scriptLoading::DefaultLuaScriptFolder + "/" + uclass->GetAuthoredName();			
		}
		//LUA_LOG("native class %s will use default path %s", *GetNameSafe(uclass), *defaultAssetScriptFilePath)
	}
	else
	{
		// Blueprint package path "/Game/Path/To/Blueprint gets mapped to DefaultScript/Game/Path/To/Blueprint"
		/*
			LoadLuaScript("//Path/To/Blueprint") checks
			- Mod1/Lua/GameModeName/Game/Path/To/Blueprint
			- Mod2/Lua/GameModeName/Game/Path/To/Blueprint
			- Lua/GameMode/Game/Path/To/Blueprint
			- Mod1/Lua/DefaultScript/Game/Path/To/Blueprint
			- Mod2/Lua/DefaultScript/Game/Path/To/Blueprint
			- Lua/DefaultScript/Game/Path/To/Blueprint
		*/
		verify(uclass->HasAnyClassFlags(EClassFlags::CLASS_CompiledFromBlueprint));
		FString packagePath = uclass->GetPackage()->GetPathName();
		
		//Optionally replace dots of packages with dots, for a shallower folder hierarchy
		if(!UUnrealLuaConfig::UsePackagePathAsFolders())
		{
			//DefaultScript/Game/Path/To/BlueprintClass.lua
			// ->
			//DefaultScript/Game.Path.To.BlueprintClass.lua
			packagePath.ReplaceCharInline(TEXT('/'), TEXT('.'), ESearchCase::CaseSensitive);
			packagePath[0] = TEXT('/');
		}
		defaultAssetScriptFilePath = UnrealLua::scriptLoading::DefaultLuaScriptFolder + packagePath;
		//LUA_LOG("Blueprint class %s will use default path %s", *GetNameSafe(uclass), *defaultAssetScriptFilePath)
	}
	if (addLuaFileExtension)
	{
		defaultAssetScriptFilePath += ".lua";
	}
	return defaultAssetScriptFilePath;
}

FString FLuaClassOverrideRegistry::GetDefaultLuaScriptPathForUClass_WithRelativeLuaRootPath(UClass* uclass, bool addLuaFileExtension)
{
	FString path = GetDefaultLuaScriptPathForUClass_LuaFolderRelative(uclass, addLuaFileExtension);
	const FString& luaRoot = UUnrealLuaFileSystem::Get()->GetLuaRootDirPath();
	return luaRoot / path;
}

void FLuaClassOverrideRegistry::RequestMakeUClassOverridable(UClass* uclass)
{
	FSoftObjectPath path(uclass);
	LUA_LOG("Request to allow override of UClass %s : %s", *GetFullNameSafe(uclass), *path.ToString());
	
	if (this->IsClassLuaOverridden(uclass))
	{
		LUA_LOG("UClass %s already overridden", *GetFullNameSafe(uclass));
		return;
	}
	//Order [direct descendants][next layer][next layer...] 
	TArray<UClass*> derivedClasses{uclass};
	GetDerivedClasses(uclass, derivedClasses, true);
	
	//Place baes class in override class info, so FLuaClassOverrideRegistry::IsClassLuaOverridable -> IsChildOfAnyOverridableClass finds it as a valid class 
	FSoftClassPath ClassPath{uclass};
	FLuaOverrideClassInfo* found = this->OverrideBaseClassInfo.Find(ClassPath);
	if (!found)
	{
		this->OverrideBaseClassInfo.Emplace(ClassPath, FLuaOverrideClassInfo{uclass, "", true} );
	}
	for (UClass* toOverride : derivedClasses)
	{
		this->TryOverrideObjectClass(uclass);
	}
}

FLuaOverrideClassInfo* FLuaClassOverrideRegistry::FindOverrideInfoForClass(UClass* uclass)
{
	while (uclass)
	{
		FSoftObjectPath myPath{uclass};
		if (FLuaOverrideClassInfo* found = this->OverrideBaseClassInfo.Find(myPath))
		{
			return found;
		}
		uclass = uclass->GetSuperClass();
	}
	return nullptr;
}

bool FLuaClassOverrideRegistry::IsChildOfAnyOverridableClass(UClass* uclass)
{
	return this->FindOverrideInfoForClass(uclass) != nullptr;
}

void FLuaClassOverrideRegistry::PrepareOverrideClassPaths()
{
	this->OverrideBaseClassInfo.Empty();
	
	TArray<FAssetData> assetsData{};
	
	TMap<FString, UClass*> nativeClasses{};
	for (UClass* Class : TObjectRange<UClass>(EObjectFlags::RF_NoFlags))
	{
		nativeClasses.Emplace(Class->GetName(), Class);
	}
	
	IAssetRegistry::Get()->WaitForCompletion();

	UUnrealLuaFileSystem* fileSystem = UUnrealLuaFileSystem::Get();
	
	TSharedPtr<FUnrealLuaFileSystemEntry> defaultScriptFolder = fileSystem->GetDefaultScriptDirectory();
	
	for (const TSharedPtr<FUnrealLuaFileSystemEntry>& child : defaultScriptFolder->AccessSubDirectories())
	{
		if (child->IsFile())
		{
			FString fileName = child->GetFileSystemName();
			fileName.RemoveFromEnd(".lua");
			//file in main folder should be a native
			UClass* uclass = nativeClasses.FindRef(fileName);
			if (uclass)
			{
				FSoftObjectPath softAssetPathFromAssetData{uclass};
				this->OverrideBaseClassInfo.Emplace(softAssetPathFromAssetData, FLuaOverrideClassInfo{softAssetPathFromAssetData, ""});				
			}
		}
		else if (child->IsDirectory())
		{
			if (child->GetFileSystemName() == "Game")
			{
				
			}
		}
	}
	
	FStringBuilderBase builder;
	builder << "LuaClassOverride Registry found " << this->OverrideBaseClassInfo.Num() << " classes to override\n";
	
	for (TTuple<FSoftObjectPath, FLuaOverrideClassInfo>& item : this->OverrideBaseClassInfo)
	{
		builder << "-\t"<< item.Key.ToString();
		if (item.Value.ScriptPath.IsEmpty())
		{
			builder << " will use default script";
		}
		else
		{
			builder << " with script path " << item.Value.ScriptPath;
		}
		builder << "\n";
	}
	LUA_LOG("%s", builder.ToString());
}

