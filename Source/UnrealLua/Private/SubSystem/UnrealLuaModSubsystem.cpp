#include "SubSystem/UnrealLuaModSubsystem.h"

#include "Utility/LuaLogMacros.h"
#include "Config/UnrealLuaConfig.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"
#include "HAL/FileManager.h"
#include "LuaContext/GameLuaContext.h"
#include "Mods/UnrealLuaMod.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "Subsystems/SubsystemCollection.h"

bool UUnrealLuaModSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return UUnrealLuaConfig::IsLuaEnabled();
}

void UUnrealLuaModSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UGameLuaContext* ctx = Collection.InitializeDependency<UGameLuaContext>();
	
	ctx->OnLuaGameModeReloadEventNative.AddUObject(this, &UUnrealLuaModSubsystem::NotifyLuaLoadUpdate);
	ctx->OnWorldBeginPlayUpdate.AddUniqueDynamic(this, &UUnrealLuaModSubsystem::NotifyWorldBeginPlayUpdate);

	this->RefreshMods();
}

void UUnrealLuaModSubsystem::RefreshMods()
{
#if 0
	if(!UUnrealLuaConfig::IsLuaEnabled() || UUnrealLuaConfig::GetModsDir().IsEmpty())
	{
		return;
	}
	UUnrealLuaEngineSubsystem* luaEngineSubsystem = UUnrealLuaEngineSubsystem::Get();
	
	this->DiscoveredMods.Empty();
	
	IFileManager& files = IFileManager::Get();
	
	struct FDirectoryVisitor : public IPlatformFile::FDirectoryVisitor
	{
		FRWLock          DirectoriesLock;
		FScopedLuaContext& Ctx;
		TArray<FUnrealLuaGameModInfo>& FoundMods;

		IFileManager& Files = IFileManager::Get();
		
		FDirectoryVisitor(TArray<FUnrealLuaGameModInfo>& InDirectories, FScopedLuaContext& ctx)
			: IPlatformFile::FDirectoryVisitor(EDirectoryVisitorFlags::ThreadSafe)
			, Ctx(ctx)
			, FoundMods(InDirectories)
		{
		}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (bIsDirectory)
			{
				FString CleanDirectoryName(FPaths::GetCleanFilename(FilenameOrDirectory));
				// Skip directories like (i.e. .git) when finding code files to improve performance.
				if (CleanDirectoryName.StartsWith(TEXT(".")))
				{
					return true;
				}
				FString directoryStr = FilenameOrDirectory;
				directoryStr.RemoveFromEnd("/");
				FString modDefFile = directoryStr + "/ModInfo.lua";
				if(!Files.FileExists(*modDefFile))
				{
					return true;					
				}

				FLoadLuaScriptResult loadResult = Ctx.FullPathLoadLuaScriptFromDisk(modDefFile);
				if(!loadResult.IsValid())
				{
					return true;
				}
				sol::table modInfoTbl = loadResult.FinalResult; 
				if(!modInfoTbl.valid())
				{
					return true;
				}

				FString modName = modInfoTbl["ModName"].get_or<sol::string_view, sol::string_view>("UnnamedMod").data();
				
				if(modName.IsEmpty())
				{
					return true;
				}

				bool bModEnabled = modInfoTbl["Enabled"].get_or<bool, bool>(false);
				FUnrealLuaGameModInfo modInfo{};
				modInfo.Directory = directoryStr;
				modInfo.bCanEverBeActive = bModEnabled;
				int32 slashidx = directoryStr.Find("/");
				FString modFolderName = directoryStr.RightChop(slashidx);
				modInfo.ModFolderName = modFolderName;
				modInfo.ModName = modName;

				FString modScriptFilePath = directoryStr + "/ModScript.lua";
				if(Files.FileExists(*modScriptFilePath))
				{
					modInfo.LuaScriptFilePath = modScriptFilePath; 
				}
				else
				{
					modInfo.LuaScriptFilePath = "";
				}
				
				{
					FRWScopeLock ScopeLock(DirectoriesLock, SLT_Write);
					FoundMods.Emplace(modInfo);
				}
			}
			return true;
		}
	};
	
	FString modsDir = FPaths::ProjectContentDir() + UUnrealLuaConfig::GetModsDirLocation();

	if(!files.DirectoryExists(*modsDir))
	{
		return;
	}
	TArray<FString> DirectoriesToVisitNext;
	TArray<FUnrealLuaGameModInfo> foundMods;
	DirectoriesToVisitNext.Add(modsDir);
	checkNoEntry();
	/*
	FDirectoryVisitor Visitor(foundMods, luaEngineSubsystem->GetScopedLuaContext());
	files.IterateDirectory(*modsDir, Visitor);
	*/
	for(auto& modInfo : foundMods)
	{
		UUnrealLuaMod* mod = NewObject<UUnrealLuaMod>(this);
		mod->ModInfo = modInfo;
		this->DiscoveredMods.Add(mod);
	}
	
	if(!this->DiscoveredMods.IsEmpty())
	{
		FString msg = "UnrealLua found mods:";
		for(auto& mod : this->DiscoveredMods)
		{
			msg += FString::Printf(TEXT("\n%s - %s : %d"), *mod->GetModName(), *mod->GetModDirectory(), static_cast<int32>(mod->IsModEnabled()));
		}
		LUA_LOG("%s", *msg)
	}
	else
	{
		LUA_LOG("UnrealLua found no mods")
	}
#endif
}

UUnrealLuaMod* UUnrealLuaModSubsystem::GetMod(const FString& modName)
{
	return FindMod(modName);
}

UUnrealLuaMod* UUnrealLuaModSubsystem::FindMod(const FString& modName)
{
	TObjectPtr<UUnrealLuaMod>* found = this->DiscoveredMods.FindByPredicate([modName](const UUnrealLuaMod* item)
	{
		return item && item->ModInfo.ModName == modName;
	});
	if(found != nullptr)
	{
		return *found;
	}
	return nullptr;
}

void UUnrealLuaModSubsystem::SetModEnabled(const FString& modName, bool bIsEnabled)
{
	if(this->AreModsLocked())
	{
		return;
	}
	TObjectPtr<UUnrealLuaMod>* foundMod = this->DiscoveredMods.FindByPredicate([modName](const UUnrealLuaMod* mod)
	{
		return mod->GetModName() == modName;
	});
	if(foundMod)
	{
		(*foundMod)->SetModEnabled(bIsEnabled);
	}
}

void UUnrealLuaModSubsystem::SetModsEnabled(const TArray<FString>& modNames, bool bIsEnabled)
{
	if(this->AreModsLocked())
	{
		return;
	}
	for(auto& modName : modNames)
	{
		this->SetModEnabled(modName, bIsEnabled);
	}	
}


TArray<UUnrealLuaMod*> UUnrealLuaModSubsystem::GetEnabledMods() const
{
	TArray<UUnrealLuaMod*> retMods;
	for(auto& mod : this->DiscoveredMods)
	{
		if(mod->IsModEnabled())
		{
			retMods.Add(mod);
		}
	}
	return retMods;	
}

TArray<FString> UUnrealLuaModSubsystem::GetModsNames() const
{
	TArray<FString> retMods;
	for(auto& mod : this->DiscoveredMods)
	{
		retMods.Add(mod->GetModName());
	}
	return retMods;
}

TArray<FString> UUnrealLuaModSubsystem::GetEnabledModsNames() const
{
	TArray<FString> retMods;
	for(auto& mod : this->DiscoveredMods)
	{
		if(mod->IsModEnabled())
		{
			retMods.Add(mod->GetModName());
		}
	}
	return retMods;
}

bool UUnrealLuaModSubsystem::AreModsLocked() const
{
	return this->bModsLocked;
}

void UUnrealLuaModSubsystem::NotifyWorldBeginPlayUpdate(bool bHasBegunPlay)
{
	if(bHasBegunPlay)
	{
		this->NotifyWorldBeginPlay();
	}
	else
	{
		this->NotifyWorldEndPlay();
	}
}

void UUnrealLuaModSubsystem::NotifyWorldBeginPlay()
{
	for(UUnrealLuaMod* mod : this->ActiveMods)
	{
		mod->NotifyWorldBeginPlay();
	}
}

void UUnrealLuaModSubsystem::NotifyWorldEndPlay()
{
	for(UUnrealLuaMod* mod : this->ActiveMods)
	{
		mod->NotifyWorldEndPlay();
	}
}

void UUnrealLuaModSubsystem::NotifyLuaLoadUpdate(TScriptInterface<ILuaContext> ctx, FName gameMode, ELuaLoadEventType loadEvent)
{
	if(loadEvent == ELuaLoadEventType::PROCESSING)
	{
		this->LockMods();
		for(UUnrealLuaMod* mod : this->ActiveMods)
		{
			if(mod->LoadLuaScript())
			{
				mod->ReceiveModLoaded();
			}
		}
	}
	else if(loadEvent == ELuaLoadEventType::UNLOADED)
	{
		this->UnlockMods();
	}
}

void UUnrealLuaModSubsystem::LockMods()
{
	this->bModsLocked = true;
	this->OnModsLocked.Broadcast(this->bModsLocked);
}

void UUnrealLuaModSubsystem::UnlockMods()
{
	this->bModsLocked = false;
	this->OnModsLocked.Broadcast(this->bModsLocked);
}
