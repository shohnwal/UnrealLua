// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "sol/sol.hpp"
#include "UObject/SoftObjectPtr.h"


class UNREALLUA_API FLuaSoftObjectWrapper
{
public:
	static void RegisterUsertype(sol::state_view& lua);
	
	FLuaSoftObjectWrapper();
	explicit FLuaSoftObjectWrapper(UObject* obj);
	explicit FLuaSoftObjectWrapper(TObjectPtr<UObject> obj);
	explicit FLuaSoftObjectWrapper(FSoftObjectPtr obj);
	explicit FLuaSoftObjectWrapper(const FSoftObjectPtr* obj);
	explicit FLuaSoftObjectWrapper(FSoftObjectPtr* obj);
	explicit FLuaSoftObjectWrapper(const UObject* obj);
	explicit FLuaSoftObjectWrapper(sol::object obj);
	//explicit FLuaSoftObjectWrapper(const FLuaUObjectWrapper& other);
	explicit FLuaSoftObjectWrapper(const FLuaSoftObjectWrapper& other);
	explicit FLuaSoftObjectWrapper(FLuaSoftObjectWrapper&& other) noexcept;
	FLuaSoftObjectWrapper& operator=(const FLuaSoftObjectWrapper& other);
	virtual ~FLuaSoftObjectWrapper();
		
	//UObject* operator()(sol::object obj);
	bool Valid() const;

	UObject* Get() const;
	void Set(const FSoftObjectPtr& Ptr);

	static int __index(lua_State* lua);
	static void __newindex(FLuaSoftObjectWrapper* self, sol::stack_object key, sol::stack_object value, sol::this_state lua);

	/*
	static sol::object __index(FLuaSoftObjectWrapper* self, sol::object key, sol::this_state lua);
	static void __newindex(FLuaSoftObjectWrapper* self, sol::object key, sol::object value, sol::this_state lua);
	*/

	bool operator==(const FLuaSoftObjectWrapper& other) const { return this->Ptr == other.Ptr; }

	FSoftObjectPtr Ptr{};

};
