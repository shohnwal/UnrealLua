// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interface/LuaScriptStructBase.h"
#include "sol/sol.hpp"

class UBlueprintFunctionLibrary;
class FLuaUStruct;


struct UNREALLUA_API FLuaScriptStruct : public FLuaScriptStructBase
{
	static void RegisterUsertype(sol::state_view& lua);
	
	FLuaScriptStruct();

	FLuaScriptStruct(const UScriptStruct* metaStruct);
	//Used by Lua-imported UStruct (FUStruct) call-operator to construct a new FLuaScriptStruct 
	FLuaScriptStruct(const FLuaUStruct* metaData, sol::variadic_args args);

	//Copy constructor
	FLuaScriptStruct(const FLuaScriptStruct& other);

	explicit FLuaScriptStruct(FLuaScriptStruct&& other) noexcept;

	/*
	 * Reference constructor
	 */
	FLuaScriptStruct(const UScriptStruct* metaStruct, void* otherMemory, bool asReference = false, bool isConst = false);
	FLuaScriptStruct(const UScriptStruct* metaStruct, const void* otherMemory, bool asReference = false);

	FLuaScriptStruct(FStructProperty* prop, const void* sourcePtr);

	virtual ~FLuaScriptStruct() override;

	static sol::object MakeFromPath(const std::string& path, sol::this_state lua);

	bool operator==(const FLuaScriptStruct& other) const
	{
		 return this->GetScriptStruct() == other.GetScriptStruct() && this->GetMemory() == other.GetMemory();
	}
	FLuaScriptStruct& operator=(const FLuaScriptStruct & other)
	{
		if(this != &other)
		{
			this->Reset();
			this->Data = other.Data;
			this->bOwnsMemory = other.OwnsMemory();
			this->PropertyMapping = other.PropertyMapping;
			if(this->OwnsMemory())
			{
				this->AddRef();
			}			
		}
		return *this;
	}

	void Reset();

	static int __index(lua_State* lua);
	static bool __newindex(FLuaScriptStruct* strct, sol::stack_object key, sol::stack_object value, sol::this_state lua);

	
	static bool __equals(FLuaScriptStruct* me, FLuaScriptStruct* other);
	static std::string __toString(FLuaScriptStruct* me);

	FString ToLuaSyntaxValueString() const;

	virtual sol::object Lua_Copy(sol::this_state lua) const override;
	FLuaScriptStruct MakeCopy() const;
	sol::variadic_results GetPropertyValues(sol::variadic_args propNames);
	void SetPropertyValues(sol::table tbl);

	template<typename T>
	static FLuaScriptStruct AsRef(T* fstruct)
	{
		return FLuaScriptStruct(fstruct->StaticStruct(), fstruct, true);
	}

	void* GetMemoryNonVirtual() const;
	virtual void* GetMemory() const override;
	virtual bool IsReference() const override;

	//virtual void SetThisPropertyReference(UObject* owner, const sol::object& value, sol::this_state lua) override;
	//void CopyFrom(const UScriptStruct* otherSS, void* memory);

	void AddRef();
	int32 RemoveRef();
	bool OwnsMemory() const;

	virtual const UScriptStruct* GetScriptStruct() const override;

	union
	{
		void* Data = nullptr;
		FLuaScriptStructMemory* LuaMemory;
	};
	bool bOwnsMemory = false;
	//bool bIsConst = false;
};

/** type traits to cover the custom aspects of a script struct **/

template<>
struct TStructOpsTypeTraits< FLuaScriptStruct > : public TStructOpsTypeTraitsBase2<FLuaScriptStruct>
{
	enum
	{
		WithCopy                       = !TIsPODType<FLuaScriptStruct>::Value, // struct can be copied via its copy assignment operator.
	};
};
