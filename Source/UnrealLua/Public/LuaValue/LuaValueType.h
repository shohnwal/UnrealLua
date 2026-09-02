#pragma once
#include "LuaValueType.generated.h"

struct FFunctionDescr;
class UObject;

UENUM(BlueprintType)
enum class ELuaValueType : uint8
{
	Uninitialized,
	Nil,
	Property,
	Boolean,
	Integer,
	Float,
	String,
	Vector2D,
	Vector,
	Rotator,
	UObject,
	Transform,
	LuaRPCFunction,
	UClass,
	UScriptStruct,
	EnumValue,
	UFunction,
	
	ScriptStruct,
	InstancedStruct,
	SharedStruct,

	LuaTable,
	LuaFunction,
	Coroutine,

	SingleDelegate,
	MulticastDelegate,
	Array,
	Set,
	Map,
	UEnum
};
ENUM_CLASS_FLAGS(ELuaValueType)