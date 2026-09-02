#pragma once
#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "LuaUObjectInputOverrides.generated.h"

class UEnhancedInputComponent;
class UInputComponent;
struct FLuaUObjectItemHandle;

USTRUCT()
struct UNREALLUA_API FLuaActionMapping
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere)
	FName FuncName = "";
	UPROPERTY(VisibleAnywhere)
	uint32 Handle = 0;
};
USTRUCT()
struct UNREALLUA_API FLuaEnhancedActionMapping
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere)
	FName FuncName = "";
	UPROPERTY(VisibleAnywhere)
	uint32 Handle = 0;
	UPROPERTY(VisibleAnywhere)
	ETriggerEvent TriggerEventType = ETriggerEvent::None;
	TUniquePtr<FEnhancedInputActionEventBinding> OriginalBinding = nullptr;
};


template<>
struct TStructOpsTypeTraits<FLuaEnhancedActionMapping> : public TStructOpsTypeTraitsBase2<FLuaEnhancedActionMapping>
{
	enum
	{
		WithZeroConstructor = false,
		WithCopy = false
	};
};

USTRUCT()
struct UNREALLUA_API FLuaEnhancedInputActionMapping
{
	GENERATED_BODY()
	FLuaEnhancedInputActionMapping() {}
	FLuaEnhancedInputActionMapping(const UInputAction*);
	FLuaEnhancedInputActionMapping(FLuaEnhancedInputActionMapping&&) = default;
	FLuaEnhancedInputActionMapping(const FLuaEnhancedInputActionMapping&) = delete;
	FLuaEnhancedInputActionMapping& operator=(const FLuaEnhancedInputActionMapping&) = delete;
	FLuaEnhancedInputActionMapping& operator=(FLuaEnhancedInputActionMapping&&) = default;

	UPROPERTY(VisibleAnywhere)
	const UInputAction* InputAction = nullptr;
	UPROPERTY(VisibleAnywhere)
	TArray<FLuaEnhancedActionMapping> EventMappings = {};
};

template<>
struct TStructOpsTypeTraits<FLuaEnhancedInputActionMapping> : public TStructOpsTypeTraitsBase2<FLuaEnhancedInputActionMapping>
{
	enum
	{
		WithZeroConstructor = false,
		WithCopy = false
	};
};

USTRUCT()
struct UNREALLUA_API FLuaUObjectInputOverrides
{
	GENERATED_BODY()
	FLuaUObjectInputOverrides() {}
	FLuaUObjectInputOverrides(UObject* owningObject, UInputComponent* inputcmp);
	~FLuaUObjectInputOverrides();

	void ClearInputMappings();
	void BindInputFunctions();

private:
	void BindEnhancedActionEvents(AActor* Actor, UEnhancedInputComponent* Cmp);
	void BindActionEvents(AActor* Actor, UInputComponent* Input);
	void BindAxisEvents(AActor* Actor, UInputComponent* Input);
	
	void NotifyInputAction(AActor* actor, FName funcName, EInputEvent IE);
	void NotifyInputAxis(float AxisValue);
	void NotifyEnhancedAction(const int32 eventActionMappingIndex, const FInputActionInstance& InputActionInstance, TWeakObjectPtr<UObject> target, const FName funcName);
	TWeakObjectPtr<UObject> OwningObject = nullptr;
	
	TWeakObjectPtr<UInputComponent> InputComponent = nullptr;
	
	UPROPERTY(VisibleAnywhere)
	TArray<FLuaActionMapping> InputMappings = {};
	UPROPERTY(VisibleAnywhere)
	TArray<FLuaEnhancedInputActionMapping> EnhancedInputActionMappings = {};
};


template<>
struct TStructOpsTypeTraits<FLuaUObjectInputOverrides> : public TStructOpsTypeTraitsBase2<FLuaUObjectInputOverrides>
{
	enum
	{
		WithZeroConstructor = false,
		WithCopy = false
	};
};
