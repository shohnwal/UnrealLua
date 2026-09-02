#pragma once
#include "CoreMinimal.h"
#include "LuaGCObject.h"
#include "sol/sol.hpp"
#include "UObject/ObjectPtr.h"

struct FLuaValue;
struct FUStructPropertyMapping;
struct FHashedFieldMapping;

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

struct UNREALLUA_API FLuaScriptStructBase
{
	FLuaScriptStructBase();
	FLuaScriptStructBase(const FLuaScriptStructBase& other);
	FLuaScriptStructBase(FLuaScriptStructBase&& other) noexcept;
	FLuaScriptStructBase(const UScriptStruct* metaStruct);
	virtual ~FLuaScriptStructBase();
	virtual void* GetMemory() const;
	virtual sol::object Lua_Copy(sol::this_state) const;
	virtual const UScriptStruct* GetScriptStruct() const;
	virtual bool IsReference() const;
	/** Returns True if the struct is valid.*/
	//bool IsValid() const;

	void UpdatePropertyMapping(const UScriptStruct* ss);
	
	FLuaScriptStructBase& operator=(const FLuaScriptStructBase& other)
	{
		this->PropertyMapping = other.PropertyMapping;
		return *this;
	}
	
	virtual void AddReferencedObjects(FReferenceCollector& Collector);
	virtual bool HasUObjectReferences();

	bool __le(sol::object other, sol::this_state lua) const;

	bool IsValid() const;
	FUStructPropertyMapping* PropertyMapping = nullptr;
	//TArray<FLuaValue>* UserValues = nullptr;
};

