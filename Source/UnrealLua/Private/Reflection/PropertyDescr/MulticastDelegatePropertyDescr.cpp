#include "Reflection/PropertyDescr/MulticastDelegatePropertyDescr.h"

#include "Utility/LuaLogMacros.h"
#include "BlueprintSupport/UnrealLuaUtility.h"
#include "Reflection/PropertyHelper.h"
#include "UObjectRegistry/LuaScriptDynamicDelegateHandler.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"
#include "StringHandling/UnrealLuaStringCache.h"
#include "UnrealLua.h"
#include "sol/sol.hpp"
#include "Utility/UnrealVersion.h"
#include "LuaContext/ScopedLuaContext.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"


uint32 FMulticastDelegatePropertyDescr::AddRef(FReferenceCollector& collector, FMulticastDelegateProperty* prop, void* objectMemory, bool container)
{
	uint32 numReferenced = 0;
	collector.AddReferencedObject(prop->SignatureFunction);
	for (int32 ArrIndex = 0; ArrIndex < prop->ArrayDim; ++ArrIndex)
	{
		//They are all weak pointers, no need to ref them?
		/*
		FMulticastScriptDelegate* value = const_cast<FMulticastScriptDelegate*>(prop->GetMulticastDelegate(prop->ContainerPtrToValuePtr<void>(objectMemory, ArrIndex)));
		TArray<UObject*> arr = value->GetAllObjects();
		collector.AddReferencedObjects(arr);
		*/
	}
	return numReferenced;
}

FString FMulticastDelegatePropertyDescr::GetPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params)
{
	return "MulticastDelegate()";
}

FMulticastDelegatePropertyProxy::FMulticastDelegatePropertyProxy(UObject* owner, const FMulticastDelegateProperty* prop)
	: DelegateOwner(owner), Prop(const_cast<FMulticastDelegateProperty*>(prop))
{
	//RegisterGCObject();
}

void FMulticastDelegatePropertyProxy::Add(sol::object self, sol::object callbackFunc, sol::variadic_args callbackArgs, sol::this_state lua) const
{
	this->AddInternal(self, callbackFunc, callbackArgs, lua, false);
}

void FMulticastDelegatePropertyProxy::AddUnique(sol::object self, sol::object callbackFunc, sol::variadic_args callbackArgs, sol::this_state lua) const
{
	this->AddInternal(self, callbackFunc, callbackArgs, lua, true);
}

void FMulticastDelegatePropertyProxy::AddInternal(sol::object self, sol::object callbackFunc, sol::variadic_args callbackArgs, sol::this_state lua, bool bUnique) const
{
	UObject* delegateOwner = this->DelegateOwner.Get(); 
	if(!delegateOwner)
	{
		return;
	}
	UObject* subscriber = UnrealLua::LightUserdata::GetUObject(self);

	if(subscriber == nullptr)
	{
		LUA_LOG_WARNING("Can't add to delegate to delegate function, passed in UObject/FLuaUObjectWrapper was not valid")
		return;
	}
	
	const sol::string_view funcName = callbackFunc.as<sol::string_view>();
	if(funcName.length() == 0)
	{
		return;
	}

	const FName callbackFuncName = UnrealLua::StringCache::GetFNameForStringLuaObject(callbackFunc);
	if(this->TryBindUFunction(delegateOwner, subscriber, callbackFuncName, bUnique))
	{
		return;
	}

	FLuaUObjectItem& subscriberItem = UnrealLua::UObjectRegistry::GetUObjectItem(subscriber);

	ULuaScriptDynamicDelegateHandler* proxy = subscriberItem.GetDelegateHandler(delegateOwner, callbackFuncName);
	if(!proxy)
	{
		return;
	}
	proxy->CallbackArgs = {};
	for(int32 index = 0; index < callbackArgs.size(); index++)
	{
		proxy->CallbackArgs.Emplace(callbackArgs[index]);	
	}
	proxy->CallbackFuncName = funcName;
	proxy->DelegateFunction = this->Prop->SignatureFunction;

	FScriptDelegate del;
	del.BindUFunction(proxy, GET_FUNCTION_NAME_CHECKED(ULuaScriptDynamicDelegateHandler, DummyFunc));

	const void* addressPtr = this->Prop->ContainerPtrToValuePtr<void>(this->DelegateOwner.Get());
	FMulticastScriptDelegate* Value = const_cast<FMulticastScriptDelegate*>(this->Prop->GetMulticastDelegate(addressPtr));
	if(bUnique)
	{
		Value->AddUnique(del);	
	}
	else
	{
		Value->Add(del);
	}
}

bool FMulticastDelegatePropertyProxy::TryBindUFunction(UObject* delegateOwner, UObject* subscriber, const FName& funcName, bool bUnique) const
{
	UFunction* func = subscriber->FindFunction(funcName);
	if(func)
	{
		const FMulticastDelegateProperty* prop = CastField<FMulticastDelegateProperty>(this->Prop);
		if(!prop->SignatureFunction->IsSignatureCompatibleWith(func))
		{
			return false;
		}
		TScriptDelegate del;
		del.BindUFunction(subscriber, funcName);

		const void* addressPtr = prop->ContainerPtrToValuePtr<void>(delegateOwner);
		FMulticastScriptDelegate* Value = const_cast<FMulticastScriptDelegate*>(prop->GetMulticastDelegate(addressPtr));
		if(bUnique)
		{
			Value->AddUnique(del);
		}
		else
		{
			Value->Add(del);
		}
		return true;
	}
	return false;
}

void FMulticastDelegatePropertyProxy::AddHandlerInternal(ULuaScriptDynamicDelegateHandler* handler, sol::state_view lua, bool bUnique) const
{
	FScriptDelegate del;
	del.BindUFunction(handler, GET_FUNCTION_NAME_CHECKED(ULuaScriptDynamicDelegateHandler, DummyFunc));

	const FMulticastDelegateProperty* prop = CastField<FMulticastDelegateProperty>(this->Prop);
	const void* addressPtr = prop->ContainerPtrToValuePtr<void>(this->DelegateOwner.Get());
	FMulticastScriptDelegate* Value = const_cast<FMulticastScriptDelegate*>(prop->GetMulticastDelegate(addressPtr));
	if(bUnique)
	{
		Value->AddUnique(del);	
	}
	else
	{
		Value->Add(del);
	}
}


void FMulticastDelegatePropertyProxy::Remove(sol::object self, sol::object callbackFunc, sol::this_state lua) const
{
	UObject* delegateOwner = this->DelegateOwner.Get();
	if(!delegateOwner || !this->Prop || callbackFunc.get_type() != sol::type::string)
	{
		return;
	}

	const sol::string_view funcName = callbackFunc.as<sol::string_view>();
	if(funcName.length() == 0)
	{
		return;
	}

	UObject* scriptOwner = UnrealLua::LightUserdata::GetUObject(self);

	if(!IsValid(scriptOwner))
	{
		return;
	}

	const FName callbackFuncName = UnrealLua::StringCache::GetFNameForStringLuaObject(callbackFunc);
	UFunction* func = scriptOwner->FindFunction(callbackFuncName);
	if(func)
	{
		TScriptDelegate del;
		del.BindUFunction(scriptOwner, callbackFuncName);

		const void* addressPtr = this->Prop->ContainerPtrToValuePtr<void>(delegateOwner);
		FMulticastScriptDelegate* Value = const_cast<FMulticastScriptDelegate*>(this->Prop->GetMulticastDelegate(addressPtr));
		Value->Remove(scriptOwner, callbackFuncName);
		return;
	}

	FLuaUObjectItem& subscriberItem = UnrealLua::UObjectRegistry::GetUObjectItem(scriptOwner);

	ULuaScriptDynamicDelegateHandler* handler = subscriberItem.RemoveDelegate(delegateOwner, callbackFuncName);
	if(handler)
	{
		static FName dummyFuncName = TEXT("DummyFunc");
		//ATTENTION : "removed" is already cleared at this point and added back to the LuaDelegateRegistry pool, so only use it for the object ptr at Value->Remove!!!
		const void* addressPtr = this->Prop->ContainerPtrToValuePtr<void>(delegateOwner);
		FMulticastScriptDelegate* Value = const_cast<FMulticastScriptDelegate*>(this->Prop->GetMulticastDelegate(addressPtr));
		Value->Remove(handler, dummyFuncName);
		//For now, destroy the proxy		
	}
}

void FMulticastDelegatePropertyProxy::Broadcast(sol::variadic_args args) const
{
	UObject* delegateOwner = this->DelegateOwner.Get(); 
	if(!delegateOwner || !this->Prop)
	{
		return;
	}
	const void* memoryAddr = this->Prop->ContainerPtrToValuePtr<void>(delegateOwner, 0);
	const FMulticastScriptDelegate* Value = const_cast<FMulticastScriptDelegate*>(this->Prop->GetMulticastDelegate(memoryAddr));

	//@TODO : Get FFunctionDescr instead to save analyzing func props
	
	const UFunction* function = this->Prop->SignatureFunction;

	void* params = FMemory_Alloca_Aligned(function->ParmsSize, function->GetMinAlignment());
	function->InitializeStruct(params);

	TArray<FProperty*, TInlineAllocator<8>> inputParms;
	for (TFieldIterator<FProperty> propIt(function); propIt; ++propIt)
	{
		FProperty* const property = *propIt;
		const uint64 propflags = property->GetPropertyFlags();
		if (propflags & CPF_Parm)
		{
			if(UnrealLua::PropertyHelper::IsInputParameter(property))
			{
				inputParms.Emplace(property);
			}
		}
	}
	
	uint32 index = 0;
	for (const auto& prop : inputParms)
	{
		sol::stack_object value{args[index]};
		TSetPropertyValueParams parms{prop, params, 0, value};
		UnrealLua::PropertyHelper::SetPropertyValue_InContainer(parms);
		index++;
	}
#if UNREALLUA_UE_VERSION_OLDER_THAN(5,8,0)
	Value->ProcessMulticastDelegate<UObject>(params);
#else
	Value->ProcessDelegate<UObject>(params);
#endif
}

void FMulticastDelegatePropertyProxy::AddReferencedObjects(FReferenceCollector& Collector)
{
	//Collector.AddReferencedObject(this->DelegateOwner);
}