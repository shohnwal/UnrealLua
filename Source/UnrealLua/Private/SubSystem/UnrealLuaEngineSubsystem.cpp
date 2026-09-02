// Fill out your copyright notice in the Description page of Project Settings.


#include "SubSystem/UnrealLuaEngineSubsystem.h"

#include "LuaContext/ScopedLuaContext.h"
#include "UnrealLua.h"
#include "sol/sol.hpp"
#include "Config/UnrealLuaConfig.h"
#include "Debug/UnrealLuaDebug.h"
#include "Engine/Engine.h"
#include "HAL/FileManager.h"
#include "Interface/LuaContext.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CoreDelegates.h"
#include "Reflection/MetaCache/LuaMetaCache.h"
#include "LuaValue/LuaFunction.h"
#include "StringHandling/UnrealLuaStringCache.h"
#include "UObject/Linker.h"
#include "Utility/AssetHelper.h"
#include "Config/UnrealLuaConstants.h"
#include "LuaCallHelpers/LuaScriptOverrideCalls.h"
#include "Subsystem/UnrealLuaFileSystem.h"
#include "Tests/LuaSelfTests.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"

#if WITH_EDITOR
#include "Settings/ProjectPackagingSettings.h"
#endif
UScriptStruct* UnrealLua::StaticPackages::VectorStruct = nullptr;
UScriptStruct* UnrealLua::StaticPackages::Vector2DStruct = nullptr;
UScriptStruct* UnrealLua::StaticPackages::RotatorStruct = nullptr;
UScriptStruct* UnrealLua::StaticPackages::TransformStruct = nullptr;
UScriptStruct* UnrealLua::StaticPackages::InstancedStruct = nullptr;
UScriptStruct* UnrealLua::StaticPackages::SharedStruct = nullptr;
UScriptStruct* UnrealLua::StaticPackages::LuaValue = nullptr;
UScriptStruct* UnrealLua::StaticPackages::LuaTable = nullptr;
UScriptStruct* UnrealLua::StaticPackages::LuaFunction = nullptr;
UScriptStruct* UnrealLua::StaticPackages::LuaCoroutine = nullptr;
UScriptStruct* UnrealLua::StaticPackages::LuaDelegate = nullptr;

FNativeFuncPtr UnrealLua::NativeFunctions::UObject_ProcessInternal = nullptr;

FString UnrealLua::Paths::FullProjectDir = "";

static UUnrealLuaEngineSubsystem* GUnrealLuaEngineSubsystem = nullptr;



UUnrealLuaEngineSubsystem* UUnrealLuaEngineSubsystem::Get()
{
	return GUnrealLuaEngineSubsystem;
}

bool UUnrealLuaEngineSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{	
	//Must always initialize this, in case of Lua-compiled unreal types
	return true;
}

void UUnrealLuaEngineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	Collection.InitializeDependency<UUnrealLuaFileSystem>();
	
	FString platformName = ANSI_TO_TCHAR(FPlatformProperties::IniPlatformName());
	LUA_LOG("UUnrealLuaEngineSubsystem::Initialize on %s", *platformName)
	
	GUnrealLuaEngineSubsystem = this;
	
	UnrealLua::StringCache::Initialize();
	

	UUnrealLuaConfig* config = UUnrealLuaConfig::Get();
	verify(IsValid(config));
	
	config->InitFromLuaFile();
	
	this->LuaGarbageCollector.Initialize(this);
	
	this->LuaImportRegistry = {};
	
	this->UObjectRegistry = NewObject<UUnrealLuaUObjectRegistry>(this);
	//this->UObjectRegistry->UClassOverrideRegistry.InitOverrideRegistry();
	//this->LuaRuntimeInspector = NewObject<UUnrealLuaDebug>(this);
	//this->LuaRuntimeInspector->EnableRealTimeTracing();
	
#if WITH_EDITOR
	//Set Editor project settings so it automatically copies the /Content/Lua folder in packaged builds
	if (UUnrealLuaConfig::Get()->bCopyLuaContentDirectoryDuringPackaging)
	{
		UProjectPackagingSettings* projectSettings = GetMutableDefault<UProjectPackagingSettings>();
		FDirectoryPath luaContentFolder{"Lua"};
		
		FDirectoryPath* found = projectSettings->DirectoriesToAlwaysStageAsNonUFS.FindByPredicate([luaContentFolder](const FDirectoryPath& item)
		{
			return luaContentFolder.Path == item.Path;
		});
		if (!found)
		{
			projectSettings->DirectoriesToAlwaysStageAsNonUFS.Add(luaContentFolder);
		}
	}
#endif

	UnrealLua::Paths::FullProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	
	FCoreDelegates::OnEndFrame.AddUObject(this, &UUnrealLuaEngineSubsystem::NotifyEndFrame);
	
	static UPackage* CoreUObjectPackage = UObject::StaticClass()->GetOutermost();
	UnrealLua::StaticPackages::VectorStruct = FindObjectChecked<UScriptStruct>(CoreUObjectPackage, TEXT("Vector"));
	UnrealLua::StaticPackages::Vector2DStruct = FindObjectChecked<UScriptStruct>(CoreUObjectPackage, TEXT("Vector2D"));
	UnrealLua::StaticPackages::RotatorStruct = FindObjectChecked<UScriptStruct>(CoreUObjectPackage, TEXT("Rotator"));
	UnrealLua::StaticPackages::TransformStruct = FindObjectChecked<UScriptStruct>(CoreUObjectPackage, TEXT("Transform"));
	UnrealLua::StaticPackages::InstancedStruct = FInstancedStruct::StaticStruct();// FindObjectChecked<UScriptStruct>(CoreUObjectPackage, TEXT("InstancedStruct"));
	UnrealLua::StaticPackages::SharedStruct = FSharedStruct::StaticStruct();
	UnrealLua::StaticPackages::LuaTable = FLuaTableHandle::StaticStruct();
	UnrealLua::StaticPackages::LuaCoroutine = FLuaCoroutineHandle::StaticStruct();
	UnrealLua::StaticPackages::LuaFunction = FLuaFunctionHandle::StaticStruct();
	UnrealLua::StaticPackages::LuaValue = FLuaValue::StaticStruct();
	UnrealLua::StaticPackages::LuaDelegate = FLuaDelegate::StaticStruct();
	//Hack to get ptr to UObject::ProcessInternal
	UFunction* func = NewObject<UFunction>(this->GetClass());
	func->Bind();
	UnrealLua::NativeFunctions::UObject_ProcessInternal = func->GetNativeFunc();
	func->ConditionalBeginDestroy();
	verify(UnrealLua::NativeFunctions::UObject_ProcessInternal != nullptr);

	
	FCoreDelegates::OnFEngineLoopInitComplete.AddUObject(this, &UUnrealLuaEngineSubsystem::NotifyEngineStartupComplete);
	FCoreDelegates::OnAllModuleLoadingPhasesComplete.AddUObject(this, &UUnrealLuaEngineSubsystem::NotifyAllModulesLoaded);
}

void UUnrealLuaEngineSubsystem::NotifyAllModulesLoaded()
{
	if(IsEngineExitRequested())
	{
		return;	
	}	

	if (GIsEditor)
	{
		this->InitCoreSystems();
	}
	this->OnAllModulesLoaded.ExecuteIfBound();
	
	if(!this->CanRunLua())
	{
		return;	
	}
	//UnrealLua::MetaCache::OnProcessUpdate.AddUObject(this, &UUnrealLuaEngineSubsystem::NotifyLuaMetaProcessUpdate);
}

void UUnrealLuaEngineSubsystem::InitCoreSystems()
{
	this->UObjectRegistry->UClassOverrideRegistry.InitOverrideRegistry();
	
	if (UUnrealLuaConfig::ShouldPerformSelfTest())
	{
		UnrealLua::SelfTest::PerformSelfTest();
		if (IsEngineExitRequested())
		{
			this->ActiveLuaContexts.Empty();		
			LUA_LOG_ERROR(" \n========================================\nUnreal Lua Engine Subsystem aborting game!!!\n=============================");
			return;
		}
	}
	
	this->OnTriggerCompiler.ExecuteIfBound();	
}


void UUnrealLuaEngineSubsystem::NotifyEngineStartupComplete()
{
	FCoreDelegates::OnFEngineLoopInitComplete.RemoveAll(this);
	
	if(!this->CanRunLua())
	{
		if (this->HasCompilerError())
		{
			RequestEngineExit("UnrealLua compiler error");
			return;			
		}
	}
}

void UUnrealLuaEngineSubsystem::Deinitialize()
{

	this->LuaGarbageCollector.PreDeinitialize();
	
	//End game session for all active LuaContexts
	if(this->IsGameSessionActive())
	{
		//need to do copy so they can remove themselves properly to make cleanups happen
		TArray<TScriptInterface<ILuaContext>> contexts = this->ActiveLuaContexts; 
		for(auto ctx : contexts)
		{
			this->NotifyEndGameSession(ctx);
		}
	}

	this->LuaGarbageCollector.Deinitialize();
	
	GUnrealLuaEngineSubsystem = nullptr;
	

	
	Super::Deinitialize();
}

FLuaImportRegistry& UUnrealLuaEngineSubsystem::GetLuaImportRegistry()
{
	return this->LuaImportRegistry;
}

void UUnrealLuaEngineSubsystem::ReloadScript(FString scriptPath)
{
	if (!this->IsGameSessionActive())
	{
		return;
	}
	for (TScriptInterface<ILuaContext> context : this->ActiveLuaContexts)
	{
		context->GetScopedLuaContext().ReloadScriptByFullFileName(scriptPath);
	}
}

void UUnrealLuaEngineSubsystem::NotifyModuleChanged(FName moduleName, EModuleChangeReason ModuleChangeReason)
{
	if (ModuleChangeReason == EModuleChangeReason::ModuleLoaded)
	{
		if (this->IsGameSessionActive())
		{
			FString ModulePackageName = TEXT("/Script/") + moduleName.ToString();
			LUA_LOG("Late registering native classes for module %s", *ModulePackageName);
			
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
			IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

			FARFilter Filter;
			Filter.PackagePaths.Add(FName(*ModulePackageName));  // Set your package path
			TArray<FAssetData> newModuleAssets;
			AssetRegistry.GetAssets(Filter, newModuleAssets);
			
			TArray<UClass*> newClasses{};
			TArray<UScriptStruct*> newScriptStructs{};
			TArray<UEnum*> newUEnums{};
			TArray<UBlueprintFunctionLibrary*> newBlueprintLibraries{};
	
			for (const FAssetData& assetData : newModuleAssets)
			{
				if (assetData.IsAssetLoaded())
				{
					UClass* assetClass = assetData.GetClass();
					if (assetClass == UObject::StaticClass())
					{
						// It's a UObject (likely a UClass)
						UClass* uclass = CastChecked<UClass>(assetData.GetAsset());
						if (uclass->IsChildOf<UBlueprintFunctionLibrary>())
						{
							UBlueprintFunctionLibrary* lib = uclass->GetDefaultObject<UBlueprintFunctionLibrary>();
							verify(IsValid(lib));
							newBlueprintLibraries.Add(lib);
						}
						newClasses.Add(uclass);
					}
					else if (assetClass == UEnum::StaticClass())
					{
						// It's a UEnum
						UEnum* uenum = CastChecked<UEnum>(assetData.GetAsset());
						newUEnums.Add(uenum);
					}
					else if (assetClass == UScriptStruct::StaticClass())
					{
						// It's a UScriptStruct
						UScriptStruct* ss = CastChecked<UScriptStruct>(assetData.GetAsset());
						newScriptStructs.Add(ss);
					}
				}
			}
			
			if (!newModuleAssets.IsEmpty())
			{
				for (auto ictx : this->ActiveLuaContexts)
				{
					FScopedLuaContext& ctx = ictx->GetScopedLuaContext();
					ctx.LateRegisterNewModuleAssets(newClasses, newScriptStructs, newUEnums, newBlueprintLibraries);
				}	
			}
		}
	}
}

void UUnrealLuaEngineSubsystem::CreateUnrealLuaMetadata()
{
	UnrealLua::MetaCache::CreateClassMetaDatabase(true);
}

void UUnrealLuaEngineSubsystem::NotifyBeginGameSession(const TScriptInterface<ILuaContext>& ictx)
{
	if(!ictx)
	{
		return;
	}
	if(!this->CanRunLua())
	{
		return;	
	}
	LUA_LOG("Unreal Lua Engine Subsystem : Lua Context %s starting game session", *GetNameSafe(ictx.GetObject()));
	
	//verify(this->LuaGCObjects.IsEmpty());
	bool bWasActive = this->IsGameSessionActive();
	this->ActiveLuaContexts.AddUnique(ictx);
	bool bIsActive = this->IsGameSessionActive();
	if(!bWasActive && bIsActive)
	{
		UUnrealLuaConfig::Get()->CommitCachedSettingsForLuaSession();

		LUA_LOG("Unreal Lua Engine Subsystem activating Object Registry");
	
		UnrealLua::LuaScriptCall::SetSuperCall(false);
		
		//these must be active before LuaConfig can perform self-starts
		this->UObjectRegistry->SetActive(true);
		
		LUA_LOG("Unreal Lua Engine Subsystem fininshed activating Object Registry");
		this->OnLuaGameSessionActiveChanged.Broadcast(this, true);
		this->OnLuaGameSessionActiveChangedNative.Broadcast(this, true);
	}
}

void UUnrealLuaEngineSubsystem::NotifyLuaContextInitialized(const TScriptInterface<ILuaContext>& ictx)
{
	this->OnLuaContextActiveChanged.Broadcast(this, ictx, true);
}

void UUnrealLuaEngineSubsystem::NotifyEndGameSession(const TScriptInterface<ILuaContext>& ictx)
{
	if(!ictx)
	{
		return;
	}
	LUA_LOG("Unreal Lua Engine Subsystem : Lua Context %s ending game session", *GetNameSafe(ictx.GetObject()));
	int32 removed = this->ActiveLuaContexts.RemoveSingle(ictx);
	verify(removed == 1);
	
	this->OnLuaContextActiveChanged.Broadcast(this, ictx, false);
	
	const bool sessionEnded = this->ActiveLuaContexts.IsEmpty();
	if (sessionEnded)
	{
		this->OnLuaGameSessionActiveChanged.Broadcast(this, false);
		this->OnLuaGameSessionActiveChangedNative.Broadcast(this, false);
	}
	
	//immediately clean up Lua state before mappings potentially go away
	this->CleanUpObjectsForLuaContext(ictx, true);
	
	if(sessionEnded)
	{
		LUA_LOG("Unreal Lua Engine Subsystem deactivating Object Registry");
		this->UObjectRegistry->SetActive(false);

		//Manually collect garbage

		UnrealLua::StringCache::CleanUp();	

		GEngine->ForceGarbageCollection(true);
		
		this->GetLuaImportRegistry().ClearImportCache();
		//This might trigger, some LuaGCObjects might end up in Blueprints or C++
		//verify(this->LuaGCObjects.IsEmpty());
		UnrealLua::LuaScriptCall::SetSuperCall(false);
		LUA_LOG("Unreal Lua Engine Subsystem finished deactivating Object Registry");
	}
}

void UUnrealLuaEngineSubsystem::CleanUpObjectsForLuaContext(const TScriptInterface<ILuaContext>& ictx, bool bShutDownLua)
{
	if(!ictx)
	{
		return;
	}
	//takes care of FLuaUObjectItem (lua values, func mappings, script handles)
	UnrealLua::UObjectRegistry::CleanUpObjectsForLuaContext(ictx);
	FScopedLuaContext& ctx = ictx->GetScopedLuaContext();
	ctx.ProcessInvalidUObjectCollection(true);
	/*
	if(bShutDownLua)
	{
		ctx.Shutdown();
	}
	else
	{
	*/
		ctx.GetLuaState().collect_garbage();
		ctx.GetLuaState().collect_garbage();	
	//}
}

bool UUnrealLuaEngineSubsystem::IsGameSessionActive()
{
	UUnrealLuaEngineSubsystem* ss = UUnrealLuaEngineSubsystem::Get();
	return ss && !ss->ActiveLuaContexts.IsEmpty();
}

TArray<TScriptInterface<ILuaContext>> UUnrealLuaEngineSubsystem::GetActiveLuaContextList()
{
	return this->ActiveLuaContexts;
}

const TArray<TScriptInterface<ILuaContext>>& UUnrealLuaEngineSubsystem::GetActiveLuaContextListRef()
{
	return this->ActiveLuaContexts;
}

void UUnrealLuaEngineSubsystem::NotifyEndFrame()
{
	this->LuaGarbageCollector.NotifyEndFrame(this);
	
	this->OnEndFrame.ExecuteIfBound();
}

void UUnrealLuaEngineSubsystem::AddReferencedObjects(UUnrealLuaEngineSubsystem* This, FReferenceCollector& collector)
{
	Super::AddReferencedObjects(This, collector);
	if(!This->HasAllFlags(RF_ClassDefaultObject))
	{
		This->LuaGarbageCollector.AddReferencedLuaObjects(collector);
	}
}

bool UUnrealLuaEngineSubsystem::HasCompilerError() const
{
	return this->OnCompilerOKCheck.IsBound() && !this->OnCompilerOKCheck.Execute();
}

bool UUnrealLuaEngineSubsystem::CanRunLua() const
{
	if (IsEngineExitRequested())
	{
		return false;
	}
	if (!UUnrealLuaConfig::IsLuaEnabled())
	{
		return false;
	}
	if (this->HasCompilerError())
	{
		return false;
	}
	return true;
}
