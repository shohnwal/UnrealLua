// Fill out your copyright notice in the Description page of Project Settings.


#include "UnrealOverrides/UnrealLuaCompiledUFunction.h"


extern "C" {
	#include "lstate.h"
#include "lobject.h"
}
#include "lua.hpp"
#include "UnrealLua.h"
#include "Blueprint/BlueprintSupport.h"
#include "Engine/World.h"
#include "LuaCallHelpers/BlueprintCallHelpers.h"
#include "LuaCallHelpers/LuaScriptOverrideCalls.h"
#include "LuaContext/GameLuaContext.h"
#include "LuaContext/ScopedLuaContext.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "SubSystem/EditorLuaContextWorldSubsystem.h"
#include "SubSystem/UnrealLuaGameWorldSubsystem.h"
#include "Utility/LuaUtility.h"

void UUnrealLuaCompiledUFunction::Bind()
{
	//Super::Bind(); Don't let it bind automatically
	if (this->GetOuterUClassUnchecked()->HasAllClassFlags(EClassFlags::CLASS_Interface))
	{
		this->SetNativeFunc(execLuaDummyInterfaceFunction);
	}
	else
	{
		this->SetNativeFunc(execLuaCompiledUFunction);
	}
}

void UUnrealLuaCompiledUFunction::Initialize()
{
	FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &UUnrealLuaCompiledUFunction::NotifyWorldTearDown);
}

DEFINE_FUNCTION(UUnrealLuaCompiledUFunction::execLuaCompiledUFunction)
{
	UUnrealLuaCompiledUFunction* ufunc = nullptr;
	UWorld* world = nullptr;
	bool staticCall = false;
	
	if (Stack.CurrentNativeFunction == nullptr)
	{
		//static function maybe, via UObject::execCallMathFunction
		//Because // TGuardValue<UFunction*> NativeFuncGuard(Stack.CurrentNativeFunction, Function); is commented out
		//we don't know what UFunction this is... Nice work, Epic!
		
		//Ugly hack to get UFunction by spooling back code
		uint8* code = Stack.Code;
		code -= sizeof(ScriptPointerType);
		ScriptPointerType Pointer = FPlatformMemory::ReadUnaligned<ScriptPointerType>(code);
		UObject* Result = (UObject*) Pointer;
		UUnrealLuaCompiledUFunction* funcPtr = nullptr;
#if UE_WITH_OBJECT_HANDLE_LATE_RESOLVE || UE_WITH_REMOTE_OBJECT_HANDLE
		TObjectPtr<UObject> ObjPtr(Result);
		funcPtr = (UUnrealLuaCompiledUFunction*)ObjPtr.Get();
#else
		funcPtr = (UUnrealLuaCompiledUFunction*)Result;
#endif
		verify(IsValid(funcPtr));
		verify(funcPtr->HasAllFunctionFlags(FUNC_Static|FUNC_Final));
		ufunc = funcPtr;
		
		FFrame* previousFrame = Stack.PreviousFrame;
		if (previousFrame && previousFrame->Object)
		{
			world = previousFrame->Object->GetWorld();
		}
		staticCall = true;
	}
	else
	{
		ufunc = CastChecked<UUnrealLuaCompiledUFunction>(Stack.CurrentNativeFunction);
		world = Context->GetWorld();
	}
	
	if (Stack.Node == Stack.CurrentNativeFunction)
	{
		//Memory is mine, params should all be set up
	
		if (ufunc->CompiledByteCode.empty() || world == nullptr)
		{
			return;
		}
		sol::function func = ufunc->GetFuncForWorld(world);
		verify(func.valid())
		
		FLuaOverrideCallParams params{ufunc, Stack, RESULT_PARAM, &func};
		params.CallingObjectReference = staticCall ? sol::nil : UnrealLua::LightUserdata::GetUObjectAsTaggedLightUserdata(Context, params.FuncMapping->lua_state());
		UnrealLua::LuaScriptCall::CallLuaImplementedUFunction(params);
	}
	else
	{
		//Stack is owned by another function		
		//Need to set up separate stack
	
		//ProcessScriptFunction pulls param values from outer stack code
		UnrealLua::ProcessScriptFunction(Context, ufunc, Stack, RESULT_PARAM, [ufunc, world, staticCall](UObject* Context, FFrame& NewStack, RESULT_DECL)
		{
			//Stack is now the new stack, set up for the originalFunc
			//params from the outer code should have been consumed by now and copied to the current stack
		
			if (world == nullptr || ufunc->CompiledByteCode.empty())
			{
				return;
			}
			sol::function func = ufunc->GetFuncForWorld(world);
			verify(func.valid())
		
			FLuaOverrideCallParams params{ufunc, NewStack, RESULT_PARAM, &func};
			params.CallingObjectReference =  staticCall ? sol::nil : UnrealLua::LightUserdata::GetUObjectAsTaggedLightUserdata(Context, params.FuncMapping->lua_state());
			UnrealLua::LuaScriptCall::CallLuaImplementedUFunction(params);
		});
	}	
}

DEFINE_FUNCTION(UUnrealLuaCompiledUFunction::execLuaDummyInterfaceFunction)
{
	UUnrealLuaCompiledUFunction* ufunc = CastChecked<UUnrealLuaCompiledUFunction>(Stack.CurrentNativeFunction);
	if(ufunc->CompiledByteCode.empty())
	{
		return;
	}
	UWorld* world = Context->GetWorld();
	if (!world)
	{
		return;
	}

	sol::function func = ufunc->GetFuncForWorld(world);
	verify(func.valid())
		
	if (Stack.Node == Stack.CurrentNativeFunction)
	{
		//Memory is mine, params should all be set up
	}
	else
	{
		//Stack is owned by another function		
		//Need to set up separate stack
		
		//ProcessScriptFunction pulls param values from outer stack code
		UnrealLua::ProcessScriptFunction(Context, ufunc, Stack, RESULT_PARAM, [ufunc, &func](UObject* Context, FFrame& NewStack, RESULT_DECL)
		{
			//Stack is now the new stack, set up for the originalFunc
			//params from the outer code should have been consumed by now and copied to the current stack
		});
	}
}

sol::variadic_results UUnrealLuaCompiledUFunction::PerformDirectLuaCall(sol::stack_object Self, const sol::variadic_args& Args)
{
	sol::function func = this->GetFuncForLuaState(Self.lua_state());
	verify(func.valid());
	return UnrealLua::LuaScriptCall::CallLuaFunctionSafe(func, Self, Args);
}

void UUnrealLuaCompiledUFunction::SetLuaBytecode(const sol::bytecode& byteCode)
{
	this->CompiledByteCode = byteCode;
}

void UUnrealLuaCompiledUFunction::RemoveLuaContext(const TScriptInterface<ILuaContext>& ictx)
{
	lua_State* L = ictx->GetScopedLuaContext().GetLuaState().lua_state();
	int32 index = this->LuaFunctionPairs.IndexOfByPredicate([L](const FLuaFunctionForWorld& item)
	{
		return L == item.Func.lua_state();
	});
	if (index != INDEX_NONE)
	{
		this->LuaFunctionPairs.RemoveAtSwap(index);
	}
}

void UUnrealLuaCompiledUFunction::NotifyWorldTearDown(UWorld* world)
{
	if(!world)
	{
		return;
	}
	int32 foundIndex = this->LuaFunctionPairs.IndexOfByPredicate([world](const FLuaFunctionForWorld& item)
	{
		return item.World == world;
	});
	if (foundIndex != INDEX_NONE)
	{
		this->LuaFunctionPairs.RemoveAtSwap(foundIndex);
	}	
}

void UUnrealLuaCompiledUFunction::SetSuperStruct(UStruct* NewSuperStruct)
{
	Super::SetSuperStruct(NewSuperStruct);
	if (NewSuperStruct->IsA(UFunction::StaticClass()))
	{
		this->SuperFunctionDescr = FFunctionDescr{CastChecked<UFunction>(NewSuperStruct)};
	}
	else
	{
		this->SuperFunctionDescr = {};
	}
}

const FFunctionDescr* UUnrealLuaCompiledUFunction::GetParentDescr() const
{
	return &this->SuperFunctionDescr;
}

sol::function UUnrealLuaCompiledUFunction::GetFuncForWorld(UWorld* world)
{
	if(!world)
	{
		return sol::nil;
	}
	
	//Check for a match for a world 
	FLuaFunctionForWorld* found = this->LuaFunctionPairs.FindByPredicate([world](const FLuaFunctionForWorld& item)
	{
		return item.World == world;
	});
	
	sol::function func{sol::nil};
	
	if (found)
	{
		func = found->Func;
	}
	else 
	{
		lua_State* L = nullptr;
		if(world->IsGameWorld())
		{
			UUnrealLuaGameWorldSubsystem* ss = world->GetSubsystem<UUnrealLuaGameWorldSubsystem>();
			if(ss && ss->bHasBegunPlay)
			{
				L = ss->LuaContext->GetLuaState().lua_state();
			}
		}
		else if (world->IsPreviewWorld())
		{
			UEditorLuaContextWorldSubsystem* ss = world->GetSubsystem<UEditorLuaContextWorldSubsystem>();
			if(ss && ss->bSubSystemHasBegunPlay)
			{
				L = ss->LuaContext->GetLuaState().lua_state();
			}
		}
		
		if (L)
		{
			FLuaFunctionForWorld& entry = this->GetFuncEntryForLuaState(L);
			verify(entry.World == nullptr);
			verify(entry.Func.valid());
			//correct the world entry
			entry.World = world;
			func = entry.Func;
		}
	}
	
	return func;	
}

sol::function UUnrealLuaCompiledUFunction::GetFuncForLuaState(lua_State* L)
{
	if(!L)
	{
		return sol::nil;
	}
	FLuaFunctionForWorld& entry = this->GetFuncEntryForLuaState(L);

	FString parentName;
	FAssetData data;
	data.GetTagValue(FBlueprintTags::NativeParentClassPath, parentName);
	
	return entry.Func;
}

FLuaFunctionForWorld& UUnrealLuaCompiledUFunction::GetFuncEntryForLuaState(lua_State* L)
{
	verify(L != nullptr)
    FLuaFunctionForWorld* found = this->LuaFunctionPairs.FindByPredicate([L](const FLuaFunctionForWorld& item)
    {
    	return item.Func.lua_state() == L;
    });
    
    if (!found)
    {
    	sol::state_view lua{L};	sol::load_result result = lua.load(this->CompiledByteCode.as_string_view());
    	verify(result.valid())
    	sol::function func = result.get<sol::function>();
    	
    	int32 top = lua_gettop(lua);
    	sol::stack::push(func.lua_state(), func);
    	sol::stack_object funcObj(L);
    	TValue* val = UnrealLua::Utility::index2value(lua, funcObj.stack_index());
    	verify(ttisfunction(val));
    	LClosure *f = clLvalue(val);
    	const LClosure* cl = static_cast<const LClosure*>(func.pointer());
    	verify(f == cl)
    	Proto* funcProto = f->p;
    	verify(funcProto->UnrealLuaCompiledFunction == nullptr);
    	funcProto->UnrealLuaCompiledFunction = this;
    	lua_settop(L, top);
    	
    	verify(func.valid())
    	found = &this->LuaFunctionPairs.Add_GetRef({nullptr, func });
    }
	return *found;
}
