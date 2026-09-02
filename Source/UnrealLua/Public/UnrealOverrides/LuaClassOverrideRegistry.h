// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FileSystem/UnrealLuaFileSystemEntry.h"
#include "UObject/SoftObjectPath.h"
#include "LuaClassOverrideRegistry.generated.h"
class UUnrealLuaOverrideFunctionHostClass;
struct FLuaUObjectItem;
class UFunction;

/**
 * 
 */

DECLARE_MULTICAST_DELEGATE_OneParam(FSimpleUClassMulticastDelegate, UClass*)

enum class ELuaScriptableObjectClass : uint8
{
	UObject,
	Actor,
	Component,
	UserWidget
};

USTRUCT()
struct UNREALLUA_API FLuaOverrideClassInfo
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere)
	FSoftObjectPath ClassToOverride = {};
	
	UPROPERTY(VisibleAnywhere)
	FString ScriptPath = "";
	
	UPROPERTY(VisibleAnywhere)
	bool bIsTemporary = false;

	bool operator==(const FLuaOverrideClassInfo& rhs) const
	{
		return ClassToOverride == rhs.ClassToOverride && ScriptPath == rhs.ScriptPath;
	}
	
	friend inline uint32 GetTypeHash(const FLuaOverrideClassInfo& This)
	{
		return HashCombineFast(GetTypeHash(This.ClassToOverride), GetTypeHash(This.ScriptPath));
	}
};

USTRUCT()
struct UNREALLUA_API FLuaOverriddenClassInfo : public FLuaOverrideClassInfo
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere)
	bool bHasGetLuaScriptSettingsFunction = false;
	UPROPERTY(VisibleAnywhere)
	bool bImplementsLuaScriptable = false;
	UPROPERTY(VisibleAnywhere)
	FString DefaultAssetScriptFilePath = "";
	UPROPERTY(VisibleAnywhere)
	UUnrealLuaOverrideFunctionHostClass* OverrideHostClass = nullptr;
};

USTRUCT(BlueprintType)
struct UNREALLUA_API FLuaClassOverrideRegistry
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere)
	TMap<UClass*, FLuaOverriddenClassInfo> OverriddenClasses = {};
		
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<UField*> Candidates = {};
	
	UPROPERTY(VisibleAnywhere)
	TMap<FSoftObjectPath, FLuaOverrideClassInfo> OverrideBaseClassInfo = {};
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool OverrideUFunctionsEnabled = true;
	
	FDelegateHandle AsyncUpdateHandle = {};
	
	void DisableUFunctionOverriding();
	void EnableUFunctionOverriding();
	
	void InitOverrideRegistry();
	void ActivateOverrideRegistry();
	
	void ForceBuildFieldMapping(const UField* metaField);
	void HandleLoadingFinishedMetaField(UField* field);
	void OnAsyncLoadingFlushUpdate();
	bool IsClassLuaOverridable(UClass* Class);
	bool HandleStillLoadingField(UField* ss);
	void ShutdownOverrideRegistry();
	void TryOverrideObjectClass(UObject* obj);
	void RemoveOverrides(UClass* Class);
	void ClearInvalidUClasses();
	bool IsClassLuaOverridden(UClass* Class);
	FLuaOverriddenClassInfo* GetOverridenClassInfo(UClass* uclass);
	
	//Gets a default script path for a given UClass.
	//This does NOT include the prefix of ../Content/Lua/, but only returns a path relative to the Lua root
	static FString GetDefaultLuaScriptPathForUClass_LuaFolderRelative(UClass* uclass, bool addLuaFileExtension);
	//Gets a full default script path for a given UClass.
	//This does include the prefix of ../Content/Lua/DefaultScript/
	static FString GetDefaultLuaScriptPathForUClass_WithRelativeLuaRootPath(UClass* uclass, bool addLuaFileExtension);

	void RequestMakeUClassOverridable(UClass* uclass);
private:
	FLuaOverrideClassInfo* FindOverrideInfoForClass(UClass* uclass);
	void OverrideClass(UClass* clazz);
	bool IsChildOfAnyOverridableClass(UClass* uclass);
	void PrepareOverrideClassPaths();
	void InitialOverridesAndBuildMappings();
	void OverrideUFunction(UFunction* originalFunc, UClass* Class, ELuaScriptableObjectClass objectClass, UUnrealLuaOverrideFunctionHostClass* overrideClass);
	void OverrideUFunction_TestDontUse(UFunction* func, UClass* clazz, ELuaScriptableObjectClass objectClass);
public:
	static bool IsOverridableUFunction(const UFunction* Function);
	FSimpleUClassMulticastDelegate OnClassOverrideFinished = {};
};