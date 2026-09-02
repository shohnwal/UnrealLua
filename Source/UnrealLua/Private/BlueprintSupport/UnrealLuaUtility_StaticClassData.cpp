#include <ranges>
#include "BlueprintSupport/UnrealLuaUtility.h"

namespace UnrealLua::StaticClassData
{
	void ProcessSetStaticClassDataValue(UClass* uclass, FFrame& Stack, RESULT_DECL)
	{
		UScriptStruct* staticDataStruct = uclass->GetSparseClassDataStruct();
		void* staticData = uclass->GetOrCreateSparseClassData();
	
		P_GET_PROPERTY(FNameProperty, propName)
	
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.MostRecentProperty = nullptr;
	
		Stack.StepCompiledIn<FProperty>(nullptr);
		check(Stack.MostRecentProperty && Stack.MostRecentPropertyAddress);
		FProperty* sourceValueProp = CastField<FProperty>(Stack.MostRecentProperty);
		const void* sourceValuePtr = sourceValueProp->ContainerPtrToValuePtr<const void>(Stack.MostRecentPropertyContainer);
	

		P_FINISH;
	
		P_NATIVE_BEGIN
	
		if (staticData && staticDataStruct)
		{
			FProperty* prop = staticDataStruct->FindPropertyByName(propName);
			if (prop)
			{
				if (prop->SameType(sourceValueProp))
				{
					prop->CopyCompleteValue(staticData, sourceValuePtr);
					*static_cast<bool*>(RESULT_PARAM) = true;
					return;
				}			
			}
		}
	
		*static_cast<bool*>(RESULT_PARAM) = false;
	
		P_NATIVE_END
	}
}

bool UUnrealLuaUtility::SetStaticClassDataValue(UObject* target, FName propertyName)
{
	return false;
}

DEFINE_FUNCTION(UUnrealLuaUtility::execSetStaticClassDataValue)
{
	P_GET_OBJECT(UObject, target)
	
	if (!target)
	{
		checkNoEntry();
		*static_cast<bool*>(RESULT_PARAM) = false;
		return;
	}
	UClass* uclass = target->GetClass();
	
	UnrealLua::StaticClassData::ProcessSetStaticClassDataValue(uclass, Stack, RESULT_PARAM);
}

bool UUnrealLuaUtility::SetStaticClassDataValueByClass(UClass* targetClass, FName propertyName, const int32& value)
{
	return false;
}

DEFINE_FUNCTION(UUnrealLuaUtility::execSetStaticClassDataValueByClass)
{
	P_GET_OBJECT(UClass, targetClass)
	
	if (!targetClass)
	{
		checkNoEntry();
		*static_cast<bool*>(RESULT_PARAM) = false;
		return;
	}

	UnrealLua::StaticClassData::ProcessSetStaticClassDataValue(targetClass, Stack, RESULT_PARAM);
}