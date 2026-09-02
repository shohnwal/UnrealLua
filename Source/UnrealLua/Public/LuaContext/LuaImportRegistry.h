// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "sol/sol.hpp"
#include "LuaImportRegistry.generated.h"

/**
 * 
 */

class UBlueprintFunctionLibrary;
enum class ELuaContextType : uint8;
class FReferenceCollector;
struct FScopedLuaContext;

struct UNREALLUA_API FLuaImportResult
{
	sol::object ImportedObject;

	static void RegisterUsertype(sol::state_view& lua);
	sol::object As(sol::object key, sol::this_state lua);
	sol::object Get();
};

USTRUCT()
struct UNREALLUA_API FLuaImportRegistry
{
	GENERATED_BODY()
	
	static FLuaImportRegistry& Get();
	
	void InitializeLuaContext(FScopedLuaContext& ctx);
	
	~FLuaImportRegistry();
	sol::object UImport(sol::object& name_o, sol::this_state thisState);
	void ClearImportCache();

private:
	sol::object UImportPath(const std::string_view& name_o, sol::this_state thisState);
public:
	void LateRegisterNewModuleAssets(FScopedLuaContext& ctx, const TArray<UClass*>& newUClasses, const TArray<UScriptStruct*>& newScriptStructs, const TArray<UEnum*>& newEnums, const TArray<UBlueprintFunctionLibrary*>& newBlueprintLibraries);
	//static bool ImportUnrealFunctionLibrary(sol::object name_o, sol::this_state thisState);
public:
	sol::object __index(sol::object key, sol::this_state lua);
	void __newindex(sol::object key, sol::object value, sol::this_state lua);
private:
	void LoadUClasses(FScopedLuaContext& ctx);
	void RegisterUClass(UClass* uclass, FScopedLuaContext& ctx);
	
	void LoadUScriptStructs(FScopedLuaContext& ctx);
	void RegisterUScriptStruct(UScriptStruct* uss, FScopedLuaContext& ctx);
	
	void LoadEnums(FScopedLuaContext& ctx);
	void RegisterUEnum(UEnum* uenum, FScopedLuaContext& ctx);
	
	void LoadBlueprintLibraries(FScopedLuaContext& ctx);
	void RegisterBlueprintLibrary(UBlueprintFunctionLibrary* lib, FScopedLuaContext& ctx);
public:
	
	UPROPERTY(VisibleAnywhere)
	TMap<FString, TSoftObjectPtr<UField>> ImportsCache = {};
	//TMap<FName, TUniquePtr<FLuaUClass>> ImportedUClasses = {};
	//TMap<FName, TUniquePtr<FLuaUStruct>> ImportedUStructs = {};
	//void CollectObjectReferences(FReferenceCollector& collector);

	//sol::table RegistryTable = sol::nil;
	//sol::table LibraryTable = sol::nil;
	
};
