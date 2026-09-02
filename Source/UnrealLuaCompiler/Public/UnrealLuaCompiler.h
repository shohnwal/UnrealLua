// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Utility/UnrealVersion.h"
#if UNREALLUA_UE_VERSION_OLDER_THAN(5,8,0)
#include "InstancedStruct.h"
#endif
#include "Interface/LuaContext.h"
#include "UObject/Object.h"
#include "InputAction.h"
#include "LuaCompilerSourceFileInfo.h"
#include "ModuleDescriptor.h"
#include "Prototypes/StructPrototypeBase.h"
#include "UnrealLuaCompiler.generated.h"


/**
 * 
 */
class UUserDefinedEnum;
struct FLuaTable;
class UStandaloneLuaContext;
struct FInstancedStruct;
class UUnrealLuaCompiler;

UENUM(BlueprintType)
enum class ELuaCompilerPropType : uint8
{
	Unknown,
	Int,
	Float
};

UENUM(BlueprintType)
enum class ELuaComplerSte : uint8
{
	Inactive,
	Setup,
	RunLuaFiles,
	Skeletons,
	
};

UCLASS(Transient)	
class UNREALLUA_API UUnrealLuaCompiler : public UEngineSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	static UUnrealLuaCompiler* Get();
	void NotifyAllModulesLoaded();
	void Start();
	
	bool HandleErrorBox(const TArray<FString>& errors);
	EAppReturnType::Type DisplayErrorBox(const TArray<FString>& errors);
	EAppReturnType::Type DisplayRunawayErrorBox(const TArray<FString>& errors);
	
	void BuildInputActions(FScopedLuaContext& ctx);
	void RunPrototypeLuaFiles(FScopedLuaContext& Ctx);

	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	bool Compile();
	bool CompileUnrealTypes(FScopedLuaContext& ctx);
	
	bool IsCompiledTypeValid(UField* newType) const;
	
	bool HasErrors() const;
	void NotifyEndFrame();
	void NotifyModuleChanged(FName moduleName, EModuleChangeReason moduleChangeReason);

	static int __UENUM(lua_State* L);
	static int __UCLASS(lua_State* L);
	static int __USTRUCT(lua_State* L);
	static int __UINTERFACE(lua_State* L);
	static int __MULTICAST_DELEGATE(lua_State* L);
	static const char* CompilerThrowErrorMsg;
	
	static void SetError(const FString& optionalErrorMsg = {});
	template<typename ...Args>
	static void SetIsErrorWithArgs(const FString& reason, Args... args)
	{
		FString output = FString::Format(*reason, {args...});
		UUnrealLuaCompiler::SetError(output);
	}
	
	void CommitPrototype(UnrealLua::Compiler::IStructPrototypeBase* commitedBy);
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStandaloneLuaContext> CompilerLuaContext = nullptr;
	lua_State* GetCompilerLuaState() const;
	sol::state_view GetCompilerLuaStateView() const;
	FScopedLuaContext& GetCompilerScopedLuaStateContext() const;
	
	UFUNCTION()
	void NotifyLuaStateActiveChange(UUnrealLuaEngineSubsystem* ss, const TScriptInterface<ILuaContext> ictx, bool isActive);
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString CurrentlyRunFilePath = "";
	UPROPERTY()
	TObjectPtr<UUnrealLuaEngineSubsystem> LuaSystem = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UPackage> UnrealLuaInputActionPackage = nullptr;
	
	/* The root lua UPackage all lua-created UTypes belong to. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UPackage> UnrealLuaCompilerUTypePackage = nullptr;
	
	UPROPERTY(VisibleAnywhere)
	TMap<FName, TObjectPtr<UClass>> CompiledClasses = {};
	UPROPERTY(VisibleAnywhere)
	TMap<FName, TObjectPtr<UScriptStruct>> CompiledStructs = {};
	UPROPERTY(VisibleAnywhere)
	TMap<FName, TObjectPtr<UEnum>> CompiledEnums = {};
	UPROPERTY(VisibleAnywhere)
	TArray<TObjectPtr<UInputAction>> InputActions = {};
	
	UPROPERTY(VisibleAnywhere)
	TMap<FString, FLuaCompilerSourceFileInfo> FileContents;

	bool CompilationInProgress() const;

	TUniquePtr<UnrealLua::Compiler::IStructPrototypeBase> CurrentlyCompiledType = nullptr;
	
	TMap<FString, TUniquePtr<UnrealLua::Compiler::IStructPrototypeBase>> UnrealLuaUTypePrototypes = {};

	TArray<FString> ErrorLog = {};
	FString GetUnrealName(bool bIsStruct, const FString& ClassName);
	
private:
	void BackupCompiledTypes();
	void TryRecoverWithBackupTypes();
	
	bool bIsExecutingLuaFile = false;
	bool ShouldThrowLuaOnError() { return bIsExecutingLuaFile; }
};
