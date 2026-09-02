
#include "BlueprintSupport/UnrealLuaUtility.h"
#include "UnrealLua.h"
#include "LuaValue/LuaScriptValue.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"

ELuaCallResult UUnrealLuaUtility::MakeLuaFunctionFromString(UObject* worldContext, const FString& functionString, FLuaFunctionHandle& outLuaFunction)
{
	outLuaFunction = {};
	TScriptInterface<ILuaContext> ctx{};
	if(!GetLuaContextFromWorldContext(worldContext, ctx))
	{
		return ELuaCallResult::Failure;
	}
	if(!ctx || !ctx->GetScopedLuaContext().IsLuaLoaded())
	{
		return ELuaCallResult::Failure;
	}
	FLuaFunctionHandle newFunction = ctx->GetScopedLuaContext().CreateNewFunctionFromString(functionString);
	if (!newFunction.IsValid())
	{
		return ELuaCallResult::Failure;
	}
	outLuaFunction = newFunction;
	
	return outLuaFunction.IsValid() ? ELuaCallResult::Success : ELuaCallResult::Failure;		
}

ELuaCallResult UUnrealLuaUtility::GetLuaFunctionFromObject(UObject* worldContext, const FString& functionString, FLuaFunctionHandle& outLuaFunction)
{
	FLuaUObjectItem* item = UnrealLua::UObjectRegistry::TryGetUObjectItem(worldContext);
	if (!item)
	{
		return ELuaCallResult::Failure;
	}
	FLuaScriptValue* val = item->GetLuaScriptValue(*functionString);
	if (!val)
	{
		return ELuaCallResult::Failure;
	}
	if (val->IsType<FLuaFunctionHandle>())
	{
		outLuaFunction = val->Get<FLuaFunctionHandle>();
		return ELuaCallResult::Success;
	}
	else if (val->IsType<sol::function>())
	{
		sol::function func = val->Get<sol::function>();
		outLuaFunction = FLuaFunctionHandle::MakeHandle(func);
		return outLuaFunction.IsValid() ? ELuaCallResult::Success : ELuaCallResult::Failure; 
	}
	return ELuaCallResult::Failure;
}

ELuaCallResult UUnrealLuaUtility::GetLuaFunctionFromTable(FLuaTableHandle handle, const FString& functionString, FLuaFunctionHandle& outLuaFunction)
{
	if (!handle.IsValid())
	{
		return ELuaCallResult::Failure;
	}
	sol::table table = handle.GetTable();
	
	sol::object maybeFunc = table[functionString];
	
	if (maybeFunc.get_type() != sol::type::function)
	{
		return ELuaCallResult::Failure;
	}
	sol::function func = maybeFunc.as<sol::function>();
	
	outLuaFunction = FLuaFunctionHandle::MakeHandle(func);
	
	return outLuaFunction.IsValid() ? ELuaCallResult::Success : ELuaCallResult::Failure;
}

ELuaCallResult UUnrealLuaUtility::GetGlobalLuaFunction(UObject* worldContext, const FString& funcName, FLuaFunctionHandle& outLuaFunction)
{
	if (!worldContext || funcName.IsEmpty() || UnrealLua::IsGameSessionActive())
	{
		return ELuaCallResult::Failure;
	}
	TScriptInterface<ILuaContext> ctx;
	if (GetLuaContextFromWorldContext(worldContext, ctx))
	{
		auto castedFuncName = StringCast<char>(*funcName);
		sol::table globals = ctx->GetScopedLuaContext().GetLuaState().globals();
		sol::optional<sol::function> maybeGlobalFunc = globals.raw_get<sol::optional<sol::function>>(castedFuncName.Get());
		if (maybeGlobalFunc.has_value())
		{
			outLuaFunction = FLuaFunctionHandle::MakeHandle(maybeGlobalFunc.value());
			return ELuaCallResult::Success;
		}
	}
	return ELuaCallResult::Failure;
}

ELuaCallResult UUnrealLuaUtility::CallLuaFunction(const FLuaFunctionHandle& luaFunction, const int32 numArguments, const int32 numResults)
 {
	return ELuaCallResult::Failure;
 }

 DEFINE_FUNCTION(UUnrealLuaUtility::execCallLuaFunction)
 {
	FLuaFunctionHandle functionHandle;
	Stack.StepCompiledIn<FStructProperty>(&functionHandle);
	
	int32 numArguments = 0;
	Stack.StepCompiledIn<FIntProperty>(&numArguments);
	
	int32 numResults = 0;
	Stack.StepCompiledIn<FIntProperty>(&numResults);
	
	TArray<FLuaValue> args;
	
	
	TArray<FLuaValue> result;
	for (int32 argIndex = 0; argIndex < numArguments; ++argIndex)
	{
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.MostRecentProperty = nullptr;
		
		Stack.StepCompiledIn<FProperty>(nullptr);
		check(Stack.MostRecentProperty && Stack.MostRecentPropertyAddress);

		FProperty* p = CastField<FProperty>(Stack.MostRecentProperty);

		const void* propertyValuePtr = p->ContainerPtrToValuePtr<const void*>(Stack.MostRecentPropertyContainer);
		
		args.Emplace(p, propertyValuePtr);
	}
	
	bool doCall = functionHandle.IsValid() && UUnrealLuaEngineSubsystem::IsGameSessionActive();
	ELuaCallResult success = ELuaCallResult::Failure;
	sol::protected_function_result funcresult{};
	if (doCall)
	{
		funcresult = UnrealLua::LuaScriptCall::CallLuaFunctionSafe(functionHandle.GetFunction(), sol::as_args(args));
		if (funcresult.valid())
		{
			success = ELuaCallResult::Success;
		}
	}

	for (int32 outIndex = 0; outIndex < numResults; ++outIndex)
	{
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.MostRecentProperty = nullptr;
		Stack.StepCompiledIn<FProperty>(nullptr);
		check(Stack.MostRecentProperty && Stack.MostRecentPropertyAddress);

		FProperty* p = CastField<FProperty>(Stack.MostRecentProperty);

		//FString propertyValueString;
		void* propertyValuePtr = p->ContainerPtrToValuePtr<void*>(Stack.MostRecentPropertyContainer);

		if (success == ELuaCallResult::Success && outIndex < funcresult.return_count())
		{
			sol::stack_object ret = funcresult[outIndex];
			TSetPropertyValueParams parms{p, Stack.MostRecentPropertyContainer, 0, ret};
			UnrealLua::PropertyHelper::SetPropertyValue_InContainer(parms);			
		}
		else
		{
			p->InitializeValue(propertyValuePtr);
		}
	}
	*static_cast<ELuaCallResult*>(RESULT_PARAM) = success;
	P_FINISH;
}
 
 
ELuaCallResult UUnrealLuaUtility::CallLuaFunctionOnObject(UObject* target, const FString& funcName, const bool selfCall, const int32 arguments, const int32 results)
{
	return ELuaCallResult::Failure;
}

DEFINE_FUNCTION(UUnrealLuaUtility::execCallLuaFunctionOnObject)
{
	UObject* target;
	Stack.StepCompiledIn<FObjectProperty>(&target);
	
	//get func name
	FString funcName;
	Stack.StepCompiledIn<FStrProperty>(&funcName);
	
	bool useTargetAsSelf;
	Stack.StepCompiledIn<FBoolProperty>(&useTargetAsSelf);
	
	int32 numArguments = 0;
	Stack.StepCompiledIn<FIntProperty>(&numArguments);
	
	int32 numResults = 0;
	Stack.StepCompiledIn<FIntProperty>(&numResults);
	
	TArray<FLuaValue> args;
	
	//self arg
	if(useTargetAsSelf)
	{
		args.Emplace(target);
	}
	
	TArray<FLuaValue> result;
	for (int32 argIndex = 0; argIndex < numArguments; ++argIndex)
	{
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.MostRecentProperty = nullptr;
		Stack.StepCompiledIn<FProperty>(nullptr);
		check(Stack.MostRecentProperty && Stack.MostRecentPropertyAddress);

		FProperty* p = CastField<FProperty>(Stack.MostRecentProperty);

		const void* propertyValuePtr = p->ContainerPtrToValuePtr<const void*>(Stack.MostRecentPropertyContainer);
		
		args.Emplace(p, propertyValuePtr);
	}
	
	bool doCall = IsValid(target) && !funcName.IsEmpty() && UUnrealLuaEngineSubsystem::IsGameSessionActive();
	ELuaCallResult success = ELuaCallResult::Failure;
	sol::protected_function_result funcresult{};
	if (doCall)
	{
		funcresult = UnrealLua::LuaScriptCall::CallLuaFunctionSafeByName(target, funcName, sol::as_args(args));
		if (funcresult.valid())
		{
			success = ELuaCallResult::Success;
		}
	}

	for (int32 outIndex = 0; outIndex < numResults; ++outIndex)
	{
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.MostRecentProperty = nullptr;
		Stack.StepCompiledIn<FProperty>(nullptr);
		check(Stack.MostRecentProperty && Stack.MostRecentPropertyAddress);

		FProperty* p = CastField<FProperty>(Stack.MostRecentProperty);

		//FString propertyValueString;
		void* propertyValuePtr = p->ContainerPtrToValuePtr<void*>(Stack.MostRecentPropertyContainer);

		if (success == ELuaCallResult::Success && outIndex < funcresult.return_count())
		{
			sol::stack_object ret = funcresult[outIndex];
			TSetPropertyValueParams parms{p, Stack.MostRecentPropertyContainer, 0, ret};
			UnrealLua::PropertyHelper::SetPropertyValue_InContainer(parms);			
		}
		else
		{
			p->InitializeValue(propertyValuePtr);
		}
	}
	*static_cast<ELuaCallResult*>(RESULT_PARAM) = success;
	P_FINISH;
}

 ELuaCallResult UUnrealLuaUtility::CallLuaFunctionOnTable(const FLuaTableHandle& table, const FString& funcName, const bool selfCall, const int32 numArguments, const int32 numResults)
 {
	return ELuaCallResult::Failure;
 }

DEFINE_FUNCTION(UUnrealLuaUtility::execCallLuaFunctionOnTable)
{
	P_GET_STRUCT_REF(FLuaTableHandle, table);
	P_GET_PROPERTY_REF(FStrProperty, funcName);
	P_GET_UBOOL(selfCall);
	P_GET_PROPERTY(FIntProperty, numArgs);
	P_GET_PROPERTY(FIntProperty, numResults);
	
	sol::table tbl = table.GetTable();
	
	TArray<FLuaValue> args;
	bool doCall = tbl.valid() && !funcName.IsEmpty() && UUnrealLuaEngineSubsystem::IsGameSessionActive();
	
	auto castedFuncName = StringCast<char>(*funcName);
	
	if (doCall && selfCall)
	{
		args.Emplace(FLuaValue{tbl});
	}
	for (int32 argIndex = 0; argIndex < numArgs; ++argIndex)
	{
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.MostRecentProperty = nullptr;
		Stack.StepCompiledIn<FProperty>(nullptr);
		check(Stack.MostRecentProperty && Stack.MostRecentPropertyAddress);

		FProperty* p = CastField<FProperty>(Stack.MostRecentProperty);

		const void* propertyValuePtr = p->ContainerPtrToValuePtr<const void*>(Stack.MostRecentPropertyContainer);
		if (doCall)
		{
			args.Emplace(p, propertyValuePtr);
		}
	}		
	
	ELuaCallResult success = ELuaCallResult::Failure;
	sol::protected_function_result funcresult{};
	if (doCall)
	{
		funcresult = UnrealLua::LuaScriptCall::CallLuaFunctionSafeByName(tbl, castedFuncName.Get(), sol::as_args(args));
		if (funcresult.valid())
		{
			success = ELuaCallResult::Success;
		}
	}
	
	TArray<FLuaValue> result;

	for (int32 outIndex = 0; outIndex < numResults; ++outIndex)
	{
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.MostRecentProperty = nullptr;
		Stack.StepCompiledIn<FProperty>(nullptr);
		check(Stack.MostRecentProperty && Stack.MostRecentPropertyAddress);

		FProperty* p = CastField<FProperty>(Stack.MostRecentProperty);

		//FString propertyValueString;
		void* propertyValuePtr = p->ContainerPtrToValuePtr<void*>(Stack.MostRecentPropertyContainer);

		if (success == ELuaCallResult::Success && outIndex < funcresult.return_count())
		{
			sol::stack_object ret = funcresult[outIndex];
			TSetPropertyValueParams parms{p, Stack.MostRecentPropertyContainer, 0, ret};
			UnrealLua::PropertyHelper::SetPropertyValue_InContainer(parms);			
		}
		else
		{
			p->InitializeValue(propertyValuePtr);
		}
	}
	*static_cast<ELuaCallResult*>(RESULT_PARAM) = success;
	P_FINISH;
}

 ELuaCallResult UUnrealLuaUtility::CallGlobalLuaFunction(UObject* worldContext, const FString& funcName, const int32 numArguments, const int32 numResults)
 {
	return ELuaCallResult::Failure;
 }

DEFINE_FUNCTION(UUnrealLuaUtility::execCallGlobalLuaFunction)
{
	P_GET_OBJECT(UObject, worldContext);
	P_GET_PROPERTY_REF(FStrProperty, funcName);
	P_GET_PROPERTY(FIntProperty, numArgs);
	P_GET_PROPERTY(FIntProperty, numResults);
	
	
	TArray<FLuaValue> args;
	bool doCall = !funcName.IsEmpty() && UUnrealLuaEngineSubsystem::IsGameSessionActive();
	
	sol::function globalFunc{};
	if (doCall)
	{
		TScriptInterface<ILuaContext> ctx;
		if (GetLuaContextFromWorldContext(worldContext, ctx))
		{
			auto castedFuncName = StringCast<char>(*funcName);
			sol::table globals = ctx->GetScopedLuaContext().GetLuaState().globals();
			sol::optional<sol::function> maybeGlobalFunc = globals.raw_get<sol::optional<sol::function>>(castedFuncName.Get());
			if (maybeGlobalFunc.has_value())
			{
				globalFunc = maybeGlobalFunc.value();
			}
		}
	}
	doCall = globalFunc.valid();
	
	for (int32 argIndex = 0; argIndex < numArgs; ++argIndex)
	{
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.MostRecentProperty = nullptr;
		Stack.StepCompiledIn<FProperty>(nullptr);
		check(Stack.MostRecentProperty && Stack.MostRecentPropertyAddress);

		FProperty* p = CastField<FProperty>(Stack.MostRecentProperty);

		const void* propertyValuePtr = p->ContainerPtrToValuePtr<const void*>(Stack.MostRecentPropertyContainer);
		if (doCall)
		{
			args.Emplace(p, propertyValuePtr);
		}
	}		
	
	ELuaCallResult success = ELuaCallResult::Failure;
	sol::protected_function_result funcresult{};
	if (doCall)
	{
		funcresult = UnrealLua::LuaScriptCall::CallLuaFunctionSafe(globalFunc, sol::as_args(args));
		if (funcresult.valid())
		{
			success = ELuaCallResult::Success;
		}
	}
	
	TArray<FLuaValue> result;

	for (int32 outIndex = 0; outIndex < numResults; ++outIndex)
	{
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.MostRecentProperty = nullptr;
		Stack.StepCompiledIn<FProperty>(nullptr);
		check(Stack.MostRecentProperty && Stack.MostRecentPropertyAddress);

		FProperty* p = CastField<FProperty>(Stack.MostRecentProperty);

		//FString propertyValueString;
		void* propertyValuePtr = p->ContainerPtrToValuePtr<void*>(Stack.MostRecentPropertyContainer);

		if (success == ELuaCallResult::Success && outIndex < funcresult.return_count())
		{
			sol::stack_object ret = funcresult[outIndex];
			TSetPropertyValueParams parms{p, Stack.MostRecentPropertyContainer, 0, ret};
			UnrealLua::PropertyHelper::SetPropertyValue_InContainer(parms);			
		}
		else
		{
			p->InitializeValue(propertyValuePtr);
		}
	}
	*static_cast<ELuaCallResult*>(RESULT_PARAM) = success;
	P_FINISH;
}