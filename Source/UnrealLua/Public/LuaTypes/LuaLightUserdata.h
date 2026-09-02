// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

extern "C" {
	#include "llimits.h"
}
#include "LuaEnum.h"
#include "sol/sol.hpp"
#include "Reflection/CPPPropertyDefs.h"
#include "Reflection/PropertyMapping.h"

/**
 * 
 */

struct FLuaCallContext;
struct FLuaUObjectItemHandle;
struct TValue;

struct FFunctionDescr;
struct FLuaUObjectItem;
struct ILuaLightUserdata;
struct FLuaUEnumEntry;
union Value;

namespace UnrealLua::LightUserdata
{
	//constexpr std::uintptr_t UnrealLuaLightUserdataTag = 0b101;
	
	enum EUnrealLuaLightUserdataType
	{
		None				= 0b000,
		UFunctionDescr		= 0b001,
		//Unused			= 0b010,
		UEnum				= 0b010,
		UEnumEntry			= 0b011,
		UObject				= 0b100,
		FunctionCallContext	= 0b101, 
		//Unused			= 0b110,
		Custom				= 0b111,
		All					= 0b111
	};
	
	UNREALLUA_API ILuaLightUserdata* GetLuaLightUserDataFromLuaObj(const sol::stack_object& obj);
	UNREALLUA_API sol::object MakeFFunctionDescrReferenceObject(lua_State* L, const FFunctionDescr*);
	UNREALLUA_API int PushFFunctionDescrReferenceObject(sol::this_state lua, const FFunctionDescr* getFunction);
	UNREALLUA_API int PushUObject(lua_State* L, ::UObject* obj);
	UNREALLUA_API int PushUEnum(const ::UEnum* Enum, lua_State* L);
	UNREALLUA_API void AddReferencedUObject(const lu_byte& tag, const Value& value, FReferenceCollector& collector);

	UNREALLUA_API sol::variadic_results Call(sol::stack_object ud, sol::stack_object self, sol::variadic_args args);
	UNREALLUA_API int Index(lua_State* L);
	UNREALLUA_API int NewIndex(lua_State* L);
	UNREALLUA_API int ToString(lua_State* L);
	UNREALLUA_API int Length(lua_State* L);
	UNREALLUA_API int Delay(lua_State* L);
	UNREALLUA_API int SetTimer(lua_State* L);
	UNREALLUA_API int __AddOnValueChanged(lua_State* L);
	UNREALLUA_API int __RemoveOnValueChanged(lua_State* L);
	UNREALLUA_API int __AddReplicatedSubobject(lua_State* L);
	UNREALLUA_API int __RemoveReplicatedSubobject(lua_State* L);
	UNREALLUA_API int __LoadLuaScript(lua_State* L);
	UNREALLUA_API int __Destroy(lua_State* L);
	UNREALLUA_API int __Super(lua_State* L);
	UNREALLUA_API int __SetLuaTickEnabled(lua_State* L);
	UNREALLUA_API int __SetBlueprintTickEnabled(lua_State* L);
	UNREALLUA_API int __SpawnActor(lua_State* L);

	UNREALLUA_API int TryPushMetaMethod(std::string_view Key, lua_State* Lua);
	
	UNREALLUA_API ::UObject* GetUObject(const sol::stack_object Self);
	UNREALLUA_API ::UObject* GetUObject(const sol::object& Self);
	UNREALLUA_API FFunctionDescr* GetFunctionDescr(const sol::stack_object Self);
	UNREALLUA_API FFunctionDescr* GetFunctionDescr(const sol::object& Self);
	
	UNREALLUA_API bool IsUObject(const sol::stack_object& obj);

	UNREALLUA_API bool IsUObject(const sol::object& obj);

	template<typename U>
	inline bool IsUObjectType(const sol::stack_object ud)
	{
		::UObject* obj = GetUObject(ud);
		if (obj)
		{
			if constexpr (std::is_same_v<::UObject, U>)
			{
				return true;
			}
			else
			{
				return Cast<U>(obj) != nullptr;
			}
		}
		return false;
	}
	
	template<typename U>
	inline bool IsUObjectType(const sol::object& ud)
	{
		::UObject* obj = GetUObject(ud);
		if (obj)
		{
			if constexpr (std::is_same_v<::UObject, U>)
			{
				return true;
			}
			else
			{
				return Cast<U>(obj) != nullptr;
			}
		}
		return false;
	}

	UNREALLUA_API bool IsEnum(const sol::stack_object& Object);
	UNREALLUA_API bool IsEnum(const sol::object& Object);
	UNREALLUA_API ::UEnum* GetUEnum(const sol::object& Object);
	UNREALLUA_API ::UEnum* GetUEnum(const sol::stack_object& Object);
	
	UNREALLUA_API bool IsEnumEntry(const sol::stack_object& Object);
	UNREALLUA_API bool IsEnumEntry(const sol::object& Object);
	UNREALLUA_API FLuaUEnumEntry* GetEnumEntry(const sol::stack_object& Object);
	UNREALLUA_API FLuaUEnumEntry* GetEnumEntry(const sol::object& Object);
	
	UNREALLUA_API sol::object GetUObjectAsTaggedLightUserdata(TObjectPtr<::UObject> object, lua_State* L);
	UNREALLUA_API sol::object GetUObjectAsTaggedLightUserdata(FLuaUObjectItemHandle* handle, lua_State* L);
	UNREALLUA_API sol::object GetUEnumAsTaggedLightUserdata(TObjectPtr<::UEnum> uenum, lua_State* Lua_State);
	UNREALLUA_API sol::object GetUEnumValueAsTaggedLightUserdata(FLuaUEnumEntry* enumEntry, lua_State* L);
	UNREALLUA_API int PushUEnumValueAsTaggedLightUserdata(FLuaUEnumEntry* enumEntry, lua_State* L);
}
/*
USTRUCT()
struct UNREALLUA_API FLuaLightUserdata
{
	GENERATED_BODY()

	static void RegisterUsertype(sol::state_view& lua);
	virtual ~FLuaLightUserdata() = default;
	
	virtual sol::variadic_results operator()(sol::stack_object self, sol::variadic_args args);
	virtual int __index(sol::stack_object key) { return 0; };
	virtual void __newindex(sol::stack_object key, sol::stack_object value) {}
	virtual UScriptStruct* GetScriptStruct() { return nullptr; }
};
*/
struct UNREALLUA_API ILuaLightUserdata
{
	static void RegisterUsertype(sol::state_view& lua);
	virtual ~ILuaLightUserdata() = default;
	
	virtual sol::variadic_results operator()(sol::stack_object self, sol::variadic_args args);
	virtual int __index(sol::stack_object key) = 0;
	virtual void __newindex(sol::stack_object key, sol::stack_object value) = 0;
	
	/*
	virtual bool IsUObject() const;
	virtual UObject* GetUObject() const;
	virtual bool IsUEnum() const;
	virtual UEnum* GetUEnum() const;
	virtual bool IsUEnumEntry() const;
	virtual FLuaUEnumEntry* GetUEnumEntry() const;
	virtual bool IsUFunctionDescr() const;
	virtual FFunctionDescr* GetUFunctionDescr() const;
	virtual bool IsLuaCallContext() const;
	virtual FLuaCallContext* GetLuaCallContext() const;
	
	
	virtual sol::variadic_results Call(sol::stack_object ud, sol::stack_object self, sol::variadic_args args);

	virtual int ToString(lua_State* L);
	virtual int Length(lua_State* L);
	virtual int Delay(lua_State* L);
	virtual int SetTimer(lua_State* L);
	virtual int __AddOnValueChanged(lua_State* L);
	virtual int __RemoveOnValueChanged(lua_State* L);
	virtual int __AddReplicatedSubobject(lua_State* L);
	virtual int __RemoveReplicatedSubobject(lua_State* L);
	virtual int __LoadLuaScript(lua_State* L);
	virtual int __Destroy(lua_State* L);
	virtual int __Super(lua_State* L);
	virtual int __SetLuaTickEnabled(lua_State* L);
	virtual int __SetBlueprintTickEnabled(lua_State* L);
	virtual int __SpawnActor(lua_State* L);
	*/
	
	virtual UnrealLua::LightUserdata::EUnrealLuaLightUserdataType GetLightUserdataType() const;
};


struct alignas(8) UNREALLUA_API FUnrealLuaLightUserdataWrapper
{
	explicit FUnrealLuaLightUserdataWrapper(const sol::stack_object obj);
	explicit FUnrealLuaLightUserdataWrapper(const sol::object& obj);
	explicit FUnrealLuaLightUserdataWrapper(void* maybeTaggedLightUserdataPtr);
	explicit FUnrealLuaLightUserdataWrapper(const FFunctionDescr* functionDescr);
	explicit FUnrealLuaLightUserdataWrapper(const UObject* obj);
	explicit FUnrealLuaLightUserdataWrapper(FLuaUObjectItemHandle* uobjectHandle);
	explicit FUnrealLuaLightUserdataWrapper(const UEnum* uenum);
	explicit FUnrealLuaLightUserdataWrapper(const FLuaUEnumEntry* uenumEntry);

	bool IsUObject() const;
	bool IsUObjectCallContext() const;
	bool IsFuncDescr() const;
	bool IsUEnum() const;
	bool IsUEnumEntry() const;
	bool IsCustom() const;

	FFunctionDescr* GetFunctionDescr() const;
	FLuaCallContext* GetUObjectCallContext() const;
	UObject* GetUObject() const;
	FLuaUObjectItemHandle* GetUObjectItemHandle() const;
	FLuaUObjectItem* GetUObjectItem() const;
	FLuaUObjectItemHandle* GetUEnumItemHandle() const;
	FLuaUEnumEntry* GetUEnumEntry() const;
	UEnum* GetUEnum() const;
	ILuaLightUserdata* GetCustom() const;
	
	void* GetTaggedLightUserdataPseudoPtr() const;
	void* GetUntaggedPtr() const;
	void SetPtrAndTag(const void* ptr, UnrealLua::LightUserdata::EUnrealLuaLightUserdataType tag);
	bool HasTag(UnrealLua::LightUserdata::EUnrealLuaLightUserdataType tag) const;
	bool HasTagAndPtrValue(const void* ptr, UnrealLua::LightUserdata::EUnrealLuaLightUserdataType tag) const;
	UnrealLua::LightUserdata::EUnrealLuaLightUserdataType GetType() const; 
	//	int32 GetIndex() const;
	//	int32 GetSerialNumber() const;
	
	sol::variadic_results Call(sol::stack_object ud, sol::stack_object self, sol::variadic_args args) const;
	int Index(lua_State* lua) const;
	int NewIndex(lua_State* lua) const;
	int ToString(lua_State* lua) const;
	int Length(lua_State* Lua) const;
	int Delay(lua_State* Lua_State) const;
	int SetTimer(lua_State* Lua_State) const;
	int __AddOnValueChanged(lua_State* L) const;
	int __RemoveOnValueChanged(lua_State* L) const;
	int __AddReplicatedSubobject(lua_State* L) const;
	int __RemoveReplicatedSubobject(lua_State* L) const;
	int __LoadLuaScript(lua_State* Lua_State) const;
	int __Destroy(lua_State* L) const;
	int __Super(lua_State* Lua_State) const;
	int __SetLuaTickEnabled(lua_State* L) const;
	int __SetBlueprintTickEnabled(lua_State* L) const;
	int __SpawnActor(lua_State* L) const;

	bool IsInvalidUObjectReference() const;
	bool IsValidUObjectReference() const;

private:
	std::uintptr_t LightUserdataPseudoPtr = 0;
};