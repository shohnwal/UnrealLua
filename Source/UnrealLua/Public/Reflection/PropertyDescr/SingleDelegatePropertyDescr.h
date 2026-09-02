// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UnrealLua.h"
#include "Utility/LuaLogMacros.h"
#include "Interface/LuaGCObject.h"
#include "Reflection/PropertyHelperTypes.h"
#include "UObject/Object.h"
#include "UObject/UnrealType.h"
#include "sol/sol.hpp"
#include "LuaTypes/LuaLightUserdata.h"
#include "UObject/ScriptDelegateFwd.h"

/**
 * 
 */

class ILuaScriptable;
struct FGetPropertyValueParams;

struct UNREALLUA_API FSingleDelegatePropertyDescr
{
	static bool IsCompatibleType(FProperty* prop, const sol::object& obj);

	static sol::object GetPropertyValue(const FGetPropertyValueParams& params);
	static int GetPropertyValue(FPushPropertyValueParams& params);

	template<typename LUAOBJ>
	static void SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params);

	//used by container wrapperrs (LuaArray, LuaSet, etc)
	static uint32 AddRef( FReferenceCollector &collector, FDelegateProperty* p, void* objectMemory,bool container = true);
};


struct UNREALLUA_API FSingleDelegatePropertyProxy : public FLuaGCObject
{
	FSingleDelegatePropertyProxy()
		: DelegateOwner(nullptr), Prop(nullptr)
	{
		//RegisterGCObject();
	}

	virtual ~FSingleDelegatePropertyProxy()
	{
		//UnregisterGCObject();
	};
	
	explicit FSingleDelegatePropertyProxy(UObject* owner, FDelegateProperty* prop)
		: DelegateOwner(owner), Prop(prop)
	{
		//RegisterGCObject();
	}

	bool operator==(const FSingleDelegatePropertyProxy& other) const
	{
		 return this == &other;
	}
	
	TWeakObjectPtr<UObject> DelegateOwner;
	FDelegateProperty* Prop;
	void Bind(sol::object self, sol::object callbackFunc, sol::variadic_args callbackArgs, sol::this_state lua) const;
	void Unbind() const;
	void Execute(sol::variadic_args args) const;
	bool IsBound();

private:
	bool TryBindUFunction(UObject* delegateOwner, UObject* subscriber, const FName& funcName) const;

public:
	void AddReferencedObjects(FReferenceCollector& Collector);
};


inline bool FSingleDelegatePropertyDescr::IsCompatibleType(FProperty* prop, const sol::object& obj)
{
	return false;
}

inline sol::object FSingleDelegatePropertyDescr::GetPropertyValue(const FGetPropertyValueParams& params)
{
	LUA_LOG_WARNING("FSingleDelegatePropertyDescr::GetPropertyValue : Using potentially unsafe MemoryPtr-propOffset to get ContainerPtr of UObject. find something better!")
	uint8* mem = static_cast<uint8*>(params.MemoryPtr);
	UObject* owner = static_cast<UObject*>(static_cast<void*>(mem - params.Prop->GetOffset_ForInternal()));
	if(IsValid(owner))
	{
		FDelegateProperty* delProp = CastField<FDelegateProperty>(params.Prop);
		return sol::object(params.Lua.lua_state(), sol::in_place_type<FSingleDelegatePropertyProxy>,owner, delProp);
	}
	return sol::nil;
}

inline int FSingleDelegatePropertyDescr::GetPropertyValue(FPushPropertyValueParams& params)
{
	LUA_LOG_WARNING("FSingleDelegatePropertyDescr::GetPropertyValue : Using potentially unsafe MemoryPtr-propOffset to get ContainerPtr of UObject. find something better!")
	uint8* mem = static_cast<uint8*>(params.MemoryPtr);
	UObject* owner = static_cast<UObject*>(static_cast<void*>(mem - params.Prop->GetOffset_ForInternal()));

	if(IsValid(owner))
	{
		FDelegateProperty* delProp = CastField<FDelegateProperty>(params.Prop);
		return sol::stack::push<FSingleDelegatePropertyProxy>(params.Lua.lua_state(), owner, delProp);
	}
	return sol::stack::push(params.Lua, sol::nil);
}


template<typename LUAOBJ>
void FSingleDelegatePropertyDescr::SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params)
{
	checkNoEntry();
	UObject* owner = static_cast<UObject*>(params.MemoryPtr);
	if(!IsValid(owner))
	{
		return;
	}
	if(params.LuaValue == sol::nil)
	{
		FScriptDelegate del{};
		FDelegateProperty* prop = CastField<FDelegateProperty>(params.Prop);
		prop->SetPropertyValue(params.MemoryPtr, del);
	}
	else if(params.LuaValue.valid() && params.LuaValue.get_type() == sol::type::table)
	{
		sol::table tbl = params.LuaValue.template as<sol::table>();
		sol::object obj = tbl[1]; //subscriber
		sol::object func = tbl[2]; //callback func name

		if(obj.valid() && UnrealLua::LightUserdata::IsUObject(obj) && func.valid() && func.get_type() == sol::type::string)
		{
			FSingleDelegatePropertyProxy proxy{owner, CastField<FDelegateProperty>(params.Prop)};
			proxy.Bind(obj, func,{}, tbl.lua_state());
		}
	}
}

template void FSingleDelegatePropertyDescr::SetPropertyValue(const TSetPropertyValueParams<sol::object>& params);
template void FSingleDelegatePropertyDescr::SetPropertyValue(const TSetPropertyValueParams<sol::stack_object>& params);

inline uint32 FSingleDelegatePropertyDescr::AddRef(FReferenceCollector& collector, FDelegateProperty* prop, void* objectMemory, bool container)
{
	uint32 numReferenced = 0;
	for (int32 ArrIndex = 0; ArrIndex < prop->ArrayDim; ++ArrIndex)
	{
		//Delegates only reference weak ptrs, so no need to refcount them
		collector.AddReferencedObject(prop->SignatureFunction);
		//collector.AddReferencedObject(Value->GetUObjectRef());
	}
	return numReferenced;
}