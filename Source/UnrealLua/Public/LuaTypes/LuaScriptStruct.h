// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interface/LuaScriptStructBase.h"
#include "Misc/TVariant.h"
#include "sol/sol.hpp"
#include "UObject/ObjectPtr.h"
#include "UObject/StructOpsTypeTraits.h"

class UBlueprintFunctionLibrary;
class FLuaUStruct;


struct UNREALLUA_API FLuaScriptStructMemory : public FLuaGCObject
{
	FLuaScriptStructMemory(const UScriptStruct* ss, const void* memToCopyFrom);
	virtual ~FLuaScriptStructMemory() override;

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	void AddRef();
	int32 RemoveRef();

	static FLuaScriptStructMemory* Allocate(const UScriptStruct* InScriptStruct, const void* memToCopyFrom);

	uint8* GetMemory() const;
	const UScriptStruct* GetScriptStruct() const;
	TObjectPtr<const UScriptStruct> ScriptStruct = nullptr;
	int32 RefCount = 0;
	uint8 Data[];
};

/*
 std::nullptr_t						- uninitialized
 void*								- native FStructProperty memory ptr reference
 FInstancedStruct*					- native FInstancedStruct property memory reference
 FSharedStruct*						- either native FSharedStruct property reference or a Lua-allocated shared struct, added a ref to the ref counter
 FLuaScriptStructMemory*			- Lua-allocated script struct memory
 FLuaInstancedScriptStructMemory*	- Lua-allocated instanced script struct
 */
typedef TVariant<std::nullptr_t, void*, FInstancedStruct*, FSharedStruct*, FLuaScriptStructMemory*, FLuaInstancedStructMemory*> FLuaScriptStructMemoryData;


struct UNREALLUA_API FLuaScriptStruct : public FLuaScriptStructBase
{
	enum EInstancedStruct {};
	enum ESharedStruct {};
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
	bool IsInitialized() const;

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
			this->MemoryVariant = other.MemoryVariant;
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

	TVariant<std::nullptr_t, void*, FLuaScriptStructMemory*> MemoryVariant = {};
};

static_assert(sizeof(FLuaScriptStruct) <= 32);

/** type traits to cover the custom aspects of a script struct **/

template<>
struct TStructOpsTypeTraits< FLuaScriptStruct > : public TStructOpsTypeTraitsBase2<FLuaScriptStruct>
{
	enum
	{
		WithCopy                       = !TIsPODType<FLuaScriptStruct>::Value, // struct can be copied via its copy assignment operator.
	};
};
