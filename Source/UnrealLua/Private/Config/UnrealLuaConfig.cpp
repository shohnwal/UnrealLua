// Fill out your copyright notice in the Description page of Project Settings.


#include "Config/UnrealLuaConfig.h"

#include "AssetRegistry/IAssetRegistry.h"
#include "Utility/LuaLogMacros.h"
#include "Engine/Engine.h"
#include "Engine/Level.h"
#include "HAL/FileManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Reflection/PropertyHelper_SetProperty.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"

UUnrealLuaConfig::UUnrealLuaConfig()
{
	this->CategoryName = "Project";
	this->SectionName = "UnrealLua";
	
}


bool FUnrealLuaConfigData::EnsureValidity()
{
	//TSet<FName> luaDisabledMaps = TSet<FName>(this->LuaDisabledMaps);
	//
	//TArray<FAssetData> levels;
	//IAssetRegistry::Get()->GetAssetsByClass(UObject::StaticClass()->GetClassPathName(),levels, true);
	//
	//for (auto& mapdata : levels)
	//{
	//	LUA_LOG("Found map %s", *mapdata.AssetName.ToString());
	//	luaDisabledMaps.Remove(mapdata.AssetName);
	//	return true;
	//}
	//
	//if (!luaDisabledMaps.IsEmpty())
	//{
	//	for (auto& mapname : luaDisabledMaps)
	//	{
	//		LUA_LOG_WARNING("Lua disabled map %s does not exist", *mapname.ToString() );
	//	}
	//}
	
	//this->LuaDisabledGameModes = this->LuaDisabledGameModes;
	//this->LuaEnabledClasses = this->LuaEnabledClasses;
	//this->LuaTickEnabledClasses = this->LuaTickEnabledClasses;
	//this->LuaGCStepPause = this->LuaGCStepPause; //maybe 0 as a default?
	//this->LuaGCStepMultiplier = this->LuaGCStepMultiplier; // maybe 0 as a default?
	//this->LuaGCStepSize = this->LuaGCStepSize; // maybe 0 as a default?
	//this->LuaIncrementalGCLimit = this->LuaIncrementalGCLimit; // maybe 0 as a default?
	//this->ModsDirLocation = this->ModsDirLocation;
	//this->ScriptLoadModFileExtension = this->ScriptLoadModFileExtension;
	return true;
}

UUnrealLuaConfig* UUnrealLuaConfig::Get()
{
	return GetMutableDefault<UUnrealLuaConfig>();
}

void UUnrealLuaConfig::BeginDestroy()
{
	LUA_LOG("UUnrealLuaConfig::BeginDestroy")
	Super::BeginDestroy();
}

/*
 Called during UUnrealEngineSubsystem startup to load override data from LuaConfig.lua file in Content/Lua/ folder
 If the LuaConfig.lua file is not present, create a default Content/Lua/LuaConfig.lua file
 Also applies the cached runtime settings
 */
void UUnrealLuaConfig::InitFromLuaFile()
{
	//initialiization sbould only be done if no game session is active and before registry exists
	verify(!UUnrealLuaEngineSubsystem::IsGameSessionActive())
	verify(!UUnrealLuaUObjectRegistry::Get());
	
	//look in main game Lua/ folder for a LuaConfig.lua
	FUnrealLuaConfigData configData{};
	this->InitializeConfigSettingsStruct(configData);
	
	IFileManager& fileManager = IFileManager::Get();
	FString luaFile = FPaths::ProjectContentDir() + "Lua/LuaConfig.lua";
	if (fileManager.FileExists(*luaFile))
	{
		FScopedLuaContext loadLuaConfig {nullptr, ELuaContextType::Minimal, "InitPreEngineSubsystemInitialize"};
		
		loadLuaConfig.SetupLuaStateForGameMode(NAME_None, ELuaPathFlags::BaseGameRoot);
		std::string scriptFileName = "LuaConfig";
		FLoadLuaScriptResult result = loadLuaConfig.LoadLuaScriptFromDisk(scriptFileName, false, nullptr);
		if (result.IsValid())
		{
			LUA_LOG("Overriding UUnreaLLuaConfig with LuaConfig.lua table")
			UnrealLua::PropertyHelper::InitializeStructFromTable(configData, result.FinalResult, false);
		}
		else
		{
			LUA_LOG_WARNING("LuaConfig file Content/Lua/LuaConfig.lua exists, but did not return a valid table!")
		}
	}
	else
	{
		LUA_LOG("Creating default Content/Lua/LuaConfig.lua file")
		this->WriteDefaultConfigToLuaFile();
	}
	
	this->ApplySettingsToCache(configData);
}

void UUnrealLuaConfig::CreateDefaultLuaConfigFile()
{
	
}

void UUnrealLuaConfig::InitializeConfigSettingsStruct(FUnrealLuaConfigData& toInitialize)
{
	const FName cachedSettingsName = GET_MEMBER_NAME_CHECKED(UUnrealLuaConfig, CachedSettings);
	const FName appliedSettingsName = GET_MEMBER_NAME_CHECKED(UUnrealLuaConfig, AppliedGameSessionSettings);
	for (TFieldIterator<FProperty> it(this->GetClass()); it; ++it)
	{
		FProperty* configProp = *it;
		if (configProp->IsEditorOnlyProperty())
		{
			continue;
		}
		if (configProp->GetFName() == cachedSettingsName || configProp->GetFName() == appliedSettingsName)
		{
			continue;
		}
		FProperty* toInitProp = FUnrealLuaConfigData::StaticStruct()->FindPropertyByName(configProp->GetFName());
		verify(configProp->SameType(toInitProp));
		void* propMem = configProp->ContainerPtrToValuePtr<void>(this);
		void* targetMem = toInitProp->ContainerPtrToValuePtr<void>(&toInitialize);
		toInitProp->CopyCompleteValue(targetMem, propMem);
		
	}
	//toInitialize.bAllowWriteOnCDO = this->bAllowWriteOnCDO;
	//toInitialize.bSelfTestOnStartup = this->bSelfTestOnStartup;
	//toInitialize.bAllowWriteOnReadOnlyProperties = this->bAllowWriteOnReadOnlyProperties;
	//toInitialize.bClearLuaMetaCache = this->bClearLuaMetaCache;
	//toInitialize.bIgnoreInvalidServerRPC = this->bIgnoreInvalidServerRPC;
	//toInitialize.bLuaEnabled = this->bLuaEnabled;
	//toInitialize.bMultithreadedReplication = this->bMultithreadedReplication;
	//toInitialize.bMultithreadGC = this->bMultithreadGC;
	//toInitialize.bOverrideAllClasses = this->bOverrideAllClasses;
	//toInitialize.bOverrideInput = this->bOverrideInput;
	//toInitialize.bOverrideTick = this->bOverrideTick;
	//toInitialize.bUsePackagePathAsFolders = this->bUsePackagePathAsFolders;
	//toInitialize.GCMode = this->GCMode;
	//toInitialize.EditorGameMode = this->EditorGameMode;
	//toInitialize.LuaDisabledMaps = this->LuaDisabledMaps;
	//toInitialize.LuaDisabledGameModes = this->LuaDisabledGameModes;
	//toInitialize.LuaEnabledClasses = this->LuaEnabledClasses;
	//toInitialize.LuaGCStepPause = this->LuaGCStepPause; //maybe 0 as a default?
	//toInitialize.LuaGCStepMultiplier = this->LuaGCStepMultiplier; // maybe 0 as a default?
	//toInitialize.LuaGCStepSize = this->LuaGCStepSize; // maybe 0 as a default?
	//toInitialize.LuaIncrementalGCLimit = this->LuaIncrementalGCLimit; // maybe 0 as a default?
	//toInitialize.ModsDirLocation = this->ModsDirLocation;
	//toInitialize.ScriptLoadModFileExtension = this->ScriptLoadModFileExtension;
}

void UUnrealLuaConfig::InitializeConfigSettingsStructFromCachedRuntimeData(FUnrealLuaConfigData& toInitialize)
{
	toInitialize = this->CachedSettings;
}

bool UUnrealLuaConfig::ApplySettingsToCache(FUnrealLuaConfigData toApply)
{
	//Check validity of values
	if (!toApply.EnsureValidity())
	{
		return false;
	}
	LUA_LOG("Appling cached lua settings")
	this->CachedSettings = toApply;
	//if (UUnrealLuaEngineSubsystem::IsGameSessionActive())
	//{
	//	//Apply a select few parameters at runtime that are safe to update immediately
	//	this->AppliedGameSessionSettings.bIgnoreInvalidServerRPC = this->CachedSettings.bIgnoreInvalidServerRPC;
	//	this->AppliedGameSessionSettings.DebugKey = this->CachedSettings.DebugKey;
	//	this->AppliedGameSessionSettings.LuaDisabledMaps = this->CachedSettings.LuaDisabledMaps;
	//}
	return true;
}

void UUnrealLuaConfig::CommitCachedSettingsForLuaSession()
{
	//Verify that a game session has started, but we haven't initialized the registry yet
	verify(UUnrealLuaEngineSubsystem::IsGameSessionActive())
	verify(!UUnrealLuaUObjectRegistry::Get()->IsActive());
	
	LUA_LOG("Committing Lua Config for game session")
	
	this->AppliedGameSessionSettings = this->CachedSettings;
}


FName UUnrealLuaConfig::GetContainerName() const
{
	static const FName ProjectName(TEXT("Project"));
	return ProjectName;
}

FName UUnrealLuaConfig::GetCategoryName() const
{
	static const FName name(TEXT("Engine"));
	return name;
}

FName UUnrealLuaConfig::GetSectionName() const
{
	static const FName name(TEXT("UnrealLua"));
	return name;
}
#if WITH_EDITOR
FText UUnrealLuaConfig::GetSectionText() const
{
	static const FText txt = FText::FromString( TEXT("UnrealLua"));
	return txt;
}

FText UUnrealLuaConfig::GetSectionDescription() const
{
	static const FText txt = FText::FromString( TEXT("Configure UnrealLua behavior"));
	return txt;
}

bool UUnrealLuaConfig::SupportsAutoRegistration() const
{
	return Super::SupportsAutoRegistration();
}

void UUnrealLuaConfig::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
TSharedPtr<SWidget> UUnrealLuaConfig::GetCustomSettingsWidget() const
{
	return Super::GetCustomSettingsWidget();
}

bool UUnrealLuaConfig::ShouldOverrideInput()
{
	return UUnrealLuaConfig::GetAppliedLuaSessionSettings().bOverrideInput;
}

bool UUnrealLuaConfig::AllowOverrideTick()
{
	return UUnrealLuaConfig::GetAppliedLuaSessionSettings().bOverrideTick;
}

bool UUnrealLuaConfig::UsePackagePathAsFolders()
{
	return UUnrealLuaConfig::GetAppliedLuaSessionSettings().bUsePackagePathForBlueprintDefaultScripts;
}

bool UUnrealLuaConfig::IsMultithreadReplicationEnabled()
{
	return UUnrealLuaConfig::GetAppliedLuaSessionSettings().bMultithreadedReplication;
}

bool UUnrealLuaConfig::IsLuaWriteOnCDOAllowed()
{
	return UUnrealLuaConfig::GetAppliedLuaSessionSettings().bAllowWriteOnCDO;
}

bool UUnrealLuaConfig::ShouldAllowWriteOnReadOnlyProperties()
{
	return UUnrealLuaConfig::GetAppliedLuaSessionSettings().bAllowWriteOnReadOnlyProperties;
}

bool UUnrealLuaConfig::ShouldIgnoreInvalidServerRPC()
{
	return UUnrealLuaConfig::GetAppliedLuaSessionSettings().bIgnoreInvalidServerRPC;
}

int32 UUnrealLuaConfig::GetLuaGCStepSize()
{
	return UUnrealLuaConfig::GetAppliedLuaSessionSettings().LuaGCStepSize;
}

int32 UUnrealLuaConfig::GetLuaGCStepMultiplier()
{
	return UUnrealLuaConfig::GetAppliedLuaSessionSettings().LuaGCStepMultiplier;
}

int32 UUnrealLuaConfig::GetLuaGCStepPause()
{
	return UUnrealLuaConfig::GetAppliedLuaSessionSettings().LuaGCStepPause;
}

const FString& UUnrealLuaConfig::GetModsDirLocation()
{
	return UUnrealLuaConfig::GetAppliedLuaSessionSettings().ModsDirectoryLocation;
}

const FString& UUnrealLuaConfig::GetLuaScriptModFileExtension()
{
	return UUnrealLuaConfig::GetAppliedLuaSessionSettings().ScriptLoadModFileExtension;
}

bool UUnrealLuaConfig::ShouldMultithreadGC()
{
	return UUnrealLuaConfig::GetAppliedLuaSessionSettings().bMultithreadGC;
}

int32 UUnrealLuaConfig::GetLuaIncrementalGCLimit()
{
	return UUnrealLuaConfig::GetAppliedLuaSessionSettings().LuaIncrementalGCLimit;
}

const FUnrealLuaConfigData& UUnrealLuaConfig::GetCachedSettings()
{
	return UUnrealLuaConfig::Get()->CachedSettings;
}

const FUnrealLuaConfigData& UUnrealLuaConfig::GetAppliedLuaSessionSettings()
{
	return UUnrealLuaConfig::Get()->AppliedGameSessionSettings;
}

bool UUnrealLuaConfig::IsLuaEnabled()
{
	return UUnrealLuaConfig::GetAppliedLuaSessionSettings().bLuaEnabled;
}

bool UUnrealLuaConfig::ShouldPerformSelfTest()
{
	return UUnrealLuaConfig::GetAppliedLuaSessionSettings().bSelfTestOnStartup;
}

void UnrealLuaConfig::Unload()
{
}

bool UUnrealLuaConfig::IsGameModeDisabledForLua(const FString& gamemode)
{
	return UUnrealLuaConfig::GetAppliedLuaSessionSettings().LuaDisabledGameModes.Contains(gamemode);
}

bool UUnrealLuaConfig::IsMapDisabledForLua(const FString& map)
{
	return UUnrealLuaConfig::GetAppliedLuaSessionSettings().LuaDisabledMaps.Contains(map);
}

FString UUnrealLuaConfig::GetModsDir()
{
	return UUnrealLuaConfig::GetAppliedLuaSessionSettings().ModsDirectoryLocation;
}

EUnrealLuaGCMode UUnrealLuaConfig::GetGCMode()
{
	return UUnrealLuaConfig::GetAppliedLuaSessionSettings().GCMode;
}
