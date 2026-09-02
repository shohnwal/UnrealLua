// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "sol/sol.hpp"
#include "UObject/ObjectPtr.h"
#include "LuaEnum.generated.h"
/**
 * 
 */
struct UNREALLUA_API FLuaUEnumEntry// : public FLuaGCObject
{
	FLuaUEnumEntry(){}
	FLuaUEnumEntry(TObjectPtr<UEnum> uenum, int64 val)
		: uenum(uenum), Value(val)
	{}

	~FLuaUEnumEntry()// override
	{
		uenum = nullptr;
		Value = -1;
	}
	
	TObjectPtr<UEnum> uenum = nullptr;
	int64 Value = -1;

	bool operator==(const FLuaUEnumEntry& other) const
	{
		return this->uenum == other.uenum && this->Value == other.Value;
	}
	bool __equals(const sol::stack_object& other) const;
	std::string ToString() const;
	int __toString(lua_State* L);
	int64 __toNumber(sol::this_state luaState) const;
	void AddReferencedObjects(FReferenceCollector& collector);// override;
	bool IsValid() const;
};

USTRUCT()
struct UNREALLUA_API FLuaUEnumMapping
{
	GENERATED_BODY()
	static void RegisterUsertype(sol::state_view& lua);
	
	FLuaUEnumMapping();
	FLuaUEnumMapping(const FLuaUEnumMapping& other);
	FLuaUEnumMapping(FLuaUEnumMapping&& other) noexcept;
	explicit FLuaUEnumMapping(UEnum* e);

	~FLuaUEnumMapping()
	{
		this->Entries.Empty();
		this->Uenum = nullptr;
	}
	
	FLuaUEnumMapping& operator=(const FLuaUEnumMapping& other)
	{
		this->SetEnum(other.GetEnum());
		return *this;
	}

	FString GetName();
	int64 operator[](sol::object obj);

	int __index(const sol::stack_object& Key);
	sol::object __Index(sol::object nameStr, sol::this_state lua);

	FLuaUEnumEntry* GetEnumEntryByNumberValue(int64 enumValue);
	sol::object GetEnumEntryLuaObjectByNumberValue(int64 enumValue, sol::this_state lua);
	int PushEnumEntryByNumberValue(int64 value, sol::this_state lua);

	FLuaUEnumEntry* GetEnumEntryByStringValue(std::string_view enumString);
	sol::object GetEnumEntryLuaObjectByStringValue(std::string_view enumString, sol::this_state lua);
	int PushEnumEntryByStringValue(std::string_view enumString, sol::this_state lua);

	sol::object GetEnumEntryLuaObjectByIndex(int32 index, sol::this_state lua);

	void AddReferencedObjects(FReferenceCollector& Collector);
	void __newIndex(sol::object key,sol::object value);
	sol::object __toString(sol::this_state lua);
	int __tostring(sol::this_state lua);
	sol::object GetThisPropertyReference(UObject* owner, sol::this_state lua) const;
	
	void SetEnum(UEnum* enum_);
	UEnum* GetEnum() const { return this->Uenum;}
	bool IsValid() const;

private:
	int32 GetEnumIndexByNumberValueInternal(int64 enumValue);
	int32 GetEnumIndexByStringValueInternal(std::string_view enumString);
	sol::object GetEnumEntryLuaObjectByIndexInternal(int32 index, sol::this_state lua);

	UPROPERTY(meta=(AllowPrivateAccess))
	TObjectPtr<UEnum> Uenum;
	TArray<FLuaUEnumEntry> Entries;
};

