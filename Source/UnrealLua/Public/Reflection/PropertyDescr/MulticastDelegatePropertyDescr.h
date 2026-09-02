#pragma once
#include "CoreMinimal.h"
#include <vector>

#include "Utility/LuaLogMacros.h"
#include "Interface/LuaGCObject.h"
#include "sol/sol.hpp"
#include "Interface/LuaScriptable.h"
#include "Reflection/PropertyHelperTypes.h"
#include "UObject/Object.h"

class ULuaScriptDynamicDelegateHandler;
class FReferenceCollector;
class FStructOnScope;
class UFunction;
//class FScriptDelegate;

struct UNREALLUA_API FMulticastDelegatePropertyDescr
{
	static bool IsCompatibleType(FProperty* prop, const sol::object& obj);

	static sol::object GetPropertyValue(const FGetPropertyValueParams& params);
	static int GetPropertyValue(FPushPropertyValueParams& params);

	template<typename LUAOBJ>
	static void SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params);

	static uint32 AddRef( FReferenceCollector &collector, FMulticastDelegateProperty* p, void* objectMemory,bool container = true);
	static FString GetPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params);
};


struct UNREALLUA_API FMulticastDelegatePropertyProxy : public FLuaGCObject
{
	FMulticastDelegatePropertyProxy()
		: DelegateOwner(nullptr), Prop(nullptr)
	{
		//RegisterGCObject();
	}

	virtual ~FMulticastDelegatePropertyProxy()
	{
		//UnregisterGCObject();
	}

	
	explicit FMulticastDelegatePropertyProxy(UObject* owner, const FMulticastDelegateProperty* prop);

	bool operator==(const FMulticastDelegatePropertyProxy& other) const
	{
		return this == &other;
	}

	TWeakObjectPtr<UObject> DelegateOwner;
	FMulticastDelegateProperty* Prop;
	//const FFunctionDescr FunctionDescr;
	void Add(sol::object self, sol::object callbackFunc, sol::variadic_args callbackArgs, sol::this_state lua) const;
	void AddUnique(sol::object self, sol::object callbackFunc, sol::variadic_args callbackArgs, sol::this_state lua) const;
	void Remove(sol::object self, sol::object callbackFunc, sol::this_state lua) const;
	void Broadcast(sol::variadic_args args) const;

private:
	void AddInternal(sol::object self, sol::object callbackFunc, sol::variadic_args callbackArgs, sol::this_state lua, bool bUnique) const;
	bool TryBindUFunction(UObject* delegateOwner, UObject* subscriber, const FName& funcName, bool bUnique = false) const;
	void AddHandlerInternal(ULuaScriptDynamicDelegateHandler* handler, sol::state_view lua, bool bUnique) const;

public:
	void AddReferencedObjects(FReferenceCollector& Collector);
};

inline bool FMulticastDelegatePropertyDescr::IsCompatibleType(FProperty* p, const sol::object& obj)
{
	return false;
}

inline sol::object FMulticastDelegatePropertyDescr::GetPropertyValue(const FGetPropertyValueParams& params)
{
	static bool once = false;
	if (!once)
	{
		LUA_LOG_WARNING("FMulticastDelegatePropertyDescr::GetPropertyValue : Using potentially unsafe MemoryPtr-propOffset to get ContainerPtr of UObject. find something better!")
		once = true;
	}
	uint8* mem = static_cast<uint8*>(params.MemoryPtr);
	UObject* owner = static_cast<UObject*>(static_cast<void*>(mem - params.Prop->GetOffset_ForInternal()));	if(IsValid(owner))
	if (IsValid(owner))
	{
		const FMulticastDelegateProperty* delProp = CastField<FMulticastDelegateProperty>(params.Prop);
		return sol::object(params.Lua.lua_state(), sol::in_place_type<FMulticastDelegatePropertyProxy>, owner, delProp);
	}
	return sol::nil;
}
inline int FMulticastDelegatePropertyDescr::GetPropertyValue(FPushPropertyValueParams& params)
{
	static bool once = false;
	if (!once)
	{
		LUA_LOG_WARNING("FMulticastDelegatePropertyDescr::GetPropertyValue : Using potentially unsafe MemoryPtr-propOffset to get ContainerPtr of UObject. find something better!")
		once = true;
	}
	uint8* mem = static_cast<uint8*>(params.MemoryPtr);
	UObject* owner = static_cast<UObject*>(static_cast<void*>(mem - params.Prop->GetOffset_ForInternal()));
	//UObject* owner = static_cast<UObject*>(params.MemoryPtr);
	if(IsValid(owner))
	{
		const FMulticastDelegateProperty* delProp = CastField<FMulticastDelegateProperty>(params.Prop);
		return sol::stack::push<FMulticastDelegatePropertyProxy>(params.Lua.lua_state(), owner, delProp);
	}
	return sol::stack::push(params.Lua, sol::nil);
}

template<typename LUAOBJ>
void FMulticastDelegatePropertyDescr::SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params)
{
	LUA_LOG_WARNING("Setting of Multicast delegate properties is not supported yet!")
	checkNoEntry();
	if(params.LuaValue == sol::nil)
	{
		const FMulticastDelegateProperty* prop = CastField<FMulticastDelegateProperty>(params.Prop);
		prop->ClearDelegate(static_cast<UObject*>(params.MemoryPtr));
	}
}

template void FMulticastDelegatePropertyDescr::SetPropertyValue(const TSetPropertyValueParams<sol::object>& params);
template void FMulticastDelegatePropertyDescr::SetPropertyValue(const TSetPropertyValueParams<sol::stack_object>& params);

/*
void FMulticastDelegatePropertyDescr::GetUFunctionReturnValue(FProperty* prop, void* funcMemory, sol::this_state luaState, sol::variadic_results& outFuncReturns)
{
	LUA_LOG_WARNING("Can not get Multicast delegate property as a function return value!")
}
*/
