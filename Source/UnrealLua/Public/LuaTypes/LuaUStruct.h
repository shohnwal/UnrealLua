// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "sol/sol.hpp"
#include "UObject/SoftObjectPtr.h"

/**
 * 
 */
class UNREALLUA_API FLuaUStruct
{
public:
	static void RegisterUsertype(sol::state_view& lua);

	FLuaUStruct();
	FLuaUStruct(UScriptStruct* metaData);
	FLuaUStruct(const UScriptStruct* metaData);
	explicit FLuaUStruct(FLuaUStruct* metaData);
	explicit FLuaUStruct(const FLuaUStruct* metaData);
	FLuaUStruct(const FLuaUStruct& metaData);
	explicit FLuaUStruct(FLuaUStruct&& metaData) noexcept;
	explicit FLuaUStruct(const FSoftObjectPath& path);
	~FLuaUStruct();// override;
	bool operator==(const FLuaUStruct& other) const
	{
		return this->ScriptStruct == other.ScriptStruct;
	}

	sol::object operator()(sol::variadic_args args, sol::this_state lua);
	UScriptStruct* TryLoad() const;
	FSoftObjectPath GetPath() const;
	std::string ToString();

private:
	TSharedPtr<FSoftObjectPath> ScriptStruct;
};