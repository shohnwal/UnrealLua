// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaPrimitives.h"
#include "LuaUStruct.h"
#include "LuaValue/LuaFunction.h"
#include "LuaValue/LuaTable.h"
#include "Misc/TVariant.h"
#include "StringHandling/UnrealLuaStringEntryKey.h"
#include "UObject/ScriptDelegateFwd.h"
#include "LuaDelegate.generated.h"
//#include "LuaDelegate.generated.h"

/**
 * 
 */


USTRUCT(BlueprintType)
struct UNREALLUA_API FLuaDelegate
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "UnrealLua")
	TWeakObjectPtr<UObject> Object = {};
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "UnrealLua")
	FString CallbackFunctionName = {};
	bool IsBound() const;
};

USTRUCT(BlueprintType)
struct UNREALLUA_API FLuaDelegateHandle
{
	GENERATED_BODY()
	static FLuaDelegateHandle MakeHandle();
	bool IsBound() const { return Handle > 0; }
	int64 ToInteger() const { return Handle; }
	bool operator==(const FLuaDelegateHandle& other) const
	{
		return this->ToInteger() == other.ToInteger(); 
	};
	int64 Handle = 0;
};

struct UNREALLUA_API FLuaDelegateTableCallback
{
	FLuaDelegateTableCallback() = default;
	FLuaDelegateTableCallback(const FLuaTableHandle& TableHandle, FUnrealLuaNameEntryKey CallbackStringKey, FLuaDelegateHandle& DelHandle);
	
	bool IsBound() const ;
	void Execute(sol::variadic_args args) const;
	void Execute(const TArray<FLuaValue>& args) const;
	bool operator==(const sol::table& tbl) const
	{
		return this->TableHandle == tbl;
	}

	FLuaTableHandle TableHandle = {};
	FUnrealLuaNameEntryKey CallbackFunctionName = {};
	FLuaDelegateHandle DelegateHandle = {};
};

struct UNREALLUA_API FLuaDelegateFunctionCallback
{
	FLuaDelegateFunctionCallback() = default;
	FLuaDelegateFunctionCallback(const FLuaFunctionHandle& funcHandle, FLuaDelegateHandle delHandle);
	bool IsBound() const;
	void Execute(const TArray<FLuaValue>& args) const;
	bool operator==(const sol::function& func) const
	{
		return this->Callback == func;
	}

	FLuaFunctionHandle Callback = {};
	FLuaDelegateHandle DelegateHandle = {};
};

struct UNREALLUA_API FLuaDelegateUObjectCallback
{
	FLuaDelegateUObjectCallback() = default;
	FLuaDelegateUObjectCallback(UObject* objTarget, FUnrealLuaNameEntryKey callbackStringKey, const FLuaDelegateHandle& delHandle);
	bool IsBound() const;

	bool Execute(const TArray<FLuaValue>& args) const;
	bool operator==(UObject* obj) const
	{
		return this->Object == obj;
	}

	TWeakObjectPtr<UObject> Object = {};
	FUnrealLuaNameEntryKey CallbackFunctionName = {} ;
	FLuaDelegateHandle DelegateHandle = {};
};

template<typename T>
using TLuaDelegateVariantPtr = TPimplPtr<T, EPimplPtrMode::DeepCopy>;

typedef TVariant<std::nullptr_t, TLuaDelegateVariantPtr<FLuaDelegateUObjectCallback>, TLuaDelegateVariantPtr<FLuaDelegateTableCallback>, TLuaDelegateVariantPtr<FLuaDelegateFunctionCallback>> FLuaDelegateCallbackVariant; 

class UNREALLUA_API FLuaScriptDelegate
{
public:
	static void RegisterUsertype(sol::state_view& lua);

	/** Default constructor */
	FLuaScriptDelegate()
	{
	}
	FLuaScriptDelegate(lua_State* L);
	FLuaScriptDelegate(UObject* obj, const FString& funcName, FLuaDelegateHandle handle);
	FLuaScriptDelegate(UObject* obj, const std::string_view& funcName, FLuaDelegateHandle handle);
	FLuaScriptDelegate(FLuaTableHandle& tableHandle, const std::string_view& funcName, FLuaDelegateHandle handle);
	FLuaScriptDelegate(FLuaFunctionHandle& funcHandle, FLuaDelegateHandle handle);
	/** Execute the delegate.  If the function pointer is not valid, an error will occur. */
	bool Execute( const TArray<FLuaValue>& values ) const;
	void Lua_Execute(sol::variadic_args args) const;

	sol::variadic_results operator()(sol::variadic_args args);
	
	bool operator==(const FLuaScriptDelegate& other) const
	{
		return this->Callback.GetIndex() == other.Callback.GetIndex();
	}

	bool IsBound() const;

	void Reset()
	{
		this->Callback = {};
	}
	
	FLuaDelegateHandle Add(const FLuaDelegate& delToBind);
	FLuaDelegateHandle Add(UObject* obj, const FString& callback);

	int64 Lua_Add(sol::stack_object target, sol::stack_object funcName, sol::variadic_args captureArgs, sol::this_state lua);
	//if funcname is nil, remove all bindings for target
	void Lua_Remove(sol::stack_object target, sol::stack_object funcName, sol::this_state lua);
	void RemoveHandle(FLuaDelegateHandle Handle);
	void Clear();

	
	FLuaDelegateHandle GetHandle() const;
	int64 GetHandleAsInteger() const;
	
	const FLuaDelegateCallbackVariant& GetCallback() const
	{
		return this->Callback;
	}

	template<typename T>
	bool IsCallbackType() const
	{
		return this->Callback.IsType<TLuaDelegateVariantPtr<T>>();
	}
	
	template<typename T>
	T& GetCallbackType() const
	{
		return *this->Callback.Get<TLuaDelegateVariantPtr<T>>().Get();
	}
	
	template<typename T>
	static constexpr SIZE_T IndexOfCallbackType()
	{
		return FLuaDelegateCallbackVariant::IndexOfType<TLuaDelegateVariantPtr<T>>();
	}
	
	template<typename T, typename... ArgTypes>
	void EmplaceCallbackType(ArgTypes&&... args)
	{
		this->Callback.Emplace<TLuaDelegateVariantPtr<T>>(MakePimpl<T, EPimplPtrMode::DeepCopy>(std::forward<ArgTypes>(args)...));
	}
	
	template<typename T, typename... ArgTypes>
	T& EmplaceCallbackType_GetRef(ArgTypes&&... args)
	{
		this->EmplaceCallbackType(std::forward<ArgTypes>(args)...);
		return this->GetCallbackType<T>();
	}
	
private:
	static void FDLuaDelegate_DelegateWrapper(const FScriptDelegate&, TArray<FLuaValue> values);
	
	FLuaDelegateCallbackVariant Callback = {};
};

