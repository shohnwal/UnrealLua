// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPath.h"
#include "UnrealLuaConfig.generated.h"

/**
 * 
 */

//The Lua garbage collection mode
UENUM()
enum class EUnrealLuaGCMode : uint8
{
	Incremental,
	PostDestroy,
};

USTRUCT()
struct UNREALLUA_API FUnrealLuaEnabledClassConfig
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, Category = "UnrealLua")
	FString ScriptPath = "";
	
	UPROPERTY(VisibleAnywhere, Category = "UnrealLua")
	TArray<FName> OverrideFunctions = {};
};

namespace UnrealLuaConfig
{
	static void Unload();
};

//This must be kept in sync with UUnrealLuaConfig fields!
USTRUCT(BlueprintType)
struct UNREALLUA_API FUnrealLuaConfigData
{
	GENERATED_BODY()
	
	UPROPERTY(Config, EditAnywhere, Category="General Settings") //
	bool bLuaEnabled = true;
	
	UPROPERTY(Config, EditAnywhere, Category="General Settings") //
	FKey UnrealLuaToolsMenuKey = EKeys::F6;
	
	UPROPERTY(Config, EditAnywhere, Category="General Settings") //
	bool bOverrideTick = true;
	
	UPROPERTY(Config, EditAnywhere, Category="General Settings") //
	bool bCompilerEnabled = true;
	
	UPROPERTY(Config, EditAnywhere, Category="General Settings") //
	bool bAllowWriteOnReadOnlyProperties = false;
	
	UPROPERTY(Config, EditAnywhere, Category="General Settings") //
	bool bAllowWriteOnCDO = false;
	
	UPROPERTY(Config, EditAnywhere, Category="General Settings") //
	bool bOverrideInput = true;
	
	UPROPERTY(Config, EditAnywhere, Category="General Settings") //@TODO : need edit array widget
	TArray<FName> LuaDisabledMaps = {"Untitled"};
	
	UPROPERTY(Config, EditAnywhere, Category="General Settings") //@TODO : need edit array widget
	TArray<FName> LuaDisabledGameModes = {};
	
	/*
	 * Normally, default scripts paths for native classes are just using their class names,
	 * i.e. Lua/DefaultScript/AActor.lua
	 * Setting this to true also makes the came include the package path when searching for default scripts:
	 * AActor's package path is /Script/Engine.Actor, so with this set to true, the loading path would be
	 * Lua/DefaultScript/Engine/AActor.lua
	 * 
	 * This allows the user to configure whether to have all native scripts in the top Lua/ScriptsDefault/
	 * folder, or whether they should be nested in subfolders per native module
	 */
	UPROPERTY(Config, EditAnywhere, Category="General Settings") //
	bool bUsePackagePathForNativeDefaultScripts = false;
	
	UPROPERTY(Config, EditAnywhere, Category="General Settings") //
	bool bUsePackagePathForBlueprintDefaultScripts = true;
	
	UPROPERTY(Config, EditAnywhere, Category="Modding")
	FString ModsDirectoryLocation = "~Mods";
	
	UPROPERTY(Config, EditAnywhere, Category="Modding")
	FString ScriptLoadModFileExtension = ".mod.lua";
	
	//UPROPERTY(Config, EditAnywhere, Category="General Settings")
	//bool bCreateLocalPlayerLuaStateSubsystem = false;
	
	UPROPERTY(Config, EditAnywhere, Category="Lua Replication")
	bool bEnableLuaReplication = true;
	
	UPROPERTY(Config, EditAnywhere, Category="Lua Replication")
	bool bAllowClientToServerRPCs = true;
	
	UPROPERTY(Config, EditAnywhere, Category="Lua Replication")
	bool bIgnoreInvalidServerRPC = true;
	
	UPROPERTY(Config, EditAnywhere, Category="Lua Replication")
	bool bMultithreadedReplication = true;

	UPROPERTY(Config, EditAnywhere, Category="Garbage Collection")
	bool bMultithreadGC = true;
	
	UPROPERTY(Config, EditAnywhere, Category="Garbage Collection")
	EUnrealLuaGCMode GCMode = EUnrealLuaGCMode::Incremental;

	/*
		The garbage-collector pause controls how long the collector waits before starting a new cycle.
		The collector starts a new cycle when the use of memory hits n% of the use after the previous collection.
		Larger values make the collector less aggressive. Values equal to or less than 100 mean the collector
		will not wait to start a new cycle. A value of 200 means that the collector waits for the total memory
		in use to double before starting a new cycle. The default value is 200; the maximum value is 1000.
	*/
	UPROPERTY(Config, EditAnywhere, Category="Garbage Collection", meta=(ClampMin=100, ClampMax=1000, UIMin=100, UIMax=1000))
	int32 LuaGCStepPause = 120;

	/*
		The garbage-collector step multiplier controls the speed of the collector relative to memory allocation,
		that is, how many elements it marks or sweeps for each kilobyte of memory allocated. Larger values make
		the collector more aggressive but also increase the size of each incremental step. You should not use values
		less than 100, because they make the collector too slow and can result in the collector never finishing a cycle.
		The default value is 100; the maximum value is 1000.
	 */
	UPROPERTY(Config, EditAnywhere, Category="Garbage Collection", meta=(ClampMin=100, ClampMax=1000, UIMin=100, UIMax=1000))
	int32 LuaGCStepMultiplier = 100;
	
	/*
		The garbage-collector step size controls the size of each incremental step, specifically how many bytes the
		interpreter allocates before performing a step. This parameter is logarithmic: A value of n means the interpreter
		will allocate 2n bytes between steps and perform equivalent work during the step. A large value (e.g., 60) makes
		the collector a stop-the-world (non-incremental) collector. The default value is 13, which means steps
		of approximately 8 Kbytes. 
	 */
	UPROPERTY(Config, EditAnywhere, Category="Garbage Collection", meta=(ClampMin=10, ClampMax=100, UIMin=10, UIMax=100))
	int32 LuaGCStepSize = 13;
	
	/*
		Has only an effect if UnrealLua GC mode is set to incremental mode.
		At the end of each frame, UnrealLua incrementally scans a list of Lua GC items (tables, upvalues, etc), attempting to
		nil any invalid UObject references. This value controls the maximum number of GC items get scanned at the end of each frame for 
		invalid UObject references. 
		
		A lower a value means it might take more frames for all invalid UObject references in Lua to be set to nil, but improving performance.
		A higher value allows for more GC items to be examined at the end of each frame, and thus a faster nilling of 
		invalid UObject references, but at the cost of lower performance.
		
		A too low value might result in the reference collector getting outpaced by many newly allocated userdata objects (more
		references created than examined), while a large value might cause stuttering due to too many items	being scanned at 
		the end of the frame. The default value is 10.
		
		Note: This is just a guidance value, the incremental reference collector will adjust its real value depending on how many
		memory allocations are performed during gameplay, but takes this limit value into account
	 */
	UPROPERTY(Config, EditAnywhere, Category="Garbage Collection", meta=(ClampMin=5, ClampMax=1000, UIMin=5, UIMax=1000))
	int32 LuaIncrementalGCLimit = 50;
	
	UPROPERTY(Config, EditAnywhere, Category="Debug Settings") ///
	bool bSelfTestOnStartup = true;
	
	UPROPERTY(Config, EditAnywhere, Category="Editor")
	FString EditorGameMode = "Editor";
	
	//Ensures that values are valid by adjusting invalid values
	//If returns false, fatal errors were made and we should not accept this config data
	bool EnsureValidity();
};

UCLASS(Config="UnrealLua", defaultconfig, AutoExpandCategories=("General Settings"))
class UNREALLUA_API UUnrealLuaConfig : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	static UUnrealLuaConfig* Get();
	
	UUnrealLuaConfig();
	
	virtual void BeginDestroy() override;

	void InitFromLuaFile();
	void CreateDefaultLuaConfigFile();
	
	void InitializeConfigSettingsStruct(FUnrealLuaConfigData& toInitialize);
	void InitializeConfigSettingsStructFromCachedRuntimeData(FUnrealLuaConfigData& toInitialize);
	
	bool ApplySettingsToCache(FUnrealLuaConfigData toApply);
	
	void CommitCachedSettingsForLuaSession();

	static FString GetConfigPropertyDescriptionForPropertyName(FName propName);
	static FString GetConfigLuaAnnotationForPropertyName(FName propName);
	
	void WriteConfigToLuaFile();
	void WriteDefaultConfigToLuaFile();
	
	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	static FKey GetMainMenuKey();

	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	static bool IsLuaEnabled();
	
    UFUNCTION(BlueprintCallable, Category= "UnrealLua")
    static bool IsGameModeDisabledForLua(const FString& gamemode);
	
    UFUNCTION(BlueprintCallable, Category= "UnrealLua")
    static bool IsMapDisabledForLua(const FString& map);

	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
    static FString GetModsDir();
	
    UFUNCTION(BlueprintCallable, Category= "UnrealLua")
    static EUnrealLuaGCMode GetGCMode();
	
    UFUNCTION(BlueprintCallable, Category= "UnrealLua")
    static bool ShouldPerformSelfTest();

	virtual FName GetContainerName() const override;
	virtual FName GetCategoryName() const override;
	virtual FName GetSectionName() const override;
#if WITH_EDITOR
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
	virtual bool SupportsAutoRegistration() const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	virtual TSharedPtr<SWidget> GetCustomSettingsWidget() const override;
	static bool ShouldOverrideInput();

	static bool AllowOverrideTick();

	static bool UsePackagePathAsFolders();
	static bool IsMultithreadReplicationEnabled();
	static bool IsLuaWriteOnCDOAllowed();

	static bool ShouldAllowWriteOnReadOnlyProperties();
	static bool ShouldIgnoreInvalidServerRPC();

	static const FString& GetModsDirLocation();
	static const FString& GetLuaScriptModFileExtension();
		
	static int32 GetLuaGCStepSize();
	static int32 GetLuaGCStepMultiplier();
	static int32 GetLuaGCStepPause();
	static bool ShouldMultithreadGC();
	static int32 GetLuaIncrementalGCLimit();
	
	UPROPERTY(Config, EditAnywhere, Category="General Settings")
	bool bLuaEnabled = true;
	
	UPROPERTY(Config, EditAnywhere, Category="General Settings")
	FKey UnrealLuaToolsMenuKey = EKeys::F6;
	
	UPROPERTY(Config, EditAnywhere, Category="General Settings")
	bool bOverrideTick = true;
	
	UPROPERTY(Config, EditAnywhere, Category="General Settings")
	bool bCompilerEnabled = true;
	
	UPROPERTY(Config, EditAnywhere, Category="General Settings")
	bool bAllowWriteOnReadOnlyProperties = false;
	
	UPROPERTY(Config, EditAnywhere, Category="General Settings")
	bool bAllowWriteOnCDO = false;
	
	UPROPERTY(Config, EditAnywhere, Category="General Settings")
	bool bOverrideInput = true;
	
	UPROPERTY(Config, EditAnywhere, Category="General Settings")
	TArray<FName> LuaDisabledMaps = {"Untitled"};
	
	UPROPERTY(Config, EditAnywhere, Category="General Settings")
	TArray<FName> LuaDisabledGameModes = {};
	
	/*
	 * Normally, default scripts paths for native classes are just using their class names,
	 * i.e. Lua/DefaultScript/AActor.lua
	 * Setting this to true also makes the came include the package path when searching for default scripts:
	 * AActor's package path is /Script/Engine.Actor, so with this set to true, the loading path would be
	 * Lua/DefaultScript/Engine/AActor.lua
	 * 
	 * This allows the user to configure whether to have all native scripts in the top Lua/ScriptsDefault/
	 * folder, or whether they should be nested in subfolders per native module
	 */
	UPROPERTY(Config, EditAnywhere, Category="General Settings")
	bool bUsePackagePathForNativeDefaultScripts = false;
	
	UPROPERTY(Config, EditAnywhere, Category="General Settings")
	bool bUsePackagePathForBlueprintDefaultScripts = true;
	
	UPROPERTY(Config, EditAnywhere, Category="Modding") //
	FString ModsDirectoryLocation = "~Mods";
	
	UPROPERTY(Config, EditAnywhere, Category="Modding") //
	FString ScriptLoadModFileExtension = ".mod.lua";

	UPROPERTY(Config, EditAnywhere, Category="Lua Replication") //
	bool bEnableLuaReplication = true;
	
	UPROPERTY(Config, EditAnywhere, Category="Lua Replication")//
	bool bAllowClientToServerRPCs = true;
	
	UPROPERTY(Config, EditAnywhere, Category="Lua Replication")//
	bool bIgnoreInvalidServerRPC = true;
	
	UPROPERTY(Config, EditAnywhere, Category="Lua Replication")//
	bool bMultithreadedReplication = true;
	
	UPROPERTY(Config, EditAnywhere, Category="Garbage Collection") //
	bool bMultithreadGC = true;
	
	UPROPERTY(Config, EditAnywhere, Category="Garbage Collection")
	EUnrealLuaGCMode GCMode = EUnrealLuaGCMode::Incremental;

	/*
		The garbage-collector pause controls how long the collector waits before starting a new cycle.
		The collector starts a new cycle when the use of memory hits n% of the use after the previous collection.
		Larger values make the collector less aggressive. Values equal to or less than 100 mean the collector
		will not wait to start a new cycle. A value of 200 means that the collector waits for the total memory
		in use to double before starting a new cycle. The default value is 200; the maximum value is 1000.
	*/
	UPROPERTY(Config, EditAnywhere, Category="Garbage Collection", meta=(ClampMin=100, ClampMax=1000, UIMin=100, UIMax=1000))
	int32 LuaGCStepPause = 120;

	/*
		The garbage-collector step multiplier controls the speed of the collector relative to memory allocation,
		that is, how many elements it marks or sweeps for each kilobyte of memory allocated. Larger values make
		the collector more aggressive but also increase the size of each incremental step. You should not use values
		less than 100, because they make the collector too slow and can result in the collector never finishing a cycle.
		The default value is 100; the maximum value is 1000.
	 */
	UPROPERTY(Config, EditAnywhere, Category="Garbage Collection", meta=(ClampMin=100, ClampMax=1000, UIMin=100, UIMax=1000))
	int32 LuaGCStepMultiplier = 100;
	
	/*
		The garbage-collector step size controls the size of each incremental step, specifically how many bytes the
		interpreter allocates before performing a step. This parameter is logarithmic: A value of n means the interpreter
		will allocate 2n bytes between steps and perform equivalent work during the step. A large value (e.g., 60) makes
		the collector a stop-the-world (non-incremental) collector. The default value is 13, which means steps
		of approximately 8 Kbytes. 
	 */
	UPROPERTY(Config, EditAnywhere, Category="Garbage Collection", meta=(ClampMin=10, ClampMax=100, UIMin=10, UIMax=100))
	int32 LuaGCStepSize = 13;
	
	/*
		Has only an effect if UnrealLua GC mode is set to incremental mode.
		At the end of each frame, UnrealLua incrementally scans a list of Lua GC items (tables, upvalues, etc), attempting to
		nil any invalid UObject references. This value controls the maximum number of GC items get scanned at the end of each frame for 
		invalid UObject references. 
		
		A lower a value means it might take more frames for all invalid UObject references in Lua to be set to nil, but improving performance.
		A higher value allows for more GC items to be examined at the end of each frame, and thus a faster nilling of 
		invalid UObject references, but at the cost of lower performance.
		
		A too low value might result in the reference collector getting outpaced by many newly allocated userdata objects (more
		references created than examined), while a large value might cause stuttering due to too many items	being scanned at 
		the end of the frame. The default value is 10.
		
		Note: This is just a guidance value, the incremental reference collector will adjust its real value depending on how many
		memory allocations are performed during gameplay, but takes this limit value into account
	 */
	UPROPERTY(Config, EditAnywhere, Category="Garbage Collection", meta=(ClampMin=5, ClampMax=1000, UIMin=5, UIMax=1000))
	int32 LuaIncrementalGCLimit = 50;
	
	UPROPERTY(Config, EditAnywhere, Category="Debug Settings") ///
	bool bSelfTestOnStartup = true;
	
	UPROPERTY(Config, EditAnywhere, Category="Editor")
	FString EditorGameMode = "Editor";
	
#if WITH_EDITORONLY_DATA
	UPROPERTY(Config, EditAnywhere, Category="Editor")
	bool bCopyLuaContentDirectoryDuringPackaging = true;
#endif
	//Get settings used when starting a new lua session
	static const FUnrealLuaConfigData& GetCachedSettings();
	
	//Get settings used by Lua runtime when a game session is active
	static const FUnrealLuaConfigData& GetAppliedLuaSessionSettings();
private:
	UPROPERTY(Transient)
	FUnrealLuaConfigData CachedSettings = {};
	
	UPROPERTY(Transient)
	FUnrealLuaConfigData AppliedGameSessionSettings = {};
};
