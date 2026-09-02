#include "Reflection/PropertyDescr/SingleDelegatePropertyDescr.h"

#include "Utility/LuaLogMacros.h"
#include "Reflection/PropertyHelper.h"
#include "UObjectRegistry/LuaScriptDynamicDelegateHandler.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"
#include "StringHandling/UnrealLuaStringCache.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"

void FSingleDelegatePropertyProxy::Bind(sol::object self, sol::object callbackFunc, sol::variadic_args callbackArgs, sol::this_state lua) const
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
	
	if(this->TryBindUFunction(delegateOwner, subscriber, callbackFuncName))
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
	
	const void* addressPtr = this->Prop->ContainerPtrToValuePtr<void>(delegateOwner);
	FScriptDelegate* value = const_cast<FScriptDelegate*>(static_cast<const FScriptDelegate*>(addressPtr));
	*value = del;
}

void FSingleDelegatePropertyProxy::Unbind() const
{
	UObject* delegateOwner = this->DelegateOwner.Get(); 
	if(!delegateOwner)
	{
		return;
	}
	const void* addressPtr = this->Prop->ContainerPtrToValuePtr<void>(delegateOwner);
	FScriptDelegate* value = const_cast<FScriptDelegate*>(static_cast<const FScriptDelegate*>(addressPtr));
	value->Unbind();
}

void FSingleDelegatePropertyProxy::Execute(sol::variadic_args args) const
{
	UObject* delegateOwner = this->DelegateOwner.Get(); 
	if(!delegateOwner)
	{
		return;
	}
	const void* addressPtr = this->Prop->ContainerPtrToValuePtr<void>(delegateOwner);
	FScriptDelegate* value = const_cast<FScriptDelegate*>(static_cast<const FScriptDelegate*>(addressPtr));

	if(!value->IsBound())
	{
		return;
	}

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
		sol::stack_object stackValue = args[index];
		TSetPropertyValueParams parms{prop, params, 0, stackValue};
		UnrealLua::PropertyHelper::SetPropertyValue_InContainer(parms);
		index++;
	}

	value->ProcessDelegate<UObject>(params);
}

bool FSingleDelegatePropertyProxy::IsBound()
{
	UObject* delegateOwner = this->DelegateOwner.Get(); 
	if(!delegateOwner)
	{
		return false;
	}
	const void* addressPtr = this->Prop->ContainerPtrToValuePtr<void>(delegateOwner);
	FScriptDelegate* value = const_cast<FScriptDelegate*>(static_cast<const FScriptDelegate*>(addressPtr));

	return value->IsBound();
}

bool FSingleDelegatePropertyProxy::TryBindUFunction(UObject* delegateOwner, UObject* subscriber, const FName& funcName) const
{
	UFunction* func = subscriber->FindFunction(funcName);
	if(func)
	{
		if(!this->Prop->SignatureFunction->IsSignatureCompatibleWith(func))
		{
			return false;
		}
		TScriptDelegate del;
		del.BindUFunction(subscriber, funcName);

		const void* addressPtr = this->Prop->ContainerPtrToValuePtr<void>(delegateOwner);
		FScriptDelegate* value = const_cast<FScriptDelegate*>(static_cast<const FScriptDelegate*>(addressPtr));
		*value = del;
		return true;
	}
	return false;
}

void FSingleDelegatePropertyProxy::AddReferencedObjects(FReferenceCollector& Collector)
{
	//Collector.AddReferencedObject(this->DelegateOwner);
}
