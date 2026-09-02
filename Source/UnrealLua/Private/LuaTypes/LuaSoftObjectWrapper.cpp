// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaTypes/LuaSoftObjectWrapper.h"

#include "LuaCoreDelegates.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"

static const FDelegateHandle fLuaSoftObjectLuaTypeHandle = FLuaCoreDelegates::OnRegisterLuaUsertypes.AddStatic(&FLuaSoftObjectWrapper::RegisterUsertype);

void FLuaSoftObjectWrapper::RegisterUsertype(sol::state_view& lua)
{
	lua.new_usertype<FLuaSoftObjectWrapper>(
		"FSoftObjectPtr",
		"IsValid", &FLuaSoftObjectWrapper::Valid,
		"Get", &FLuaSoftObjectWrapper::Get,
		sol::meta_function::index, &FLuaSoftObjectWrapper::__index,
		sol::meta_function::new_index, &FLuaSoftObjectWrapper::__newindex,
		sol::meta_function::call, &FLuaSoftObjectWrapper::Get,
		sol::meta_function::to_string, [](FLuaSoftObjectWrapper* self){ return std::string{StringCast<char>(*("SoftObject: " + self->Ptr.ToString())).Get()}; }
	);
}

FLuaSoftObjectWrapper::FLuaSoftObjectWrapper()
	:Ptr(nullptr)
{
}

FLuaSoftObjectWrapper::FLuaSoftObjectWrapper(TObjectPtr<UObject> obj)
	: Ptr(obj.Get())
{
}

FLuaSoftObjectWrapper::FLuaSoftObjectWrapper(UObject* obj)
	: Ptr(obj)
{
	
}

FLuaSoftObjectWrapper::FLuaSoftObjectWrapper(FSoftObjectPtr obj)
	: Ptr(obj)
{
}

FLuaSoftObjectWrapper::FLuaSoftObjectWrapper(const FSoftObjectPtr* obj)
	: Ptr(*obj)
{
}

FLuaSoftObjectWrapper::FLuaSoftObjectWrapper(FSoftObjectPtr* obj)
	: Ptr(*obj)
{
}

FLuaSoftObjectWrapper::FLuaSoftObjectWrapper(const UObject* obj)
	: Ptr(obj)
{
}

FLuaSoftObjectWrapper::FLuaSoftObjectWrapper(sol::object obj)
{
	if(obj.get_type() == sol::type::lightuserdata)
	{
		UObject* uobj = UnrealLua::LightUserdata::GetUObject(obj);
		if(IsValid(uobj))
		{
			this->Ptr = uobj;
		}
		else
		{
			this->Ptr = nullptr;
		}
	}
	else if(obj.is<FLuaSoftObjectWrapper>())
	{
		this->Ptr = obj.as<FLuaSoftObjectWrapper&>().Get();
	}
}
/*
FLuaSoftObjectWrapper::FLuaSoftObjectWrapper(const FLuaUObjectWrapper& other)
	: Ptr(nullptr)
{
	FLuaUObjectItem* item = const_cast<FLuaUObjectWrapper*>(&other)->GetItemPtr();
	if(item && IsValid(item->Object))
	{
		this->Ptr = FSoftObjectPath{ item->Object };	
	}
	else
	{
		this->Ptr = nullptr;
	}
}
*/

FLuaSoftObjectWrapper::FLuaSoftObjectWrapper(const FLuaSoftObjectWrapper& other)
	:Ptr(other.Ptr)
{
}

FLuaSoftObjectWrapper::FLuaSoftObjectWrapper(FLuaSoftObjectWrapper&& other) noexcept
	:Ptr(other.Ptr)
{
	other.Ptr = nullptr;
}

FLuaSoftObjectWrapper& FLuaSoftObjectWrapper::operator=(const FLuaSoftObjectWrapper& other)
{
	this->Ptr = other.Ptr;
	return *this;
}

FLuaSoftObjectWrapper::~FLuaSoftObjectWrapper()
{
	this->Ptr = nullptr;
}

bool FLuaSoftObjectWrapper::Valid() const
{
	return !this->Ptr.IsNull();
}

UObject* FLuaSoftObjectWrapper::Get() const
{
	UObject* obj = this->Ptr.LoadSynchronous();
	return obj;
}

void FLuaSoftObjectWrapper::Set(const FSoftObjectPtr& ptr)
{
	this->Ptr = ptr;
}

int FLuaSoftObjectWrapper::__index(lua_State* lua)
{
	sol::stack_object self{lua,1};
	sol::stack_object key{lua,2};
	if(!key.valid())
	{
		return sol::stack::push(lua, sol::nil);
	}

	UObject* obj = self.as<FLuaSoftObjectWrapper>().Ptr.LoadSynchronous();
	if (!IsValid(obj))
	{
		return 0;
	}
	FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(obj);
	return item.__index(key);	
}

void FLuaSoftObjectWrapper::__newindex(FLuaSoftObjectWrapper* self, sol::stack_object key, sol::stack_object value, sol::this_state lua)
{
	UObject* obj = self->Ptr.LoadSynchronous();
	if (!IsValid(obj))
	{
		return;
	}
	FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(obj);
	item.__newindex(key, value, lua);
}