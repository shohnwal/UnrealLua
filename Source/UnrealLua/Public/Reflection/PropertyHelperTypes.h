#pragma once
#include "CoreMinimal.h"
#include "sol/sol.hpp"
#include "Runtime/CoreUObject/Public/UObject/ObjectMacros.h"

struct FHashedFieldMapping;
class FProperty;




enum class EGetPropertyType : bool
{
	VALUE,
	REFERENCE
};

enum ELuaSupportedClassCastFlags : uint64
{
	LUA_CASTCLASS_None = 0x0000000000000000,

	LUA_CASTCLASS_FInt8Property					= CASTCLASS_FInt8Property,
	LUA_CASTCLASS_FByteProperty					= CASTCLASS_FByteProperty,
	LUA_CASTCLASS_FIntProperty					= CASTCLASS_FIntProperty,
	LUA_CASTCLASS_FFloatProperty				= CASTCLASS_FFloatProperty,
	LUA_CASTCLASS_FUInt64Property				= CASTCLASS_FUInt64Property,
	LUA_CASTCLASS_FClassProperty				= CASTCLASS_FClassProperty,
	LUA_CASTCLASS_FUInt32Property				= CASTCLASS_FUInt32Property,
	LUA_CASTCLASS_FInterfaceProperty			= CASTCLASS_FInterfaceProperty,
	LUA_CASTCLASS_FNameProperty					= CASTCLASS_FNameProperty,
	LUA_CASTCLASS_FStrProperty					= CASTCLASS_FStrProperty,
	LUA_CASTCLASS_FObjectProperty				= CASTCLASS_FObjectProperty,
	LUA_CASTCLASS_FBoolProperty					= CASTCLASS_FBoolProperty,
	LUA_CASTCLASS_FUInt16Property				= CASTCLASS_FUInt16Property,
	LUA_CASTCLASS_FStructProperty				= CASTCLASS_FStructProperty,
	LUA_CASTCLASS_FArrayProperty				= CASTCLASS_FArrayProperty,
	LUA_CASTCLASS_FInt64Property				= CASTCLASS_FInt64Property,
	LUA_CASTCLASS_FSingleDelegateProperty		= CASTCLASS_FDelegateProperty,
	LUA_CASTCLASS_FMulticastDelegateProperty	= CASTCLASS_FMulticastDelegateProperty,
	LUA_CASTCLASS_FWeakObjectProperty			= CASTCLASS_FWeakObjectProperty,
	LUA_CASTCLASS_FSoftObjectProperty			= CASTCLASS_FSoftObjectProperty,
	LUA_CASTCLASS_FTextProperty					= CASTCLASS_FTextProperty,
	LUA_CASTCLASS_FInt16Property				= CASTCLASS_FInt16Property,
	LUA_CASTCLASS_FDoubleProperty				= CASTCLASS_FDoubleProperty,
	LUA_CASTCLASS_FMapProperty					= CASTCLASS_FMapProperty,
	LUA_CASTCLASS_FSetProperty					= CASTCLASS_FSetProperty,
	LUA_CASTCLASS_FEnumProperty					= CASTCLASS_FEnumProperty,
};


constexpr static uint64 supportedPropTypeFlags =
	LUA_CASTCLASS_FBoolProperty
	+ LUA_CASTCLASS_FIntProperty
	+ LUA_CASTCLASS_FFloatProperty
	+ LUA_CASTCLASS_FUInt64Property
	+ LUA_CASTCLASS_FObjectProperty
	+ LUA_CASTCLASS_FWeakObjectProperty
	+ LUA_CASTCLASS_FClassProperty
	+ LUA_CASTCLASS_FInt8Property
	+ LUA_CASTCLASS_FStructProperty
	+ LUA_CASTCLASS_FTextProperty
	+ LUA_CASTCLASS_FStrProperty
	+ LUA_CASTCLASS_FNameProperty
	+ LUA_CASTCLASS_FInt16Property
	+ LUA_CASTCLASS_FInt64Property
	+ LUA_CASTCLASS_FByteProperty
	+ LUA_CASTCLASS_FDoubleProperty
	+ LUA_CASTCLASS_FUInt32Property
	+ LUA_CASTCLASS_FUInt16Property
	+ LUA_CASTCLASS_FEnumProperty
	+ LUA_CASTCLASS_FArrayProperty
	+ LUA_CASTCLASS_FMapProperty
	+ LUA_CASTCLASS_FSingleDelegateProperty
	+ LUA_CASTCLASS_FMulticastDelegateProperty
	+ LUA_CASTCLASS_FInterfaceProperty
	+ LUA_CASTCLASS_FSetProperty
	+ LUA_CASTCLASS_FSoftObjectProperty
;

struct UNREALLUA_API FUFunctionCallInputLuaObjectRecordItem
{
	FProperty* InputProp;
	sol::object LuaObj;
};

typedef TArray<FUFunctionCallInputLuaObjectRecordItem,TInlineAllocator<4>> FUFunctionCallInputLuaObjectRecord;
//typedef TArray<FUFunctionCallInputLuaObjectRecordItem,TInlineAllocator<UnrealLua::Compilation::NUM_PREALLOCATED_FUNCDESCR_ARGVALS>> FUFunctionCallInputLuaObjectRecord;

struct UNREALLUA_API FGetPropertyValueAsLuaSyntaxStringParams
{
	FProperty* Prop;
	void* MemoryPtr;
	bool ContainerAsTable;
	int32 ArrayIndex = 0;
};

struct UNREALLUA_API FPushPropertyValueParams
{
	FProperty* Prop;
	void* MemoryPtr;
	int32 ArrayIndex;
	sol::this_state Lua;
	FUFunctionCallInputLuaObjectRecord* InputRecord = nullptr;
};

struct UNREALLUA_API  FGetPropertyValueParams
{
	FProperty* Prop;
	void* MemoryPtr;
	int32 ArrayIndex;
	sol::this_state Lua;
	FUFunctionCallInputLuaObjectRecord* InputRecord = nullptr;
};

struct UNREALLUA_API FWriteScriptValueToPropertyAsValueParams
{
	UObject* const ScriptOwner;
	FProperty* TargetProp;
	void* TargetMemAddress;
};

template<typename LUAOBJ>
struct UNREALLUA_API TSetPropertyValueParams
{
	FProperty* Prop;
	void* MemoryPtr;
	int32 ArrayIndex;
	const LUAOBJ& LuaValue;
	FUFunctionCallInputLuaObjectRecord* InputRecord = nullptr;
};

struct UNREALLUA_API FSetLuaScriptUObjectMemberPropertyWrapperParams
{
	UObject* const ScriptOwner;
	const FHashedFieldMapping& PropMapping;
	FName GetMappingFName() const;
};

struct UNREALLUA_API FLuaCallArgsWrapper
{
	sol::this_state Lua;
	sol::variadic_args vargs;
	std::vector<sol::object> vobjects;
};

constexpr static uint64 NUMCASTCLASS = 65;

using isSupportedValueLUTFunc = bool(*)(FProperty*, const sol::object&);
extern const isSupportedValueLUTFunc isCombatibleTypeLUT[NUMCASTCLASS];

using getPropertyValueLUTFunc = sol::object(*)(const FGetPropertyValueParams&);
extern const getPropertyValueLUTFunc GetPropertyValueLUT[NUMCASTCLASS];

//using applyToLuaValuePtrForPropertyLUTFunc = sol::object(*)(FProperty* prop, const sol::object& luaValue, const TFunction<sol::object(void*)>& func);
//extern const applyToLuaValuePtrForPropertyLUTFunc ApplyToLuaValuePtrForPropertyLUT[NUMCASTCLASS];

DECLARE_DELEGATE_TwoParams(FOnSetCustomStructPropertyValueDelegate, TSetPropertyValueParams<sol::object>, UScriptStruct*);
DECLARE_DELEGATE_TwoParams(FOnSetCustomStructPropertyValueDelegate_Stack, TSetPropertyValueParams<sol::stack_object>, UScriptStruct*);
DECLARE_DELEGATE_TwoParams(FOnSetCustomStructPropertyValueDelegate, TSetPropertyValueParams<sol::object>, UScriptStruct*);

DECLARE_DELEGATE_RetVal_ThreeParams(bool, FOnGetCustomStructPropertyValueDelegate, FGetPropertyValueParams, UScriptStruct*, sol::object*);

namespace UnrealLua::PropertyHelper
{
	static FOnGetCustomStructPropertyValueDelegate OnGetCustomStructPropertyValue;
	static FOnSetCustomStructPropertyValueDelegate_Stack OnSetCustomStructPropertyValue_Stack;
	static FOnSetCustomStructPropertyValueDelegate OnSetCustomStructPropertyValue;
}