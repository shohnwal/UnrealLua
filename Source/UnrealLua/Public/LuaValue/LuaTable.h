// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "sol/sol.hpp"
#include "LuaTable.generated.h"
struct FWeakLuaTableHandle;
struct FScopedLuaContext;
class ULuaScriptInstance;
class ULuaContext;
struct FLuaValue;
/**
 * 
 */
struct FLuaScriptInstance;


struct UNREALLUA_API FLuaTable
{
	FLuaTable() {}
	FLuaTable(sol::table tbl);
	void Invalidate();
	
	FLuaValue Index(const FLuaValue& key);
	void NewIndex(const FLuaValue& key, const FLuaValue& value);
    	
	sol::table Table = sol::nil;
};


//A class holding a Lua table and a reference to the Lua state
USTRUCT(BlueprintType)
struct UNREALLUA_API FLuaTableHandle
{
	GENERATED_BODY()
	FLuaTableHandle()
	{
	}
	
	explicit FLuaTableHandle(const TSharedPtr<FLuaTable>& tableSharedPtr);

	explicit FLuaTableHandle(FLuaTableHandle&& other) noexcept
		: LuaTableWrapper(other.LuaTableWrapper)
	{
		other.LuaTableWrapper = nullptr;
	}

	static FLuaTableHandle MakeHandle(const sol::table& tbl);

	FLuaTableHandle& operator=(const FLuaTableHandle& other)
	{
		this->LuaTableWrapper = other.LuaTableWrapper;
		return *this;
	}

	void Invalidate();
	bool operator==(const sol::table& table) const;
	bool operator==(const FLuaTableHandle& other) const
	{
		return this->GetTable() == other.GetTable();
	}

	FLuaTableHandle(const FLuaTableHandle& other)
		: LuaTableWrapper(other.LuaTableWrapper)
	{}

	bool IsValid() const;

	FLuaValue Index(const FLuaValue& key) const;
	void NewIndex(const FLuaValue& key, const FLuaValue& value) const;
	sol::table GetTable() const;
	TSharedPtr<FLuaTable> LuaTableWrapper = nullptr;
};


struct UNREALLUA_API FWeakLuaTableHandle
{
	FWeakLuaTableHandle(TSharedPtr<FLuaTable>& tblContainer);
	void Invalidate();
	
	TWeakPtr<FLuaTable> LuaTableContainer{};
};