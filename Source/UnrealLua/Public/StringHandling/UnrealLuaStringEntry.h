#pragma once
#include "CoreMinimal.h"
#include <string>

#include "Utility/UnrealLuaHash.h"

namespace UnrealLua::StringCache
{
	struct UNREALLUA_API FUnrealLuaNameEntry
	{
		explicit FUnrealLuaNameEntry(const std::string_view strv, uint64 hash)
			: String(strv.data()), UnrealString(strv.data()), StringHash(hash)
		{}
		explicit FUnrealLuaNameEntry(const FStringView strv, uint64 hash)
			: String(), UnrealString(strv.GetData()), StringHash(hash)
		{
			String = StringCast<char>(strv.GetData()).Get();
		}
		std::string String = "";
		FString UnrealString = "";
		FName UnrealName = NAME_None;
		uint32 StringHash = 0;
		
		FName GetFName();
		bool Matches(std::string_view strv) const;
		bool Matches(std::string_view strv, uint32 hash) const;
		bool Matches(FStringView strv) const;
		bool Matches(FStringView strv, uint32 hash) const;
	};

	inline FName FUnrealLuaNameEntry::GetFName()
	{
		if (this->UnrealName == NAME_None)
		{
			this->UnrealName = *this->UnrealString;
		}
		return this->UnrealName;
	}

	inline bool FUnrealLuaNameEntry::Matches(std::string_view strv) const
	{
		uint32 hash = UnrealLua::HashUtility::StrCrc64(strv.data());
		return this->Matches(strv, hash);
	}

	inline bool FUnrealLuaNameEntry::Matches(std::string_view strv, uint32 hash) const
	{
		return this->StringHash == hash && strv == this->String;
	}

	inline bool FUnrealLuaNameEntry::Matches(FStringView strv) const
	{
		uint32 hash = UnrealLua::HashUtility::StrCrc64(strv.GetData());
		return this->Matches(strv, hash);
	}

	inline bool FUnrealLuaNameEntry::Matches(FStringView strv, uint32 hash) const
	{
		return this->StringHash == hash && strv == this->UnrealString;
	}
}