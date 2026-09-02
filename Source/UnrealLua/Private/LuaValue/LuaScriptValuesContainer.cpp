#include "LuaValue/LuaScriptValuesContainer.h"

#include "LuaValue/LuaScriptValue.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Utility/UnrealLuaHash.h"

bool FLuaScriptValuesContainer::IsEmpty() const
{
	return this->LuaScriptValues.IsEmpty();
}

sol::object FLuaScriptValuesContainer::GetScriptValue(const std::string_view& key, sol::this_state luat)
{
	lua_State* lua = luat.lua_state();
	
	//FCPUCycleTimer timer{FString("PushValueFromUObjectProperty ") + strv.data()};
	
	//FCPUCycleTimer timer{FString("GetValueFromUObjectProperty ") + strv.data()};

	FLuaScriptValue* val = this->GetLuaScriptValue(key);
	
	if(val && val->HasInitializedValue())
	{
		//found a script value (got it from a sol::object or from a Wrapper in LuaScriptValue)
		return val->GetValue(lua);
	}
	else
	{
		if (UObject* owner = this->GetUObjectVirtual())
		{
			if(const FHashedFieldMapping* found = this->GetPropertyMapping(key))
			{
				//FPlatformMisc::Prefetch(item.Object);
				//regardless of lua world availability, we can always store stuff in the cache
				FSetLuaScriptUObjectMemberPropertyWrapperParams params{owner, *found};
				val = this->SetPropertyWrapperLuaScriptValue(params); 
				return val->GetValue(lua);
			}		
		}
	}

	return sol::nil;
}

void FLuaScriptValuesContainer::SetNetDirty()
{
	this->bIsScriptNetDirty = true;
#if WITH_PUSH_MODEL
    //@TODO : somehow mark object dirty
#endif
}

void FLuaScriptValuesContainer::ClearNetDirty()
{
	this->bIsScriptNetDirty = false;
}

bool FLuaScriptValuesContainer::IsNetDirty() const
{
	return this->bIsScriptNetDirty;
}

TArray<FLuaScriptValue>& FLuaScriptValuesContainer::GetLuaScriptValues()
{
	return this->LuaScriptValues;
}

bool FLuaScriptValuesContainer::GetScriptValue(const std::string_view& key, FProperty* targetProperty, void* targetMemAddress)
{
	FLuaScriptValue* val = this->GetLuaScriptValue(key);
	
	if(val && val->HasInitializedValue())
	{
		//found a script value (got it from a sol::object or from a Wrapper in LuaScriptValue)
		val->WriteValueToPropertyMemoryAddress(targetProperty, targetMemAddress);
		return true;
	}
	else
	{	
		if (UObject* owner = this->GetUObjectVirtual())
		{
			if(const FHashedFieldMapping* found = this->GetPropertyMapping(key))
			{
				//FPlatformMisc::Prefetch(item.Object);
				//regardless of lua world availability, we can always store stuff in the cache
				FSetLuaScriptUObjectMemberPropertyWrapperParams params{owner, *found};
				val = this->SetPropertyWrapperLuaScriptValue(params); 
				val->WriteValueToPropertyMemoryAddress(targetProperty, targetMemAddress);
				return true;
			}
		}
	}
	targetProperty->InitializeValue(targetMemAddress);
	return false;
}

FLuaScriptValue* FLuaScriptValuesContainer::GetLuaScriptValueInternal(std::string_view key) const
{
	uint32 hash = UnrealLua::HashUtility::StrCrc32(key.data());

	FLuaScriptValue* found = const_cast<FLuaScriptValue*>(this->LuaScriptValues.FindByPredicate([&key, hash](const FLuaScriptValue& item)
	{
		return item.KeyMatches(key, hash); 
	}));
	return found;
}

FLuaScriptValue* FLuaScriptValuesContainer::GetLuaScriptValueInternal(FStringView key) const
{
	uint32 hash = UnrealLua::HashUtility::StrCrc32(key.GetData());
    FLuaScriptValue* found = const_cast<FLuaScriptValue*>(this->LuaScriptValues.FindByPredicate([&key, hash](const FLuaScriptValue& item)
    {
    	return item.KeyMatches(key, hash); 
    }));
    return found;
}

int FLuaScriptValuesContainer::PushScriptValueInternal(const std::string_view& key, sol::this_state luat)
{
	lua_State* lua = luat.lua_state();

	//FScopeLock lock{&this->Lock};

	FLuaScriptValue* val = this->GetLuaScriptValue(key);
	
	if(val && val->HasInitializedValue())
	{
		//found a script value (got it from a sol::object or from a Wrapper in LuaScriptValue)
		return val->PushValue(lua);
	}
	else
	{
		if (UObject* owner = this->GetUObjectVirtual())
		{
			if(const FHashedFieldMapping* found = this->GetPropertyMapping(key))
			{
				//FPlatformMisc::Prefetch(item.Object);
				//regardless of lua world availability, we can always store stuff in the cache
				FSetLuaScriptUObjectMemberPropertyWrapperParams params{owner, *found};
				val = this->SetPropertyWrapperLuaScriptValue(params); 
				return val->PushValue(lua);
			}
			else
			{
				return UnrealLua::LightUserdata::TryPushMetaMethod(key, lua);	
			}
		}
	}

	return sol::stack::push(lua,sol::nil);
}

void FLuaScriptValuesContainer::ResetNonPropertyWrapperValuesButKeepListeners()
{
	for(FLuaScriptValue& luaVal : this->LuaScriptValues)
	{
		if(luaVal.IsPropertyOrUFunction())
		{
			continue;
		}
		FLuaValue temp{};
		std::string key = "";
		luaVal.SetScriptValue(temp);
	}
	this->SetNetDirty();
}

void FLuaScriptValuesContainer::EmptyAllLuaScriptValues()
{
	this->LuaScriptValues.Empty();
	this->SetNetDirty();
	this->OnNumberOfValuesChanged.Broadcast();
}

bool FLuaScriptValuesContainer::HasAnyLuaScriptValues() const
{
	return !this->LuaScriptValues.IsEmpty();
}

void FLuaScriptValuesContainer::CleanUpLuaScriptValuesForLuaState(lua_State* L)
{
	for(FLuaScriptValue& val : this->LuaScriptValues)
	{
		val.CleanUpForLuaState(L);
	}	
	this->SetNetDirty();
}

FLuaScriptValue* FLuaScriptValuesContainer::SetPropertyWrapperLuaScriptValue(const FSetLuaScriptUObjectMemberPropertyWrapperParams params)
{
	FLuaScriptValue* scriptValue = this->GetLuaScriptValueInternal(params.PropMapping.GetMappingFName().ToString());
	if(!scriptValue)
	{
		// no listeners yet -> create new value
		scriptValue = &this->LuaScriptValues.Emplace_GetRef(params);
		auto casted = StringCast<char>(*params.GetMappingFName().ToString()); 
		std::string_view strv = casted.Get();
		scriptValue->SetKey(strv);
		this->OnNumberOfValuesChanged.Broadcast();
		//Since params doesn't contain any Lua function, no need to check for update tick func mappings
	}
	else
	{
		ESetLuaValueResult result = scriptValue->ChangeToPropertyReference(params);
		if (EnumHasAllFlags(result, ESetLuaValueResult::TickFunctionModified))
		{
			this->UpdateTickFuncMapping(scriptValue);
		}
	}
	return scriptValue;
}

sol::function FLuaScriptValuesContainer::GetLuaScriptFunction(const char* key) const
{
	sol::string_view strv{key};
	FLuaScriptValue* value = this->GetLuaScriptValue(strv);
	if(!value)
	{
		return sol::nil;
	}
	return value->GetLuaScriptFunction();
}

sol::function FLuaScriptValuesContainer::GetLuaScriptFunction(const TCHAR* key) const
{
	FLuaScriptValue* value = this->GetLuaScriptValueInternal(key);
	if(!value)
	{
		return sol::nil;
	}
	return value->GetLuaScriptFunction();
}

sol::function FLuaScriptValuesContainer::GetLuaScriptFunction(const FString& key) const
{
	return this->GetLuaScriptFunction(*key);
}

sol::function* FLuaScriptValuesContainer::GetUFunctionOverrideLuaScriptFunction(const FName& key) const
{
	if (key == NAME_None)
	{
		return nullptr;
	}
	for(const FLuaScriptValue& val : this->LuaScriptValues)
	{
		if(val.IsType<FLuaUFunctionReference>())
		{
			FLuaUFunctionReference& ref = val.GetMutable<FLuaUFunctionReference>();
			if(ref.Func->Func->GetFName() == key)
			{
				if(ref.LuaFunc.valid())
				{
					return &ref.LuaFunc;
				}
				else
				{
					return nullptr;	
				}
			}
		}
	}
	return nullptr;
}


FLuaScriptValue* FLuaScriptValuesContainer::GetLuaScriptValueOrCreateEmpty(const std::string_view key, bool bCreateEvenIfNotExist)
{
	if(key.empty())
	{
		return nullptr;
	}
	FLuaScriptValue* scriptValue = this->GetLuaScriptValue(key);
	if(!scriptValue)
	{
		if (UObject* owner = this->GetUObjectVirtual())
		{
			if(const FHashedFieldMapping* mapping = GetPropertyMapping(key))
			{
				FSetLuaScriptUObjectMemberPropertyWrapperParams params{owner, *mapping};
				scriptValue = &this->LuaScriptValues.Emplace_GetRef(params);
				scriptValue->SetKey(key);
				this->OnNumberOfValuesChanged.Broadcast();
				return scriptValue;
			}
		}
		
		if(!bCreateEvenIfNotExist)
		{
			return nullptr;
		}
		scriptValue = &this->LuaScriptValues.Emplace_GetRef();
		scriptValue->SetKey(key);
		this->OnNumberOfValuesChanged.Broadcast();
	}
	return scriptValue;
}

FLuaScriptValue* FLuaScriptValuesContainer::GetLuaScriptValueOrCreateEmpty(const FUnrealLuaNameEntryKey& key, bool bCreateEvenIfNotExist)
{
	if(!key.IsValid())
	{
		return nullptr;
	}
	FLuaScriptValue* scriptValue = this->GetLuaScriptValue(key);
	if(!scriptValue)
	{
		if (UObject* owner = this->GetUObjectVirtual())
		{
			if(const FHashedFieldMapping* mapping = GetPropertyMapping(key.CachedHash))
			{
				FSetLuaScriptUObjectMemberPropertyWrapperParams params{owner, *mapping};
				scriptValue = &this->LuaScriptValues.Emplace_GetRef(params);
				scriptValue->SetKey(key);
				this->OnNumberOfValuesChanged.Broadcast();
				return scriptValue;
			}
		}
		
		if(!bCreateEvenIfNotExist)
		{
			return nullptr;
		}
		scriptValue = &this->LuaScriptValues.Emplace_GetRef();
		scriptValue->SetKey(key);
		this->OnNumberOfValuesChanged.Broadcast();
	}
	return scriptValue;
}

template<typename T> 
requires (std::is_same_v<std::remove_cvref_t<sol::object>,T> || std::is_same_v<std::remove_cvref_t<sol::stack_object>,T>)
void FLuaScriptValuesContainer::SetScriptValueInternal(const sol::string_view& key, const T& newValue, bool bCallNotify)
{
	FLuaScriptValue* scriptValue = this->GetLuaScriptValueOrCreateEmpty(key, newValue.valid());

	if(!scriptValue)
	{
		return;
	}
	
	ESetLuaValueResult setResult = ESetLuaValueResult::Success;
	
	if (scriptValue->IsNetProperty())
	{
		this->SetNetDirty();
	}

	setResult |= scriptValue->SetScriptValue(newValue, key);
	
	if(EnumHasAllFlags(setResult, ESetLuaValueResult::TickFunctionModified))
	{
		this->UpdateTickFuncMapping(scriptValue);
	}
	if(bCallNotify)
	{
		scriptValue->BroadcastValue();
	}
}

template void FLuaScriptValuesContainer::SetScriptValueInternal<sol::stack_object>(const sol::string_view& key, const sol::stack_object& newValue, bool bCallNotify);
template void FLuaScriptValuesContainer::SetScriptValueInternal<sol::object>(const sol::string_view& key, const sol::object& newValue, bool bCallNotify);


void FLuaScriptValuesContainer::SetScriptValueInternal(const sol::string_view& key, const FProperty* sourceProperty, const void* sourceMemoryAddress, bool bCallNotify)
{
	FLuaScriptValue* scriptValue = this->GetLuaScriptValueOrCreateEmpty(key);

	if(!scriptValue)
	{
		return;
	}
	
	ESetLuaValueResult setResult = ESetLuaValueResult::Success;
	
	if (scriptValue->IsNetProperty())
	{
		this->SetNetDirty();
	}
	
	setResult |= scriptValue->SetScriptValue(sourceProperty, sourceMemoryAddress);
	
	if(EnumHasAllFlags(setResult, ESetLuaValueResult::TickFunctionModified))
	{
		this->UpdateTickFuncMapping(scriptValue);
	}
	
	if(bCallNotify)
	{
		scriptValue->BroadcastValue();
	}
}


void FLuaScriptValuesContainer::SetScriptValueInternal(const sol::string_view& key, const FLuaValue& newValue, bool bCallNotify)
{
	FLuaScriptValue* scriptValue = this->GetLuaScriptValueOrCreateEmpty(key, !newValue.IsNil());

	if(!scriptValue)
	{
		return;
	}
	ESetLuaValueResult setResult = ESetLuaValueResult::Success;

	if (scriptValue->IsNetProperty())
	{
		this->SetNetDirty();
	}
	
	setResult |= scriptValue->SetScriptValue(newValue);
	
	if(EnumHasAllFlags(setResult, ESetLuaValueResult::TickFunctionModified))
	{
		this->UpdateTickFuncMapping(scriptValue);
	}
	
	if(bCallNotify)
	{
		scriptValue->BroadcastValue();
	}
}


FLuaDelegateHandle FLuaScriptValuesContainer::BindEventToDelegate(const FString& String, const FLuaDelegate& Delegate,bool bCreateOnTargetIfNotFound)
{
	auto casted = StringCast<char>(*String);
	FLuaScriptValue* value = this->GetLuaScriptValueOrCreateEmpty(casted.Get(), bCreateOnTargetIfNotFound);;
	if (value)
	{
		return value->AddDelegateListener(Delegate);
	}
	return {};
}

FLuaDelegateHandle FLuaScriptValuesContainer::BindEventToMulticastDelegate(const FString& String, const FLuaDelegate& Delegate, bool bCreateOnTargetIfNotFound)
{
	auto casted = StringCast<char>(*String);
	FLuaScriptValue* value = this->GetLuaScriptValueOrCreateEmpty(casted.Get(), bCreateOnTargetIfNotFound);;
	if (value)
	{
		return value->AddMulticastDelegateListener(Delegate);
	}
	return {};
}

bool FLuaScriptValuesContainer::UnbindEventToDelegate(const FString& String, const FLuaDelegate& Delegate)
{
	auto casted = StringCast<char>(*String);
	FLuaScriptValue* value = this->GetLuaScriptValue(casted.Get());;
	if (value)
	{
		return value->UnbindMulticastDelegateListener(Delegate);
	}
	return false;
}

bool FLuaScriptValuesContainer::UnbindEventToDelegate(const FString& key, FLuaDelegateHandle handle)
{
	auto casted = StringCast<char>(*key);
	FLuaScriptValue* value = this->GetLuaScriptValue(casted.Get());;
	if (value)
	{
		return value->UnbindMulticastDelegateListener(handle);
	}
	return false;
}

bool FLuaScriptValuesContainer::BroadcastLuaDelegate(const FString& key, const TArray<FLuaValue>& args)
{
	auto casted = StringCast<char>(*key);
	FLuaScriptValue* value = this->GetLuaScriptValue(casted.Get());;
	if (value)
	{
		return value->BroadcastLuaDelegate(args);
	}
	return false;
}

uint64 FLuaScriptValuesContainer::AddOnValueChangedListener(sol::string_view propStrv, UObject* subscriber, sol::string_view callbackStrv, const sol::variadic_args& variadic_Args)
{
	if(propStrv.empty() || !IsValid(subscriber) || callbackStrv.empty())
	{
		return 0;
	}
	FLuaScriptValue* value = this->GetLuaScriptValueOrCreateEmpty(propStrv, true);
	return value->AddOnValueChangedLuaScriptListener(subscriber, callbackStrv/*, additionalCallbackArgs*/);
}

void FLuaScriptValuesContainer::RemoveOnValueChangedListenerViaHandle(uint64 handle)
{
	for(FLuaScriptValue& val : this->LuaScriptValues)
	{
		if(val.RemoveLuaScriptListener(handle))
		{
			return;
		}
	}
}

void FLuaScriptValuesContainer::RemoveOnValueChangedListenerViaObject(UObject* subscriber)
{
	for(FLuaScriptValue& val : this->LuaScriptValues)
	{
		val.RemoveLuaScriptListener(subscriber);
	}
}

void FLuaScriptValuesContainer::RemoveOnValueChangedListenerFromScriptValueViaHandle(sol::string_view propStr, uint64 handle)
{
	FLuaScriptValue* val = this->GetLuaScriptValue(propStr);
	if(val)
	{
		(void)val->RemoveLuaScriptListener(handle);
	}
}

void FLuaScriptValuesContainer::RemoveOnValueChangedListenerFromScriptValueViaObject(sol::string_view propStr, UObject* subscriber)
{
	FLuaScriptValue* val = this->GetLuaScriptValue(propStr);
	if(val)
	{
		(void)val->RemoveLuaScriptListener(subscriber);
	}
}

void FLuaScriptValuesContainer::BroadcastValue(const std::string_view& key) const
{
	FLuaScriptValue* existing = this->GetLuaScriptValue(key);
	if(existing)
	{
		existing->BroadcastValue();
	}
}

void FLuaScriptValuesContainer::BroadcastValue(const TCHAR* key) const
{
	FLuaScriptValue* existing = this->GetLuaScriptValueInternal(key);
	if(existing)
	{
		existing->BroadcastValue();
	}
}

void FLuaScriptValuesContainer::CheckLuaScriptValueReferences()
{
	bool removedValues = false;
	for(auto it = this->LuaScriptValues.CreateIterator(); it; ++it)
	{
		it->PostGCHandleUObjectPtrs();
		if(it->ShouldBeRemoved())
		{
			//remove it
			//LUA_LOG("FLuaUObjectItem::NotifyPostGarbageCollection : Removing item %hs", it->GetKeyName().data())
			it.RemoveCurrent();
			removedValues = true;
		}
	}
	if(removedValues)
	{
		this->SetNetDirty();
		this->OnNumberOfValuesChanged.Broadcast();
	}
}

void FLuaScriptValuesContainer::AddReferencedObjects(FReferenceCollector& Collector)
{
	for(FLuaScriptValue& item : this->LuaScriptValues)
	{
		item.AddStructReferencedObjects(Collector);
	}
}

void FLuaScriptValuesContainer::ClearScriptValues(bool bBroadcast)
{	
	//@TODO : broadcast?
	this->LuaScriptValues.Empty();
	this->OnNumberOfValuesChanged.Broadcast();
}

FLuaScriptValue* FLuaScriptValuesContainer::GetLuaScriptValue(const sol::object& key) const
{
	if(key.get_type() != sol::type::string)
	{
		return nullptr;
	}
	sol::string_view strv  = key.as<sol::string_view>();
	return GetLuaScriptValue(strv);
}

FLuaScriptValue* FLuaScriptValuesContainer::GetLuaScriptValue(const std::string_view& key) const
{
	if(key.empty())
	{
		return nullptr;
	}
	return this->GetLuaScriptValueInternal(key);
}

FLuaScriptValue* FLuaScriptValuesContainer::GetLuaScriptValue(const char* key) const
{
	const std::string_view strv{key};
	return this->GetLuaScriptValue(strv);
}

FLuaScriptValue* FLuaScriptValuesContainer::GetLuaScriptValue(const TCHAR* key) const
{
	return this->GetLuaScriptValueInternal(key);
}

FLuaScriptValue* FLuaScriptValuesContainer::GetLuaScriptValue(const FName& key) const
{
	return this->GetLuaScriptValueInternal(key.ToString());
}

FLuaScriptValue* FLuaScriptValuesContainer::GetLuaScriptValue(const FUnrealLuaNameEntryKey& key)
{
	FLuaScriptValue* found = const_cast<FLuaScriptValue*>(this->LuaScriptValues.FindByPredicate([&key](const FLuaScriptValue& item)
	{
		return item.KeyMatches(key); 
	}));
	return found;
}

FLuaScriptValue* FLuaScriptValuesContainer::GetLuaScriptValueOrCreateEmpty(const TCHAR* Key, bool bCreateEvenIfNotExist)
{
	auto casted = StringCast<char>(Key);
	return GetLuaScriptValueOrCreateEmpty(casted.Get(), bCreateEvenIfNotExist);
}

/*
FLuaScriptValue* FLuaScriptValuesContainer::GetLuaScriptValue(const uint32 keyhash) const
{
	return this->GetLuaScriptValueInternal(keyhash);
}
*/
sol::function FLuaScriptValuesContainer::GetLuaScriptFunction(const sol::object& key) const
{
	if(key.get_type() != sol::type::string)
	{
		return sol::nil;
	}
	sol::string_view strv = key.as<sol::string_view>();
	return this->GetLuaScriptFunction(strv.data());
}