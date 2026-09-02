// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaValue/LuaScriptValue.h"

#include "LuaValue/LuaValue.h"
#include "BlueprintSupport/UnrealLuaUtility.h"
#include "UnrealLua.h"
#include "sol/sol.hpp"
#include "LuaValue/LuaValueType.h"
#include "StringHandling/UnrealLuaStringCache.h"

bool FLuaScriptValueKey::Matches(const FUnrealLuaNameEntryKey& key) const
{
	return this->KeyNameEntry == key.Entry;
}

bool FLuaScriptValueKey::Matches(std::string_view& key, uint32 hash) const
{
	return CachedHash == hash && KeyNameEntry->Matches(key, hash);
}

bool FLuaScriptValueKey::Matches(FStringView& key, uint32 hash) const
{
	return CachedHash == hash && KeyNameEntry->Matches(key, hash);
}

uint32 FLuaScriptValueKey::GetKeyHash() const
{
	return this->CachedHash;
}

std::string_view FLuaScriptValueKey::GetKeyName() const
{
	verify(this->KeyNameEntry != nullptr);
	return this->KeyNameEntry->String.data();
}

FString FLuaScriptValueKey::GetKeyNameString() const
{
	return this->KeyNameEntry ? this->KeyNameEntry->UnrealString : "";
}

FName FLuaScriptValueKey::GetFName() const
{
	return this->KeyNameEntry ? this->KeyNameEntry->GetFName() : NAME_None;
}

ESetLuaValueResult FLuaScriptValue::ChangeToPropertyReference(const FSetLuaScriptUObjectMemberPropertyWrapperParams& params)
{
	ESetLuaValueResult result = ESetLuaValueResult::Success;
	//save old Lua value, if any, and apply it to new references, if possible
	if(params.PropMapping.IsFunction())
	{
		sol::function previousFunc{sol::nil};
		if(this->IsType<sol::function>())
		{
			previousFunc = this->Get<sol::function>();
		}
		else if (this->IsType<FLuaFunctionHandle>())
		{
			previousFunc = this->Get<FLuaFunctionHandle>().GetFunction();
		}
		this->Value = FLuaValue{params};
		verify(this->Value.IsType<FLuaUFunctionReference>());
		//this sets the sol::function into the reference
		result |= this->Value.SetValue(previousFunc, "");
	}
	else
	{		
		verify(params.PropMapping.IsProperty())
		if(!this->Value.IsType<FPropertyReferenceWrapper>())
		{
			FLuaValue oldData = MoveTemp(this->Value);
			new (&this->Value) FLuaValue{params};
			this->Value.SetValue(oldData);
			UnrealLua::PropertyHelper::HandleSetPropertyNetBehavior(params.ScriptOwner, params.PropMapping.GetProperty());
		}
		else
		{
			//ScriptValue is already a FProperty, make sure it's actually pointing to the same
			verify(this->Value.Get<FPropertyReferenceWrapper>().Prop == params.PropMapping.GetProperty())
		}
		return result;
	}
	this->Value.MarkAsScriptValue();
	return result;
}

void FLuaScriptValue::AddStructReferencedObjects(FReferenceCollector& collector)
{
	if(this->Value.AddStructReferencedObjects(collector))
	{
		//@TODO mark script value in need for broadcasting after garbage collection?
	}
}


FDelegateHandle FLuaScriptValue::AddOnValueChangedDelegate(const FOnLuaScriptValueChangedNativeDelegate& del)
{
	if(del.IsBound())
	{
		this->CreateBroadcastMulticastDelegate();
		return OnValueChanged->Add(del);
	}	
	return {};
}

FDelegateHandle FLuaScriptValue::AddOnValueChangedDelegate(FOnLuaScriptValueChangedDelegate del)
{
	if(del.IsBound())
	{
		this->CreateBroadcastMulticastDelegate();
		UObject* obj = del.GetUObject();
		FName funcName = del.GetFunctionName();
		//this->OnValueChanged->RemoveAll(obj);
		return OnValueChanged->AddUFunction(obj, funcName);
	}
	return {};
}

void FLuaScriptValue::RemoveOnValueChangedDynamicListener(FOnLuaScriptValueChangedDelegate del)
{
	if(del.IsBound())
	{
		if(this->OnValueChanged.IsValid())
		{
			UObject* obj = del.GetUObject();
			OnValueChanged->RemoveAll(obj);
			this->RemoveBroadcastMulticastDelegateIfEmpty();
		}		
	}
}

void FLuaScriptValue::RemoveOnValueChangedByHandle(FDelegateHandle delHandle)
{
	if(this->OnValueChanged.IsValid())
	{
		OnValueChanged->Remove(delHandle);
		this->RemoveBroadcastMulticastDelegateIfEmpty();
	}		
}

uint64 FLuaScriptValue::AddOnValueChangedLuaScriptListener(UObject* listener, const std::string_view callbackStrv/*, sol::variadic_args additionalCallbackArgs_stack*/)
{
	this->CreateBroadcastMulticastDelegate();
	//OnValueChanged->RemoveAll(listener);
	FDelegateHandle handle;
	/*
	if(additionalCallbackArgs_stack.size() > 0)
	{
		TArray<sol::object> additionalCallbackArgs;
		for(auto arg : additionalCallbackArgs_stack)
		{
			additionalCallbackArgs.Emplace(arg);
		}
		handle = OnValueChanged->AddWeakLambda(listener, [listener, callbackStr, additionalCallbackArgs](FLuaValue value)
		{
			UnrealLua::LuaScriptCall::CallLuaFunctionSafeByName(listener, callbackStr.data(), listener, MoveTemp(value), sol::as_args(additionalCallbackArgs));
		});		
	}
	else
	{*/
	std::string callbackStr{callbackStrv};
	handle = OnValueChanged->AddWeakLambda(listener, [listener, callbackStr](FLuaValue value)
	{
		UnrealLua::LuaScriptCall::CallLuaFunctionSafeByName(listener, callbackStr.data(), listener, MoveTemp(value));
	});
	//}

	return *reinterpret_cast<uint64*>(&handle);
}

void FLuaScriptValue::RemoveLuaScriptListener(void* listener)
{
	if(this->OnValueChanged != nullptr)
	{
		OnValueChanged->RemoveAll(listener);
		this->RemoveBroadcastMulticastDelegateIfEmpty();
	}	
}

void FLuaScriptValue::RemoveLuaScriptListener(UObject* listener)
{
	if(this->OnValueChanged != nullptr)
	{
		OnValueChanged->RemoveAll(listener);
		this->RemoveBroadcastMulticastDelegateIfEmpty();
	}
}

bool FLuaScriptValue::RemoveLuaScriptListener(uint64 handleID)
{
	bool removed = false;
	if(this->OnValueChanged != nullptr)
	{
		FDelegateHandle handle;
		*reinterpret_cast<uint64*>(&handle) = handleID;
		removed = OnValueChanged->Remove(handle);
		this->RemoveBroadcastMulticastDelegateIfEmpty();
	}
	return removed;
}


ELuaValueType FLuaScriptValue::GetType() const
{
	return this->Value.GetType();
}



//Lua values without listeners and with nil values can be removed
bool FLuaScriptValue::ShouldBeRemoved() const
{
	return !this->IsNetProperty() && this->OnValueChanged == nullptr && (this->Value.IsType<std::nullptr_t>() || this->Value.IsType<sol::nil_t>());
}
void FLuaScriptValue::SetKey(std::string_view strv)
{
	this->SetKey(UnrealLua::StringCache::GetStringEntryKey(strv));
}

void FLuaScriptValue::SetKey(const FUnrealLuaNameEntryKey& key)
{
	//make sure key is only set once!
	verify(this->Key.KeyNameEntry == nullptr);
	this->Key = key;
	verify(this->Key.KeyNameEntry != nullptr);
	this->Value.MarkAsScriptValue();
}

bool FLuaScriptValue::KeyMatches(std::string_view& key, uint32 hash) const
{
	return this->Key.Matches(key, hash);
}

bool FLuaScriptValue::KeyMatches(FStringView& key, uint32 hash) const
{
	return this->Key.Matches(key, hash);
}

bool FLuaScriptValue::KeyMatches(const FUnrealLuaNameEntryKey& key) const
{
	return this->Key.Matches(key);
}

void FLuaScriptValue::PostGCHandleUObjectPtrs()
{
	if(this->Value.PostGCHandleUObjectPtrs())
	{
		//LUA_LOG("FLuaScriptValue::PostGCHandleUObjectPtrs : Item %hs had invalid UObject, broadcasting nil", this->GetKeyName().data())
	}
}

void FLuaScriptValue::CleanUpForLuaState(sol::this_state lua)
{
	this->Value.CleanUpForLuaState(lua);
}

FLuaDelegateHandle FLuaScriptValue::AddDelegateListener(const FLuaDelegate& delToAdd)
{
	return this->Value.AddDelegateListener(delToAdd);
}

FLuaDelegateHandle FLuaScriptValue::AddMulticastDelegateListener(const FLuaDelegate& delToAdd)
{
	return this->Value.AddMulticastDelegateListener(delToAdd);
}

bool FLuaScriptValue::UnbindMulticastDelegateListener(const FLuaDelegate& delToRemove)
{
	return this->Value.UnbindMulticastDelegateListener(delToRemove);
}

bool FLuaScriptValue::UnbindMulticastDelegateListener(FLuaDelegateHandle handle)
{
	return this->Value.UnbindMulticastDelegateListener(handle);
}

bool FLuaScriptValue::BroadcastLuaDelegate(const TArray<FLuaValue>& args)
{
	return this->Value.BroadcastLuaDelegate(args);
}

bool FLuaScriptValue::IsDead() const
{
	return this->Value.IsDead();
}

void FLuaScriptValue::RemoveBroadcastMulticastDelegate()
{
	//delete this->OnValueChanged;
	this->OnValueChanged = nullptr;
}
