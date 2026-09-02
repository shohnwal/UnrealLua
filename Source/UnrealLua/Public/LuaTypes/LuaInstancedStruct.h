#pragma once
#include "LuaScriptStruct.h"
#include "Misc/TVariant.h"
#include "sol/sol.hpp"
#include "StructUtils/InstancedStruct.h"
//#include "LuaInstancedStruct.generated.h"

struct FInstancedStruct;

struct UNREALLUA_API FLuaInstancedStructMemory : public FLuaGCObject
{
	FLuaInstancedStructMemory();
	FLuaInstancedStructMemory(const UScriptStruct* ss, const void* memToCopyFrom);
	virtual ~FLuaInstancedStructMemory() override;

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	void AddRef();
	int32 RemoveRef();
	void* GetMutableMemory();
	const UScriptStruct* GetScriptStruct();
	int32 RefCount = 0;
	FInstancedStruct InstancedStruct = {};
};

struct UNREALLUA_API FLuaInstancedStruct : public FLuaScriptStructBase
{
	static void RegisterUsertype(sol::state_view& pairs);
	
	FLuaInstancedStruct()
	: FLuaScriptStructBase(nullptr), Data()
	{
	}


	//constrructor for creating new instances
	FLuaInstancedStruct(UScriptStruct* metaData);

	//reference constructor for getting propery values
	FLuaInstancedStruct(const FInstancedStruct* instance, bool asRef = false);

	//reference constructor for getting propery values
	//FLuaInstancedStruct(FInstancedStruct* instance);
	
	//copy constructor for getting propery values
	//explicit FLuaInstancedStruct(const FInstancedStruct& instance);

	//copy constructor for getting propery values
	//explicit FLuaInstancedStruct(FInstancedStruct& instance);

	//copy constructor from a LuaUStruct
	FLuaInstancedStruct(const FLuaScriptStruct& other);

	//copy constructor
	FLuaInstancedStruct(const FLuaInstancedStruct& other, bool bAsRef = false);

	//move constructor
	FLuaInstancedStruct(FLuaInstancedStruct&& other) noexcept;

	virtual ~FLuaInstancedStruct() override;

	bool operator==(const FLuaInstancedStruct& other) const
	{
		return this->GetScriptStruct() == other.GetScriptStruct() && this->GetMemory() == other.GetMemory();
	}

	FLuaInstancedStruct& operator=(const FLuaInstancedStruct& other)
	{
		this->CopyFrom(other.GetScriptStruct(), other.GetMemory());
		return *this;
	}
	
	void AddRef();
	int32 RemoveRef();
	void Reset();
	bool OwnsMemory() const;
	
	void CopyFrom(const UScriptStruct* ss, void* memory);
	//virtual void SetThisPropertyReference(UObject* owner, const sol::object& value, sol::this_state lua) override;
	
	static sol::object MakeFromDataStruct(const FLuaScriptStruct& dataStruct, sol::this_state lua);
	static sol::object MakeFromMetaStruct(const FLuaUStruct& metaStruct, sol::this_state lua);
	static sol::object MakeFromPath(const sol::string_view& path, sol::this_state lua);

	TVariant<std::nullptr_t, FInstancedStruct*, FLuaInstancedStructMemory*> Data;
	
	FInstancedStruct* GetInstancedStruct() const;
	//union
	//{
	//	FInstancedStruct* InstancedStruct = nullptr;
	//	FLuaInstancedStructMemory* LuaInstancedStructMemory;
	//};
	//bool bOwnsMemory = false;

	virtual sol::object Lua_Copy(sol::this_state lua) const override;
	FLuaInstancedStruct Copy() const;

	void Lua_InitializeAs(sol::object newStruct);
	void InitializeAs(const UScriptStruct* meta, const void* copyFromMemory = nullptr);
	//static void __gc(FLuaInstancedStruct* me);
	static sol::object __index(FLuaInstancedStruct* strct, sol::object key, sol::this_state lua);
	static bool __newindex(FLuaInstancedStruct* strct, sol::stack_object key, sol::stack_object value, sol::this_state lua);
	static bool __equals(FLuaInstancedStruct* me, FLuaInstancedStruct* other);
	virtual void* GetMemory() const override;
	virtual bool IsReference() const override;
	virtual const UScriptStruct* GetScriptStruct() const override;

	bool IsUPropertyReference() const;
};
