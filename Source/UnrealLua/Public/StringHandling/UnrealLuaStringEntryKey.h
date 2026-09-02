#pragma once
#include "CoreTypes.h"
#include "UnrealLuaStringEntry.h"
#include "UnrealLuaStringEntryKey.generated.h"

namespace UnrealLua::StringCache
{
	struct FUnrealLuaNameEntry;
}
USTRUCT()
struct UNREALLUA_API FUnrealLuaNameEntryKey
{
	GENERATED_BODY()
	UnrealLua::StringCache::FUnrealLuaNameEntry* Entry = nullptr;
	uint32 CachedHash = 0;
	bool Matches(const FName key) const;
	bool Matches(const FString& Key) const;
	bool Matches(const std::string_view& key) const;
	bool Matches(const std::string_view& key, uint32 Hash) const;
	bool Matches(FStringView& Key, uint32 Hash) const;
	uint32 GetKeyHash() const;
	std::string_view GetKeyStringView() const;
	FName GetFName() const;
	bool IsValid() const;
	bool operator==(const std::string_view& strv) const;
	bool operator==(const FUnrealLuaNameEntryKey& other) const
	{
		return other.Entry == this->Entry;
	}
};

inline bool FUnrealLuaNameEntryKey::Matches(FStringView& key, uint32 hash) const
{
	return CachedHash == hash && Entry->Matches(key, hash);
}

inline uint32 FUnrealLuaNameEntryKey::GetKeyHash() const
{
	return this->CachedHash;
}

inline std::string_view FUnrealLuaNameEntryKey::GetKeyStringView() const
{
	verify(this->Entry != nullptr);
	return this->Entry->String.data();
}

inline FName FUnrealLuaNameEntryKey::GetFName() const
{
	return this->Entry ? this->Entry->GetFName() : NAME_None;
}

inline bool FUnrealLuaNameEntryKey::IsValid() const
{
	return this->Entry != nullptr;
}

inline bool FUnrealLuaNameEntryKey::operator==(const std::string_view& strv) const
{
	uint32 hash = UnrealLua::HashUtility::StrCrc32(strv.data(), strv.size());
	return this->Matches(strv, hash);
}

inline bool FUnrealLuaNameEntryKey::Matches(const FName key) const
{
	return this->GetFName() == key;
}

inline bool FUnrealLuaNameEntryKey::Matches(const FString& key) const
{
	return this->Entry->Matches(*key);
}

inline bool FUnrealLuaNameEntryKey::Matches(const std::string_view& strv) const
{
	return *this == strv;
}

inline bool FUnrealLuaNameEntryKey::Matches(const std::string_view& key, uint32 hash) const
{
	return CachedHash == hash && Entry->Matches(key, hash);
}
