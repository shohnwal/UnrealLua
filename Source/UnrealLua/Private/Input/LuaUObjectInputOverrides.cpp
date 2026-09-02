#include "Input/LuaUObjectInputOverrides.h"

#include "EnhancedInputComponent.h"
#include "UnrealLua.h"
#include "GameFramework/Actor.h"
#include "GameFramework/InputSettings.h"
#include "LuaContext/LuaScripts/LoadedLuaScriptCollection.h"
#include "Input/LuaStateInputHandler.h"
#include "Interface/LuaScriptable.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"
#include "InputTriggers.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"
#include "Utility/UnrealVersion.h"
namespace UnrealLua::InputOverride
{
	static const TCHAR* SReadableInputEvent[] = { TEXT("Pressed"), TEXT("Released"), TEXT("Repeat"), TEXT("DoubleClick"), TEXT("Axis"), TEXT("Max") };
	static const TCHAR* SEnhancedInputActionTypes[] = { TEXT("None"), TEXT("Triggered"), TEXT("Started"), TEXT("Ongoing"), TEXT("Canceled"), TEXT("Completed") };
}


FLuaEnhancedInputActionMapping::FLuaEnhancedInputActionMapping(const UInputAction* inputAction)
	: InputAction(inputAction), EventMappings()
{	
	this->EventMappings.AddDefaulted(6);
	this->EventMappings[0].TriggerEventType = ETriggerEvent::None;
	this->EventMappings[1].TriggerEventType = ETriggerEvent::Triggered;
	this->EventMappings[2].TriggerEventType = ETriggerEvent::Started;
	this->EventMappings[3].TriggerEventType = ETriggerEvent::Ongoing;
	this->EventMappings[4].TriggerEventType = ETriggerEvent::Canceled;
	this->EventMappings[5].TriggerEventType = ETriggerEvent::Completed;
}

FLuaUObjectInputOverrides::FLuaUObjectInputOverrides(UObject* owningObject, UInputComponent* inputcmp)
	: OwningObject(owningObject), InputComponent(inputcmp)
{
	
}

FLuaUObjectInputOverrides::~FLuaUObjectInputOverrides()
{
	this->ClearInputMappings();
}

void FLuaUObjectInputOverrides::BindInputFunctions()
{
	this->ClearInputMappings();
	
	UObject* obj = this->OwningObject.Get();
	if (!obj)
	{
		return;
	}
	FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(obj);
	
	ULoadedLuaScriptCollection* coll = item.GetLuaScriptHandle().GetLuaScriptCollection();
	
	if(!coll || !coll->ShouldOverrideInput())
	{
		return;
	}
	
	AActor* myActor = Cast<AActor>(obj);
	if(!myActor)
	{
		return;
	}
	UInputComponent* input = myActor->InputComponent;
	if(!input)
	{
		return;
	}

	this->InputComponent = input;
	
	if(UEnhancedInputComponent* cmp = Cast<UEnhancedInputComponent>(input))
	{
		this->BindEnhancedActionEvents(myActor, cmp);
	}
	else
	{
		this->BindActionEvents(myActor, input);
		this->BindAxisEvents(myActor, input);
	}
}

void FLuaUObjectInputOverrides::ClearInputMappings()
{
	if(this->InputComponent.IsValid())
	{
		UInputComponent* icmp = this->InputComponent.Get();
		UEnhancedInputComponent* ecmp = Cast<UEnhancedInputComponent>(icmp);
		for(const FLuaEnhancedInputActionMapping& binding : this->EnhancedInputActionMappings)
		{
			for (auto& triggerEventBinding : binding.EventMappings)
			{
			if(ecmp)
			{
				ecmp->RemoveBindingByHandle(triggerEventBinding.Handle);
				const TUniquePtr<FEnhancedInputActionEventBinding>& ptr = triggerEventBinding.OriginalBinding;
				if (ptr)
				{
					if (FEnhancedInputActionEventDelegateBinding<FEnhancedInputActionHandlerDynamicSignature>* orig = 
					static_cast<FEnhancedInputActionEventDelegateBinding<FEnhancedInputActionHandlerDynamicSignature>*>(ptr.Get()))
					{
						//ecmp->BindAction(orig->Delegate);
						//orig->Delegate.
					}
					//FEnhancedInputActionEventBinding& originalBinding = *ptr;
					//ecmp->BindAction(originalBinding.GetAction(), originalBinding.GetTriggerEvent(), originalBinding.GetUObject(), originalBinding.)
				}
			}
			}
		}
		for (const FLuaActionMapping& binding : this->InputMappings)
		{
			if (icmp)
			{
				icmp->RemoveActionBindingForHandle(binding.Handle);
			}
		}
		this->InputComponent = nullptr;
	}
	this->EnhancedInputActionMappings.Empty();
}

void FLuaUObjectInputOverrides::BindEnhancedActionEvents(AActor* myActor, UEnhancedInputComponent* myInputComponent)
{
	verify(myActor->Implements<ULuaScriptable>());
	verify(this->EnhancedInputActionMappings.IsEmpty())
	
	//APlayerController* pc = Cast<APlayerController>(myActor); //could be null, if this is not a pc
	
	const TArray<TUniquePtr<FEnhancedInputActionEventBinding>>& bindings = myInputComponent->GetActionEventBindings();
	
	int32 numBindings = bindings.Num();
	//ULuaPlayerInputHandler* handler = UUnrealLuaEngineSubsystem::Get()->GetInputHandler();

	TArray<uint32> bindingsToRemove{};
	
	const bool useIndividualLuaFunctions = false;
	
	auto funcNamBuilder = [](const UInputAction* action, ETriggerEvent event, bool individualFunctionNames) -> FString
	{
		FString ActionName = action->GetFName().ToString();
		uint8 eventIndex = std::countr_zero(static_cast<uint8>(event));
		++eventIndex;
		FString luaFuncStr = "";
		if (individualFunctionNames)
		{
			luaFuncStr = event == ETriggerEvent::None 
			? *ActionName 
			: *FString::Printf(TEXT("%s_%s"), *ActionName, UnrealLua::InputOverride::SEnhancedInputActionTypes[eventIndex]);
		}
		else
		{
			luaFuncStr = "InputAction_" + ActionName;
		}
		return luaFuncStr;
	};
	
	for(int32 bindingIndex = 0; bindingIndex < numBindings; bindingIndex++)
	{
		FEnhancedInputActionEventBinding* binding = bindings[bindingIndex].Get();

		if (binding->IsBoundToObject(this))
		{
			continue;
		}
		
		TWeakObjectPtr<UObject> boundObject = binding->GetUObject();
		const UInputAction* action = binding->GetAction();
		ETriggerEvent event = binding->GetTriggerEvent();
		
		if (!action || !boundObject.IsValid() || event == ETriggerEvent::None)
		{
			continue;
		}
		bindingsToRemove.Emplace(binding->GetHandle());
		
		verify(boundObject.Get() == myActor);
		
		int32 foundIndex = this->EnhancedInputActionMappings.IndexOfByPredicate([action](const FLuaEnhancedInputActionMapping& item)
		{
			return item.InputAction == action;
		});
		if (foundIndex == INDEX_NONE)
		{
			foundIndex = this->EnhancedInputActionMappings.Emplace(action);
		}
		
		FLuaEnhancedInputActionMapping& actionInputMapping = this->EnhancedInputActionMappings[foundIndex];
		
		//Type of action (Started, Ongoing, Completed, etc) 
		uint8 eventIndex = std::countr_zero(static_cast<uint8>(event));
		++eventIndex;
		FLuaEnhancedActionMapping& eventTriggerMapping = actionInputMapping.EventMappings[eventIndex];
		
		verify(eventTriggerMapping.TriggerEventType == event);
		//const_cast<UInputAction*>(action)->bConsumeInput = true;
		
		//Build function name ActionName_Action, i.e. IA_Jump_Pressed, IA_Fire_Released, except for ETriggerEvent::None,
		//in which case the original action name is used without any suffix : IA_Move

		FString luaFuncStr = funcNamBuilder(action, event, useIndividualLuaFunctions);
		
		FName luaFuncName{*luaFuncStr};

		eventTriggerMapping.FuncName = *luaFuncStr;
		eventTriggerMapping.Handle = 0;
		eventTriggerMapping.OriginalBinding = binding->Clone();
		
		FEnhancedInputActionEventBinding& newBinding = myInputComponent->BindActionInstanceLambda(action, event, [this, foundIndex, boundObject, luaFuncName](const FInputActionInstance& actionInstance)
		{
			this->NotifyEnhancedAction(foundIndex, actionInstance, boundObject, luaFuncName);
		});
		
		eventTriggerMapping.Handle = newBinding.GetHandle();
	}

	for(int32 handle : bindingsToRemove)
	{
		myInputComponent->RemoveBindingByHandle(handle);
	}
	
	//bind rest action mapping events that were not overridden
	for (int32 actionMappingIndex = 0; actionMappingIndex < this->EnhancedInputActionMappings.Num(); ++actionMappingIndex)
	{
		FLuaEnhancedInputActionMapping& actionMapping = this->EnhancedInputActionMappings[actionMappingIndex];
	
		const UInputAction* action = actionMapping.InputAction;
	
		for (int32 eventTriggerMappingIndex = 1; eventTriggerMappingIndex < actionMapping.EventMappings.Num(); ++eventTriggerMappingIndex)
		{
			FLuaEnhancedActionMapping& eventTriggerMapping = actionMapping.EventMappings[eventTriggerMappingIndex];
			ETriggerEvent event = eventTriggerMapping.TriggerEventType;
			verify(event != ETriggerEvent::None);
			
			if (eventTriggerMapping.Handle != 0)
			{
				verify(eventTriggerMapping.OriginalBinding.IsValid())
				verify(eventTriggerMapping.FuncName != NAME_None);
				continue;
			}
			
			verify(!eventTriggerMapping.OriginalBinding.IsValid())
			verify(eventTriggerMapping.FuncName == NAME_None);
			
			
			FString luaFuncStr = funcNamBuilder(action, event, useIndividualLuaFunctions);
		
			FName luaFuncName{*luaFuncStr};

			eventTriggerMapping.FuncName = *luaFuncStr;
			eventTriggerMapping.Handle = 0;
			eventTriggerMapping.TriggerEventType = event;
		
			FEnhancedInputActionEventBinding& newBinding = myInputComponent->BindActionInstanceLambda(action, event, [this, actionMappingIndex, myActor, luaFuncName](const FInputActionInstance& actionInstance)
			{
				this->NotifyEnhancedAction(actionMappingIndex, actionInstance, myActor, luaFuncName);
			});
		
			eventTriggerMapping.Handle = newBinding.GetHandle();
		}
	}
}

void FLuaUObjectInputOverrides::BindActionEvents(AActor* actor, UInputComponent* input)
{
	checkNoEntry();
	UInputSettings* InputSettings = UInputSettings::GetInputSettings();

	TArray<FName> allInputActions;
	InputSettings->GetActionNames(allInputActions);
	FLuaUObjectItem& hostItem = UnrealLua::UObjectRegistry::GetUObjectItem(actor);
	ULuaStateInputHandler* handler = nullptr; //UUnrealLuaEngineSubsystem::Get()->GetInputHandler();

	//First, handle actions that are already present in the Blueprint bindings
	int32 NumActionBindings = input->GetNumActionBindings();
	for (int32 i = 0; i < NumActionBindings; ++i)
	{
		FInputActionBinding &binding = input->GetActionBinding(i);
		FName Name = binding.GetActionName();

		allInputActions.Remove(Name);
		
		FString ActionName = Name.ToString();

		//Build function name ActionName_Action, i.e. Jump_Pressed, Fire_Released
		FString FuncStr = *FString::Printf(TEXT("%s_%s"), *ActionName, UnrealLua::InputOverride::SReadableInputEvent[binding.KeyEvent]);
		FName FuncName = *FuncStr;

		LUA_LOG("Trying to find lua function %s for action %s", *FuncName.ToString(), *ActionName)

		sol::object maybeFunc = hostItem.GetLuaScriptFunction(StringCast<char>(*FuncStr).Get());

		if(maybeFunc.valid() && maybeFunc.get_type() == sol::type::function)
		{
			EInputEvent IE = binding.KeyEvent;
			//found a matching Lua function with that name
			LUA_LOG("Found lua func %s for action %s", *FuncStr, *ActionName)
			//replace delegate binding
			//binding.ActionDelegate.BindDelegate<FOnActionInputDelegate>(this ,&FLuaUObjectInputOverrides::NotifyInputAction, actor, FuncName, IE);
			
			//save binding data
			//this->InputMappings.Emplace(FLuaEnhancedActionMapping{FuncName, binding.GetHandle(), false, nullptr});
		}

		//If it's not paired, create a new binding for it to make it pair up
		//If it's already paired, then it should be an entry in the input->GetActionBinding(i)  
		if (!binding.IsPaired())
		{
			EInputEvent IE_Other = binding.KeyEvent == IE_Pressed ? IE_Released : IE_Pressed;
			
			FName otherFuncName = FName(*FString::Printf(TEXT("%s_%s"), *ActionName, UnrealLua::InputOverride::SReadableInputEvent[IE_Other]));
			
			sol::object maybeOtherFunc = hostItem.GetLuaScriptFunction(StringCast<char>(*FuncStr).Get());
				
			if (maybeFunc.valid() && maybeOtherFunc.get_type() == sol::type::function)
			{
				LUA_LOG("Found lua func %s for action %s to pair", *otherFuncName.ToString(), *ActionName)
				
				FInputActionBinding ab2(Name, IE_Other);
				//ab2.ActionDelegate.BindDelegate<FOnActionInputDelegate>(this ,&FLuaUObjectInputOverrides::NotifyInputAction, actor, otherFuncName, IE_Other);
					
				FInputActionBinding& otherBindingResult = input->AddActionBinding(ab2);
				//this->InputMappings.Emplace(FLuaEnhancedActionMapping{otherFuncName, otherBindingResult.GetHandle()});
			}
		}
	}
	//Check all actions remaining that were not previously bound by the Blueprint
	for(FName action : allInputActions)
	{
		FString actionNameStr = action.ToString();

		//Key pressed event
		{
			EInputEvent pressedEvent = EInputEvent::IE_Pressed;
			//Build function name ActionName_Action, i.e. Jump_Pressed, Fire_Released
			FString FuncStr = *FString::Printf(TEXT("%s_%s"), *actionNameStr, UnrealLua::InputOverride::SReadableInputEvent[pressedEvent]);
			FName FuncName = *FuncStr;

			LUA_LOG("Trying to find lua function %s for action %s", *FuncName.ToString(), *actionNameStr)

			sol::object maybeFunc = hostItem.GetLuaScriptFunction(StringCast<char>(*FuncStr).Get());

			if(maybeFunc.valid() && maybeFunc.get_type() == sol::type::function)
			{
				//found a matching Lua function with that name
				LUA_LOG("Found lua func %s for action %s", *FuncStr, *actionNameStr)
				//replace delegate binding
				FInputActionBinding ab2(action, pressedEvent);
				//ab2.ActionDelegate.BindDelegate<FOnActionInputDelegate>(this ,&FLuaUObjectInputOverrides::NotifyInputAction, actor, FuncName, pressedEvent);
					
				FInputActionBinding& otherBindingResult = input->AddActionBinding(ab2);
				//this->InputMappings.Emplace(FLuaEnhancedActionMapping{FuncName, otherBindingResult.GetHandle()});
			}
		}

		//Key released event
		{
			EInputEvent releasedEvent = EInputEvent::IE_Released;
			//Build function name ActionName_Action, i.e. Jump_Pressed, Fire_Released
			FString FuncStr = *FString::Printf(TEXT("%s_%s"), *actionNameStr, UnrealLua::InputOverride::SReadableInputEvent[releasedEvent]);
			FName FuncName = *FuncStr;

			LUA_LOG("Trying to find lua function %s for action %s", *FuncName.ToString(), *actionNameStr)

			sol::object maybeFunc = hostItem.GetLuaScriptFunction(StringCast<char>(*FuncStr).Get());

			if(maybeFunc.valid() && maybeFunc.get_type() == sol::type::function)
			{
				//found a matching Lua function with that name
				LUA_LOG("Found lua func %s for action %s", *FuncStr, *actionNameStr)
				//replace delegate binding
				FInputActionBinding ab2(action, releasedEvent);
				//ab2.ActionDelegate.BindDelegate<FOnActionInputDelegate>(this, &FLuaUObjectInputOverrides::NotifyInputAction, actor, FuncName, releasedEvent);
					
				FInputActionBinding& otherBindingResult = input->AddActionBinding(ab2);
				//this->InputMappings.Emplace(FLuaEnhancedActionMapping{FuncName, otherBindingResult.GetHandle()});
			}
		}
	}
}

void FLuaUObjectInputOverrides::BindAxisEvents(AActor* Actor, UInputComponent* Input)
{
}

void FLuaUObjectInputOverrides::NotifyInputAction(AActor* actor, FName funcName, EInputEvent IE)
{
	
}


void FLuaUObjectInputOverrides::NotifyEnhancedAction(const int32 eventActionMappingIndex, const FInputActionInstance& actionInstance, TWeakObjectPtr<UObject> target, const FName funcName)
{
	UObject* obj = target.Get();
	if(obj)
	{
		ETriggerEvent eventType = actionInstance.GetTriggerEvent();
#if UNREALLUA_UE_VERSION_OLDER_THAN(5,8,0)
		UEnum* uenum = StaticEnum<ETriggerEvent>();
#else
		UEnum* uenum = FindObjectChecked<UEnum>(nullptr, TEXT("/Script/EnhancedInput.ETriggerEvent"));
#endif
		FLuaUEnumEntry enumEntry{uenum, (int64)eventType};
		
		const FInputActionValue& inputVal =  actionInstance.GetValue();

		sol::protected_function_result bCallSuper{};
		switch(inputVal.GetValueType())
		{
		case EInputActionValueType::Boolean:
			{
				bool inputValue = inputVal.Get<bool>();
				bCallSuper = UnrealLua::LuaScriptCall::CallLuaFunctionSafeByName(obj, funcName, obj, inputValue, enumEntry);		
			}
			break;
		case EInputActionValueType::Axis1D:
			{
				// input is a float
				float movementVector = inputVal.Get<float>();
				bCallSuper = UnrealLua::LuaScriptCall::CallLuaFunctionSafeByName(obj, funcName, obj, movementVector, enumEntry);
			}
			break;
		case EInputActionValueType::Axis2D:
			{
				// input is a Vector2D
				FVector movementVector = inputVal.Get<FVector>();
				bCallSuper = UnrealLua::LuaScriptCall::CallLuaFunctionSafeByName(obj, funcName, obj, movementVector, enumEntry);
			}
			break;
		case EInputActionValueType::Axis3D:
			{
				// input is a Vector
				FVector movementVector = inputVal.Get<FVector>();
				bCallSuper = UnrealLua::LuaScriptCall::CallLuaFunctionSafeByName(obj, funcName, obj, movementVector, enumEntry);
			}
			break;
		}

		if(bCallSuper.valid() && bCallSuper.get<bool>())
		{
			uint8 eventIndex = std::countr_zero(static_cast<uint8>(eventType));
			++eventIndex;
		
			FLuaEnhancedInputActionMapping& mapping = this->EnhancedInputActionMappings[eventActionMappingIndex];
			FEnhancedInputActionEventBinding* binding = mapping.EventMappings[eventIndex].OriginalBinding.Get();
			if (binding != nullptr)
			{
				binding->Execute(actionInstance);
			}	
		}
	}
}
