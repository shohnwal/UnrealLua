#include "UObjectRegistry/LuaUObjectItem.h"
#include "LuaValue/LuaScriptValue.h"
#include "Reflection/PropertyHelper.h"
#include "UObjectRegistry/LuaScriptDynamicDelegateHandler.h"
#include "Utility/UnrealLuaHash.h"
#include "Blueprint/UserWidget.h"
#include "UnrealLua.h"
#include "Config/UnrealLuaConfig.h"
#include "sol/sol.hpp"

ULuaScriptDynamicDelegateHandler* FLuaUObjectItem::GetDelegateHandler(UObject* delegateHost, FName funcName)
{
	for(ULuaScriptDynamicDelegateHandler* handler : this->DelegateHandlers)
	{
		if(handler->DelegateOwner == delegateHost && handler->FuncName == funcName)
		{
			return handler;			
		}
	}
	ULuaScriptDynamicDelegateHandler* newHandler = this->DelegateHandlers.Add_GetRef(NewObject<ULuaScriptDynamicDelegateHandler>(this->Object));
	newHandler->FuncName = funcName;
	newHandler->DelegateOwner = delegateHost;
	newHandler->OnProcessEvent.BindRaw(this, &FLuaUObjectItem::NotifyDelegateProcessEvent);
	return newHandler;
}

ULuaScriptDynamicDelegateHandler* FLuaUObjectItem::RemoveDelegate(UObject* delegateOwner, FName subscribedFuncName)
{
	int32 index = this->DelegateHandlers.IndexOfByPredicate([delegateOwner, subscribedFuncName](ULuaScriptDynamicDelegateHandler* handler)
	{
		return handler->DelegateOwner == delegateOwner && handler->FuncName == subscribedFuncName;
	});
	if(index == INDEX_NONE)
	{
		return nullptr;
	}
	ULuaScriptDynamicDelegateHandler* handler = this->DelegateHandlers[index];
	this->DelegateHandlers.RemoveAt(index);
	return handler;
}


void FLuaUObjectItem::UpdateTickFuncMapping(FLuaScriptValue* scriptValue)
{
	if (!UUnrealLuaConfig::AllowOverrideTick())
	{
		return;
	}
	constexpr const char* tickNameStr = "Tick";
	constexpr const char* receiveTickNameStr = "ReceiveTick";

	std::string_view key = scriptValue->GetKeyName();
	
	bool updateTick = false;
	
	if(this->Object->IsA<UUserWidget>())
	{
		if(key == tickNameStr)
		{
			updateTick = true;	
		}
	}
	else if(key == receiveTickNameStr)
	{
		updateTick = true;
	}
	
	if(updateTick)
	{
		//must update func mapping beforehand, before a broadcast might want to use the new function
		this->TickFunc.SetFunction(scriptValue->GetLuaScriptFunction());
	}
}

void FLuaUObjectItem::MarkAsMetaItem()
{
	this->bIsMetaItem = true;
}

void FLuaUObjectItem::NotifyDelegateProcessEvent(ULuaScriptDynamicDelegateHandler* handler, void* params)
{
	UFunction* function = handler->DelegateFunction.Get();
	std::string_view funcName = handler->CallbackFuncName;
	TArray<sol::object>& additionalArgs = handler->CallbackArgs;

	if(!this->Object)
	{
		return;
	}

	UObject* subscriber = this->Object; 
	if(!IsValid(subscriber))
	{
		return;
	}

	sol::function func = this->GetLuaScriptFunction(funcName.data());
	
	if (!func.valid())
	{
		return;
	}

	sol::this_state lua = func.lua_state();

	TArray<sol::object> args;
	for (TFieldIterator<FProperty> propIt(function); propIt; ++propIt)
	{
		FProperty* property = *propIt;
		const uint64 propflags = property->GetPropertyFlags();
		if (propflags & CPF_Parm)
		{
			if(UnrealLua::PropertyHelper::IsInputParameter(property))
			{
				FGetPropertyValueParams getparams{property, params, 0, lua};
				args.Emplace(UnrealLua::PropertyHelper::GetPropertyValue(getparams));
			}
		}
	}
	args.Append(additionalArgs);

	UnrealLua::LuaScriptCall::CallLuaFunctionSafe(func, subscriber, sol::as_args(args));
}


/*
void FLuaUObjectItem::NotifyPreGarbageCollectConditionalBeginDestroy()
{
	if(this->Object == nullptr)
	{
		verify(this->VerifyIsClean())
		return;
	}
	if(!IsValid(this->Object) || this->Object->HasAnyFlags(RF_BeginDestroyed | RF_MirroredGarbage))
	{
		//LUA_LOG("Invalidating FLuaUObjectItem %s : Is Garbage", *GetFullNameSafe(this->Object))
		this->Reset();
		return;
	}
	if(this->Object->IsUnreachable())
	{
		//LUA_LOG("Invalidating FLuaUObjectItem %s : Is Unreachable", *GetFullNameSafe(this->Object))
		this->Reset();
		return;
	}
	verify(this->Object->IsValidLowLevel());
}
*/

void FLuaUObjectItem::NotifyPostGarbageCollectConditionalBeginDestroy()
{ 
	if(this->Object == nullptr)
	{
		if(this->CurrentHandle)
		{
			this->Reset();
		}
		verify(this->VerifyIsClean())
		return;
	}
	if(!IsValid(this->Object) || this->Object->HasAnyFlags(RF_BeginDestroyed | RF_MirroredGarbage))
	{
		//LUA_LOG("Invalidating FLuaUObjectItem %s : Is Garbage", *GetFullNameSafe(this->Object))
		this->Reset();
		return;
	}
	verify(this->Object->IsValidLowLevel());
	this->CheckLuaScriptValueReferences();
}
//Used by FScopedLuaContext::AddReferencedObjects
//This lets the reference collector know which LuaScriptValues and metaitems are referenced
//This does NOT add a reference to the item-UObject!
//The item-UObject-reference is added by FLuaUObjectItemHandle::AddReferencedUObject
void FLuaUObjectItem::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObjects(this->DelegateHandlers);

	//non-metaitems keep their meta ustructs alive by referencing their mappings-UStruct
	if(FUStructPropertyMapping* mapping = this->PropertyMapping.GetPtr<FUStructPropertyMapping>())
	{
		if(!this->bIsMetaItem)
		{
			//this is a normal UObject -> Reference the UClass
			Collector.AddReferencedObject(mapping->OwningField);
		}
	}
	FLuaScriptValuesContainer::AddReferencedObjects(Collector);
}

const FHashedFieldMapping* FLuaUObjectItem::GetPropertyMapping(const sol::object& obj_o)
{
	uint32 hash = UnrealLua::HashUtility::StrCrc32(obj_o.as<sol::string_view>().data());
	return this->GetPropertyMapping(hash);	
}

const FHashedFieldMapping* FLuaUObjectItem::GetPropertyMapping(const sol::stack_object& obj_o)
{
	uint32 hash = UnrealLua::HashUtility::StrCrc32(obj_o.as<sol::string_view>().data());
	return this->GetPropertyMapping(hash);	
}
