// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Reflection/PropertyHelper.h"
#include "Reflection/PropertyHelperTypes.h"
#include "sol/sol.hpp"
#include "LuaValue.h"
#include "StringHandling/UnrealLuaStringEntryKey.h"
#include "LuaScriptValue.generated.h"

class SWidget;

struct UNREALLUA_API FLuaScriptValueKey
{
	FLuaScriptValueKey() {}
	FLuaScriptValueKey(const FUnrealLuaNameEntryKey& stringKey)
		: CachedHash(stringKey.CachedHash), KeyNameEntry(stringKey.Entry)
	{}
	
	bool Matches(const FUnrealLuaNameEntryKey& Key) const;
	bool Matches(std::string_view& key, uint32 Hash) const;
	bool Matches(FStringView& Key, uint32 Hash) const;
	uint32 GetKeyHash() const;
	std::string_view GetKeyName() const;
	FString GetKeyNameString() const;
	FName GetFName() const;

	uint32 CachedHash = 0;
	bool bIsNetDirty = false;
	bool bIsNetProperty = false;
	bool bUnused1 = false;
	bool bUnused2 = false;
	UnrealLua::StringCache::FUnrealLuaNameEntry* KeyNameEntry = nullptr;
};

USTRUCT()
struct UNREALLUA_API FLuaScriptValue
{
	GENERATED_BODY()

	FLuaScriptValue()
	: Value(), OnValueChanged()//, KeyHash(0)
	{}
	//explicit FLuaScriptValue(const FSetLuaScriptFuncDescrParams& params);
	explicit FLuaScriptValue(const FSetLuaScriptUObjectMemberPropertyWrapperParams& params)
	: Value(params),OnValueChanged()
	{
		
	}

	FLuaScriptValue(const FLuaScriptValue& other)
	: Value(),OnValueChanged(nullptr)//, KeyHash(other.KeyHash)
	{
		this->Key = other.Key;
		if(other.OnValueChanged.IsValid())
		{
			FOnLuaScriptValueChangedMulticastDelegate* del = other.OnValueChanged.Get();
			FOnLuaScriptValueChangedMulticastDelegate* del2 = new FOnLuaScriptValueChangedMulticastDelegate();
			*del2 = *del;
			this->OnValueChanged.Reset(del2);
		}
		this->Value = other.Value;
		this->Value.MarkAsScriptValue();
	}

	FLuaScriptValue(FLuaScriptValue&& other) noexcept
		: Value(), OnValueChanged(MoveTemp(other.OnValueChanged))
	{
		this->Key = other.Key;
		this->Value = MoveTemp(other.Value);
		this->Value.MarkAsScriptValue();
		const char* nokey = "";
		other.Value = {sol::nil,nokey};
		other.ClearNetDirty();
	}


	
	~FLuaScriptValue()
	{
		this->Value.SetDead();
		this->BroadcastValue();
	}

	FLuaScriptValue& operator=(const FLuaScriptValue& other)
	{
		if(this != &other)
		{
			this->Key = other.Key;
			if(other.OnValueChanged.IsValid())
			{
				FOnLuaScriptValueChangedMulticastDelegate* del = other.OnValueChanged.Get();
				FOnLuaScriptValueChangedMulticastDelegate* del2 = new FOnLuaScriptValueChangedMulticastDelegate();
				*del2 = *del;
				this->OnValueChanged.Reset(del2);
			}
			this->Value = other.Value;
			this->Value.MarkAsScriptValue();
		}
		
		return *this;
	}
	
	ESetLuaValueResult ChangeToPropertyReference(const FSetLuaScriptUObjectMemberPropertyWrapperParams& params);

	template<typename T>
	ESetLuaValueResult SetScriptValue(const sol::basic_object<T>& value, const sol::string_view& key);
	ESetLuaValueResult SetScriptValue(const FProperty* prop, const void* inputValueAddress);
	ESetLuaValueResult SetScriptValue(const FLuaValue& source);
	void BroadcastValue() const;

	void AddStructReferencedObjects(FReferenceCollector& collector);
public:
	template<typename T>
	inline bool HasValueType() const
	{
		return this->Value.IsType<T>();
	}
	template<typename T>
	inline bool IsValue(const T& value) const
	{
		return this->Value.IsType<T>() && this->Value.Get<T>() == value;
	}
	
	inline bool GetTypeIndex() const
	{
		return this->Value.GetTypeIndex();
	}
	
	int PushValue(sol::this_state lua) const;
	sol::object GetValue(sol::this_state lua) const;
	bool WriteValueToPropertyMemoryAddress(FProperty* targetPropertyToWriteTo, void* memAddressToWriteTo) const;

	FDelegateHandle AddOnValueChangedDelegate(const FOnLuaScriptValueChangedNativeDelegate& del);
	FDelegateHandle AddOnValueChangedDelegate(FOnLuaScriptValueChangedDelegate del);
	void RemoveOnValueChangedDynamicListener(FOnLuaScriptValueChangedDelegate del);
	void RemoveOnValueChangedByHandle(FDelegateHandle delHandle);

	uint64 AddOnValueChangedLuaScriptListener(UObject* listener, const std::string_view callbackStr/*, sol::variadic_args additionalCallbackArgs = {}*/);
	void RemoveLuaScriptListener(void* listener);
	void RemoveLuaScriptListener(UObject* listener);
	bool RemoveLuaScriptListener(uint64 handle);

	void ClearNetDirty();
	bool IsNetDirty() const;
	ELuaValueType GetType() const;

	//bool IsPropertyReference() const;
	template<typename T>
	inline bool IsType() const
	{
		return this->Value.IsType<T>();
	}

	template<typename T>
	inline const T& Get() const
	{
		return this->Value.Get<T>();
	}
	
	template<typename T>
	inline T& GetMutable() const
	{
		return this->Value.GetMutable<T>();
	}
	
	const FLuaValue& GetLuaValue() const
	{
		return this->Value;
	}
	
	sol::function GetLuaScriptFunction() const;
	bool ShouldBeRemoved() const;

	bool HasInitializedValue() const;
	bool IsNil() const;
	bool IsPropertyOrUFunction() const;

	void SetIsNetProperty()
	{
		this->Key.bIsNetProperty = true;
	}
	bool IsNetProperty() const
	{
		return this->Key.bIsNetProperty;
	}

	FString GetKeyNameString() const
	{
		return this->Key.GetKeyNameString();
	}
	std::string_view GetKeyName() const
	{
		return this->Key.GetKeyName();
	}
	
	uint32 GetKeyHash() const
	{
		return Key.GetKeyHash();
	}

	bool operator<(const FLuaScriptValue& other) const
	{
		return this->GetKeyHash() < other.GetKeyHash();
	}
	
	void SetKey(std::string_view strv);
	void SetKey(const FUnrealLuaNameEntryKey& key);
	bool KeyMatches(std::string_view& key, uint32 hash) const;
	bool KeyMatches(FStringView& Key, uint32 Hash) const;
	bool KeyMatches(const FUnrealLuaNameEntryKey& Key) const;

	void PostGCHandleUObjectPtrs();
	void CleanUpForLuaState(sol::this_state lua);

	FLuaDelegateHandle AddDelegateListener(const FLuaDelegate& Delegate);
	FLuaDelegateHandle AddMulticastDelegateListener(const FLuaDelegate& delToAdd);
	bool UnbindMulticastDelegateListener(const FLuaDelegate& delToRemove);
	bool UnbindMulticastDelegateListener(FLuaDelegateHandle handle);
	bool BroadcastLuaDelegate(const TArray<FLuaValue>& args);
	bool IsDead() const;
	const FLuaScriptValueKey& GetKey() const
	{
		return this->Key;
	};

private:
	void SetNetDirty();
	void CreateBroadcastMulticastDelegate();
	void RemoveBroadcastMulticastDelegateIfEmpty();
	void RemoveBroadcastMulticastDelegate();
	
	FLuaScriptValueKey Key = {}; //16
	UPROPERTY(VisibleAnywhere)
	FLuaValue Value = {}; //56 (FLuaValue is 40 in shipping build)
	TUniquePtr<FOnLuaScriptValueChangedMulticastDelegate> OnValueChanged = nullptr; //64
	
	static_assert(sizeof(FLuaScriptValueKey) == 16);
	static_assert(sizeof(FLuaValue) == 40);
	static_assert(sizeof(decltype(FLuaScriptValue::OnValueChanged)) == 8);
};
static_assert(sizeof(FLuaScriptValue) <= 64);

/*To squeeze it into 32 bytes it would have to be
	8 byte stringptr
	4 byte keyhash
	8 byte for value/ptr
	1 byte for type TVariant
	1 byte for flags (net dirty)
	
	8 broadcaster

*/


template<>
struct TStructOpsTypeTraits<FLuaScriptValue> : public TStructOpsTypeTraitsBase2<FLuaScriptValue>
{
	enum
	{
		WithCopy = false,
		WithAddStructReferencedObjects = true		
		//WithNetSerializer = true,
		//WithIdenticalViaEquality = true
	};
};

template<typename T>
inline ESetLuaValueResult FLuaScriptValue::SetScriptValue(const sol::basic_object<T>& value, const sol::string_view& key)
{
	this->SetNetDirty();
	return this->Value.SetValue(value, key);	
}

inline ESetLuaValueResult FLuaScriptValue::SetScriptValue(const FProperty* prop, const void* inputValueAddress)
{
	this->SetNetDirty();
	return this->Value.SetValue(prop, inputValueAddress);
}

inline ESetLuaValueResult FLuaScriptValue::SetScriptValue(const FLuaValue& source)
{
	bool bIsNetProperty = this->IsNetProperty();
	ESetLuaValueResult setResult = this->Value.SetValue(source);
	verify(this->IsNetProperty() == bIsNetProperty);
	return setResult;
}

inline void FLuaScriptValue::BroadcastValue() const
{
	if(this->Value.IsType<FPropertyReferenceWrapper>())
	{
		const FPropertyReferenceWrapper& wrapper = this->Value.Get<FPropertyReferenceWrapper>();
		UObject* scriptOwner = wrapper.Owner;
		FProperty* prop = wrapper.Prop;
		UnrealLua::PropertyHelper::HandleSetPropertyNetBehavior(scriptOwner, prop);
	}
	if(this->OnValueChanged == nullptr)
	{
		return;
	}
	
	if(this->Value.IsType<FPropertyReferenceWrapper>())
	{
		FLuaValue temp = this->Value.MakeCopy(false, false);
		this->OnValueChanged->Broadcast(temp);
	}
	else
	{
		FLuaValue temp = this->Value.MakeCopy(true, false);
		this->OnValueChanged->Broadcast(temp);
	}
}

inline int FLuaScriptValue::PushValue(sol::this_state lua) const
{
	return this->Value.PushValue(lua);
}

inline sol::object FLuaScriptValue::GetValue(sol::this_state lua) const
{
	return this->Value.GetValue(lua);
}

inline bool FLuaScriptValue::WriteValueToPropertyMemoryAddress(FProperty* targetPropertyToWriteTo, void* memAddressToWriteTo) const
{
	return this->Value.WriteValueToPropertyMemoryAddress_WithPropertyTypeCheck(targetPropertyToWriteTo, memAddressToWriteTo);
}


inline void FLuaScriptValue::ClearNetDirty()
{
	this->Key.bIsNetDirty = false;
}

inline bool FLuaScriptValue::IsNetDirty() const
{
	return this->Key.bIsNetDirty;
}

inline void FLuaScriptValue::SetNetDirty()
{
	this->Key.bIsNetDirty = true;
}

inline sol::function FLuaScriptValue::GetLuaScriptFunction() const
{
	sol::function ret {sol::nil};
	if(this->IsType<sol::function>())
	{
		ret = this->Get<sol::function>();
	}
	else if(this->IsType<FLuaFunctionHandle>())
	{
		sol::object obj = this->Get<FLuaFunctionHandle>().GetFunction();
		if(obj.get_type() == sol::type::function)
		{
			ret = obj.as<sol::function>();
		}
	}
	else if(this->IsType<FLuaRPCFunction>())
	{
		sol::object obj = this->Get<FLuaRPCFunction>().LuaFunc;
		if(obj.valid() && obj.get_type() == sol::type::function)
		{
			ret = obj.as<sol::function>();
		}
	}
	else if(this->IsType<FLuaUFunctionReference>())
	{
		ret = this->Get<FLuaUFunctionReference>().LuaFunc;
	}
	return ret;
}

inline bool FLuaScriptValue::HasInitializedValue() const
{
	return this->Value.IsInitialized();
}

inline bool FLuaScriptValue::IsNil() const
{
	return this->Value.IsNil();
}

inline bool FLuaScriptValue::IsPropertyOrUFunction() const
{
	return this->Value.IsPropertyOrUFunctionReference();
}

inline void FLuaScriptValue::CreateBroadcastMulticastDelegate()
{
	if(this->OnValueChanged == nullptr)
	{
		this->OnValueChanged.Reset(new FOnLuaScriptValueChangedMulticastDelegate());
	}
}

inline void FLuaScriptValue::RemoveBroadcastMulticastDelegateIfEmpty()
{
	if(this->OnValueChanged.IsValid() && !this->OnValueChanged->IsBound())
	{
		this->RemoveBroadcastMulticastDelegate();
	}
}