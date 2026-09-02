#include "LuaTypes/LuaMulticastDelegate.h"
#include "LuaCoreDelegates.h"
#include "UnrealLua.h"
#include "LuaContext/ScopedLuaContext.h"

static const FDelegateHandle fLuaMulticastDelegateLuaTypeHandle = FLuaCoreDelegates::OnRegisterLuaUsertypes.AddStatic(&FLuaScriptMulticastDelegate::RegisterUsertype);

void FLuaScriptMulticastDelegate::RegisterUsertype(sol::state_view& lua)
{
	sol::usertype<FLuaScriptMulticastDelegate> mcut = lua.new_usertype<FLuaScriptMulticastDelegate>(
		"MulticastDelegate",
		"new", sol::no_constructor,
		sol::call_constructor, [](sol::this_state lua) ->sol::object
		{
			return sol::object(lua, sol::in_place_type<FLuaScriptMulticastDelegate>, FLuaScriptMulticastDelegate());
		},
		sol::meta_function::call, &FLuaScriptMulticastDelegate::Lua_Execute,
		"Add", &FLuaScriptMulticastDelegate::Lua_Add,
		"Remove", &FLuaScriptMulticastDelegate::Lua_Remove,
		"Clear", &FLuaScriptMulticastDelegate::Clear,
		"Execute", &FLuaScriptMulticastDelegate::Lua_Execute,
		"Broadcast", &FLuaScriptMulticastDelegate::Lua_Execute
	);
}


FLuaScriptMulticastDelegate::FLuaScriptMulticastDelegate(sol::variadic_args args)
{
	for(sol::object obj : args)
	{
		sol::type type = obj.get_type();
		if (type == sol::type::userdata)
		{
			if (obj.is<FLuaPrimitiveCPPType>())
			{
				FLuaPrimitiveCPPType cppType = obj.as<FLuaPrimitiveCPPType>();
				//this->Signature.AddDefaulted_GetRef().Emplace<FLuaPrimitiveCPPType>(cppType);
			}
		}
	}
}

void FLuaScriptMulticastDelegate::Lua_Execute(sol::object self, sol::variadic_args args, sol::this_state lua)
{
}

void FLuaScriptMulticastDelegate::Execute(const TArray<FLuaValue>& args) const
{
	/* Verify that the user object is still valid.  We only have a weak reference to it. */

	TArray<FLuaScriptDelegate> ListCopy = this->Callbacks;
	for(FLuaScriptDelegate& del : ListCopy)
	{
		if(!del.IsBound())
		{
			const_cast<FLuaScriptMulticastDelegate*>(this)->Callbacks.Remove(del);
			continue;
		}
		(void)del.Execute(args);
	}
}

void FLuaScriptMulticastDelegate::Clear()
{
	this->Callbacks.Empty();
}

int64 FLuaScriptMulticastDelegate::Lua_Add(sol::stack_object target, sol::stack_object funcName, sol::variadic_args captureArgs, sol::this_state lua)
{
	FLuaScriptDelegate del{};
	del.Lua_Add(target, funcName, captureArgs, lua);
	if (del.IsBound())
	{
		this->Callbacks.Emplace(del);
		return del.GetHandleAsInteger();
	}
	return 0;
}

FLuaDelegateHandle FLuaScriptMulticastDelegate::AddDynamicListener(FLuaDelegate del)
{
	if (del.IsBound())
	{
		FLuaDelegateHandle handle = FLuaDelegateHandle::MakeHandle();
		this->Callbacks.Emplace(del.Object.Get(), del.CallbackFunctionName, handle);
		return handle;
	}
	return {};
}

FLuaDelegateHandle FLuaScriptMulticastDelegate::AddLuaScriptListener(UObject* listener, const std::string funcName)
{
	if (IsValid(listener) && !funcName.empty())
	{
		FLuaDelegateHandle handle = FLuaDelegateHandle::MakeHandle();
		this->Callbacks.Emplace(listener, funcName, handle);
		return handle;
	}
	return {};
}

void FLuaScriptMulticastDelegate::RemoveDynamicListener(const FLuaDelegate& del)
{
	if (del.Object.IsValid())
	{
		if (del.CallbackFunctionName.IsEmpty())
		{
			this->Remove(del.Object.Get());
			return;
		}
		else
		{
			this->Remove(del.Object.Get(), del.CallbackFunctionName);
			return;
		}
	}
}

void FLuaScriptMulticastDelegate::RemoveHandle(FLuaDelegateHandle handle)
{
	if (handle.IsBound())
	{
		int32 index = this->Callbacks.IndexOfByPredicate([handle](const FLuaScriptDelegate& item)
		{
			return item.GetHandleAsInteger() == handle.ToInteger();
		});
		if (index != INDEX_NONE)
		{
			this->Callbacks.RemoveAtSwap(index);
		}
	}
}

void FLuaScriptMulticastDelegate::Lua_Remove(sol::stack_object target, sol::stack_object funcName, sol::this_state lua)
{
	sol::type targetType = target.get_type();
	if (targetType == sol::type::number)
	{
		if (target.is<int>())
		{
			const int64 handle = target.as<int64>();
			this->Remove(handle);
			return;
		}
		return;
	}
	else if (targetType == sol::type::lightuserdata)
	{
		UObject* obj = UnrealLua::GetUObject(target);
		if (!obj)
		{
			return;
		}
		if (!funcName.valid())
		{
			this->Remove(obj);
			return;
		}	
		else if (funcName.get_type() == sol::type::string)
		{
			const std::string_view strv = funcName.as<sol::string_view>();
			this->Remove(obj, strv);
			return;
		}
		return;
	}
	else if (targetType == sol::type::table)
	{
		sol::table tbl = target.as<sol::table>();
		if (!funcName.valid())
		{
			this->Remove(tbl);
			return;
		}	
		else if (funcName.get_type() == sol::type::string)
		{
			const std::string_view strv = funcName.as<sol::string_view>();
			this->Remove(tbl, strv);
			return;
		}
		return;
	}
	else if (targetType == sol::type::function)
	{
		sol::function func = target.as<sol::function>();
		this->Remove(func);
		return;
	}
	return;
}

void FLuaScriptMulticastDelegate::Remove(int64 handle)
{
	const int32 foundIndex = this->Callbacks.IndexOfByPredicate([handle](const FLuaScriptDelegate& item)
	{
		return item.GetHandleAsInteger() == handle;
	});
	if (foundIndex != INDEX_NONE)
	{
		this->Callbacks.RemoveAtSwap(foundIndex);
	}
	this->ClearInvalidEntries();
}

//remove all callbacks for an UObject
void FLuaScriptMulticastDelegate::Remove(UObject* obj)
{
	this->Callbacks.RemoveAllSwap([obj](const FLuaScriptDelegate& item)
	{
		if (item.IsCallbackType<FLuaDelegateUObjectCallback>())
		{
			const FLuaDelegateUObjectCallback& callbackItem = item.GetCallbackType<FLuaDelegateUObjectCallback>();
			if (callbackItem.Object == obj)
			{
				return true;
			}
		}		
		return false;
	});
	this->ClearInvalidEntries();
}

void FLuaScriptMulticastDelegate::Remove(UObject* obj, const std::string_view& funcName)
{
	const int32 foundIndex = this->Callbacks.IndexOfByPredicate([obj, &funcName](const FLuaScriptDelegate& item)
	{
		if (item.IsCallbackType<FLuaDelegateUObjectCallback>())
		{
			const FLuaDelegateUObjectCallback& callbackItem = item.GetCallbackType<FLuaDelegateUObjectCallback>();
			if (callbackItem.Object == obj && callbackItem.CallbackFunctionName.Matches(funcName))
			{
				return true;
			}
		}		
		return false;
	});
	if (foundIndex != INDEX_NONE)
	{
		this->Callbacks.RemoveAtSwap(foundIndex);
	}
	this->ClearInvalidEntries();
}

void FLuaScriptMulticastDelegate::Remove(UObject* obj, const FString& funcName)
{
	const int32 foundIndex = this->Callbacks.IndexOfByPredicate([obj, &funcName](const FLuaScriptDelegate& item)
	{
		if (item.IsCallbackType<FLuaDelegateUObjectCallback>())
		{
			const FLuaDelegateUObjectCallback& callbackItem = item.GetCallbackType<FLuaDelegateUObjectCallback>();
			if (callbackItem.Object == obj && callbackItem.CallbackFunctionName.Matches(funcName))
			{
				return true;
			}
		}		
		return false;
	});
	if (foundIndex != INDEX_NONE)
	{
		this->Callbacks.RemoveAtSwap(foundIndex);
	}
	this->ClearInvalidEntries();
}

//remove all callbacks for a table
void FLuaScriptMulticastDelegate::Remove(const sol::table& table)
{
	this->Callbacks.RemoveAllSwap([&table](const FLuaScriptDelegate& item)
	{
		if (item.IsCallbackType<FLuaDelegateTableCallback>())
		{
			const FLuaDelegateTableCallback& callbackItem = item.GetCallbackType<FLuaDelegateTableCallback>();
			if (callbackItem.TableHandle == table)
			{
				return true;
			}
		}		
		return false;
	});
	this->ClearInvalidEntries();
}

void FLuaScriptMulticastDelegate::Remove(const sol::table& table, const std::string_view& funcName)
{	
	const int32 foundIndex = this->Callbacks.IndexOfByPredicate([&table, &funcName](const FLuaScriptDelegate& item)
	{
		if (item.IsCallbackType<FLuaDelegateTableCallback>())
		{
			const FLuaDelegateTableCallback& callbackItem = item.GetCallbackType<FLuaDelegateTableCallback>();
			if (callbackItem.TableHandle == table && callbackItem.CallbackFunctionName.Matches(funcName))
			{
				return true;
			}
		}		
		return false;
	});
	if (foundIndex != INDEX_NONE)
	{
		this->Callbacks.RemoveAtSwap(foundIndex);
	}
	this->ClearInvalidEntries();
}

void FLuaScriptMulticastDelegate::Remove(const sol::function& func)
{
	const int32 foundIndex = this->Callbacks.IndexOfByPredicate([&func](const FLuaScriptDelegate& item)
	{
		if (item.IsCallbackType<FLuaDelegateFunctionCallback>())
		{
			const FLuaDelegateFunctionCallback& callbackItem = item.GetCallbackType<FLuaDelegateFunctionCallback>();
			if (callbackItem.Callback == func)
			{
				return true;
			}
		}		
		return false;
	});
	if (foundIndex != INDEX_NONE)
	{
		this->Callbacks.RemoveAtSwap(foundIndex);
	}
	this->ClearInvalidEntries();
}

void FLuaScriptMulticastDelegate::ClearInvalidEntries()
{
	this->Callbacks.RemoveAllSwap([](const FLuaScriptDelegate& del)
	{
		return !del.IsBound();
	});
}