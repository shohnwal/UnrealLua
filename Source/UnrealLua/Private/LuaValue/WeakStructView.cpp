#include "BlueprintSupport/WeakStructView.h"

#include "LuaTypes/LuaScriptStruct.h"
#include "Reflection/PropertyHelper.h"
#include "LuaTypes/LuaSharedStruct.h"
#include "LuaTypes/LuaInstancedStruct.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "BlueprintSupport/WeakSharedStruct.h"

FWeakStructView::FWeakStructView()
{
}

FWeakStructView::FWeakStructView(const FWeakStructView& other)
{
	this->ScriptStruct = other.ScriptStruct;
	this->StructMemory = other.StructMemory;
}

FWeakStructView::FWeakStructView(FWeakStructView&& other) noexcept
{
	this->ScriptStruct = other.ScriptStruct;
	this->StructMemory = other.StructMemory;

	other.ScriptStruct = nullptr;
	other.StructMemory = nullptr;
}

FWeakStructView::~FWeakStructView()
{
}

FWeakStructView::FWeakStructView(const FWeakSharedStruct& SharedStruct)
	: FWeakStructView(SharedStruct.GetScriptStruct(), SharedStruct.GetMemory())
{
	
}

bool FWeakStructView::IsValid() const
{
	return this->GetScriptStruct() != nullptr && this->GetMemory() != nullptr;
}

sol::object FWeakStructView::__index(sol::stack_object key, sol::this_state lua)
{
	if(!this->IsValid())
	{
		return sol::nil;
	}
	FLuaScriptStruct ss{this->GetScriptStruct(), this->GetMemory(), true};
	return UnrealLua::PropertyHelper::GetValueFromScriptStructProperty(key, ss, lua);
}

void FWeakStructView::__newindex(sol::stack_object key, sol::stack_object value, sol::this_state lua)
{
	if(!this->IsValid())
	{
		return;
	}
	FLuaScriptStruct ss{this->GetScriptStruct(), this->GetMemory(), true};
	UnrealLua::PropertyHelper::SetValueInScriptStructProperty(key, ss, value);
}

sol::object FWeakStructView::Lua_Copy(sol::this_state lua) const
{
	if(!this->IsValid())
	{
		return sol::nil;
	}
	return sol::object(lua, sol::in_place_type<FLuaScriptStruct>, this->GetScriptStruct(), this->GetMemory(), false);
}

sol::object FWeakStructView::MakeSharedStruct(sol::this_state lua)
{
	if(!this->IsValid())
	{
		return sol::nil;
	}
	FSharedStruct ss = FSharedStruct::Make(this->GetScriptStruct(), this->GetMemory());
	return sol::object(lua, sol::in_place_type<FLuaSharedStruct>, ss);
}

sol::object FWeakStructView::MakeInstancedStruct(sol::this_state lua)
{
	if(!this->IsValid())
	{
		return sol::nil;
	}
	FLuaInstancedStruct instance{};
	instance.CopyFrom(this->GetScriptStruct(), this->GetMemory());
	return sol::object(lua, sol::in_place_type<FLuaInstancedStruct>, instance);
}

uint8* FWeakStructView::GetMemory() const
{
	return this->StructMemory;
}
