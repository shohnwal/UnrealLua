
#include "Reflection/PropertyDescr/StructPropertyDescr.h"
#include "LuaValue/LuaValue.h"
#include "Reflection/PropertyHelper_ToString.h"

template <typename LUAOBJ>
void FStructPropertyDescr::SetLuaValuePropertyValue(const TSetPropertyValueParams<LUAOBJ>& params)
{
	FStructProperty* prop = CastField<FStructProperty>(params.Prop);
	FLuaValue* luaValue = static_cast<FLuaValue*>(params.MemoryPtr);
	luaValue->SetValue(params.LuaValue);
	luaValue->ConvertLuaObjectsToHandles();
}

template void FStructPropertyDescr::SetLuaValuePropertyValue(const TSetPropertyValueParams<sol::object>& params);
template void FStructPropertyDescr::SetLuaValuePropertyValue(const TSetPropertyValueParams<sol::stack_object>& params);


sol::object FStructPropertyDescr::GetLuaValuePropertyValue(const FGetPropertyValueParams& params)
{
	FStructProperty* prop = CastField<FStructProperty>(params.Prop);
	FLuaValue* luaValue = static_cast<FLuaValue*>(params.MemoryPtr);
	return luaValue->GetValue(params.Lua);	
}

int FStructPropertyDescr::GetLuaValuePropertyValue(const FPushPropertyValueParams& params)
{
	FStructProperty* prop = CastField<FStructProperty>(params.Prop);
	FLuaValue* luaValue = static_cast<FLuaValue*>(params.MemoryPtr);
	sol::object obj = luaValue->GetValue(params.Lua);
	return sol::stack::push(params.Lua, obj);
}

FString FStructPropertyDescr::GetInstancedStructPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params)
{
	return "nil";
}

FString FStructPropertyDescr::GetSharedStructPropertyValueAsLuaSyntaxValidString(
	const FGetPropertyValueAsLuaSyntaxStringParams& params)
{
	return "nil";
}

FString FStructPropertyDescr::GetLuaValuePropertyValueAsLuaSyntaxValidString(
	const FGetPropertyValueAsLuaSyntaxStringParams& params)
{
	return "nil";
}

FString FStructPropertyDescr::GetLuaFunctionPropertyValueAsLuaSyntaxValidString(
	const FGetPropertyValueAsLuaSyntaxStringParams& params)
{
	return "nil";
}

FString FStructPropertyDescr::GetLuaTablePropertyValueAsLuaSyntaxValidString(
	const FGetPropertyValueAsLuaSyntaxStringParams& params)
{
	return "nil";
}

FString FStructPropertyDescr::GetPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params)
{
	FStructProperty* prop = CastField<FStructProperty>(params.Prop);
	if(prop->Struct->PropertyLink && prop->Struct->PropertyLink->Next == nullptr)
	{
		FGetPropertyValueAsLuaSyntaxStringParams subParams{prop->Struct->PropertyLink, params.MemoryPtr, params.ContainerAsTable, 0};
		return UnrealLua::PropertyHelper::GetPropertyValueAsLuaSyntaxValidString(subParams);
	}
	if (params.MemoryPtr == nullptr)
	{
		return UnrealLua::PropertyHelper::GetPropertyTypeName(prop, true);
	}
	else
	{
		return UnrealLua::PropertyHelper::GetPropertyTypeName(prop, true);
	}
}
