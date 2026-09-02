#include "UnrealOverrides/UnrealLuaOverrideUFunction.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "LuaCallHelpers/BlueprintCallHelpers.h"
#include "LuaCallHelpers/LuaScriptOverrideCalls.h"
#include "ScriptableUObject/LuaUserWidget.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"
#include "SubSystem/UnrealLuaGameWorldSubsystem.h"
//#include "Utility/BlueprintScriptDecompiler.h"
#include "UObject/Class.h"
#include "UObject/Object.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"

#define PRINT_BLUEPRINT_SCRIPT 1
#if PRINT_BLUEPRINT_SCRIPT && 0
	FBlueprintScriptDecompiler Disasm(*GLog);
	
UE_LOG(LogTemp, Log, TEXT("\n\n[function %s]:\n"), *(originalFunc->GetName()));
Disasm.DisassembleStructure(originalFunc);
UE_LOG(LogTemp, Log, TEXT("\n\n[function %s]:\n"), *(luaFunc->GetName()));
Disasm.DisassembleStructure(luaFunc);

Disasm.DisassembleCode(Stack.Code);
#endif

/*
ULuaFunction::ULuaFunction()
	: UFunction()
{
	if(this->HasAllFlags(RF_ClassDefaultObject))
	{
		return;
	}
	verify(UUnrealLuaEngineSubsystem::Get()->IsGameSessionActive());
}

ULuaFunction::ULuaFunction(const FObjectInitializer& ObjectInitializer)
	: UFunction(ObjectInitializer)
{
	if(this->HasAllFlags(RF_ClassDefaultObject))
	{
		return;
	}
	verify(UUnrealLuaEngineSubsystem::Get()->IsGameSessionActive());
}*/

UUnrealLuaOverrideUFunction::~UUnrealLuaOverrideUFunction()
{
	this->SetPropertiesSize(0);
	this->NumParms = 0;
	this->ChildProperties = nullptr;
	this->Children = nullptr;
	this->ParmsSize = 0;
	this->PropertiesSize = 0;
	this->FirstPropertyToInit = nullptr;
	this->DestructorLink = nullptr;
	this->PropertyLink = nullptr;
#if WITH_EDITOR
	this->PropertyWrappers = {};
#endif
	this->ReturnValueOffset = 0;
	this->EventGraphCallOffset = 0;
	this->EventGraphFunction = 0;
}

//Actors
DEFINE_FUNCTION(UUnrealLuaOverrideUFunction::execActorUserConstructionScriptLuaCall)
{
	//LUA_LOG("ULuaFunction::execActorUserConstructionScriptLuaCall: Main call for Actor %s", *Context->GetFullName())
	UUnrealLuaOverrideUFunction* luaFunc = CastChecked<UUnrealLuaOverrideUFunction>(Stack.CurrentNativeFunction);

	UWorld* world = Context->GetWorld();

	if(world)
	{
		if(world->IsGameWorld())
		{
			//LUA_LOG("ULuaFunction::execActorUserConstructionScriptLuaCall: Is Game World for Actor %s", *Context->GetFullName())

			if (APawn* pawn = Cast<APawn>(Context))
			{
				pawn->ReceiveRestartedDelegate.AddUniqueDynamic(UUnrealLuaUObjectRegistry::Get(), &UUnrealLuaUObjectRegistry::NotifyPawnRestart);
			}
			else if (APlayerController* pc = Cast<APlayerController>(Context))
			{
				if (pc->IsLocalPlayerController())
				{
					pc->OnPossessedPawnChanged.AddUniqueDynamic(UUnrealLuaUObjectRegistry::Get(), &UUnrealLuaUObjectRegistry::NotifyPlayerControllerPossessedPawn);
				}
			}
			
			if(world->HasBegunPlay())
			{
				//LUA_LOG("ULuaFunction::execActorUserConstructionScriptLuaCall: Game World already has begun play, attempt to load Lua for Actor %s", *Context->GetFullName())
				UnrealLua::UObjectRegistry::LoadLuaScript(Context, false);
			}
			else
			{
				
				UUnrealLuaGameWorldSubsystem* ss = world->GetSubsystem<UUnrealLuaGameWorldSubsystem>();
				if(ss->bHasBegunPlay)
				{
					//LUA_LOG("ULuaFunction::execActorUserConstructionScriptLuaCall: Worldsubsystem has begun play, loading lua script for Actor %s", *Context->GetFullName())
					UnrealLua::UObjectRegistry::LoadLuaScript(Context, false);
				}
				else
				{
					//LUA_LOG("ULuaFunction::execActorUserConstructionScriptLuaCall: World has not begun play, enqueueing callback for %s", *Context->GetFullName())
					ss->OnLuaReady.AddWeakLambda(Context, [Context]()
					{
						UnrealLua::UObjectRegistry::LoadLuaScript(Context, false);
					});	
				}
			}
		}
		else if(world->IsPreviewWorld())
		{
			//by this point, the world should have begun play, if simulating
			if(world->HasBegunPlay())
			{
				//LUA_LOG("ULuaFunction::execActorUserConstructionScriptLuaCall: Preview world has already begun play, loading Lua script for actor %s", *Context->GetFullName())
				UnrealLua::UObjectRegistry::LoadLuaScript(Context, false);
			}
		}
	}

	
	if(Stack.Code)
	{
		UFunction* originalFunc = luaFunc->Overridden;
		TGuardValue<UFunction*> NativeFuncGuard(Stack.CurrentNativeFunction, originalFunc);
		TGuardValue<UFunction*> NativeFuncGuardNode(Stack.Node, originalFunc);
		TGuardValue<FField*> NativeFuncGuardProps(Stack.PropertyChainForCompiledIn, Stack.PropertyChainForCompiledIn != nullptr ? originalFunc->ChildProperties : nullptr);

		originalFunc->Invoke(Context, Stack, RESULT_PARAM);
	}
	
	verify(!Stack.Code || *Stack.Code == EX_EndOfScript)
}

//Generic calls
DEFINE_FUNCTION(UUnrealLuaOverrideUFunction::execNativeLuaCall)
{
	UUnrealLuaOverrideUFunction* luaFunc = CastChecked<UUnrealLuaOverrideUFunction>(Stack.CurrentNativeFunction);
	UFunction* originalFunc = luaFunc->Overridden;
	
	if (Stack.Node == Stack.CurrentNativeFunction)
	{
		//Memory is mine, params should all be set up
		
		if(UnrealLua::LuaScriptCall::GetSuperCall() == false)
		{
			FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(Context);

			FLuaOverrideCallParams params{originalFunc, Stack, RESULT_PARAM, nullptr, sol::nil};
			if(item.ProcessEvent(params))
			{
				//override call handled
				//Skip Blueprint code that is still in the pipeline
				//UnrealLua::SkipFunction(Stack, RESULT_PARAM, originalFunc);
				//override call handled
				return;
			}
		}
		UnrealLua::LuaScriptCall::SetSuperCall(false);
			
		if (Stack.Code)
		{
			FFrame newStack{Context, originalFunc, Stack.Locals, nullptr, originalFunc->ChildProperties};
			newStack.MostRecentProperty = Stack.MostRecentProperty;
			newStack.MostRecentPropertyAddress = Stack.MostRecentPropertyAddress;
			newStack.MostRecentPropertyContainer = Stack.MostRecentPropertyContainer;
			newStack.Code = Stack.Code;
			newStack.OutParms = Stack.OutParms;
			originalFunc->Invoke(Context, newStack, RESULT_PARAM);
			Stack.Code = newStack.Code;	
		}
		else
		{
			if (luaFunc->bCallOriginalNativeImplementationFunction)
			{
				FFrame newStack{Context, originalFunc, Stack.Locals, nullptr, originalFunc->ChildProperties};
				newStack.MostRecentProperty = Stack.MostRecentProperty;
				newStack.MostRecentPropertyAddress = Stack.MostRecentPropertyAddress;
				newStack.MostRecentPropertyContainer = Stack.MostRecentPropertyContainer;
				newStack.OutParms = Stack.OutParms;
				originalFunc->Invoke(Context, newStack, RESULT_PARAM);
			}
		}
	}
	else
	{
		//Stack is owned by another function		
		//Need to set up separate stack
		
		//ProcessScriptFunction pulls param values from outer stack code
		UnrealLua::ProcessScriptFunction(Context, originalFunc, Stack, RESULT_PARAM, [](UObject* Context, FFrame& NewStack, RESULT_DECL)
		{
			//Stack is now the new stack, set up for the originalFunc
			//params from the outer code should have been consumed by now and copied to the current stack
			
			UFunction* originalFunc = NewStack.Node;
				
			if(UnrealLua::LuaScriptCall::GetSuperCall() == false)
			{
				FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(Context);

				FLuaOverrideCallParams params{originalFunc, NewStack, RESULT_PARAM, nullptr, sol::nil};
				if(item.ProcessEvent(params))
				{
					return;
				}
			}
			UnrealLua::LuaScriptCall::SetSuperCall(false);
				
			if (originalFunc->Script.Num() > 0)
			{
				UnrealLua::ProcessLocalScriptFunction(Context, NewStack, RESULT_PARAM);
			}
		});
	}
	
	/*
	
	UUnrealLuaOverrideUFunction* luaFunc = CastChecked<UUnrealLuaOverrideUFunction>(Stack.CurrentNativeFunction);
	UFunction* originalFunc = luaFunc->Overridden;
	
	verify(!Stack.Code);
	if(UnrealLua::LuaScriptCall::GetSuperCall() == false)
	{
		FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(Context);
		FLuaOverrideCallParams params{originalFunc, Stack, RESULT_PARAM};
		if(item.ProcessEvent(params))
		{
			//override call handled
			return;
		}
	}
	UnrealLua::LuaScriptCall::SetSuperCall(false);
	
	TGuardValue<UFunction*> NativeFuncGuard(Stack.CurrentNativeFunction, originalFunc);
	TGuardValue<UFunction*> NativeFuncGuardNode(Stack.Node, originalFunc);
	TGuardValue<FField*> NativeFuncGuardProps(Stack.PropertyChainForCompiledIn, Stack.PropertyChainForCompiledIn != nullptr ? originalFunc->ChildProperties : nullptr);

	originalFunc->Invoke(Context, Stack, RESULT_PARAM);
	*/
}
//Tick calls

DEFINE_FUNCTION(UUnrealLuaOverrideUFunction::execNativeTickLuaCall)
{
	verify(!Stack.Code);
	//lua super calls to here should never arrive
	verify(!UnrealLua::LuaScriptCall::GetSuperCall())
	
	UUnrealLuaOverrideUFunction* luaFunc = static_cast<UUnrealLuaOverrideUFunction*>(Stack.CurrentNativeFunction);
	UFunction* originalFunc = luaFunc->Overridden;
	FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(Context);

	//@TODO : Let user decide to tick before and/or after UE Tick ("PreTick"/"PostTick"? SetPreTickEnabled/SetPostTickEnabled?)
	if(item.bLuaTickEnabled)
	{
		FLuaOverrideCallParams params{originalFunc, Stack, RESULT_PARAM};
		item.ProcessTickEvent(params);
	}
	if(!item.bBlueprintTickEnabled)
	{
		return;
	}
	TGuardValue<UFunction*> NativeFuncGuard(Stack.CurrentNativeFunction, originalFunc);
	TGuardValue<UFunction*> NativeFuncGuardNode(Stack.Node, originalFunc);
	TGuardValue<FField*> NativeFuncGuardProps(Stack.PropertyChainForCompiledIn, Stack.PropertyChainForCompiledIn != nullptr ? originalFunc->ChildProperties : nullptr);

	originalFunc->Invoke(Context, Stack, RESULT_PARAM);
}

//Components
//Native

DEFINE_FUNCTION(UUnrealLuaOverrideUFunction::execNativeLuaScriptableComponentBeginPlayLuaCall)
{
	verify(!Stack.Code);
	UUnrealLuaOverrideUFunction* luaFunc = CastChecked<UUnrealLuaOverrideUFunction>(Stack.CurrentNativeFunction);
	UFunction* originalFunc = luaFunc->Overridden;

	UnrealLua::UObjectRegistry::LoadLuaScript(Context, false);
	
	if(UnrealLua::LuaScriptCall::GetSuperCall() == false)
	{
		FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(Context);
		FLuaOverrideCallParams params{originalFunc, Stack, RESULT_PARAM};
		if(item.ProcessEvent(params))
		{
			//override call handled
			return;
		}
	}
	UnrealLua::LuaScriptCall::SetSuperCall(false);

	TGuardValue<UFunction*> NativeFuncGuard(Stack.CurrentNativeFunction, originalFunc);
	TGuardValue<UFunction*> NativeFuncGuardNode(Stack.Node, originalFunc);
	TGuardValue<FField*> NativeFuncGuardProps(Stack.PropertyChainForCompiledIn, Stack.PropertyChainForCompiledIn != nullptr ? originalFunc->ChildProperties : nullptr);

	originalFunc->Invoke(Context, Stack, RESULT_PARAM);
}

DEFINE_FUNCTION(UUnrealLuaOverrideUFunction::execNativeLuaScriptableComponentEndPlayLuaCall)
{
	verify(!Stack.Code);
	UUnrealLuaOverrideUFunction* luaFunc = CastChecked<UUnrealLuaOverrideUFunction>(Stack.CurrentNativeFunction);
	UFunction* originalFunc = luaFunc->Overridden;
	FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(Context);
	if(UnrealLua::LuaScriptCall::GetSuperCall() == false)
	{
		FLuaOverrideCallParams params{originalFunc, Stack, RESULT_PARAM};
		if(item.ProcessEvent(params))
		{
			//override call handled
			return;
		}
	}
	UnrealLua::LuaScriptCall::SetSuperCall(false);

	TGuardValue<UFunction*> NativeFuncGuard(Stack.CurrentNativeFunction, originalFunc);
	TGuardValue<UFunction*> NativeFuncGuardNode(Stack.Node, originalFunc);
	TGuardValue<FField*> NativeFuncGuardProps(Stack.PropertyChainForCompiledIn, Stack.PropertyChainForCompiledIn != nullptr ? originalFunc->ChildProperties : nullptr);

	originalFunc->Invoke(Context, Stack, RESULT_PARAM);

	item.RemoveLuaScript();
}

//Blueprint

DEFINE_FUNCTION(UUnrealLuaOverrideUFunction::execBlueprintLuaScriptableComponentBeginPlayLuaCall)
{
	UUnrealLuaOverrideUFunction* luaFunc = CastChecked<UUnrealLuaOverrideUFunction>(Stack.CurrentNativeFunction);
	UFunction* originalFunc = luaFunc->Overridden;
	
	UnrealLua::UObjectRegistry::LoadLuaScript(Context, false);

	if(UnrealLua::LuaScriptCall::GetSuperCall() == false)
	{
		FLuaUObjectItem* item = UnrealLua::UObjectRegistry::TryGetUObjectItem(Context);
		if (item)
		{
			FLuaOverrideCallParams params{originalFunc, Stack, RESULT_PARAM};
			if(item->ProcessEvent(params))
			{
				//override call handled
				//Skip Blueprint code that is still in the pipeline
				UnrealLua::SkipFunction(Stack, RESULT_PARAM, originalFunc);
				//verify(!Stack.Code || *Stack.Code == EX_EndOfScript)
				return;
			}
		}
	}
	UnrealLua::LuaScriptCall::SetSuperCall(false);
	
	if(Stack.Code)
	{
		TGuardValue<UFunction*> NativeFuncGuard(Stack.CurrentNativeFunction, originalFunc);
		TGuardValue<UFunction*> NativeFuncGuardNode(Stack.Node, originalFunc);
		TGuardValue<FField*> NativeFuncGuardProps(Stack.PropertyChainForCompiledIn, Stack.PropertyChainForCompiledIn != nullptr ? originalFunc->ChildProperties : nullptr);

		originalFunc->Invoke(Context, Stack, RESULT_PARAM);
	}
}

DEFINE_FUNCTION(UUnrealLuaOverrideUFunction::execBlueprintLuaScriptableComponentEndPlayLuaCall)
{
	UUnrealLuaOverrideUFunction* luaFunc = CastChecked<UUnrealLuaOverrideUFunction>(Stack.CurrentNativeFunction);
	UFunction* originalFunc = luaFunc->Overridden;
	
	FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(Context);
	
	if(UnrealLua::LuaScriptCall::GetSuperCall() == false)
	{
		FLuaOverrideCallParams params{originalFunc, Stack, RESULT_PARAM};
		if(item.ProcessEvent(params))
		{
			//override call handled
			//Skip Blueprint code that is still in the pipeline
			UnrealLua::SkipFunction(Stack, RESULT_PARAM, originalFunc);

			verify(!Stack.Code || *Stack.Code == EX_EndOfScript)
			return;
		}
	}
	UnrealLua::LuaScriptCall::SetSuperCall(false);

	if(Stack.Code)
	{
		TGuardValue<UFunction*> NativeFuncGuard(Stack.CurrentNativeFunction, originalFunc);
		TGuardValue<UFunction*> NativeFuncGuardNode(Stack.Node, originalFunc);
		TGuardValue<FField*> NativeFuncGuardProps(Stack.PropertyChainForCompiledIn, Stack.PropertyChainForCompiledIn != nullptr ? originalFunc->ChildProperties : nullptr);

		originalFunc->Invoke(Context, Stack, RESULT_PARAM);
	}
	//verify(!Stack.Code || *Stack.Code == EX_EndOfScript)

	item.RemoveLuaScript();
}

//Widgets
//Native

DEFINE_FUNCTION(UUnrealLuaOverrideUFunction::execNativeLuaScriptableUserWidgetConstructLuaCall)
{
	verify(!Stack.Code);
	UUnrealLuaOverrideUFunction* luaFunc = CastChecked<UUnrealLuaOverrideUFunction>(Stack.CurrentNativeFunction);
	UFunction* originalFunc = luaFunc->Overridden;

	if(UnrealLua::LuaScriptCall::GetSuperCall() == false)
	{
		FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(Context);
		FLuaOverrideCallParams params{originalFunc, Stack, RESULT_PARAM};
		if(item.ProcessEvent(params))
		{
			//override call handled
			return;
		}
	}
	UnrealLua::LuaScriptCall::SetSuperCall(false);

	TGuardValue<UFunction*> NativeFuncGuard(Stack.CurrentNativeFunction, originalFunc);
	TGuardValue<UFunction*> NativeFuncGuardNode(Stack.Node, originalFunc);
	TGuardValue<FField*> NativeFuncGuardProps(Stack.PropertyChainForCompiledIn, Stack.PropertyChainForCompiledIn != nullptr ? originalFunc->ChildProperties : nullptr);

	originalFunc->Invoke(Context, Stack, RESULT_PARAM);
}

DEFINE_FUNCTION(UUnrealLuaOverrideUFunction::execNativeLuaScriptableUserWidgetDestructLuaCall)
{
	verify(!Stack.Code);
	UUnrealLuaOverrideUFunction* luaFunc = CastChecked<UUnrealLuaOverrideUFunction>(Stack.CurrentNativeFunction);
	UFunction* originalFunc = luaFunc->Overridden;
	FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(Context);
	if(UnrealLua::LuaScriptCall::GetSuperCall() == false)
	{
		FLuaOverrideCallParams params{originalFunc, Stack, RESULT_PARAM};
		if(item.ProcessEvent(params))
		{
			//override call handled
			return;
		}
	}
	UnrealLua::LuaScriptCall::SetSuperCall(false);

	TGuardValue<UFunction*> NativeFuncGuard(Stack.CurrentNativeFunction, originalFunc);
	TGuardValue<UFunction*> NativeFuncGuardNode(Stack.Node, originalFunc);
	TGuardValue<FField*> NativeFuncGuardProps(Stack.PropertyChainForCompiledIn, Stack.PropertyChainForCompiledIn != nullptr ? originalFunc->ChildProperties : nullptr);

	originalFunc->Invoke(Context, Stack, RESULT_PARAM);

	item.RemoveLuaScript();
}

//BLueprint

DEFINE_FUNCTION(UUnrealLuaOverrideUFunction::execBlueprintLuaScriptableUserWidgetConstructLuaCall)
{
	UUnrealLuaOverrideUFunction* luaFunc = CastChecked<UUnrealLuaOverrideUFunction>(Stack.CurrentNativeFunction);
	UFunction* originalFunc = luaFunc->Overridden;

	if(UnrealLua::LuaScriptCall::GetSuperCall() == false)
	{
		FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(Context);
		FLuaOverrideCallParams params{originalFunc, Stack, RESULT_PARAM};
		if(item.ProcessEvent(params))
		{
			//override call handled
			//Skip Blueprint code that is still in the pipeline
			UnrealLua::SkipFunction(Stack, RESULT_PARAM, originalFunc);
			//verify(!Stack.Code || *Stack.Code == EX_EndOfScript)
			return;
		}
	}
	UnrealLua::LuaScriptCall::SetSuperCall(false);
	
	if(Stack.Code)
	{
		TGuardValue<UFunction*> NativeFuncGuard(Stack.CurrentNativeFunction, originalFunc);
		TGuardValue<UFunction*> NativeFuncGuardNode(Stack.Node, originalFunc);
		TGuardValue<FField*> NativeFuncGuardProps(Stack.PropertyChainForCompiledIn, Stack.PropertyChainForCompiledIn != nullptr ? originalFunc->ChildProperties : nullptr);

		originalFunc->Invoke(Context, Stack, RESULT_PARAM);
	}
	//verify(!Stack.Code || *Stack.Code == EX_EndOfScript)
}

DEFINE_FUNCTION(UUnrealLuaOverrideUFunction::execBlueprintLuaScriptableUserWidgetDestructLuaCall)
{
	UUnrealLuaOverrideUFunction* luaFunc = CastChecked<UUnrealLuaOverrideUFunction>(Stack.CurrentNativeFunction);
	UFunction* originalFunc = luaFunc->Overridden;
	
	FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(Context);
	
	if(UnrealLua::LuaScriptCall::GetSuperCall() == false)
	{
		FLuaOverrideCallParams params{originalFunc, Stack, RESULT_PARAM};
		if(item.ProcessEvent(params))
		{
			//override call handled
			//Skip Blueprint code that is still in the pipeline
			UnrealLua::SkipFunction(Stack, RESULT_PARAM, originalFunc);

			//verify(!Stack.Code || *Stack.Code == EX_EndOfScript || *Stack.Code == EX_EndFunctionParms);
			return;
		}
	}
	UnrealLua::LuaScriptCall::SetSuperCall(false);
	
	if(Stack.Code)
	{
		TGuardValue<UFunction*> NativeFuncGuard(Stack.CurrentNativeFunction, originalFunc);
		TGuardValue<UFunction*> NativeFuncGuardNode(Stack.Node, originalFunc);
		TGuardValue<FField*> NativeFuncGuardProps(Stack.PropertyChainForCompiledIn, Stack.PropertyChainForCompiledIn != nullptr ? originalFunc->ChildProperties : nullptr);

		originalFunc->Invoke(Context, Stack, RESULT_PARAM);
	}

	//verify(!Stack.Code || *Stack.Code == EX_EndOfScript)

	item.RemoveLuaScript();
}

DEFINE_FUNCTION(UUnrealLuaOverrideUFunction::execBlueprintLuaCall)
{
	UUnrealLuaOverrideUFunction* luaFunc = CastChecked<UUnrealLuaOverrideUFunction>(Stack.CurrentNativeFunction);
	UFunction* originalFunc = luaFunc->Overridden;

	
	if (Stack.Node == Stack.CurrentNativeFunction)
	{
		//Memory is mine, params should all be set up
		if(UnrealLua::LuaScriptCall::GetSuperCall() == false)
		{
			FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(Context);

			FLuaOverrideCallParams params{originalFunc, Stack, RESULT_PARAM, nullptr, sol::nil};
			if(item.ProcessEvent(params))
			{
				//override call handled
				//Skip Blueprint code that is still in the pipeline
				//UnrealLua::SkipFunction(Stack, RESULT_PARAM, originalFunc);
				//override call handled
				return;
			}
		}
		UnrealLua::LuaScriptCall::SetSuperCall(false);
			
		if (Stack.Code)
		{
			FFrame newStack{Context, originalFunc, Stack.Locals, nullptr, originalFunc->ChildProperties};
			newStack.MostRecentProperty = Stack.MostRecentProperty;
			newStack.MostRecentPropertyAddress = Stack.MostRecentPropertyAddress;
			newStack.MostRecentPropertyContainer = Stack.MostRecentPropertyContainer;
			newStack.Code = Stack.Code;
			newStack.OutParms = Stack.OutParms;
			originalFunc->Invoke(Context, newStack, RESULT_PARAM);
			Stack.Code = newStack.Code;	
		}
		else
		{
			if (luaFunc->bCallOriginalNativeImplementationFunction)
			{
				FFrame newStack{Context, originalFunc, Stack.Locals, nullptr, originalFunc->ChildProperties};
				newStack.MostRecentProperty = Stack.MostRecentProperty;
				newStack.MostRecentPropertyAddress = Stack.MostRecentPropertyAddress;
				newStack.MostRecentPropertyContainer = Stack.MostRecentPropertyContainer;
				newStack.OutParms = Stack.OutParms;
				originalFunc->Invoke(Context, newStack, RESULT_PARAM);
			}
		}
	}
	else
	{
		//Stack is owned by another function		
		//Need to set up separate stack
		
		//ProcessScriptFunction pulls param values from outer stack code
		UnrealLua::ProcessScriptFunction(Context, originalFunc, Stack, RESULT_PARAM, [](UObject* Context, FFrame& NewStack, RESULT_DECL)
		{
			//Stack is now the new stack, set up for the originalFunc
			//params from the outer code should have been consumed by now and copied to the current stack
			
			UFunction* originalFunc = NewStack.Node;
				
			if(UnrealLua::LuaScriptCall::GetSuperCall() == false)
			{
				FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(Context);

				FLuaOverrideCallParams params{originalFunc, NewStack, RESULT_PARAM, nullptr, sol::nil};
				if(item.ProcessEvent(params))
				{
					return;
				}
			}
			UnrealLua::LuaScriptCall::SetSuperCall(false);
			if (originalFunc->Script.Num() > 0)
			{
				UnrealLua::ProcessLocalScriptFunction(Context, NewStack, RESULT_PARAM);
			}
		});
	}
}

DEFINE_FUNCTION(UUnrealLuaOverrideUFunction::execBlueprintTickLuaCall)
{
	//super calls to here should never arrive
	//verify(!UnrealLua::LuaScriptCall::bSuperCall)

	UUnrealLuaOverrideUFunction* luaFunc = static_cast<UUnrealLuaOverrideUFunction*>(Stack.CurrentNativeFunction);	
	UFunction* originalFunc = luaFunc->Overridden;
	
	FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(Context);
	
	if(item.bLuaTickEnabled)
	{
		//@TODO : Let user decide to tick before and/or after UE Tick? ("PreTick"/"PostTick"? SetPreTickEnabled/SetPostTickEnabled?)
		FLuaOverrideCallParams params{originalFunc, Stack, RESULT_PARAM};
		item.ProcessTickEvent(params);
	}
	
	if(Stack.Code)
	{
		if(!item.bBlueprintTickEnabled)
		{
			UnrealLua::SkipFunction(Stack, RESULT_PARAM, Stack.CurrentNativeFunction);
			return;
		}
		TGuardValue<UFunction*> NativeFuncGuard(Stack.CurrentNativeFunction, originalFunc);
		TGuardValue<UFunction*> NativeFuncGuardNode(Stack.Node, originalFunc);
		TGuardValue<FField*> NativeFuncGuardProps(Stack.PropertyChainForCompiledIn, Stack.PropertyChainForCompiledIn != nullptr ? originalFunc->ChildProperties : nullptr);

		originalFunc->Invoke(Context, Stack, RESULT_PARAM);
	}
}

DEFINE_FUNCTION(UUnrealLuaOverrideUFunction::execBlueprintWidgetTickLuaCall)
{
	//super calls to here should never arrive
	verify(!UnrealLua::LuaScriptCall::GetSuperCall())

	UUnrealLuaOverrideUFunction* luaFunc = static_cast<UUnrealLuaOverrideUFunction*>(Stack.CurrentNativeFunction);	
	UFunction* originalFunc = luaFunc->Overridden;	

	FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(Context);
	if(item.bLuaTickEnabled)
	{
		//@TODO : Let user decide to tick before and/or after UE Tick ("PreTick"/"PostTick"? SetPreTickEnabled/SetPostTickEnabled?)
		FLuaOverrideCallParams params{originalFunc, Stack, RESULT_PARAM};
		item.ProcessWidgetTickEvent(params);
	}
	
	if(Stack.Code)
	{
		if(!item.bBlueprintTickEnabled)
		{
			UnrealLua::SkipFunction(Stack, RESULT_PARAM, Stack.CurrentNativeFunction);
			return;
		}
		TGuardValue<UFunction*> NativeFuncGuard(Stack.CurrentNativeFunction, originalFunc);
		TGuardValue<UFunction*> NativeFuncGuardNode(Stack.Node, originalFunc);
		TGuardValue<FField*> NativeFuncGuardProps(Stack.PropertyChainForCompiledIn, Stack.PropertyChainForCompiledIn != nullptr ? originalFunc->ChildProperties : nullptr);

		originalFunc->Invoke(Context, Stack, RESULT_PARAM);
	}
}