
#include "BlueprintSupport/UnrealLuaUtility.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"


 bool UUnrealLuaUtility::MakeLuaCoroutineFromString(UObject* worldContext, const FString& functionString, FLuaCoroutineHandle& outCoroutine)
{
	outCoroutine = {};
	TScriptInterface<ILuaContext> ctx{};
	if(!GetLuaContextFromWorldContext(worldContext, ctx))
	{
		return false;
	}
	if(!ctx->GetScopedLuaContext().IsLuaLoaded())
	{
		return false;
	}
	FLuaCoroutineHandle newCoroutine = ctx->GetScopedLuaContext().CreateNewCoroutineFromString(functionString);
	if (!newCoroutine.IsValid())
	{
		return false;
	}
 	verify(newCoroutine.GetCoroutine().status() == sol::call_status::yielded);
	outCoroutine = newCoroutine;
	return outCoroutine.IsValid();	
}

 bool UUnrealLuaUtility::MakeLuaCoroutineFromLuaFunction(const FLuaFunctionHandle& functionValue, FLuaCoroutineHandle& outCoroutine)
 {
	outCoroutine = {};
	if (!functionValue.IsValid())
	{
		return false;
	}
	sol::function func = functionValue.GetFunction();
	verify(func.valid())
	
	FScopedLuaContext* ctx = FScopedLuaContext::GetLuaContextFromLuaState(func.lua_state());
	FLuaCoroutineHandle newCoroutine = ctx->CreateCoroutineHandleForLuaFunction(func);
	if (!newCoroutine.IsValid())
	{
		return false;
	}
 	verify(newCoroutine.GetCoroutine().status() == sol::call_status::yielded);
	outCoroutine = newCoroutine;
	return true;
 }

 ELuaCoroutineCallStatus UUnrealLuaUtility::CallLuaCoroutine(const FLuaCoroutineHandle& coroutine, const int32 numArguments, const int32 numResults)
 {
	return ELuaCoroutineCallStatus::Invalid;
 }


DEFINE_FUNCTION(UUnrealLuaUtility::execCallLuaCoroutine)
{
	FLuaCoroutineHandle coroutineHandle;
	Stack.StepCompiledIn<FStructProperty>(&coroutineHandle);
	
	int32 numArguments = 0;
	Stack.StepCompiledIn<FIntProperty>(&numArguments);
	
	int32 numResults = 0;
	Stack.StepCompiledIn<FIntProperty>(&numResults);
	
	TArray<FLuaValue> args;
	
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
	
	bool doCall = coroutineHandle.IsValid() && UUnrealLuaEngineSubsystem::IsGameSessionActive();
	bool success = false;
	FLuaCoroutineCallResult coroutineCallResult{};
	if (doCall)
	{
		sol::call_status costatus = coroutineHandle.GetCoroutine().status();
		LUA_LOG("Callingg coroutine with status %d", costatus)
		
		coroutineCallResult = coroutineHandle.CallCoroutine(args);
		success = coroutineCallResult.result.valid();
	}
	sol::protected_function_result& luaReturnValuss = coroutineCallResult.result;

 	if (!luaReturnValuss.valid())
 	{
 		sol::error err = luaReturnValuss;
 		std::string errMsg = err.what();
 		LUA_LOG_ERROR("Error during Lua Coroutine Call: %hs", errMsg.c_str());
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

		if (success && outIndex < luaReturnValuss.return_count())
		{
			sol::stack_object ret = luaReturnValuss[outIndex];
			TSetPropertyValueParams parms{p, Stack.MostRecentPropertyContainer, 0, ret};
			UnrealLua::PropertyHelper::SetPropertyValue_InContainer(parms);			
		}
		else
		{
			p->InitializeValue(propertyValuePtr);
		}
	}
	*static_cast<ELuaCoroutineCallStatus*>(RESULT_PARAM) = coroutineCallResult.CoroutineCallStatus;
	P_FINISH;
}