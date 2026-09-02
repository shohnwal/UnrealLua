// Fill out your copyright notice in the Description page of Project Settings.


#include "Interface/LuaScriptable.h"

#include "Utility/LuaLogMacros.h"
#include "LuaContext/ScopedLuaContext.h"
#include "Interface/LuaContext.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"

FLuaScriptSettings ILuaScriptable::GetLuaScriptSettings_Implementation()
{
	UObject* obj = this->_getUObject();
	LUA_LOG_WARNING("Object class %s did not override GetLuaScriptSettings, will use empty LuaScriptSettings", *GetNameSafe(obj->GetClass()));
	static FLuaScriptSettings dummySettings{};
	return dummySettings;
}

void ILuaScriptable::SetLuaScriptSettings_Implementation(FLuaScriptSettings newSettings)
{
	UObject* obj = this->_getUObject();
	LUA_LOG_WARNING("Object class %s did not override SetLuaScriptSettings, will not apply new settings", *GetNameSafe(obj->GetClass()));
}

bool ILuaScriptable::LoadLuaScript()
{
	UObject* myobj = this->_getUObject();
	return UnrealLua::UObjectRegistry::LoadLuaScript(myobj, true);
}

FLuaNetHandle ILuaScriptable::GetUniqueLuaNetHandle_Implementation(int32 input)
{
	return FLuaNetHandle{};
}

FLuaUObjectItem& ILuaScriptable::GetUObjectItem()
{
	UObject* obj = this->_getUObject();
	return UnrealLua::UObjectRegistry::GetUObjectItem(obj);
}
/*
void ILuaScriptable::InputAction(AActor* actor, FName funcName, EInputEvent IE)
{
	FLuaUObjectItem& entry = UnrealLua::UObjectRegistry::GetUObjectEntry(actor);
	UEnum* uenum = StaticEnum<ETriggerEvent>();
	FLuaUEnumEntry enumEntry{uenum, (int64)IE};
	UnrealLua::LuaScriptCall::CallLuaFunctionSafeByName(&entry, funcName, actor, enumEntry);
}

void ILuaScriptable::EnhancedActionAxis1D(const FInputActionValue& Value,AActor* actor, FName funcName, ETriggerEvent triggerEvent)
{
	// input is a float
	float movementVector = Value.Get<float>();
	UEnum* uenum = StaticEnum<ETriggerEvent>();
	FLuaUEnumEntry enumEntry{uenum, (int64)triggerEvent};
	FLuaUObjectItem& entry = UnrealLua::UObjectRegistry::GetUObjectEntry(actor);
	UnrealLua::LuaScriptCall::CallLuaFunctionSafeByName(&entry, funcName, actor, movementVector,enumEntry);
}

void ILuaScriptable::EnhancedActionAxis2D(const FInputActionValue& Value, AActor* actor, FName funcName, ETriggerEvent triggerEvent)
{
	// input is a Vector2D
	FVector2D movementVector = Value.Get<FVector2D>();

	FLuaUObjectItem& entry = UnrealLua::UObjectRegistry::GetUObjectEntry(actor);
	UEnum* uenum = StaticEnum<ETriggerEvent>();
	FLuaUEnumEntry enumEntry{uenum, (int64)triggerEvent};
	UnrealLua::LuaScriptCall::CallLuaFunctionSafeByName(&entry, funcName, actor, movementVector, enumEntry);
}

void ILuaScriptable::EnhancedActionAxis3D(const FInputActionValue& Value, AActor* actor, FName funcName, ETriggerEvent triggerEvent)
{
	// input is a Vector2D
	FVector movementVector = Value.Get<FVector>();

	FLuaUObjectItem& entry = UnrealLua::UObjectRegistry::GetUObjectEntry(actor);
	UEnum* uenum = StaticEnum<ETriggerEvent>();
	FLuaUEnumEntry enumEntry{uenum, (int64)triggerEvent};
	UnrealLua::LuaScriptCall::CallLuaFunctionSafeByName(&entry, funcName, actor, movementVector, enumEntry);
}
*/

/*
TScriptInterface<ILuaContext> ILuaScriptable::GetLuaContext(UObject* obj)
{
	TScriptInterface<ILuaScriptable>ls{obj};
	if(ls)
	{
		verify(IsValid(obj));
		FLuaScriptSettings settings = ILuaScriptable::Execute_GetLuaScriptSettings(obj); 
		return settings.GetLuaContext(obj);		
	}
	return nullptr;
}
*/
