#pragma once

#include "UObject/Class.h"
#include "UnrealLuaOverrideFunctionHostClass.generated.h"

UCLASS()
class UNREALLUA_API UUnrealLuaOverrideFunctionHostClass : public UClass
{
	GENERATED_BODY()
public:
	static UUnrealLuaOverrideFunctionHostClass* Create(UClass* baseClass);

	virtual void BeginDestroy() override;
	virtual uint8* GetPersistentUberGraphFrame(UObject* Obj, UFunction* FuncToCheck) const override;

	UPROPERTY()
	TObjectPtr<UClass> Owner = nullptr;
};
