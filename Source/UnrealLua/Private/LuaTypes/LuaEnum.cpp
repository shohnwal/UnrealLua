// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaTypes/LuaEnum.h"

#include "LuaCoreDelegates.h"
#include "LuaTypes/LuaLightUserdata.h"
#include "Utility/LuaLogMacros.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"
#include "UObject/Class.h"
#include "StringHandling/UnrealLuaStringCache.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"
//static const FDelegateHandle fFuEnumLuaTypeHandle = FLuaCoreDelegates::OnRegisterLuaUsertypes.AddStatic(&FLuaUEnumMapping::RegisterUsertype);

void FLuaUEnumMapping::RegisterUsertype(sol::state_view& lua)
{
	checkNoEntry()
	lua.new_usertype<UEnum>(
		"UEnum",
		"new", sol::no_constructor
	);	
	lua.new_usertype<FLuaUEnumMapping>(
		"FLuaUEnum",
		"new", sol::no_constructor,
		sol::meta_function::index, &FLuaUEnumMapping::__Index,
		sol::meta_function::new_index, &FLuaUEnumMapping::__newIndex,
		sol::meta_function::to_string, &FLuaUEnumMapping::__toString
	);

	auto ut = lua.new_usertype<FLuaUEnumEntry>(
		"FLuaUEnumEntry",
		"new", sol::no_constructor,
		//sol::base_classes, sol::bases<FLuaGCObject>(),
		sol::meta_function::to_string, sol::c_call<decltype(&FLuaUEnumEntry::__toString), &FLuaUEnumEntry::__toString>,
		"ToString", sol::c_call<decltype(&FLuaUEnumEntry::__toString), &FLuaUEnumEntry::__toString>,
		sol::meta_function::length, &FLuaUEnumEntry::__toNumber,
		sol::meta_function::equal_to, &FLuaUEnumEntry::__equals, //&FLuaUEnumEntry::operator==,
		"ToNumber", &FLuaUEnumEntry::__toNumber,
		"Equals", &FLuaUEnumEntry::__equals, 
		"Is", &FLuaUEnumEntry::__equals,
		"Value", sol::readonly(&FLuaUEnumEntry::Value)
	);
}

FLuaUEnumMapping::FLuaUEnumMapping(UEnum* e): Uenum(e)
{
}

FLuaUEnumMapping::FLuaUEnumMapping(): Uenum(nullptr)
{
}

FLuaUEnumMapping::FLuaUEnumMapping(const FLuaUEnumMapping& other)
	: Uenum(nullptr)
{
	this->SetEnum(other.GetEnum());
}

FLuaUEnumMapping::FLuaUEnumMapping(FLuaUEnumMapping&& other) noexcept
	: Uenum()
{
	this->SetEnum(other.GetEnum());
}

FString FLuaUEnumMapping::GetName()
{
	return GetNameSafe(this->Uenum);
}


int64 FLuaUEnumMapping::operator[](sol::object obj)
{
	if(Uenum)
	{
		if (obj.get_type() != sol::type::string)
		{
			return 0;
		}
		FName name = UnrealLua::StringCache::GetFNameForStringLuaObject(obj);
		return Uenum->GetIndexByName(name, EGetByNameFlags::None);
	}
	return 0;
}

int FLuaUEnumMapping::__index(const sol::stack_object& name)
{
	if(!Uenum)
	{
		return 0;
	}
	lua_State* L = name.lua_state();
	sol::type type = name.get_type();
	if(type == sol::type::number)
	{
		int64 val = name.as<int64>();
		return this->PushEnumEntryByNumberValue(val, L);
	}
	else if(type == sol::type::string)
	{
		sol::string_view str = name.as<sol::string_view>();
		return this->PushEnumEntryByStringValue(str, L);
	}
	return 0;
}

sol::object FLuaUEnumMapping::__Index(sol::object name, sol::this_state lua)
{
	if(!Uenum)
	{
		return sol::nil;
	}
	sol::type type = name.get_type();
	if(type == sol::type::number)
	{
		int64 val = name.as<int64>();
		return this->GetEnumEntryLuaObjectByNumberValue(val, lua);
	}
	else if(type == sol::type::string)
	{
		sol::string_view str = name.as<sol::string_view>();
		return this->GetEnumEntryLuaObjectByStringValue(str, lua);
	}
	return sol::nil;
}

FLuaUEnumEntry* FLuaUEnumMapping::GetEnumEntryByNumberValue(int64 enumValue)
{
	if(!Uenum)
	{
		return nullptr;
	}
	int32 index = GetEnumIndexByNumberValueInternal(enumValue);
	if(index == INDEX_NONE)
	{
		return nullptr;
	}
	return &this->Entries[index];
}

int32 FLuaUEnumMapping::GetEnumIndexByNumberValueInternal(int64 enumValue)
{
	return Uenum->GetIndexByValue(enumValue); 
}

sol::object FLuaUEnumMapping::GetEnumEntryLuaObjectByNumberValue(int64 enumValue, sol::this_state lua)
{
	FLuaUEnumEntry* entry = this->GetEnumEntryByNumberValue(enumValue);
	return UnrealLua::LightUserdata::GetUEnumValueAsTaggedLightUserdata(entry, lua);}

int FLuaUEnumMapping::PushEnumEntryByNumberValue(int64 value, sol::this_state lua)
{
	FLuaUEnumEntry* entry = this->GetEnumEntryByNumberValue(value);
	return UnrealLua::LightUserdata::PushUEnumValueAsTaggedLightUserdata(entry, lua);
}


FLuaUEnumEntry* FLuaUEnumMapping::GetEnumEntryByStringValue(std::string_view enumString)
{
	if(!Uenum)
	{
		return nullptr;
	}
	int32 index = Uenum->GetIndexByNameString(enumString.data()); 
	if(index == INDEX_NONE)
	{
		return nullptr;
	}
	return &this->Entries[index];
}

int32 FLuaUEnumMapping::GetEnumIndexByStringValueInternal(std::string_view enumString)
{
	return Uenum->GetIndexByName(enumString.data());
}

sol::object FLuaUEnumMapping::GetEnumEntryLuaObjectByStringValue(std::string_view enumString, sol::this_state lua)
{
	FLuaUEnumEntry* entry = this->GetEnumEntryByStringValue(enumString);
	return UnrealLua::LightUserdata::GetUEnumValueAsTaggedLightUserdata(entry, lua);
}

int FLuaUEnumMapping::PushEnumEntryByStringValue(std::string_view enumString, sol::this_state lua)
{
	FLuaUEnumEntry* entry = this->GetEnumEntryByStringValue(enumString);
	return UnrealLua::LightUserdata::PushUEnumValueAsTaggedLightUserdata(entry, lua);
}

sol::object FLuaUEnumMapping::GetEnumEntryLuaObjectByIndex(int32 index, sol::this_state lua)
{
	if(index >= this->Entries.Num())
	{
		return sol::nil;
	}
	return this->GetEnumEntryLuaObjectByIndexInternal(index, lua);
}

void FLuaUEnumMapping::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(this->Uenum);
}

void FLuaUEnumMapping::__newIndex(sol::object key, sol::object value)
{
	LUA_LOG_ERROR("Can't write values to UEnum!")
}

sol::object FLuaUEnumMapping::__toString(sol::this_state lua)
{
	return sol::object(lua, sol::in_place_type<char*>, StringCast<char>(*GetNameSafe(this->Uenum)).Get());
}

int FLuaUEnumMapping::__tostring(sol::this_state lua)
{
	auto casted = StringCast<char>(*GetNameSafe(this->Uenum));
	return sol::stack::push<char*>(lua, casted.Get());
}

sol::object FLuaUEnumMapping::GetThisPropertyReference(UObject* owner, sol::this_state lua) const
{
	return UnrealLua::UObjectRegistry::GetEnumWrapperLuaObject(this->Uenum, lua);
}

void FLuaUEnumMapping::SetEnum(UEnum* uenum)
{
	this->Uenum = uenum;
	int64 maxVal = this->Uenum->GetMaxEnumValue();
	int64 maxValIndex = this->Uenum->GetIndexByValue(maxVal);
	if(maxValIndex == INDEX_NONE)
	{
		return;
	}
	for(int32 i = 0; i <= maxValIndex; i++)
	{
		int64 val = this->Uenum->GetValueByIndex(i);
		this->Entries.Emplace(this->Uenum, val);
	}
}


bool FLuaUEnumMapping::IsValid() const
{
	return ::IsValid(this->Uenum);
}

sol::object FLuaUEnumMapping::GetEnumEntryLuaObjectByIndexInternal(int32 index, sol::this_state lua)
{
	if(index == INDEX_NONE)
	{
		return sol::nil;
	}
	int64 value = this->Uenum->GetValueByIndex(index);
	
	return sol::object(lua, sol::in_place_type<FLuaUEnumEntry>, FLuaUEnumEntry{this->GetEnum(), value});
	//FLuaUEnumEntry entry = this->Entries[index];
	//FCPUCycleTimer timer{"GetEnumEntry"};
	//return sol::object(lua, sol::in_place_type<FLuaUEnumEntry>, entry);
}

/*
void FLuaUEnum::SetThisPropertyReference(UObject* owner, const sol::object& value, sol::this_state lua)
{
	LUA_LOG_WARNING("Attempting to set FLuaUEnum!")
}
*/


bool FLuaUEnumEntry::__equals(const sol::stack_object& other) const
{
	if(!uenum)
	{
		return false;
	}
	if(other.get_type() == sol::type::number)
	{
		int64 val = other.as<int64>();
		return this->Value == val;
	}
	else if(other.get_type() == sol::type::string)
	{
		sol::string_view strv = other.as<sol::string_view>();
		FString str = uenum->GetNameByValue(this->Value).ToString(); 
		return strcmp(StringCast<char>(*str).Get(), strv.data()) == 0;		
	}
	else if(other.is<FLuaUEnumEntry>())
	{
		return other.as<FLuaUEnumEntry&>() == *this;
	}
	return false;
}

std::string FLuaUEnumEntry::ToString() const
{
	if(uenum)
	{
		FString str = uenum->GetNameByValue(this->Value).ToString(); 
		return StringCast<char>(*str).Get();
	}
	return "Invalid UEnum Entry";	
}

int FLuaUEnumEntry::__toString(lua_State* L)
{
	if(uenum)
	{
		FString str = uenum->GetNameByValue(this->Value).ToString(); 
		auto casted = StringCast<char>(*str);
		return sol::stack::push<char*>(L, casted.Get());
	}
	return 0;
}

int64 FLuaUEnumEntry::__toNumber(sol::this_state luaState) const
{
	return this->Value;
}

void FLuaUEnumEntry::AddReferencedObjects(FReferenceCollector& collector)
{
	collector.AddReferencedObject(this->uenum);
}

bool FLuaUEnumEntry::IsValid() const
{
	return ::IsValid(this->uenum) && this->uenum->IsValidEnumValue(this->Value);
}
