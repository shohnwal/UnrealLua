#pragma once
#include "CoreMinimal.h"
#include "LoadedLuaGameModeSettings.h"
#include "UObject/NoExportTypes.h"
#include "LuaPath.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LuaValue/LuaCoroutine.h"
#include "LuaValue/LuaTable.h"
#include "LuaValue/LuaFunction.h"
#include "LuaContext/LuaScripts/LoadedLuaScriptCollection.h"
#include "LuaContext/LuaScripts/LoadedLuaScriptResult.h"
#include "LuaContext/LuaScripts/LuaScriptInstanceHandle.h"
#include "ScriptableUObject/LuaScriptSettings.h"
#include "sol/sol.hpp"
#include "UObject/UObjectIterator.h"
#include "UObject/GCObject.h"
#include "UObject/ScriptInterface.h"
#include "BlueprintSupport/WeakStructView.h"
#include "ScopedLuaContext.generated.h"

class ULuaStateInputHandler;
struct FLuaScriptSettings;
class UUnrealLuaMod;
class LuaUObjectRegistry;
class ILuaContext;
struct GCObject;

UENUM()
enum class ELuaContextType : uint8
{
	None,
	Engine,
	Game,
	Editor,
	Minimal,
	SelfTest,
	UnrealLuaTypeCompiler, LuaScriptEditor
};

enum ELuaContextLoadFlags
{
	NoFlags = 0,
	NoLoadUnrealClasses = 1 << 0,
	
};

struct UNREALLUA_API FLuaScriptImportStackItem
{
	sol::table importedTable;
	const std::string& ImportPath;
};

struct UNREALLUA_API FLuaImportStack
{
	bool IsEmpty() const { return ImportStacks.IsEmpty(); }
	int32 Num() const { return ImportStacks.Num(); }
	TArray<FLuaScriptImportStackItem*> ImportStacks;
};


//USTRUCT()
struct UNREALLUA_API FScopedLuaContext : public FGCObject, public TSharedFromThis<FScopedLuaContext>
{
	//GENERATED_BODY()
	FScopedLuaContext();
	explicit FScopedLuaContext(TScriptInterface<ILuaContext> owningLuaContext, ELuaContextType type, const FString& name = "Unnamed");
	//FScopedLuaContext(TArray<FString>& ModFolders,const FName & gameMode);
	virtual ~FScopedLuaContext() override;
	
	const FString& GetLuaContextName() const;

	static FScopedLuaContext* GetLuaContextFromLuaState(lua_State* L);

	sol::table_proxy<sol::global_table&, sol::detail::proxy_key_t<const char* const>> operator[](const char* const key);
	void Shutdown();
	void UnloadGameMode();
private:
	void UnloadGameModeInternal();
public:
	void SetupUnrealTypes();
	void InitializeLuaState();
	void LateRegisterNewModuleAssets(const TArray<UClass*>& newUClasses, const TArray<UScriptStruct*>& newScriptStructs, const TArray<UEnum*>& newEnums, const TArray<UBlueprintFunctionLibrary*>& newBlueprintLibraries);
public:
	void SetupLuaStateForGameMode(const FName& gameMode, const ELuaPathFlags pathFlags = ELuaPathFlags::AnyExceptUTypes);
	bool IsReadyForFinishDestroy();
	bool IsLuaLoaded();
	bool PerformSelfTest(TScriptInterface<ILuaContext> ctx);
	
	void Tick(float deltaTime);

	void TickFileScanner(float deltaTime);

	static bool IsUObjectValid(sol::stack_object obj_o);

	//static sol::object ImportUnrealType(sol::object name_o, sol::this_state lua);

	sol::state_view GetLuaState() const;
	sol::this_state GetLuaThisState() const;
	static sol::table CopyTable(sol::table targetTable, sol::table templateTable, bool bShallowCopy = true);
	static void PatchTable(sol::table templateTable, sol::table targetTable);
private:
	void StartLuaState();
	void SetupLogFunctions();

public:
	ULoadedLuaScriptCollection* GetOrCreateLuaScriptCollection(const FName& fileName);
    //FLuaScriptInstanceHandle GetSharedLuaScript(const FName& fileName);
	//FLuaScriptInstanceHandle GetInstancedLuaScript(const FName& fileName, FLuaUObjectItem& scriptOwner);
	FLuaScriptInstanceHandle GetLuaScriptHandle(const FLuaScriptSettings& scriptSettings);

	sol::table ImportLuaScript(const std::string_view path, bool bAllowModding = true, bool bTrackScript = false);
	sol::table ImportLuaScript(const std::string& path, bool bAllowModding = true, bool bTrackScript = false);
	sol::table ImportLuaScript(const FString& filePath, bool bAllowModding = true, bool bTrackScript = false);
	void MixinScript(sol::stack_object mixinPath, sol::this_state lua);
	sol::protected_function_result RunScript(sol::stack_object mixinPath, sol::variadic_args args);
	sol::protected_function_result RunScript(const std::string& mixinPath, sol::variadic_args args);

	sol::protected_function_result RunScriptFile(FString pathToFile, sol::variadic_args args);
	
	sol::protected_function_result RunSingleScriptFile(const std::string_view& fullPath, sol::variadic_args args = {});

	sol::protected_function_result RunString(const TCHAR* stringToRun, const TArray<sol::object>& args = {});
	sol::protected_function_result RunString(const std::string_view& stringToRun, const TArray<sol::object>& args = {});	
	bool IsInitialized() const;

private:
	FLuaImportStack ImportStack = {};
	bool bIsInitialized = false;

	friend class ULoadedLuaScriptCollection;
public:
	/**
	 * Loads a Lua script file and passes it through *.mod.lua files for allowing modifications.
	 * IF the path does not end with .lua, it will be automatically added
	 */
	FLoadLuaScriptResult LoadLuaScriptFromDisk(const FString& string, bool bIsAbsolutePath, bool bAllowModding, const FLuaPath* luaPathOverride = nullptr, ELuaPathFlags requiredFlags = ELuaPathFlags::Any, ELuaPathFlags excludedLocationFlags = ELuaPathFlags::None);
	FLoadLuaScriptResult LoadLuaScriptFromDisk(const FString& filePath,  bool bAllowModding, const FLuaPath* luaPathOverride = nullptr, ELuaPathFlags requiredFlags = ELuaPathFlags::Any, ELuaPathFlags excludedLocationFlags = ELuaPathFlags::None);
	FLoadLuaScriptResult LoadLuaScriptFromDisk(const std::string& filePath, bool bAllowMods, const FLuaPath* luaPathOverride = nullptr, ELuaPathFlags requiredFlags = ELuaPathFlags::Any, ELuaPathFlags excludedLocationFlags = ELuaPathFlags::None);
	sol::table ModTable(const std::string& filePath, sol::table table, const FLuaPath* luaPathOverride = nullptr, ELuaPathFlags requiredFlags = ELuaPathFlags::Any, ELuaPathFlags excludedLocationFlags = ELuaPathFlags::None);
	
	/**
	 * Load lua script from specific path
	 * @param filePath full path to file to load, must include .lua extension
	 * @return FLoadLuaScriptResult load result 
	 */
	FLoadLuaScriptResult FullPathLoadLuaScriptFromDisk(const FString& filePath);
	FLoadLuaScriptResult FullPathLoadLuaScriptFromDisk(const std::string& filePath);

	bool ReloadAllScripts();
	bool ReloadScript(const FName& fileName);
	void ReloadScriptByFullFileName(const FString& fullFileName);
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("ScopedLuaContext"); }
public:
	void ProcessInvalidUObjectCollection(bool bForcedCollection = false);
	void IncrementalProcessInvalidLuaUObjects();
	void ResetIncrementalGC();
	//UPROPERTY(VisibleAnywhere)
	
	
	FLuaTableHandle CreateNewLuaTable();
	FLuaTableHandle CreateLuaTableHandleForTable(const sol::table& table);
	FLuaFunctionHandle CreateNewFunctionFromString(const FString& funcString);
	FLuaFunctionHandle CreateFunctionHandleForLuaFunction(const sol::function& func);
	FLuaCoroutineHandle CreateNewCoroutineFromString(const FString& funcString);
	FLuaCoroutineHandle CreateCoroutineHandleForLuaFunction(sol::function& func);
	FLuaCoroutineHandle CreateCoroutineHandleForLuaFunction(sol::thread& t, sol::function& func);
	
	TArray<FWeakLuaTableHandle> ExternalLuaTables = {};
	TArray<FWeakLuaCoroutineHandle> ExternalLuaCoroutines = {};
	TArray<FWeakLuaFunctionHandle> ExternalLuaFunctions = {};
private:
	ELuaContextType ContextType = ELuaContextType::None;
public:
	
	//UPROPERTY(VisibleAnywhere)
	FLuaPath LuaPath = {};
private:
	sol::state LuaState = {};
	TScriptInterface<ILuaContext> OwningLuaContext;
	sol::table RegistryTable = {};
public:
	sol::table& GetRegistryTable(); 
	const sol::table& GetRegistryTable() const; 
	//UPROPERTY(VisibleAnywhere)
	TMap<FName, TObjectPtr<ULoadedLuaScriptCollection>> LoadedScripts = {};

	TArray<TObjectPtr<ULoadedLuaScriptCollection>> LoadedScriptsArray = {};

	TObjectPtr<ULoadedLuaScriptCollection> LastCheckedScriptCollection = nullptr;

	int32 NextIndexToHotReloadCheck = 0;
	
	FCriticalSection LuaStateLock = {};
	
	FLoadedLuaGameModeSettings LoadedGameModeSettings = {};
	
	FString LuaContextName = "Unnamed";

	int32 IncrementalNumObjectsToCheck = 10;
	GCObject* NextGCObjectToCheck = nullptr;
	
	ULuaStateInputHandler* GetInputHandler() const;
	TObjectPtr<ULuaStateInputHandler> PlayerInputHandler = nullptr;
};

template<>
struct TStructOpsTypeTraits< FScopedLuaContext > : public TStructOpsTypeTraitsBase2<FScopedLuaContext>
{
	enum
	{
		WithCopy = false,
    };
};
