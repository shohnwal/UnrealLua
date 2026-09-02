// Fill out your copyright notice in the Description page of Project Settings.


#include "StringHandling/UnrealLuaStringCache.h"

#include "StringHandling/UnrealLuaStringEntry.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"
#include "Utility/UnrealLuaHash.h"

namespace UnrealLua::StringCache
{
    static TMultiMap<uint32, TUniquePtr<FUnrealLuaNameEntry>> NameEntryMap;
}

void UnrealLua::StringCache::Initialize()
{
	verify(!UUnrealLuaEngineSubsystem::IsGameSessionActive())
	NameEntryMap.Empty();
}

void UnrealLua::StringCache::CleanUp()
{
	verify(!UUnrealLuaEngineSubsystem::IsGameSessionActive())
	NameEntryMap.Empty();
}

FName UnrealLua::StringCache::GetFNameForStringView(const std::string_view& key)
{
	FUnrealLuaNameEntryKey keyItem = UnrealLua::StringCache::GetStringEntryKey(key);
	return keyItem.GetFName();
}

FUnrealLuaNameEntryKey UnrealLua::StringCache::GetStringEntryKey(const std::string_view& key)
{
	if (key.empty())
	{
		return FUnrealLuaNameEntryKey();
	}
	uint32 hash = UnrealLua::HashUtility::StrCrc32(key.data());
	for (TMultiMap<uint32, TUniquePtr<FUnrealLuaNameEntry>>::TConstKeyIterator it(NameEntryMap, hash); it; ++it)
	{
		FUnrealLuaNameEntry* entry = it->Value.Get();
		if (entry->Matches(key, hash))
		{
			return {entry, hash};
		}
	}
	
	FUnrealLuaNameEntry* newEntry = NameEntryMap.Emplace(hash, MakeUnique<FUnrealLuaNameEntry>(key, hash)).Get();
	return {newEntry, hash};
}

FUnrealLuaNameEntryKey UnrealLua::StringCache::GetStringEntryKey(FStringView key)
{
	uint32 hash = UnrealLua::HashUtility::StrCrc32(key.GetData());
	for (TMultiMap<uint32, TUniquePtr<FUnrealLuaNameEntry>>::TConstKeyIterator it(NameEntryMap, hash); it; ++it)
	{
		FUnrealLuaNameEntry* entry = it->Value.Get();
		if (entry->Matches(key, hash))
		{
			return {entry, hash};
		}
	}
	
	FUnrealLuaNameEntry* newEntry = NameEntryMap.Emplace(hash, MakeUnique<FUnrealLuaNameEntry>(key, hash)).Get();
	return {newEntry, hash};
}

FName UnrealLua::StringCache::GetFNameForStringLuaObject(const sol::object& obj)
{
	if(obj.get_type() != sol::type::string)
	{
		return NAME_None;
		
	}
	sol::string_view strv = obj.as<sol::string_view>();
	return GetFNameForStringView(strv);
}

FName UnrealLua::StringCache::GetFNameForStringLuaObject(const sol::stack_object& obj)
{
	if(obj.get_type() != sol::type::string)
	{
		return NAME_None;
		
	}
	sol::string_view strv = obj.as<sol::string_view>();
	return GetFNameForStringView(strv);
}