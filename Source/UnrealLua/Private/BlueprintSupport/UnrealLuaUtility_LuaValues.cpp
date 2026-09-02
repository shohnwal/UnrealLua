
#include "BlueprintSupport/UnrealLuaUtility.h"
#include "LuaValue/LuaValueType.h"


bool UUnrealLuaUtility::MakeLuaValue(const int32 numItems)
{
	return false;
}

DEFINE_FUNCTION(UUnrealLuaUtility::execMakeLuaValue)
{
	int32 numItems = 0;
	Stack.StepCompiledIn<FIntProperty>(&numItems);
	
	ESetLuaValueResult setLuaValueResult = ESetLuaValueResult::Success;
	for (int32 argIndex = 0; argIndex < numItems; ++argIndex)
	{
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.MostRecentProperty = nullptr;
		
		Stack.StepCompiledIn<FProperty>(nullptr);
		check(Stack.MostRecentProperty && Stack.MostRecentPropertyAddress);
		FProperty* sourceValueProp = CastField<FProperty>(Stack.MostRecentProperty);
		const void* sourceValuePtr = sourceValueProp->ContainerPtrToValuePtr<const void*>(Stack.MostRecentPropertyContainer);
		
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.MostRecentProperty = nullptr;
		Stack.StepCompiledIn<FStructProperty>(nullptr);
		check(Stack.MostRecentProperty && Stack.MostRecentPropertyAddress);

		FStructProperty* targetProperty = CastFieldChecked<FStructProperty>(Stack.MostRecentProperty);
		FLuaValue* outPropertyValueAddress = targetProperty->ContainerPtrToValuePtr<FLuaValue>(Stack.MostRecentPropertyContainer);
		
		setLuaValueResult |= outPropertyValueAddress->SetValue(sourceValueProp, sourceValuePtr);
		
		outPropertyValueAddress->ConvertLuaObjectsToHandles();
		outPropertyValueAddress->ClearIsScriptValue();
	}
	
	P_FINISH;
	
	*(bool*)RESULT_PARAM = !EnumHasAllFlags(setLuaValueResult, ESetLuaValueResult::Error);
}

 bool UUnrealLuaUtility::MakeLuaValuesArray(const int32 numItems, TArray<FLuaValue>& outValuesArray)
 {
	return false;
 }

DEFINE_FUNCTION(UUnrealLuaUtility::execMakeLuaValuesArray)
{
	int32 numItems = 0;
	Stack.StepCompiledIn<FIntProperty>(&numItems);
	
	P_GET_TARRAY_REF(FLuaValue, outItems);

	for (int32 argIndex = 0; argIndex < numItems; ++argIndex)
	{
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.MostRecentProperty = nullptr;
		
		Stack.StepCompiledIn<FProperty>(nullptr);
		check(Stack.MostRecentProperty && Stack.MostRecentPropertyAddress);
		FProperty* p = CastField<FProperty>(Stack.MostRecentProperty);
		const void* propertyValuePtr = p->ContainerPtrToValuePtr<const void*>(Stack.MostRecentPropertyContainer);
		
		FLuaValue& val = outItems.Emplace_GetRef(p, propertyValuePtr);
		val.ConvertLuaObjectsToHandles();
		val.ClearIsScriptValue();
	}
	
	P_FINISH;
	
	*(bool*)RESULT_PARAM = true;
}


bool UUnrealLuaUtility::GetFromLuaValue(const int32 numItems)
{
	return false;
}

DEFINE_FUNCTION(UUnrealLuaUtility::execGetFromLuaValue)
{
	int32 numItems = 0;
	Stack.StepCompiledIn<FIntProperty>(&numItems);
	
	ESetLuaValueResult setLuaValueResult = ESetLuaValueResult::Success;
	for (int32 argIndex = 0; argIndex < numItems; ++argIndex)
	{
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.MostRecentProperty = nullptr;
		
		Stack.StepCompiledIn<FStructProperty>(nullptr);
		check(Stack.MostRecentProperty && Stack.MostRecentPropertyAddress);
		FStructProperty* luaValueProperty = CastFieldChecked<FStructProperty>(Stack.MostRecentProperty);
		FLuaValue* luaValueAddress = luaValueProperty->ContainerPtrToValuePtr<FLuaValue>(Stack.MostRecentPropertyContainer);
		
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.MostRecentProperty = nullptr;
		
		Stack.StepCompiledIn<FProperty>(nullptr);
		check(Stack.MostRecentProperty && Stack.MostRecentPropertyAddress);
		FProperty* outValueProperty = CastField<FProperty>(Stack.MostRecentProperty);
		void* outValueAddress = outValueProperty->ContainerPtrToValuePtr<void*>(Stack.MostRecentPropertyContainer);

		setLuaValueResult |= luaValueAddress->WriteValueToPropertyMemoryAddress_WithPropertyTypeCheck(outValueProperty, outValueAddress);
	}
	P_FINISH;
	*(bool*)RESULT_PARAM = !EnumHasAllFlags(setLuaValueResult, ESetLuaValueResult::Error);
}

void GetNumLuaValuesFromArray(int32 numToRead, UObject* Context, FFrame& Stack, RESULT_DECL)
{
	TArray<FLuaValue> inputArray{};
	Stack.StepCompiledIn<FArrayProperty>(&inputArray);
	UObject* obj = Stack.Object;
	
	for(auto& val : inputArray)
	{
		if(numToRead == 0)
		{
			break;
		}
		numToRead--;
		Stack.Step(Stack.Object, nullptr);
		FProperty* outputValueProp = Stack.MostRecentProperty; 
		void* outputValueContainer = Stack.MostRecentPropertyContainer;

		val.WriteValueToPropertyMemoryAddress_WithPropertyTypeCheck(outputValueProp, Stack.MostRecentPropertyAddress);
	}
	inputArray.Empty();
}

ELuaValueType UUnrealLuaUtility::SwitchOnLuaValueType(const FLuaValue& inLuaValue)
{
	return ELuaValueType::Nil;
}

DEFINE_FUNCTION(UUnrealLuaUtility::execSwitchOnLuaValueType)
{
	// Steps into the stack, walking to the next property in it
	Stack.Step(Stack.Object, NULL);

	// Grab the last property found when we walked the stack
	// This does not contains the property value, only its type information
	FProperty* luaValueProp = CastField<FProperty>(Stack.MostRecentProperty);
	
	// Grab the base address where the struct actually stores its data
	// This is where the property value is truly stored
	void* luaValueContainerPtr = Stack.MostRecentPropertyContainer;

	const FLuaValue* luaValuePtr = luaValueProp->ContainerPtrToValuePtr<FLuaValue>(luaValueContainerPtr);
	
	*(ELuaValueType*)RESULT_PARAM = luaValuePtr->GetType();

}

ELuaValueType UUnrealLuaUtility::SwitchOnLuaValueTypeWithValue(const FLuaValue& inLuaValue, int32& outValue)
{
	return ELuaValueType::Nil;
}

DEFINE_FUNCTION(UUnrealLuaUtility::execSwitchOnLuaValueTypeWithValue)
{
	// Steps into the stack, walking to the next property in it
	Stack.Step(Stack.Object, NULL);

	// Grab the last property found when we walked the stack
	// This does not contains the property value, only its type information
	FProperty* luaValueProp = CastField<FProperty>(Stack.MostRecentProperty);
	
	// Grab the base address where the struct actually stores its data
	// This is where the property value is truly stored
	void* luaValueContainerPtr = Stack.MostRecentPropertyContainer;

	FLuaValue* luaValuePtr = luaValueProp->ContainerPtrToValuePtr<FLuaValue>(luaValueContainerPtr);

	//go to next property, which is the outValue property
	Stack.Step(Stack.Object, NULL);

	FProperty* outProp = CastField<FProperty>(Stack.MostRecentProperty);
	void* outContainerPtr = Stack.MostRecentPropertyContainer;
	
	//set outValue property
	luaValuePtr->WriteValueToPropertyMemoryAddress_WithPropertyTypeCheck(outProp, Stack.MostRecentPropertyAddress);

	*(ELuaValueType*)RESULT_PARAM = luaValuePtr->GetType();
}