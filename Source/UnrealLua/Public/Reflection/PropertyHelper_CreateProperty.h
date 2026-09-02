#pragma once
#include "CoreMinimal.h"
class FProperty;

enum ELuaSupportedClassCastFlags : uint64;

namespace UnrealLua::PropertyHelper
{
	UNREALLUA_API FProperty* CreateNewProperty(uint64 classFlags, UObject* propClass, FName propName, EObjectFlags propFlags);
	UNREALLUA_API FProperty* CreateNewProperty(ELuaSupportedClassCastFlags typeFlags, UObject* propClass, FName propName, EObjectFlags propFlags);
	//Used by TArray/TMap/TSet stackhandler, with an optional object class for scriptstruct/uobject/uclass properties
	UNREALLUA_API FProperty* CreateNewProperty(FFieldClass* fieldClass, UObject* propClass, FName propName = NAME_None, EObjectFlags propFlags = RF_Public);
	
	//used by Lua TArray/TMap/TSet constructors, will make a copy of "otherProp" 
	UNREALLUA_API FProperty* CreateNewProperty(FProperty* otherProp, FName propName = NAME_None, EObjectFlags propFlags = RF_Public);
	
	//Used by UnrealLua compiler and Lua Array/Map/Set __call constructors to create a property based on 'luaParam' type
	UNREALLUA_API FProperty* CreateNewProperty(const sol::object& luaParam, FName propName = NAME_None, EPropertyFlags propFlags = CPF_None, EObjectFlags objectFlags = RF_Public);
}