#pragma once
#include "sol/sol.hpp"
#include "LuaValue/LuaValue.h"
#include "Reflection/PropertyHelperTypes.h"
#include "LuaScriptValuesContainer.generated.h"

struct FUnrealLuaNameEntryKey;
struct FLuaScriptValue;

USTRUCT()
struct UNREALLUA_API FLuaScriptValuesContainer
{
	GENERATED_BODY()
	
	virtual ~FLuaScriptValuesContainer() = default;
	bool IsEmpty() const;
public:
	
	virtual UObject* GetUObjectVirtual() const { return nullptr; }
	
	
	void SetNetDirty();
	void ClearNetDirty();
	bool IsNetDirty() const;
	
	TArray<FLuaScriptValue>& GetLuaScriptValues();
	
	virtual const FHashedFieldMapping* GetPropertyMapping(const sol::string_view& strv) { return nullptr; }
	virtual const FHashedFieldMapping* GetPropertyMapping(const uint32 hash) { return nullptr; }
	virtual const FHashedFieldMapping* GetPropertyMapping(const sol::object& obj) { return nullptr; }
	virtual const FHashedFieldMapping* GetPropertyMapping(const sol::stack_object& obj) { return nullptr; }
	virtual void* GetOwningContainer() { return nullptr; }
	
	// Value changed events
	
	uint64 AddOnValueChangedListener(sol::string_view propStrv, UObject* subscriber, sol::string_view callbackStrv, const sol::variadic_args& variadic_Args);
	void RemoveOnValueChangedListenerViaHandle(uint64 handle);
	void RemoveOnValueChangedListenerViaObject(UObject* subscriber);
	void RemoveOnValueChangedListenerFromScriptValueViaHandle(sol::string_view propStr, uint64 handle);
	void RemoveOnValueChangedListenerFromScriptValueViaObject(sol::string_view propStr, UObject* subscriber);
	void BroadcastValue(const TCHAR* Key) const;
	void BroadcastValue(const std::string_view& Key) const;

public:
	//ScriptValue Getters
	FLuaScriptValue* GetLuaScriptValue(const sol::object& key) const;
	FLuaScriptValue* GetLuaScriptValue(const std::string_view& key) const;
	FLuaScriptValue* GetLuaScriptValue(const char* key) const;
	FLuaScriptValue* GetLuaScriptValue(const TCHAR* key) const;
	FLuaScriptValue* GetLuaScriptValue(const FName& key) const;
	FLuaScriptValue* GetLuaScriptValue(const FUnrealLuaNameEntryKey& Key);
//	FLuaScriptValue* GetLuaScriptValue(const uint32 keyhash) const;
	FLuaScriptValue* GetLuaScriptValueOrCreateEmpty(const TCHAR* Key, bool bCreateEvenIfNotExist = true);
	FLuaScriptValue* GetLuaScriptValueOrCreateEmpty(const std::string_view Key, bool bCreateEvenIfNotExist = true);
	FLuaScriptValue* GetLuaScriptValueOrCreateEmpty(const FUnrealLuaNameEntryKey& key, bool bCreateEvenIfNotExist = true);
private:
	FLuaScriptValue* GetLuaScriptValueInternal(std::string_view key) const;
	FLuaScriptValue* GetLuaScriptValueInternal(FStringView key) const;

public:
	//Value Getters
	//Copy a ScriptValue / PropertyValue and copies content to memAddressToWriteTo 
	bool GetScriptValue(const std::string_view& key, FProperty* targetProperty, void* targetMemAddress);
	sol::object GetScriptValue(const sol::stack_object& key);
	sol::object GetScriptValue(const sol::object& key, sol::this_state lua);
	sol::object GetScriptValue(const std::string_view& key, sol::this_state lua);
	int PushScriptValue(const sol::object& key);
	int PushScriptValue(const sol::stack_object& key);
	sol::function GetLuaScriptFunction(const sol::object& key) const;
	sol::function GetLuaScriptFunction(const char* key) const;
	sol::function GetLuaScriptFunction(const TCHAR* key) const;
	sol::function GetLuaScriptFunction(const FString& key) const;
	sol::function* GetUFunctionOverrideLuaScriptFunction(const FName& key) const;

private:
	int PushScriptValueInternal(const std::string_view& key, sol::this_state luat);

public:
	//Setters
	void ResetNonPropertyWrapperValuesButKeepListeners();
	void EmptyAllLuaScriptValues();
	bool HasAnyLuaScriptValues() const;
	void CleanUpLuaScriptValuesForLuaState(lua_State* L);
	
	//Sets a script value from a source Property + memory address
	//This will do a full copy (struct, array, etc)
	void SetScriptValue(const sol::string_view& key, const FProperty* sourceProperty, const void* sourceMemoryAddress, bool bCallNotify = true);
	//Sets a script value from another FLuaScriptValueData
	//Used by Lua script value replicator component
	//In case of struct-like datatype (struct, FLuaArray, etc), will make a reference
	void SetScriptValue(const sol::string_view& key, const FLuaValue& value, bool bCallNotify = true);
	void SetScriptValue(const TCHAR* key, const FLuaValue& value, bool bCallNotify = true);
	void SetScriptValue(const FName key, const FLuaValue& value, bool bCallNotify = true);
	//Set script value from Lua value
	template<typename T>
	void SetScriptValue(const sol::basic_object<T>& key, const sol::basic_object<T>& value, bool bCallNotify = true);
	template<typename T>
	void SetScriptValue(const std::string_view& key, const sol::basic_object<T>& value, bool bCallNotify = true);
	template<typename T>
	void SetScriptValue(const TCHAR* key, const sol::basic_object<T>& value, bool bCallNotify = true);
	void SetScriptValue(const std::string_view& key, const sol::nil_t, bool bCallNotify = true);
private:	
	template<typename T>
	requires (std::is_same_v<std::remove_cvref_t<sol::object>,T> || std::is_same_v<std::remove_cvref_t<sol::stack_object>,T>)
	void SetScriptValueInternal(const std::string_view& key, const T& newValue, bool bCallNotify = true);

	FLuaScriptValue* SetPropertyWrapperLuaScriptValue(const FSetLuaScriptUObjectMemberPropertyWrapperParams Params);
	void SetScriptValueInternal(const sol::string_view& key, const FProperty* sourceProperty, const void* sourceMemoryAddress, bool bCallNotify);
	void SetScriptValueInternal(const sol::string_view& key, const FLuaValue& newValue, bool bCallNotify);
	
public:
	FLuaDelegateHandle BindEventToDelegate(const FString& String, const FLuaDelegate& Delegate, bool bCreateOnTargetIfNotFound);
	FLuaDelegateHandle BindEventToMulticastDelegate(const FString& String, const FLuaDelegate& Delegate, bool bCreateOnTargetIfNotFound);
	bool UnbindEventToDelegate(const FString& String, const FLuaDelegate& Delegate);
	bool UnbindEventToDelegate(const FString& key, FLuaDelegateHandle handle);
	bool BroadcastLuaDelegate(const FString& key, const TArray<FLuaValue>& args);
	
private:
	UPROPERTY(VisibleAnywhere)
	TArray<FLuaScriptValue> LuaScriptValues = {};
public:
	UPROPERTY(VisibleAnywhere)
	bool bIsScriptNetDirty = false;

	FSimpleMulticastDelegate OnNumberOfValuesChanged = {};
protected:
	virtual void UpdateTickFuncMapping(FLuaScriptValue* keyname) {};
public:
		void CheckLuaScriptValueReferences();
    	virtual void AddReferencedObjects(FReferenceCollector& Collector);
    private:
    	void ClearScriptValues(bool bBroadcast = false);
    public:
};


template<typename T>
inline void FLuaScriptValuesContainer::SetScriptValue(const std::string_view& key, const sol::basic_object<T>& value, bool bCallNotify)
{
	this->SetScriptValueInternal(key, value, bCallNotify);
}

template<typename T>
inline void FLuaScriptValuesContainer::SetScriptValue(const TCHAR* key, const sol::basic_object<T>& value, bool bCallNotify)
{
	auto casted = StringCast<char>(key);
	this->SetScriptValueInternal(casted.Get(), value, bCallNotify);
}

inline void FLuaScriptValuesContainer::SetScriptValue(const std::string_view& key, const sol::nil_t, bool bCallNotify)
{
	sol::object nil = sol::nil;
	this->SetScriptValueInternal(key, nil, bCallNotify);
}

inline void FLuaScriptValuesContainer::SetScriptValue(const sol::string_view& key, const FProperty* sourceProperty, const void* sourceMemoryAddress, bool bCallNotify)
{
	this->SetScriptValueInternal(key, sourceProperty, sourceMemoryAddress, bCallNotify);
}

inline void FLuaScriptValuesContainer::SetScriptValue(const sol::string_view& key, const FLuaValue& value, bool bCallNotify)
{
	this->SetScriptValueInternal(key, value, bCallNotify);
}


inline void FLuaScriptValuesContainer::SetScriptValue(const FName key, const FLuaValue& value, bool bCallNotify)
{
	this->SetScriptValue(*key.ToString(), value, bCallNotify);
}


inline void FLuaScriptValuesContainer::SetScriptValue(const TCHAR* key, const FLuaValue& value, bool bCallNotify)
{
	auto casted = StringCast<char>(key);
	this->SetScriptValueInternal(casted.Get(), value, bCallNotify);
}

template<typename T>
inline void FLuaScriptValuesContainer::SetScriptValue(const sol::basic_object<T>& key, const sol::basic_object<T>& value, bool bCallNotify)
{
	if(key.get_type() != sol::type::string)
	{
		return;
	}
	sol::string_view strv = key.template as<sol::string_view>();
	this->SetScriptValue(strv, value);
}

inline sol::object FLuaScriptValuesContainer::GetScriptValue(const sol::stack_object& key)
{
	if(key.get_type() != sol::type::string)
	{
		return sol::nil;
	}
	sol::string_view strv  = key.as<sol::string_view>();
	return this->GetScriptValue(strv, key.lua_state());
}

inline sol::object FLuaScriptValuesContainer::GetScriptValue(const sol::object& key, sol::this_state luat)
{
	if(key.get_type() != sol::type::string)
	{
		return sol::nil;
	}
	sol::string_view strv  = key.as<sol::string_view>();
	return this->GetScriptValue(strv, key.lua_state());
}

inline int FLuaScriptValuesContainer::PushScriptValue(const sol::object& key)
{
	if(key.get_type() != sol::type::string)
	{
		return 0;
	}
	sol::string_view strv  = key.as<sol::string_view>();
	return this->PushScriptValueInternal(strv, key.lua_state());
}

inline int FLuaScriptValuesContainer::PushScriptValue(const sol::stack_object& key)
{
	if(key.get_type() != sol::type::string)
	{
		return 0;
	}
	sol::string_view strv  = key.as<sol::string_view>();
	return this->PushScriptValueInternal(strv, key.lua_state());
}