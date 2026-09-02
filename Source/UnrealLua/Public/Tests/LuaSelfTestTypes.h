#pragma once
#include "CoreMinimal.h"
#include "Interface/LuaScriptable.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StructUtils/InstancedStruct.h"
#include "StructUtils/SharedStruct.h"
#include "sol/sol.hpp"
#include "Runtime/CoreUObject/Public/Templates/SubclassOf.h"
#include "LuaSelfTestTypes.generated.h"

class UTestScriptStructLibrary;

USTRUCT(BlueprintType)
struct UNREALLUA_API FTestScriptStruct
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite)
	FString msg = "";
	UPROPERTY(BlueprintReadWrite)
	int32 x = 0;
	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<UTestScriptStructLibrary> LuaStructUFunctionLibrary = nullptr;
};

UCLASS(BlueprintType)
class UNREALLUA_API UTestScriptStructLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION()
	static bool TestFunc(UPARAM(ref) FTestScriptStruct& strct);
	/*
	UFUNCTION(BlueprintCallable, CustomThunk, meta=(DefaultToSelf=worldContext, CustomStructureParam=input))
	static bool LoadStructLuaScript(UObject* owningObject, int32 input);
	DECLARE_FUNCTION(execLoadStructLuaScript);
	*/
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FLuaTestMultiDelegate, bool, b, int32, i, float, f, FString, s, FTestScriptStruct, strct, UObject*, obj);
DECLARE_DYNAMIC_DELEGATE_SixParams(FLuaTestSingleDelegate, bool, b, int32, i, float, f, FString, s, FTestScriptStruct, strct, UObject*, obj);

template<typename T>
struct UNREALLUA_API FLuaSelfTestScopedKey
{
	FLuaSelfTestScopedKey(const char* key, T& value, sol::state_view& lua)
		: key(key), lua(lua)
	{
		lua[key] = value;
	}
	~FLuaSelfTestScopedKey()
	{
		lua[key] = sol::nil;
	}
	const char* key;
	sol::state_view& lua;
};
UENUM()
enum class EUnrealLuaTestEnum : uint8
{
	One,
	Two,
	Three
};

UCLASS(BlueprintType, Transient)
class UNREALLUA_API UUnrealLuaTestObject : public UObject, public ILuaScriptable
{
	GENERATED_BODY()
public:
	UPROPERTY()
	bool Bool = true;
	
	UPROPERTY()
	int8 Int8 = INT8_MAX;
	UPROPERTY()
	int16 Int16 = INT16_MAX;
	UPROPERTY()
	int32 Int32 = INT32_MAX;
	UPROPERTY()
	int64 Int64 = INT64_MAX;
	UPROPERTY()
	uint8 UInt8 = UINT8_MAX;
	UPROPERTY()
	uint16 UInt16 = UINT16_MAX;
	UPROPERTY()
	uint32 UInt32 = UINT32_MAX;
	UPROPERTY()
	uint64 UInt64 = UINT64_MAX;

	virtual bool IsSupportedForNetworking() const override
	{
		return false;
	}

	UPROPERTY()
	float Float = PI;
	UPROPERTY()
	double Double = DOUBLE_PI;

	UPROPERTY()
	FString String = "yay";
	UPROPERTY()
	FName Name = "nay";
	UPROPERTY()
	FText Text = FText::FromString(FString("may"));

	UPROPERTY()
	EUnrealLuaTestEnum TestEnum = EUnrealLuaTestEnum::Two;
	//UPROPERTY()
	//TEnumAsByte<EUnrealLuaTestEnum> TestEnumAsByte;

	UPROPERTY()
	FTestScriptStruct TestScriptStruct = {};
	UPROPERTY()
	FInstancedStruct TestInstancedStruct = {};
	UPROPERTY()
	FSharedStruct TestSharedStruct = {};
	UPROPERTY()
	TObjectPtr<UObject> Object = nullptr;
	
	UPROPERTY(BlueprintReadWrite, BlueprintAssignable)
	FLuaTestMultiDelegate MultiDelegate = {};
	UPROPERTY(BlueprintReadWrite)
	FLuaTestSingleDelegate SingleDelegate = {};

	UPROPERTY()
	TArray<FString> StringArray;
	UPROPERTY()
	TSet<FString> StringSet;
	UPROPERTY()
	TMap<FString, int32> StringInt32Map;

	UFUNCTION()
	bool TestStringRef(UPARAM(ref) FString& param) { param = "yay"; return true; }
	UFUNCTION()
	bool TestVectorRef(UPARAM(ref) FVector& param) { param.X = 1; param.Y = 2; param.Z = 3; return true; }
	UFUNCTION()
	bool TestArrayRef(UPARAM(ref) TArray<int32>& param)
	{
		param.Add(123);
		param.Add(456);
		return true;
	}
	UFUNCTION()
	bool TestSetRef(UPARAM(ref) TSet<int32>& param)
	{
		param.Add(123);
		param.Add(456);
		return true;
	}
	UFUNCTION()
	bool TestMapRef(UPARAM(ref) TMap<int32, FString>& param)
	{
		param.Add(123, "chirp");
		param.Add(456, "...");
		return true;
	}
	UFUNCTION()
	bool TestStructRef(UPARAM(ref) FTestScriptStruct& param)
	{
		param.x = 999; param.msg = "cat"; return true;
	}
	UFUNCTION()
	bool TestUObjectRef(UObject*& param) { param = this; return true; }

	UFUNCTION()
	int32 TestFuncArgs(int32 onetwothree, float aboutpi, bool istrue, FString meow, FName barf, UObject* self)
	{
		verify(onetwothree == 123);
		verify(FMath::IsNearlyEqual(aboutpi, 3.14f, 0.01f));
		verify(istrue == true);
		verify(meow == "meow");
		verify(barf == "barf");
		verify(self == this);
		int32 fourfivesix = 456;
		return 456;
	}
	
	UFUNCTION()
	int32 TestDelCallbackArgs(int32 onetwothree, float aboutpi, bool istrue, FString meow, FName barf, UObject* self)
	{
		this->Int32 = onetwothree;
		this->Float = aboutpi;
		this->Bool = istrue;
		this->String = meow;
		this->Name = barf;
		this->Object = self;
		return 999;
	}

	UFUNCTION()
	int32 FuncInt32(int32 x)
	{
		return x;
	}

	UFUNCTION()
	int32 FuncInt64(int64 x)
	{
		return x;
	}

	UFUNCTION()
	float FuncFloat(float x)
	{
		return x;
	}

	UFUNCTION()
	double FuncDouble(double x)
	{
		return x;
	}

	UFUNCTION()
	FString FuncFString(FString x)
	{
		return x;
	}

	UFUNCTION()
	FName FuncFName(FName x)
	{
		return x;
	}

	UFUNCTION()
	FTestScriptStruct FuncStructCopy(FTestScriptStruct x)
	{
		return x;
	}

	UFUNCTION()
	FTestScriptStruct& FuncStructRef(FTestScriptStruct& x)
	{
		return x;
	}
	
	UFUNCTION()
	UObject* FuncUObject(UObject* x)
	{
		return x;
	}

	UFUNCTION()
	TArray<int32> FuncArrayCopy(TArray<int32> x)
	{
		return x;
	}

	UFUNCTION()
	void FuncArrayRef(UPARAM(ref) TArray<int32>& x)
	{
		x.Add(999);
	}

	UFUNCTION()
	TMap<int32, int32> FuncMapCopy(TMap<int32, int32> x)
	{
		return x;
	}

	UFUNCTION()
	void FuncMapRef(UPARAM(ref) TMap<int32, int32>& x)
	{
		x.Add(777, 999);
	}

	UFUNCTION()
	TSet<int32> FuncSetCopy(TSet<int32> x)
	{
		return x;
	}

	UFUNCTION()
	void FuncSetRef(UPARAM(ref) TSet<int32>& x)
	{
		x.Add(999);
	}

	
	
	virtual void BeginDestroy() override;
	virtual void OnClusterMarkedAsPendingKill() override;
};

UCLASS(BlueprintType, Transient)
class UNREALLUA_API UUnrealLuaTestNetObject : public UUnrealLuaTestObject
{
	GENERATED_BODY()
	virtual bool IsSupportedForNetworking() const override
	{
		return true;
	}
};

UCLASS(BlueprintType, Transient)
class UNREALLUA_API ULuaScriptableTestObject : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite)
	FString msg = "boo";
	UFUNCTION()
	void OnMultiDelegate(bool b, int32 i, float f, FString str, FTestScriptStruct strct, UObject* obj);
	
	UFUNCTION()
	void OnSingleDelegate(bool b, int32 i, float f, FString str, FTestScriptStruct strct, UObject* obj);
};