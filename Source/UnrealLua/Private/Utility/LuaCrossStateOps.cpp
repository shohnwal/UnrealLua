// Fill out your copyright notice in the Description page of Project Settings.


#include "Utility/LuaCrossStateOps.h"

#include "LuaValue/LuaValueType.h"
#include "Utility/UnrealLuaHash.h"
#include "Reflection/FunctionDescr.h"
#include "UnrealLua.h"
#include "LuaTypes/LuaScriptStruct.h"
#include "LuaTypes/LuaUClass.h"
#include "LuaTypes/LuaUStruct.h"
#include "sol/sol.hpp"

sol::object UnrealLua::CrossLuaState::CopyLuaObjectToAnotherLuaState(sol::object& from, sol::this_state to)
{
	if(!from.valid() || to.lua_state() == nullptr)
	{
		return sol::nil;
	}
	ELuaValueType type = UnrealLua::HashUtility::GetLuaValueType(from);
	switch(type)
	{
	case ELuaValueType::Nil:
		return sol::nil;
	case ELuaValueType::Boolean:
		return sol::object(to, sol::in_place, from.as<bool>());
	case ELuaValueType::Integer:
		return sol::object(to, sol::in_place, from.as<int64>());
	case ELuaValueType::Float:
		return sol::object(to, sol::in_place, from.as<double>());
	case ELuaValueType::String:
		return sol::object(to, sol::in_place, from.as<std::string>());
	case ELuaValueType::Vector2D:
		{
			FVector2D val = from.as<FVector2D>();
			return sol::object(to, sol::in_place_type<FVector2D>, val);
		}
	case ELuaValueType::Vector:
		{
			FVector val = from.as<FVector>();
			return sol::object(to, sol::in_place_type<FVector>, val);
		}
	case ELuaValueType::Rotator:
		{
			FRotator val = from.as<FRotator>();
			return sol::object(to, sol::in_place_type<FRotator>, val);
		}
	case ELuaValueType::UObject:
		{
			UObject* val = UnrealLua::LightUserdata::GetUObject(from); //from.as<FLuaUObjectWrapper>().Get();
			return sol::object(to, sol::in_place, val);
		}
	case ELuaValueType::UClass:
		{
			FLuaUClass val = from.as<FLuaUClass>();
			return sol::object(to, sol::in_place, val);
		}
	case ELuaValueType::UScriptStruct:
		{
			FLuaUStruct val = from.as<FLuaUStruct>();
			return sol::object(to, sol::in_place,  val);
		}
	case ELuaValueType::ScriptStruct:
		{
			FLuaScriptStruct val = from.as<FLuaScriptStruct>();
			return sol::object(to, sol::in_place	, val);
		}
	case ELuaValueType::LuaTable:
		return sol::nil;
	case ELuaValueType::LuaFunction:
		return sol::nil;
	case ELuaValueType::UFunction:
		{
			FFunctionDescr* val = from.as<FFunctionDescr*>();
			return sol::make_object(to, sol::light(val));
		}
	case ELuaValueType::Coroutine:
	case ELuaValueType::SingleDelegate:
	case ELuaValueType::MulticastDelegate:
	case ELuaValueType::Array:
	case ELuaValueType::Set:
	case ELuaValueType::Map:
	case ELuaValueType::Transform:
	case ELuaValueType::Property:
		break;
	default: ;
	}
	return sol::nil;
}
