// Fill out your copyright notice in the Description page of Project Settings.

#include "LuaTypes/LuaDelegate.h"

#include "LuaCoreDelegates.h"
#include "UnrealLua.h"
#include "LuaContext/ScopedLuaContext.h"
#include "StringHandling/UnrealLuaStringCache.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"


static const FDelegateHandle fLuaDelegateLuaTypeHandle = FLuaCoreDelegates::OnRegisterLuaUsertypes.AddStatic(&FLuaScriptDelegate::RegisterUsertype);

bool FLuaDelegate::IsBound() const
{
	return !this->CallbackFunctionName.IsEmpty() && this->Object.IsValid();
}

FLuaDelegateHandle FLuaDelegateHandle::MakeHandle()
{
	static int64 LastUsedHandle = 0;
	int64 newHandle = ++LastUsedHandle;
	return FLuaDelegateHandle{ newHandle };
}

FLuaDelegateTableCallback::FLuaDelegateTableCallback(const FLuaTableHandle& tableHandle, FUnrealLuaNameEntryKey callbackStringKey, FLuaDelegateHandle& delHandle)
	: TableHandle(tableHandle), CallbackFunctionName(callbackStringKey)
{
	this->DelegateHandle = delHandle;
}

bool FLuaDelegateTableCallback::IsBound() const
{
	return this->DelegateHandle.IsBound() && this->TableHandle.IsValid() && this->CallbackFunctionName.IsValid();
}

void FLuaDelegateTableCallback::Execute(sol::variadic_args args) const
{
	if (this->IsBound())
	{
		sol::table tbl = this->TableHandle.GetTable();
		sol::optional<sol::function> maybeFunc = tbl[this->CallbackFunctionName.GetKeyStringView()];
		if (maybeFunc)
		{
			sol::function func = maybeFunc.value();
			UnrealLua::LuaScriptCall::CallLuaFunctionSafe(func, tbl, sol::as_args(args));
		}
	}	
}

void FLuaDelegateTableCallback::Execute(const TArray<FLuaValue>& args) const
{
	if (this->IsBound())
	{
		sol::table tbl = this->TableHandle.GetTable();
		sol::optional<sol::function> maybeFunc = tbl[this->CallbackFunctionName.GetKeyStringView()];
		if (maybeFunc)
		{
			sol::function func = maybeFunc.value();
			UnrealLua::LuaScriptCall::CallLuaFunctionSafe(func, tbl, sol::as_args(args));
		}
	}
}

FLuaDelegateFunctionCallback::FLuaDelegateFunctionCallback(const FLuaFunctionHandle& funcHandle, FLuaDelegateHandle delHandle)
{
	this->Callback = funcHandle;
	this->DelegateHandle = delHandle;
}

bool FLuaDelegateFunctionCallback::IsBound() const
{
	return this->DelegateHandle.IsBound() && this->Callback.IsValid();
}

void FLuaDelegateFunctionCallback::Execute(const TArray<FLuaValue>& args) const
{
	if (this->IsBound())
	{
		sol::function func = this->Callback.GetFunction();
		if (func.valid())
		{
			UnrealLua::LuaScriptCall::CallLuaFunctionSafe(func, sol::as_args(args));
		}
	}
}

FLuaDelegateUObjectCallback::FLuaDelegateUObjectCallback(UObject* objTarget, FUnrealLuaNameEntryKey callbackStringKey, const FLuaDelegateHandle& delHandle)
	: Object(objTarget), CallbackFunctionName(callbackStringKey)
{
	this->DelegateHandle = delHandle;
}

bool FLuaDelegateUObjectCallback::IsBound() const
{
	return this->DelegateHandle.IsBound() && this->Object.IsValid() && this->CallbackFunctionName.IsValid();
}

bool FLuaDelegateUObjectCallback::Execute(const TArray<FLuaValue>& args) const
{
	UObject* obj = this->Object.Get();
	if (obj)
	{
		FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(obj);
					
		FLuaScriptValue* value = item.GetLuaScriptValueOrCreateEmpty(this->CallbackFunctionName, false);
		if (value)
		{
			if (value->IsType<FLuaFunctionHandle>())
			{
				sol::function func = value->Get<FLuaFunctionHandle>().GetFunction();
				if (func.valid())
				{
					UnrealLua::LuaScriptCall::CallLuaFunctionSafe(func, obj, sol::as_args(args));
					return true;
				}
				else
				{
					return false;
				}
			}
			else if (value->IsType<FLuaUFunctionReference>())
			{
				const FLuaUFunctionReference& ref = value->Get<FLuaUFunctionReference>();
				if (ref.LuaFunc.valid())
				{
					UnrealLua::LuaScriptCall::CallLuaFunctionSafe(ref.LuaFunc, obj, sol::as_args(args));
					return true;
				}
				else
				{
					ref.Func->PerformCall_NoReturnValues(obj, args);
				}
			}
		}
	}
	return false;
}

void FLuaScriptDelegate::RegisterUsertype(sol::state_view& lua)
{
	sol::usertype<FLuaScriptDelegate> ut = lua.new_usertype<FLuaScriptDelegate>(
		"Delegate",
		"new", sol::no_constructor,
		sol::call_constructor, [](sol::this_state lua) ->sol::object
		{
			return sol::object(lua, sol::in_place_type<FLuaScriptDelegate>, FLuaScriptDelegate());
		},
		//sol::meta_function::call, &FLuaScriptDelegate::Lua_Execute,
		"Add", &FLuaScriptDelegate::Lua_Add,
		"Bind", &FLuaScriptDelegate::Lua_Add,
		"Remove", &FLuaScriptDelegate::Lua_Remove,
		"Clear", &FLuaScriptDelegate::Clear,
		"Execute", &FLuaScriptDelegate::Lua_Execute,
		"IsBound", &FLuaScriptDelegate::IsBound
	);
}

FLuaScriptDelegate::FLuaScriptDelegate(lua_State* L)
{
}

FLuaScriptDelegate::FLuaScriptDelegate(UObject* obj, const FString& funcName, FLuaDelegateHandle handle)
{
	FUnrealLuaNameEntryKey key = UnrealLua::StringCache::GetStringEntryKey(funcName);
	this->EmplaceCallbackType<FLuaDelegateUObjectCallback>(obj,key, handle);
}

FLuaScriptDelegate::FLuaScriptDelegate(UObject* obj, const std::string_view& funcName, FLuaDelegateHandle handle)
{
	FUnrealLuaNameEntryKey key = UnrealLua::StringCache::GetStringEntryKey(funcName);
	this->EmplaceCallbackType<FLuaDelegateUObjectCallback>(obj,key, handle);
}

FLuaScriptDelegate::FLuaScriptDelegate(FLuaTableHandle& tableHandle, const std::string_view& funcName, FLuaDelegateHandle handle)
{
	FUnrealLuaNameEntryKey key = UnrealLua::StringCache::GetStringEntryKey(funcName);
	this->EmplaceCallbackType<FLuaDelegateTableCallback>(tableHandle, key, handle);
}

FLuaScriptDelegate::FLuaScriptDelegate(FLuaFunctionHandle& funcHandle, FLuaDelegateHandle handle)
{
	this->EmplaceCallbackType<FLuaDelegateFunctionCallback>(funcHandle, handle);
}

bool FLuaScriptDelegate::Execute(const TArray<FLuaValue>& args) const
{
	auto* This = const_cast<FLuaScriptDelegate*>(this);
	if( this->IsBound() )
	{
		switch (this->Callback.GetIndex())
		{
		case FLuaScriptDelegate::IndexOfCallbackType<FLuaDelegateUObjectCallback>():
			{
				const FLuaDelegateUObjectCallback& uobjectCallback = This->GetCallbackType<FLuaDelegateUObjectCallback>();
				uobjectCallback.Execute(args);
				break;
			}
			case FLuaScriptDelegate::IndexOfCallbackType<FLuaDelegateTableCallback>():
			{
				const FLuaDelegateTableCallback& tableCallback = This->GetCallbackType<FLuaDelegateTableCallback>();
				tableCallback.Execute(args);
				break;
			}
			case FLuaScriptDelegate::IndexOfCallbackType<FLuaDelegateFunctionCallback>():
			{
				const FLuaDelegateFunctionCallback& funcCallback = This->GetCallbackType<FLuaDelegateFunctionCallback>();
				funcCallback.Execute(args);
				break;
			}
		}

		return true;
	}
	else
	{
		This->Reset();
		return false;
	}
	
}

void FLuaScriptDelegate::Lua_Execute(sol::variadic_args args_) const
{
	TArray<FLuaValue> args;
	for(int32 index = 0; index < args_.size(); index++)
	{
		sol::stack_object obj = args_[index]; 
		args.Emplace(obj);
	}
	(void)this->Execute(args);
}

sol::variadic_results FLuaScriptDelegate::operator()(sol::variadic_args args)
{
	this->Lua_Execute(args);;
	return {};
}

bool FLuaScriptDelegate::IsBound() const
{
	switch (this->Callback.GetIndex())
	{
	case FLuaScriptDelegate::IndexOfCallbackType<FLuaDelegateUObjectCallback>():
		{
			return this->GetCallbackType<FLuaDelegateUObjectCallback>().IsBound();
		}
	case FLuaScriptDelegate::IndexOfCallbackType<FLuaDelegateTableCallback>():
		{
			return this->GetCallbackType<FLuaDelegateTableCallback>().IsBound();
		}
	case FLuaScriptDelegate::IndexOfCallbackType<FLuaDelegateFunctionCallback>():
		{
			return this->GetCallbackType<FLuaDelegateFunctionCallback>().IsBound();
		}
	default:
		return false;
	}
}

FLuaDelegateHandle FLuaScriptDelegate::Add(const FLuaDelegate& delToBind)
{
	return this->Add(delToBind.Object.Get(), delToBind.CallbackFunctionName);
}

FLuaDelegateHandle FLuaScriptDelegate::Add(UObject* obj, const FString& callback)
{
	if (!obj || callback.IsEmpty())
	{
		this->Callback.Emplace<std::nullptr_t>();
		return {};
	}
	FUnrealLuaNameEntryKey key = UnrealLua::StringCache::GetStringEntryKey(callback);
	this->EmplaceCallbackType<FLuaDelegateUObjectCallback>(obj, key, FLuaDelegateHandle::MakeHandle());
	return this->GetCallbackType<FLuaDelegateUObjectCallback>().DelegateHandle;
}

int64 FLuaScriptDelegate::Lua_Add(sol::stack_object target, sol::stack_object funcName, sol::variadic_args captureArgs, sol::this_state lua)
{
	if (target.get_type() == sol::type::function)
	{
		sol::function func = target.as<sol::function>();
		if (func.valid())
		{
			FLuaDelegateHandle newHandle = FLuaDelegateHandle::MakeHandle();
			this->EmplaceCallbackType<FLuaDelegateFunctionCallback>(FLuaFunctionHandle::MakeHandle(func), newHandle);
			return newHandle.ToInteger();			
		}
	}
	else if (funcName.get_type() == sol::type::string)
	{
		std::string_view callbackStr = funcName.as<std::string_view>();
		if (!callbackStr.empty())
		{
			if (UnrealLua::IsUObject(target))
			{
				FUnrealLuaNameEntryKey key = UnrealLua::StringCache::GetStringEntryKey(callbackStr);
				FLuaDelegateHandle newHandle = FLuaDelegateHandle::MakeHandle();
				UObject* obj = UnrealLua::GetUObject(target);
				this->EmplaceCallbackType<FLuaDelegateUObjectCallback>(obj, key, newHandle);
				return newHandle.ToInteger();
			}
			else if (target.get_type() == sol::type::table)
			{
				sol::table tbl = target.as<sol::table>();
				if (tbl.valid())
				{
					FUnrealLuaNameEntryKey key = UnrealLua::StringCache::GetStringEntryKey(callbackStr);
					FLuaDelegateHandle newHandle = FLuaDelegateHandle::MakeHandle();
					this->EmplaceCallbackType<FLuaDelegateTableCallback>(FLuaTableHandle::MakeHandle(tbl), key, newHandle);
					return newHandle.ToInteger();					
				}
			}
		}
	}
	
	return 0;
}

void FLuaScriptDelegate::Lua_Remove(sol::stack_object target, sol::stack_object funcName, sol::this_state lua)
{
	sol::type targetType = target.get_type();
	if (targetType == sol::type::function)
	{
		sol::function func = target.as<sol::function>();
		if (this->IsCallbackType<FLuaDelegateFunctionCallback>())
		{
			const FLuaDelegateFunctionCallback& callback = this->GetCallbackType<FLuaDelegateFunctionCallback>();
			if (callback == func)
			{
				this->Clear();
			}
		}
	}
	else if (targetType == sol::type::table)
	{
		sol::table tbl = target.as<sol::table>();
		if (this->IsCallbackType<FLuaDelegateTableCallback>())
		{
			const FLuaDelegateTableCallback& callback = this->GetCallbackType<FLuaDelegateTableCallback>();
			if (callback == tbl)
			{
				this->Clear();
			}
		}
	}
	else if (targetType == sol::type::number)
	{
		int64 handle = target.as<int64>();
		if (this->GetHandleAsInteger() == handle)
		{
			this->Clear();
		}		
	}
	else if (UnrealLua::IsUObject(target))
	{
		UObject* obj = UnrealLua::GetUObject(target);
		if (this->IsCallbackType<FLuaDelegateUObjectCallback>())
		{
			const FLuaDelegateUObjectCallback& callback = this->GetCallbackType<FLuaDelegateUObjectCallback>();
			if (callback == obj)
			{
				this->Clear();
			}
		}
	}
}

void FLuaScriptDelegate::RemoveHandle(FLuaDelegateHandle handle)
{
	if (this->GetHandle() == handle)
	{
		this->Clear();
	}
}

void FLuaScriptDelegate::Clear()
{
	this->Callback.Emplace<std::nullptr_t>();
}

FLuaDelegateHandle FLuaScriptDelegate::GetHandle() const
{
	switch (this->Callback.GetIndex())
	{
	case FLuaScriptDelegate::IndexOfCallbackType<FLuaDelegateUObjectCallback>():
		{
			return this->GetCallbackType<FLuaDelegateUObjectCallback>().DelegateHandle;
		}
	case FLuaScriptDelegate::IndexOfCallbackType<FLuaDelegateTableCallback>():
		{
			return this->GetCallbackType<FLuaDelegateTableCallback>().DelegateHandle;
		}
	case FLuaScriptDelegate::IndexOfCallbackType<FLuaDelegateFunctionCallback>():
		{
			return this->GetCallbackType<FLuaDelegateFunctionCallback>().DelegateHandle;
		}
	default:
		return {};
	}	
}

int64 FLuaScriptDelegate::GetHandleAsInteger() const
{
	return this->GetHandle().ToInteger();
}

void FLuaScriptDelegate::FDLuaDelegate_DelegateWrapper(const FScriptDelegate& del, TArray<FLuaValue> values)
{
	const UObject* obj = del.GetUObject();
	UFunction* func = obj->FindFunction(del.GetFunctionName());
	if(func)
	{
		func->ProcessEvent(func, &values);
	}
}

