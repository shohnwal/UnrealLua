#include "BlueprintSupport/UnrealLuaUtility.h"
#include "LuaTypes/LuaDelegate.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"

FLuaDelegateHandle UUnrealLuaUtility::BindEventToLuaDelegateInObject(UObject* target, const FString& luaDelegateKeyName, const FLuaDelegate& delegate, bool createOnTargetIfNotFound)
{
	if (!delegate.IsBound() || !target || luaDelegateKeyName.IsEmpty())
	{
		return {};
	}
	UUnrealLuaUObjectRegistry* registry = UUnrealLuaUObjectRegistry::Get();
	if (!registry)
	{
		return {};
	}
	FLuaUObjectItem& item = registry->GetUObjectItem(target);
	return item.BindEventToDelegate(luaDelegateKeyName, delegate, createOnTargetIfNotFound);
}

bool UUnrealLuaUtility::UnbindEventFomLuaDelegateInObject(UObject* target, const FString& luaDelegateKeyName, const FLuaDelegate& delegate)
{
	if (!delegate.IsBound() || !target || luaDelegateKeyName.IsEmpty())
	{
		return false;
	}
	UUnrealLuaUObjectRegistry* registry = UUnrealLuaUObjectRegistry::Get();
	if (!registry)
	{
		return false;
	}
	FLuaUObjectItem& item = registry->GetUObjectItem(target);
	return item.UnbindEventToDelegate(luaDelegateKeyName, delegate);
}

bool UUnrealLuaUtility::UnbindEventFomLuaDelegateInObjectWithHandle(UObject* target, const FString& luaDelegateKeyName, const FLuaDelegateHandle& handleToRemove)
{
	if (!handleToRemove.IsBound() || !target || luaDelegateKeyName.IsEmpty())
	{
		return false;
	}
	UUnrealLuaUObjectRegistry* registry = UUnrealLuaUObjectRegistry::Get();
	if (!registry)
	{
		return false;
	}
	FLuaUObjectItem& item = registry->GetUObjectItem(target);
	return item.UnbindEventToDelegate(luaDelegateKeyName, handleToRemove);
}

FLuaDelegateHandle UUnrealLuaUtility::BindEventToLuaMulticastDelegateInObject(UObject* target, const FString& luaDelegateKeyName, const FLuaDelegate& delegate, bool createOnTargetIfNotFound)
{
	if (!delegate.IsBound() || !target || luaDelegateKeyName.IsEmpty())
	{
		return {};
	}
	UUnrealLuaUObjectRegistry* registry = UUnrealLuaUObjectRegistry::Get();
	if (!registry)
	{
		return {};
	}
	FLuaUObjectItem& item = registry->GetUObjectItem(target);
	return item.BindEventToMulticastDelegate(luaDelegateKeyName, delegate, createOnTargetIfNotFound);
}

bool UUnrealLuaUtility::BroadcastLuaDelegateInObject(UObject* target, const FString& key, int32 numArgs)
{
	checkNoEntry();
	return false;
}

DEFINE_FUNCTION(UUnrealLuaUtility::execBroadcastLuaDelegateInObject)
{
	P_GET_OBJECT(UObject, target)
	
	P_GET_PROPERTY(FStrProperty, key)
	
	P_GET_PROPERTY(FIntProperty, numArgs) 
	
	TArray<FLuaValue> args;
	for (int32 argIndex = 0; argIndex < numArgs; ++argIndex)
	{
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.MostRecentProperty = nullptr;
		
		Stack.StepCompiledIn<FProperty>(nullptr);
		check(Stack.MostRecentProperty && Stack.MostRecentPropertyAddress);

		FProperty* p = CastField<FProperty>(Stack.MostRecentProperty);

		const void* propertyValuePtr = p->ContainerPtrToValuePtr<const void*>(Stack.MostRecentPropertyContainer);
		
		args.Emplace(p, propertyValuePtr);
	}
	
	if (!UUnrealLuaEngineSubsystem::IsGameSessionActive() || key.IsEmpty())
	{
		return;
	}
	
	UUnrealLuaUObjectRegistry* registry = UUnrealLuaUObjectRegistry::Get();
	if (!registry)
	{
		return;
	}
	FLuaUObjectItem& targetItem = registry->GetUObjectItem(target);
	
	bool success = targetItem.BroadcastLuaDelegate(key, args);
}
