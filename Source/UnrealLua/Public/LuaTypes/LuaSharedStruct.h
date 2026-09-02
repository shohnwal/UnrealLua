// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "sol/sol.hpp"
#include "Interface/LuaScriptStructBase.h"
#include "StructUtils/SharedStruct.h"

class FLuaUStruct;
struct FLuaScriptStruct;
struct FSharedStruct;
/**
 * 
 */
struct UNREALLUA_API FLuaSharedStruct : public FLuaScriptStructBase
{
	static void RegisterUsertype(sol::state_view& pairs);
	
	FLuaSharedStruct();
	/*
	: FLuaScriptStructBase(nullptr), SharedStruct()
	{
		checkNoEntry();
	}
	*/
	FLuaSharedStruct(UScriptStruct* meta);

	explicit FLuaSharedStruct(const FLuaScriptStruct& dataStruct);

	FLuaSharedStruct(const FSharedStruct& instance);

	FLuaSharedStruct(const FSharedStruct* instance);

	FLuaSharedStruct(const FLuaSharedStruct* other);


	FLuaSharedStruct(const FLuaSharedStruct& other);

	FLuaSharedStruct(FLuaSharedStruct&& other) noexcept;

	bool operator==(const FLuaSharedStruct& other) const
	{
		return this->GetScriptStruct() == other.GetScriptStruct() && this->GetMemory() == other.GetMemory();
	}

	virtual ~FLuaSharedStruct() override;

	static sol::object MakeFromPath(sol::string_view path, sol::this_state lua);
	static sol::object MakeFromMetaStruct(const FLuaUStruct& metaStruct, sol::this_state lua);
	static sol::object MakeFromDataStruct(const FLuaScriptStruct& dataStruct, sol::this_state lua);

	static sol::object __index(FLuaSharedStruct* strct, sol::stack_object key, sol::this_state lua);
	static bool __newindex(FLuaSharedStruct* strct, sol::stack_object key, sol::stack_object value, sol::this_state lua);
	static bool __equals(FLuaSharedStruct* me, FLuaSharedStruct* other);

	virtual void* GetMemory() const override { return this->SharedStruct.GetMemory();}
	bool InitializeAs(sol::object structType);
	virtual bool IsReference() const override;
	//virtual void SetThisPropertyReference(UObject* owner, const sol::object& value, sol::this_state lua) override;
	void CopyFrom(const UScriptStruct* ss, void* memory);
	virtual const UScriptStruct* GetScriptStruct() const override;
	virtual sol::object Lua_Copy(sol::this_state) const override;
	FLuaSharedStruct Copy() const;

	FSharedStruct SharedStruct = {};
};
