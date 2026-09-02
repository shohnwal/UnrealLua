 // Fill out your copyright notice in the Description page of Project Settings.


#include "BlueprintSupport/UnrealLuaUtility.h"

#if WITH_EDITOR
#include "LuaContext/EditorLuaContext.h"
#include "Editor.h"
#endif
#include "LuaContext/ScopedLuaContext.h"
#include "Interface/LuaContext.h"
#include "LuaContext/GameLuaContext.h"
#include "Replication/LuaScriptReplicationComponent.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"
#include "LuaValue/LuaTable.h"
#include "Reflection/MetaCache/LuaMetaCache.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "Engine/GameInstance.h"
#include "SubSystem/EditorLuaContextWorldSubsystem.h"
#include "UnrealLua.h"
#include "sol/sol.hpp"
#include "Debug/UnrealLuaDebug.h"
#include "Input/LuaStateInputHandler.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Engine.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"
 FOnLuaScriptUpdateMulticastDelegate UUnrealLuaUtility::OnLuaScriptChanged = {};

void UUnrealLuaUtility::CreateUnrealLuaMetadata()
{
	UUnrealLuaEngineSubsystem* ss = GEngine->GetEngineSubsystem<UUnrealLuaEngineSubsystem>();
	if(ss)
	{
		ss->CreateUnrealLuaMetadata();	
	}
}

void UUnrealLuaUtility::SetObjectLuaScriptValue(UObject* target, const FString& key, const int32& input, bool broadcastNotify)
{
	
}

DEFINE_FUNCTION(UUnrealLuaUtility::execSetObjectLuaScriptValue)
{
	UObject* obj = nullptr;
	Stack.StepCompiledIn<FObjectProperty>(&obj);
	//get input prop
	FString key;
	Stack.StepCompiledIn<FStrProperty>(&key);
	
	Stack.Step(Stack.Object, nullptr);
	FProperty* inputValueProp = Stack.MostRecentProperty; 
	void* inputValueContainer = Stack.MostRecentPropertyContainer;

	bool bBroadcastNotify = false;
	Stack.StepCompiledIn<FBoolProperty>(&bBroadcastNotify);
	
	//UObject* obj = Stack.Object;
	P_FINISH;

	P_NATIVE_BEGIN;

	if(obj && !key.IsEmpty())
	{
		auto casted = StringCast<char>(*key);
		const char* keyStr = casted.Get();

		bool success = true;
		void* sourceValueAddress = inputValueProp->ContainerPtrToValuePtr<void>(inputValueContainer);

		FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(obj);
		
		item.SetScriptValue(keyStr, inputValueProp, sourceValueAddress, bBroadcastNotify);
		
		*(bool*)RESULT_PARAM = success;
		return;
	}
	*(bool*)RESULT_PARAM = false;
	P_NATIVE_END;
}

ELuaCallResult UUnrealLuaUtility::GetObjectLuaScriptValue(UObject* target, const FString& key, int32& outValue)
{
	return ELuaCallResult::Failure;
}

DEFINE_FUNCTION(UUnrealLuaUtility::execGetObjectLuaScriptValue)
{
	UObject* obj = nullptr;
	Stack.StepCompiledIn<FObjectProperty>(&obj);
	//get input prop
	FString key;
	Stack.StepCompiledIn<FStrProperty>(&key);

	Stack.Step(Stack.Object, nullptr);
	FProperty* inputValueProp = Stack.MostRecentProperty; 
	void* inputValueContainer = Stack.MostRecentPropertyContainer;

	if(!obj || key.IsEmpty())
	{
		*static_cast<ELuaCallResult*>(RESULT_PARAM) = ELuaCallResult::Failure;
		return;
	}

	FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(obj);

	FLuaScriptValue* val = item.GetLuaScriptValue(*key);
	if(!val)
	{
		*static_cast<ELuaCallResult*>(RESULT_PARAM) = ELuaCallResult::Failure;
		return;
	}
	auto castedStr = StringCast<char>(*key);
	
	ELuaCallResult result = static_cast<ELuaCallResult>(item.GetScriptValue(castedStr.Get(), inputValueProp, inputValueProp->ContainerPtrToValuePtr<void>(inputValueContainer)));
	*static_cast<ELuaCallResult*>(RESULT_PARAM) = result;
}

ELuaCallResult UUnrealLuaUtility::SetObjectLuaScriptFunctionFromString(UObject* target, const FString& key, const FString& functionString)
{
	if(!target)
	{
		return ELuaCallResult::Failure;
	}
	if(key.IsEmpty())
	{
		return ELuaCallResult::Failure;
	}
	TScriptInterface<ILuaContext> ictx = GetLuaContext(target);

	if(!ictx)
	{
		return ELuaCallResult::Failure;
	}

	FString funcString = functionString;

	FLuaUObjectItem& handle = UnrealLua::UObjectRegistry::GetUObjectItem(target);
	
	auto casted = StringCast<char>(*key);
	sol::string_view keyStr = casted.Get();
	
	if(funcString.IsEmpty())
    {
    	handle.SetScriptValue(keyStr, sol::nil);
    	return ELuaCallResult::Failure;		
    }

	if(!funcString.StartsWith("return "))
	{
		funcString = TEXT("return ") + functionString;
	}
	
	FScopedLuaContext& ctx = ictx->GetScopedLuaContext();
	sol::state_view lua = ctx.GetLuaState();
	
	std::string scriptString{StringCast<char, 1024u>(*funcString).Get()};

	sol::load_result result = lua.load(scriptString);

	if(!result.valid())
	{
		return ELuaCallResult::Failure;
	}
	sol::protected_function value = result().get<sol::protected_function>();
	if(!value.valid())
	{
		return ELuaCallResult::Failure;
	}
	
	handle.SetScriptValue(keyStr, value);
	return ELuaCallResult::Success;
}

ELuaCallResult UUnrealLuaUtility::SetObjectLuaScriptFunctionFromStringSplit(UObject* target, const FString& key, const FString& functionArgsString, const FString& functionBodyString)
{
	FWideStringBuilderBase stream{};
	stream << "return function(";
	stream << functionArgsString;
	stream << ")\n";
	stream << functionBodyString;
	stream << "\nend";
	return SetObjectLuaScriptFunctionFromString(target, key, stream.GetData());
}

void UUnrealLuaUtility::BindEventToObjectLuaScriptValueChanged(UObject* target, const FString& key, const FOnLuaScriptValueChangedDelegate& event)
{
	if(!IsValid(target) || !event.IsBound())
	{
		return;
	}
	FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(target);

	auto str = StringCast<char>(*key);

	FLuaScriptValue* val = item.GetLuaScriptValueOrCreateEmpty(str.Get());
	val->AddOnValueChangedDelegate(event);
}

void UUnrealLuaUtility::UnbindEventFromObjectLuaScriptValueChanged(UObject* target, const FString& key, const FOnLuaScriptValueChangedDelegate& event)
{
	if(!IsValid(target) || !event.IsBound())
	{
		return;
	}
	FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(target);

	FLuaScriptValue* val = item.GetLuaScriptValue(*key);
	if(val)
	{
		val->RemoveOnValueChangedDynamicListener(event);
	}
}

void UUnrealLuaUtility::BroadcastObjectLuaScriptValue(UObject* target, const FString& key)
{
	if(!IsValid(target))
	{
		return;
	}
	FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(target);

	auto str = StringCast<char>(*key);
	
	FLuaScriptValue* val = item.GetLuaScriptValue(str.Get());
	if(val)
	{
		val->BroadcastValue();
	}
}
bool UUnrealLuaUtility::LoadLuaScriptForObject(UObject* luaScriptableObject, bool bForceReload)
{
	return UnrealLua::UObjectRegistry::LoadLuaScript(luaScriptableObject, bForceReload);
}

bool UUnrealLuaUtility::AddReplicatedSubobject(UObject* parentObject, UObject* subObject, ELifetimeCondition condition)
{
	if(!IsValid(parentObject) || !IsValid(subObject))
	{
		return false;
	}
	if(AActor* meActor = Cast<AActor>(parentObject))
	{
		meActor->AddReplicatedSubObject(subObject, condition);
		return true;
	}
	else if(UActorComponent* meCmp = Cast<UActorComponent>(parentObject))
	{
		meCmp->AddReplicatedSubObject(subObject, condition);
		return true;
	}
	return false;
}

bool UUnrealLuaUtility::RemoveReplicatedSubobject(UObject* parentObject, UObject* subObject)
{
	if(!IsValid(parentObject) || !IsValid(subObject))
	{
		return false;
	}
	if(AActor* meActor = Cast<AActor>(parentObject))
	{
		meActor->RemoveReplicatedSubObject(subObject);
		return true;
	}
	else if(UActorComponent* meCmp = Cast<UActorComponent>(parentObject))
	{
		meCmp->RemoveReplicatedSubObject(subObject);
		return true;
	}
	return false;
}

TScriptInterface<ILuaContext> UUnrealLuaUtility::GetLuaContext(const UObject* worldContext, ELuaStateType luaStateType)
{
	if(!worldContext)
	{
		return nullptr;
	}
	//perhaps the object itself is a lua local context
	if(worldContext->Implements<ULuaContext>())
	{
		return {const_cast<UObject*>(worldContext)};
	}
	UWorld* world = worldContext->GetWorld();
	if(!world)
	{
		//Hack for Lua self-test UObjects
		UObject* outer = worldContext->GetOuter();
		if(outer && outer->Implements<ULuaContext>())
		{
			return outer;
		}
		return nullptr;
	}
	if(world->IsGameWorld())
	{
		return world->GetGameInstance()->GetSubsystem<UGameLuaContext>();
	}
#if WITH_EDITOR
	else if(world->IsEditorWorld())
	{
		return world->GetSubsystem<UEditorLuaContextWorldSubsystem>();
	}
#endif
	return nullptr;
}

void UUnrealLuaUtility::BindEventToLuaScriptUpdate(const FOnLuaScriptUpdateDelegate& del)
{
	OnLuaScriptChanged.Add(del);
}

void UUnrealLuaUtility::BroadcastLuaScriptUpdate(const TScriptInterface<ILuaScriptable>& luaScriptable, const TMap<FString, FString>& scriptMap)
{
	TArray<FString> values;
	scriptMap.GenerateValueArray(values);
	TArray<FString> keys;
	scriptMap.GenerateKeyArray(keys);
	OnLuaScriptChanged.Broadcast(luaScriptable, keys, values);
}

void UUnrealLuaUtility::ScanForLuaClasses()
{

}


 void GetNumLuaValuesAsArray(int32 num, UObject* Context, FFrame& Stack, RESULT_DECL)
{
	TArray<FLuaValue> outArr{};
	
	while(num > 0)
	{
		num--;
		
		Stack.Step(Stack.Object, nullptr);
		FProperty* inputValueProp = Stack.MostRecentProperty; 
		void* inputValueContainer = Stack.MostRecentPropertyContainer;
		outArr.Emplace(FLuaValue{inputValueProp, inputValueProp->ContainerPtrToValuePtr<void>(inputValueContainer)});		
	}

	P_FINISH;
	
	*(TArray<FLuaValue>*)RESULT_PARAM = outArr;
}

bool UUnrealLuaUtility::MakeLuaTable(UObject* worldContext, FLuaTableHandle& outNewTable)
{
	outNewTable = {};
	TScriptInterface<ILuaContext> ctx{};
	if(!GetLuaContextFromWorldContext(worldContext, ctx))
	{
		return false;
	}
	if(!ctx->GetScopedLuaContext().IsLuaLoaded())
	{
		return false;
	}
	FLuaTableHandle newTable = ctx->GetScopedLuaContext().CreateNewLuaTable();
	if (!newTable.IsValid())
	{
		return false;
	}
	outNewTable = newTable;
	return outNewTable.IsValid();
}

bool UUnrealLuaUtility::SetValueInLuaTable(const FLuaTableHandle& table, const int32& key, const int32& value)
{
	checkNoEntry();
	return false;
}

DEFINE_FUNCTION(UUnrealLuaUtility::execSetValueInLuaTable)
{
	//get input prop
	FLuaTableHandle table{};
	Stack.StepCompiledInRef<FStructProperty, FLuaTableHandle>(&table);
	
	Stack.Step(Stack.Object, nullptr);
	FProperty* keyProp = Stack.MostRecentProperty; 
	void* keyValue = Stack.MostRecentPropertyContainer;
	
	Stack.Step(Stack.Object, nullptr);
	FProperty* valueProp = Stack.MostRecentProperty; 
	void* valueValue = Stack.MostRecentPropertyContainer;
	P_FINISH;

	P_NATIVE_BEGIN;
	
	if (!table.IsValid())
	{
		*(bool*)RESULT_PARAM = false;
		return;
	}
	FLuaValue key{keyProp, keyProp->ContainerPtrToValuePtr<void>(keyValue)};
	FLuaValue value{valueProp, valueProp->ContainerPtrToValuePtr<void>(valueValue)};
	
	table.NewIndex(key, value);
	P_NATIVE_END
	*(bool*)RESULT_PARAM = true;
	
}

bool UUnrealLuaUtility::GetValueFromLuaTable(const FLuaTableHandle& table, const int32& key, FLuaValue& outValue)
{
	checkNoEntry();
	return false;
}

DEFINE_FUNCTION(UUnrealLuaUtility::execGetValueFromLuaTable)
{
	//get input prop
	FLuaTableHandle tempTable{};
	FLuaTableHandle& tableRef = Stack.StepCompiledInRef<FStructProperty, FLuaTableHandle>(&tempTable);
	
	Stack.Step(Stack.Object, nullptr);
	FProperty* keyProp = Stack.MostRecentProperty; 
	void* keyValue = Stack.MostRecentPropertyContainer;
	
	FLuaValue tempValue{};
	FLuaValue& valueRef = Stack.StepCompiledInRef<FStructProperty, FLuaValue>(&tempValue);
	P_FINISH;

	P_NATIVE_BEGIN;
	
	if (!tableRef.IsValid())
	{
		*(bool*)RESULT_PARAM = false;
		return;
	}
	FLuaValue key{keyProp, keyProp->ContainerPtrToValuePtr<void>(keyValue)};
	
	valueRef = tableRef.Index(key);
	valueRef.ClearIsScriptValue();
	valueRef.ConvertLuaObjectsToHandles();
	*(bool*)RESULT_PARAM = true;
	P_NATIVE_END
}

bool UUnrealLuaUtility::ClearLuaTableKey(const FLuaTableHandle& table, const int32& key)
{
	checkNoEntry();
	return false;
}

DEFINE_FUNCTION(UUnrealLuaUtility::execClearLuaTableKey)
{
	//get input prop
    FLuaTableHandle table{};
    Stack.StepCompiledInRef<FStructProperty, FLuaTableHandle>(&table);
    
    Stack.Step(Stack.Object, nullptr);
    FProperty* keyProp = Stack.MostRecentProperty; 
    void* keyValue = Stack.MostRecentPropertyContainer;
    
    P_FINISH;

    P_NATIVE_BEGIN;
    
    if (!table.IsValid())
    {
    	*(bool*)RESULT_PARAM = false;
    	return;
    }
    FLuaValue key{keyProp, keyProp->ContainerPtrToValuePtr<void>(keyValue)};
    
    table.NewIndex(key, sol::nil);
    P_NATIVE_END
    *(bool*)RESULT_PARAM = true;
}

bool UUnrealLuaUtility::ClearLuaTable(const FLuaTableHandle& table)
{
	checkNoEntry();
	return false;
}

DEFINE_FUNCTION(UUnrealLuaUtility::execClearLuaTable)
{
	//get input prop
	FLuaTableHandle table{};
	Stack.StepCompiledInRef<FStructProperty, FLuaTableHandle>(&table);
	P_FINISH;

	P_NATIVE_BEGIN;
    
	if (!table.IsValid())
	{
		*(bool*)RESULT_PARAM = false;
		return;
	}
	
	table.GetTable().clear();
	P_NATIVE_END
	*(bool*)RESULT_PARAM = true;
}


 void UUnrealLuaUtility::SetDebugWatchedActor(AActor* watchedActor)
 {
	UUnrealLuaEngineSubsystem* ss = UUnrealLuaEngineSubsystem::Get();
	if (!ss)
	{
		return;
	}
	UUnrealLuaDebug* debug = ss->GetUnrealLuaDebug();
	if (!debug)
	{
		return;
	}
	debug->SetWatchedActor(watchedActor);
 }

 void UUnrealLuaUtility::ClearDebugWatchedActor()
 {
	UUnrealLuaEngineSubsystem* ss = UUnrealLuaEngineSubsystem::Get();
	if (!ss)
	{
		return;
	}
	UUnrealLuaDebug* debug = ss->GetUnrealLuaDebug();
	if (!debug)
	{
		return;
	}
	debug->SetWatchedActor(nullptr);
 }

 bool UUnrealLuaUtility::IsWeakStructValid(FWeakStructView view)
 {
	return view.IsValid();
 }

 bool UUnrealLuaUtility::IsWeakStructOfType(FWeakStructView view, UScriptStruct* scriptStructType)
 {
	return view.GetScriptStruct() == scriptStructType;
 }

 bool UUnrealLuaUtility::GetWeakStructType(FWeakStructView view, UScriptStruct*& outScriptStructType)
 {
	if (view.IsValid())
	{
		outScriptStructType = view.GetScriptStruct();
		return true;
	}
	return false;
 }


 bool UUnrealLuaUtility::GetLuaContextFromWorldContext(UObject* worldContext, TScriptInterface<ILuaContext>& outLuaContext)
{
	UWorld* world = worldContext->GetWorld();
	if(!world)
	{
		outLuaContext = {};
		return false;
	}
	if(world->IsGameWorld())
	{
		UGameLuaContext* ctx = UGameLuaContext::Get(world);
		if (ctx)
		{
			outLuaContext = ctx;
			return true;
		}
		return false;
	}
	else if (world->IsPreviewWorld())
	{
		UEditorLuaContextWorldSubsystem* ess = world->GetSubsystem<UEditorLuaContextWorldSubsystem>();
		if(ess)
		{
			outLuaContext = ess;
			return true;
		}
		return false;
	}
	outLuaContext = {};
	return false;
}

ELuaCoroutineCallStatus UUnrealLuaUtility::GetLuaCoroutineStatus(const FLuaCoroutineHandle& coroutine)
{
	return coroutine.GetCoroutineStatus();
}

void UUnrealLuaUtility::LuaRPC(UObject* target, const FString& string, const int32 numArgs)
{
	//@TODO : make this a Variadic function
	checkNoEntry();
}

DEFINE_FUNCTION(UUnrealLuaUtility::execLuaRPC)
{
	UObject* target;
	Stack.StepCompiledIn<FObjectProperty>(&target);
	
	//get func name
	FString funcName;
	Stack.StepCompiledIn<FStrProperty>(&funcName);
	
	int32 numArguments = 0;
	Stack.StepCompiledIn<FIntProperty>(&numArguments);
	
	TArray<FLuaValue> args;
	
	TArray<FLuaValue> result;
	for (int32 argIndex = 0; argIndex < numArguments; ++argIndex)
	{
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.MostRecentProperty = nullptr;
		Stack.StepCompiledIn<FProperty>(nullptr);
		check(Stack.MostRecentProperty && Stack.MostRecentPropertyAddress);

		FProperty* p = CastField<FProperty>(Stack.MostRecentProperty);

		const void* propertyValuePtr = p->ContainerPtrToValuePtr<const void*>(Stack.MostRecentPropertyContainer);
		
		args.Emplace(p, propertyValuePtr);
	}
	
	P_FINISH;
	UUnrealLuaUtility::LuaRPC_Internal(target, funcName, args);
}

 void UUnrealLuaUtility::LuaRPC_Internal(UObject* target, const FString& string, const TArray<FLuaValue>& args)
 {
	if(!IsValid(target))
	{
		LUA_LOG_WARNING("Unable to call UUnrealLuaUtilityBlueprintFunctionLibrary::LuaRPC on target %s : object is not valid", *GetFullNameSafe(target))
		return;
	}
	AActor* actor = Cast<AActor>(target);
	if(!actor)
	{
		actor = target->GetTypedOuter<AActor>();
	}
	if(!actor)
	{
		LUA_LOG_WARNING("Unable to call UUnrealLuaUtilityBlueprintFunctionLibrary::LuaRPC on target %s : No outer actor found", *GetFullNameSafe(target))
		return;
	}
	ULuaScriptReplicationComponent* cmp = actor->GetComponentByClass<ULuaScriptReplicationComponent>();;
	if(!cmp)
	{
		LUA_LOG_WARNING("Unable to call UUnrealLuaUtilityBlueprintFunctionLibrary::LuaRPC on target %s : actor %s has no ULuaScriptReplicationComponent", *GetFullNameSafe(target), *GetNameSafe(actor))
		return;
	}
	cmp->LuaRPC(target, string, args);
 }

 int64 UUnrealLuaUtility::BindKeyEvent(FKey key, FLuaFunctionHandle function, FString eventID)
 {
	if (!key.IsValid() || !function.IsValid())
	{
		return 0;
	}
	
	FScopedLuaContext* ctx = FScopedLuaContext::GetLuaContextFromLuaState(function.GetFunction().lua_state());
	if (!ctx)
	{
		return 0;
	}
	ULuaStateInputHandler* inputHandler =  ctx->GetInputHandler();
	if (!inputHandler)
	{
		return 0;
	}
	return inputHandler->BindKeyEvent(key, function, eventID);
 }

 void UUnrealLuaUtility::UnbindKeyEvent(FKey key, FString eventID)
 {
	
 }

 void UUnrealLuaUtility::UnbindKeyEventByHandle(int64 handle)
 {
	
 }

 TMap<FString, FString> UUnrealLuaUtility::LuaScriptToString(const TScriptInterface<ILuaScriptable>& target)
{
	if(target)
	{
		return UnrealLua::UObjectRegistry::GetLuaScriptHandle(target.GetObject()).LuaScriptToString();
	}
	return {};
}

void UUnrealLuaUtility::SetLuaValueInObject(UObject* object, const FString& key, const int32& input)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UUnrealLuaUtility::execSetLuaValueInObject)
{
	UObject* object = nullptr;
	Stack.Step(Stack.Object, object);
	FString key;
	Stack.Step(Stack.Object, &key);
	//get input prop
	Stack.Step(Stack.Object, nullptr);
	FProperty* inputValueProp = Stack.MostRecentProperty; 
	void* inputValueContainer = Stack.MostRecentPropertyContainer;
	P_FINISH;

	P_NATIVE_BEGIN;
	if (!object || key.IsEmpty())
	{
		return;
	}
	FLuaValue value = FLuaValue{inputValueProp, inputValueProp->ContainerPtrToValuePtr<void>(inputValueContainer)};
	FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(object);
	item.SetScriptValue(GetData(key), value, true);
	P_NATIVE_END;
}

void UUnrealLuaUtility::ClearLuaValueInObject(UObject* object, const FString& key)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UUnrealLuaUtility::execClearLuaValueInObject)
{
	UObject* object = nullptr;
	Stack.Step(Stack.Object, object);

	FString key;
	Stack.Step(Stack.Object, &key);
	P_FINISH;
	P_NATIVE_BEGIN;
	if (!object || key.IsEmpty())
	{
		return;
	}
	FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(object);
	item.SetScriptValue(GetData(key), sol::nil, true);
	P_NATIVE_END;
}