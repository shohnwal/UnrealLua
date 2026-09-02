#include "UnrealOverrides/UnrealLuaOverrideFunctionHostClass.h"
#include "UnrealOverrides/UnrealLuaOverrideUFunction.h"
#include "UObject/Package.h"

UUnrealLuaOverrideFunctionHostClass* UUnrealLuaOverrideFunctionHostClass::Create(UClass* baseClass)
{
	FString ClassNameString = FString::Printf(TEXT("LUA_OVERRIDES_%s"), *baseClass->GetName());
	FName ClassName = MakeUniqueObjectName(GetTransientPackage(), baseClass, FName(*ClassNameString));
	UUnrealLuaOverrideFunctionHostClass* Ret = NewObject<UUnrealLuaOverrideFunctionHostClass>(GetTransientPackage(), ClassName, RF_Public | RF_Transient);
	Ret->ClassFlags |= CLASS_NewerVersionExists; // bypass FBlueprintActionDatabase::RefreshClassActions
	Ret->SetDefaultObject(baseClass->GetDefaultObject());
	Ret->SetSuperStruct(StaticClass());
	Ret->Bind();
	Ret->Owner = baseClass;
	Ret->AddToRoot();
	return Ret;
}

void UUnrealLuaOverrideFunctionHostClass::BeginDestroy()
{
	this->SetDefaultObject(nullptr);
	//this->RemoveFromRoot();
	UClass::BeginDestroy();
}

uint8* UUnrealLuaOverrideFunctionHostClass::GetPersistentUberGraphFrame(UObject* Obj, UFunction* FuncToCheck) const
{
	UFunction* func = FuncToCheck;
	if (UUnrealLuaOverrideUFunction* luaFunc = Cast<UUnrealLuaOverrideUFunction>(FuncToCheck))
	{
		func = luaFunc->Overridden;
	}
	return this->Owner->GetPersistentUberGraphFrame(Obj, func);
}
