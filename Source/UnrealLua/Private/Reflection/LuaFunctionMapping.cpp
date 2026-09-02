// Fill out your copyright notice in the Description page of Project Settings.


#include "Reflection/LuaFunctionMapping.h"

#include "Config/UnrealLuaConstants.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"

void FLuaScriptObjectFunctionMap::OverrideFuncMapping(UObject* owningObject, FName funcName, const sol::function& func)
{
	checkNoEntry();
	if(func.valid())
	{
		if(funcName == UnrealLua::PropertyNames::NAME_ReceiveTick || funcName == UnrealLua::PropertyNames::NAME_UserWidgetTick)
		{
			if(this->TickFuncMapping.LuaScriptFunction.valid() && this->TickFuncMapping.LuaScriptFunction == func)
			{
				//already set to the same func
				return;
			}
			this->TickFuncMapping = {funcName, func};
			FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(owningObject);
			//item.TickFuncMapping = &this->TickFuncMapping;	
		}
		else
		{
			FString str = funcName.ToString();
		
			FLuaFunctionMapping* existing = this->GetFuncMapping(funcName);

			if(existing)
			{
				existing->LuaScriptFunction = func;
			}
			else
			{
				this->FuncMapping.Emplace(FLuaFunctionMapping{funcName, func});
			}
		}	
	}
	else
	{
		
		if(funcName == UnrealLua::PropertyNames::NAME_ReceiveTick || funcName == UnrealLua::PropertyNames::NAME_UserWidgetTick)
		{
			this->TickFuncMapping = {};
			auto& item = UnrealLua::UObjectRegistry::GetUObjectItem(owningObject);
			//item.TickFuncMapping = nullptr;
		}
		else
		{
			this->FuncMapping.Remove({funcName, sol::nil});
		}
	}
}
/*
FLuaFunctionMapping* FLuaScriptFunctionMap::GetFuncMapping(FName funcName, FName objectName)
{
	return this->MainObjectMappings.GetFuncMapping(funcName);
}

FLuaScriptObjectFunctionMap* FLuaScriptFunctionMap::GetMappingsForObject(FName objectName)
{
	if(objectName == NAME_None)
	{
		return &this->MainObjectMappings;
	}
	return nullptr;
}
*/