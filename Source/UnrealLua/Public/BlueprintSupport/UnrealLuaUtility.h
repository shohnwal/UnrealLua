// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "WeakStructView.h"
#include "Interface/LuaScriptable.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LuaContext/GameLuaContext.h"
#include "LuaValue/LuaCoroutine.h"
#include "LuaValue/LuaTable.h"
#include "LuaValue/LuaValue.h"
#include "LuaTypes/LuaDelegate.h"
#include "UObject/CoreNetTypes.h"
#include "UnrealLuaUtility.generated.h"

class UProperty;
enum class ELuaValueType : uint8;
class ULuaContextBlueprintBase;
/**
 * 
 */

UENUM()
enum class ELuaCallResult : uint8
{
	Success,
	Failure,
};

typedef TMap<FString,FString> FStringMap;
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FOnLuaScriptUpdateDelegate, const TScriptInterface<ILuaScriptable>&, luaScriptable, const TArray<FString>&, scriptKeys, const TArray<FString>&, scriptValues);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLuaScriptUpdateMulticastDelegate, const TScriptInterface<ILuaScriptable>&, luaScriptable, const TArray<FString>&, scriptKeys, const TArray<FString>&, scriptValues);
UCLASS()
class UNREALLUA_API UUnrealLuaUtility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	static FOnLuaScriptUpdateMulticastDelegate OnLuaScriptChanged;

	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	static void CreateUnrealLuaMetadata();

	UFUNCTION(BlueprintCallable, CustomThunk, meta=(DefaultToSelf = target, AutoCreateRefTerm="key", CustomStructureParam="input"))
	static void SetObjectLuaScriptValue(UObject* target, const FString& key, const int32& input, bool broadcastNotify = true);
	DECLARE_FUNCTION(execSetObjectLuaScriptValue);

	UFUNCTION(BlueprintCallable, CustomThunk, meta=(ExpandEnumAsExecs="ReturnValue", DefaultToSelf = target, AutoCreateRefTerm="key", CustomStructureParam="outValue"))
	static ELuaCallResult GetObjectLuaScriptValue(UObject* target, const FString& key, int32& outValue);
	DECLARE_FUNCTION(execGetObjectLuaScriptValue);

	UFUNCTION(BlueprintCallable, meta=(ExpandEnumAsExecs="ReturnValue", DefaultToSelf=target, AutoCreateRefTerm="key, functionString"))
	static ELuaCallResult SetObjectLuaScriptFunctionFromString(UObject* target, const FString& key, const FString& functionString);
	
	UFUNCTION(BlueprintCallable, meta=(ExpandEnumAsExecs="ReturnValue", DefaultToSelf=target, AutoCreateRefTerm="key, functionString"))
	static ELuaCallResult SetObjectLuaScriptFunctionFromStringSplit(UObject* target, const FString& key, const FString& functionArgsString, const FString& functionBodyString);
	
	UFUNCTION(BlueprintCallable, meta=(DefaultToSelf = target, AutoCreateRefTerm="key,callback"))
	static void BindEventToObjectLuaScriptValueChanged(UObject* target, const FString& key, const FOnLuaScriptValueChangedDelegate& callback);

	UFUNCTION(BlueprintCallable, meta=(DefaultToSelf = target, AutoCreateRefTerm="key,callback"))
	static void UnbindEventFromObjectLuaScriptValueChanged(UObject* target, const FString& key, const FOnLuaScriptValueChangedDelegate& callback);

	UFUNCTION(BlueprintCallable, meta=(DefaultToSelf = target, AutoCreateRefTerm="key"))
	static void BroadcastObjectLuaScriptValue(UObject* target, const FString& key);

	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	static bool LoadLuaScriptForObject(UObject* luaScriptableObject, bool bForceReload = false);

	UFUNCTION(BlueprintCallable, meta=(DefaultToSelf=parent))
	static bool AddReplicatedSubobject(UObject* parent, UObject* subobject, ELifetimeCondition condition = ELifetimeCondition::COND_None);

	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	static bool RemoveReplicatedSubobject(UObject* parent, UObject* subobject);

	UFUNCTION(BlueprintCallable, meta=(WorldContext=worldContextObject))
	static TScriptInterface<ILuaContext> GetLuaContext(const UObject* worldContextObject, ELuaStateType luaStateName = ELuaStateType::DefaultState);

	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	static void BindEventToLuaScriptUpdate(const FOnLuaScriptUpdateDelegate& del);

	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	static void BroadcastLuaScriptUpdate(const TScriptInterface<ILuaScriptable>& luaScriptable, const TMap<FString,FString>& scriptMap);
	
	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	static void ScanForLuaClasses();

	
	/*
	 * Lua Values
	 */
 
	UFUNCTION(BlueprintPure, CustomThunk, meta=(Variadic, BlueprintInternalUseOnly="true", AdvancedDisplay=3))
    static bool MakeLuaValue(const int32 numItems);
    DECLARE_FUNCTION(execMakeLuaValue);
 
    UFUNCTION(BlueprintPure, CustomThunk, meta=(Variadic, BlueprintInternalUseOnly="true", AdvancedDisplay=3))
    static bool MakeLuaValuesArray(const int32 numItems, TArray<FLuaValue>& outValuesArray);
    DECLARE_FUNCTION(execMakeLuaValuesArray);
	
	UFUNCTION(BlueprintPure, CustomThunk, meta=(Variadic, BlueprintInternalUseOnly="true", AdvancedDisplay=3))
	static bool GetFromLuaValue(const int32 numItems);
	DECLARE_FUNCTION(execGetFromLuaValue);

	UFUNCTION(BlueprintCallable, CustomThunk, meta=(ExpandEnumAsExecs="ReturnValue", AdvancedDisplay=1))
	static ELuaValueType SwitchOnLuaValueType(UPARAM(ref) const FLuaValue& inLuaValue);
	DECLARE_FUNCTION(execSwitchOnLuaValueType);
	
	UFUNCTION(BlueprintCallable, CustomThunk, meta=(ExpandEnumAsExecs="ReturnValue",CustomStructureParam="outValue"))
	static ELuaValueType SwitchOnLuaValueTypeWithValue(UPARAM(ref) const FLuaValue& inLuaValue, int32& outValue);
	DECLARE_FUNCTION(execSwitchOnLuaValueTypeWithValue);

	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	static TMap<FString, FString> LuaScriptToString(const TScriptInterface<ILuaScriptable>& target);

	UFUNCTION(BlueprintCallable, BlueprintPure, CustomThunk, meta=(CustomStructureParam=input))
	static void SetLuaValueInObject(UObject* object, const FString& key, const int32& input);
	DECLARE_FUNCTION(execSetLuaValueInObject);

	UFUNCTION(BlueprintCallable, CustomThunk)
	static void ClearLuaValueInObject(UObject* object, const FString& key);
	DECLARE_FUNCTION(execClearLuaValueInObject);
	

	
	/*
	 * Lua Tables
	 */
	
	//Create a Lua table
	UFUNCTION(BlueprintCallable, meta=(DefaultToSelf=worldContext, ExpandBoolAsExecs="ReturnValue"))
	static bool MakeLuaTable(UObject* worldContext, FLuaTableHandle& outNewTable);
	
	UFUNCTION(BlueprintCallable, Customthunk, meta=(ExpandBoolAsExecs="ReturnValue", CustomStructureParam="key,value"))
	static bool SetValueInLuaTable(const FLuaTableHandle& table, const int32& key, const int32& value);
	DECLARE_FUNCTION(execSetValueInLuaTable);
	
	UFUNCTION(BlueprintCallable, Customthunk, meta=(ExpandBoolAsExecs="ReturnValue", CustomStructureParam="key"))
    static bool GetValueFromLuaTable(const FLuaTableHandle& table, const int32& key, FLuaValue& outValue);
    DECLARE_FUNCTION(execGetValueFromLuaTable);

	UFUNCTION(BlueprintCallable, Customthunk, meta=(ExpandBoolAsExecs="ReturnValue", CustomStructureParam="key"))
	static bool ClearLuaTableKey(const FLuaTableHandle& table, const int32& key);
	DECLARE_FUNCTION(execClearLuaTableKey);
	
	UFUNCTION(BlueprintCallable, Customthunk, meta=(ExpandBoolAsExecs="ReturnValue"))
	static bool ClearLuaTable(const FLuaTableHandle& table);
	DECLARE_FUNCTION(execClearLuaTable);

	/*
	 * Lua Functions
	 */
	
	UFUNCTION(BlueprintCallable, meta=(ExpandBoolAsExecs="ReturnValue", DefaultToSelf=worldContext, AutoCreateRefTerm="functionString"))
	static ELuaCallResult MakeLuaFunctionFromString(UObject* worldContext, UPARAM(meta=(MultiLine=True)) const FString& functionString, FLuaFunctionHandle& outLuaFunction);
	
	UFUNCTION(BlueprintCallable, meta=(ExpandBoolAsExecs="ReturnValue", DefaultToSelf=worldContext, AutoCreateRefTerm="functionString"))
	static ELuaCallResult GetLuaFunctionFromObject(UObject* worldContext, const FString& funcName, FLuaFunctionHandle& outLuaFunction);

	UFUNCTION(BlueprintCallable, meta=(ExpandBoolAsExecs="ReturnValue", AutoCreateRefTerm="functionString"))
	static ELuaCallResult GetLuaFunctionFromTable(FLuaTableHandle handle, const FString& funcName, FLuaFunctionHandle& outLuaFunction);

	UFUNCTION(BlueprintCallable, meta=(DefaultToSelf = worldContext, ExpandBoolAsExecs="ReturnValue", AutoCreateRefTerm="functionString"))
	static ELuaCallResult GetGlobalLuaFunction(UObject* worldContext, const FString& funcName, FLuaFunctionHandle& outLuaFunction);
	
	UFUNCTION(BlueprintCallable, CustomThunk, meta=(ExpandEnumAsExecs="ReturnValue", Variadic,BlueprintInternalUseOnly="true", AdvancedDisplay=1))
	static ELuaCallResult CallLuaFunction(const FLuaFunctionHandle& luaFunction, const int32 numArguments, const int32 numResults);
	DECLARE_FUNCTION(execCallLuaFunction);
	
	UFUNCTION(BlueprintCallable, CustomThunk, meta=(ExpandEnumAsExecs="ReturnValue", Variadic, BlueprintInternalUseOnly="true", AdvancedDisplay=3, DefaultToSelf=target))
	static ELuaCallResult CallLuaFunctionOnObject(UObject* target, const FString& funcName, const bool selfCall, const int32 numArguments, const int32 numResults);
	DECLARE_FUNCTION(execCallLuaFunctionOnObject);
		
	UFUNCTION(BlueprintCallable, CustomThunk, meta=(ExpandEnumAsExecs="ReturnValue", Variadic, BlueprintInternalUseOnly="true", AdvancedDisplay=3))
	static ELuaCallResult CallLuaFunctionOnTable(const FLuaTableHandle& table, const FString& funcName, const bool selfCall, const int32 numArguments, const int32 numResults);
	DECLARE_FUNCTION(execCallLuaFunctionOnTable);
	
	UFUNCTION(BlueprintCallable, CustomThunk, meta=(ExpandEnumAsExecs="ReturnValue", Variadic, BlueprintInternalUseOnly="true", AdvancedDisplay=3))
	static ELuaCallResult CallGlobalLuaFunction(UObject* worldContext, const FString& funcName, const int32 numArguments, const int32 numResults);
	DECLARE_FUNCTION(execCallGlobalLuaFunction);

	/*
	 * Lua Coroutines
	 */

	UFUNCTION(BlueprintCallable, meta=(ExpandBoolAsExecs="ReturnValue", DefaultToSelf=worldContext, AutoCreateRefTerm="functionString"))
	static bool MakeLuaCoroutineFromString(UObject* worldContext, UPARAM(meta=(MultiLine=True)) const FString& functionString, FLuaCoroutineHandle& outLuaCoroutine);

	UFUNCTION(BlueprintCallable, meta=(ExpandBoolAsExecs="ReturnValue", DefaultToSelf=target, AutoCreateRefTerm="functionString"))
	static bool MakeLuaCoroutineFromLuaFunction(const FLuaFunctionHandle& functionValue, FLuaCoroutineHandle& outCoroutine);

	UFUNCTION(BlueprintCallable, CustomThunk, meta=(ExpandEnumAsExecs="ReturnValue", Variadic, BlueprintInternalUseOnly="true", AdvancedDisplay=1))
	static ELuaCoroutineCallStatus CallLuaCoroutine(const FLuaCoroutineHandle& coroutine, const int32 numArguments, const int32 numResults);
	DECLARE_FUNCTION(execCallLuaCoroutine);
		
	UFUNCTION(BlueprintCallable, meta=(AdvancedDisplay=3))
	static ELuaCoroutineCallStatus GetLuaCoroutineStatus(const FLuaCoroutineHandle& coroutine);	
	
	
	/*
	 *	Delegates
	 */
	
	
	UFUNCTION(BlueprintCallable, meta=(DefaultToSelf=target, AutoCreateRefTerm="luaDelegateKeyName, delegate", Keywords="Bind, Event, Delegate, Lua"))
	static FLuaDelegateHandle BindEventToLuaDelegateInObject(UObject* target, const FString& luaDelegateKeyName, const FLuaDelegate& delegate, bool createNewOnTargetIfNotFound = true); 
	
	UFUNCTION(BlueprintCallable, meta=(DefaultToSelf=target, AutoCreateRefTerm="luaDelegateKeyName, delegate", Keywords="Unbind, Event, Delegate, Lua"))
	static bool UnbindEventFomLuaDelegateInObject(UObject* target, const FString& luaDelegateKeyName, const FLuaDelegate& delegate);
	
	UFUNCTION(BlueprintCallable, meta=(DefaultToSelf=target, AutoCreateRefTerm="luaDelegateKeyName, delegate", Keywords="Unbind, Event, Delegate, Lua"))
	static bool UnbindEventFomLuaDelegateInObjectWithHandle(UObject* target, const FString& luaDelegateKeyName, const FLuaDelegateHandle& handleToRemove);
	
	UFUNCTION(BlueprintCallable, meta=(DefaultToSelf=target, AutoCreateRefTerm="luaDelegateKeyName, delegate", Keywords="Bind, Event, Delegate, Lua"))
	static FLuaDelegateHandle BindEventToLuaMulticastDelegateInObject(UObject* target, const FString& luaDelegateKeyName, const FLuaDelegate& delegate, bool createOnTargetIfNotFound);
	
	UFUNCTION(BlueprintCallable, CustomThunk, meta=(ExpandEnumAsExecs="ReturnValue", Variadic, AutoCreateRefTerm=luaDelegateKeyName, BlueprintInternalUseOnly="true", AdvancedDisplay=1, Keywords="Broadcast, Execute, Delegate, Lua"))
	static bool BroadcastLuaDelegateInObject(UObject* target, const FString& luaDelegateKeyName, int32 numArguments);
	DECLARE_FUNCTION(execBroadcastLuaDelegateInObject);
	
	/*
	 *	Debug
	 */
	
	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	static void SetDebugWatchedActor(AActor* watchedActor);
	
	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	static void ClearDebugWatchedActor();
	
	/*
	 *	Structs
	 */
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool IsWeakStructValid(FWeakStructView view);
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool IsWeakStructOfType(FWeakStructView view, UScriptStruct* scriptStructType);
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool GetWeakStructType(FWeakStructView view, UScriptStruct*& outScriptStructType);
	
	/*
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, meta=(Variadic, DefaultToSelf=target, BlueprintInternalUseOnly="true", Keywords = "static, class, sparse"), DisplayName= "Set Static Class Data Value")
	static bool SetStaticClassDataValue(UObject* target, FName propertyName);
	DECLARE_FUNCTION(execSetStaticClassDataValue);
	
	UFUNCTION(BlueprintCallable, CustomThunk, meta=(DefaultToSelf=target, BlueprintInternalUseOnly="true", CustomStructureParam="value", Keywords = "static, class, sparse"), DisplayName= "Set Static Class Data Value")
	static bool SetStaticClassDataValueByClass(UClass* targetClass, FName propertyName, const int32& value);
	DECLARE_FUNCTION(execSetStaticClassDataValueByClass);
	
	
	/*
	 *	Misc
	 */
	
	
	
	UFUNCTION(BlueprintCallable, meta=(ExpandBoolAsExecs="ReturnValue", DefaultToSelf=target))
	static bool GetLuaContextFromWorldContext(UObject* worldContext, TScriptInterface<ILuaContext>& outLuaContext);

	UFUNCTION(BlueprintCallable, CustomThunk,meta=(Variadic, BlueprintInternalUseOnly="true", AdvancedDisplay=3))
	static void LuaRPC(UObject* target, const FString& string, const int32 numArguments);
	DECLARE_FUNCTION(execLuaRPC);
	static void LuaRPC_Internal(UObject* target, const FString& string, const TArray<FLuaValue>& args);
	
	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	static int64 BindKeyEvent(FKey key, FLuaFunctionHandle function, FString eventID);
	
	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	static void UnbindKeyEvent(FKey key, FString eventID);
	
	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	static void UnbindKeyEventByHandle(int64 handle);
};
