#include "LuaCallHelpers/LuaScriptOverrideCalls.h"

#include "Layout/Geometry.h"
#include "UnrealOverrides/UnrealLuaCompiledUFunction.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"


extern "C" {
	#include "lstate.h"
	#include "lobject.h"
}
#include "lua.hpp"
#include "sol/sol.hpp"
#include "Config/UnrealLuaConstants.h"
#include "Reflection/PropertyHelper.h"
#include "Utility/LuaLogMacros.h"
#include "Reflection/FunctionDescr.h"
#include "UnrealLua.h"
#include "LuaCallHelpers/LuaCallContext.h"
#include "Reflection/PropertyMapping.h"
#include "UnrealOverrides/UnrealLuaOverrideUFunction.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "UObject/Object.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"
#include "Utility/LuaUtility.h"

bool FLuaOverrideCallParams::ValidDataForExecution()
{
	constexpr EFunctionFlags disallowedFuncFlags = FUNC_Net | FUNC_Exec | FUNC_EditorOnly | FUNC_NetValidate | FUNC_Final | FUNC_UbergraphFunction;  
	return this->FuncMapping != nullptr && this->FuncMapping != nullptr && this->FuncMapping->valid() && !this->Function->HasAnyFunctionFlags(disallowedFuncFlags);
}

namespace UnrealLua::LuaScriptCall
{
	bool bSuperCall = false;
	
	inline bool CallUFunctionLuaScript(FLuaOverrideCallParams& callEventParams)
	{
		bool bStaticCall = callEventParams.CallingObjectReference == sol::nil;
		
		sol::function* mapping = callEventParams.FuncMapping;

		UObject* obj = callEventParams.Stack.Object;
		
		UFunction* func = callEventParams.Function;

		{
			//lua_State* lua = mapping->LuaScriptFunction.lua_state();
			//FScopeLock lock{UnrealLua::MultiThreading::GetStateLock(lua)};
			
			sol::protected_function_result result{};
			
			if(func->NumParms == 0)
			{
				//Fast path for functions without args
				if (bStaticCall)
				{
					result = UnrealLua::LuaScriptCall::CallLuaFunctionSafe(*mapping);
				}
				else
				{
					result = UnrealLua::LuaScriptCall::CallLuaFunctionSafe(*mapping, callEventParams.CallingObjectReference);
				}
			}
			else if(func->GetFName() == UnrealLua::PropertyNames::NAME_ReceiveTick && func->NumParms == 1)
			{
				//Fast path for ReceiveTick, which should only have a single float as argument
				float dt = *reinterpret_cast<float*>(callEventParams.Stack.Locals);
				//Tick can never be static, so no point in checking
				result = UnrealLua::LuaScriptCall::CallLuaFunctionSafe(*mapping, callEventParams.CallingObjectReference, dt);					
			}
			else
			{
				//resetve stack space for some args + self. 8 args should be enough for most cases
				TArray<sol::object, TInlineAllocator<9>> args_o{};
				
				if (!bStaticCall)
				{
					args_o.Add(callEventParams.CallingObjectReference);
				}

				void* functionArgsMemory = callEventParams.Stack.Locals;
				//func->InitializeStruct(functionArgsMemory);

				//copy from input parms to lua
				for (TFieldIterator<FProperty> It(func); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
				{
					FProperty* prop = *It;
					if (UnrealLua::PropertyHelper::IsInputParameter(prop))
					{
						FGetPropertyValueParams params {prop, functionArgsMemory, 0, mapping->lua_state()};
						args_o.Add(UnrealLua::PropertyHelper::GetPropertyValue_InContainer(params));	
					}
				}

				result = UnrealLua::LuaScriptCall::CallLuaFunctionSafe(*mapping, sol::as_args(args_o));

				if(result.valid())
				{
					const int resultCount = result.return_count();
					if(resultCount > 0)
					{
						int outValIndex = 0;
						FProperty* retprop = func->GetReturnProperty();
						if(retprop != nullptr)
						{
							outValIndex = 1;
							sol::stack_object value = result[0];
							TSetPropertyValueParams params{retprop, callEventParams.ResultParam, 0, value};
							UnrealLua::PropertyHelper::SetPropertyValue_Direct(params);
						}

						for (TFieldIterator<FProperty> it(func); it && it->HasAnyPropertyFlags(CPF_Parm); ++it)
						{
							FProperty* prop = *it;
							if (prop->HasAnyPropertyFlags(CPF_OutParm) && prop != retprop)
							{
								if(outValIndex < resultCount)
								{
									sol::stack_object value = result[outValIndex];
									TSetPropertyValueParams params{prop, functionArgsMemory, 0, value};
									UnrealLua::PropertyHelper::SetPropertyValue_InContainer(params);
								}
								else
								{
									sol::object nil = sol::nil;
									TSetPropertyValueParams params{prop, functionArgsMemory, 0, nil};
									UnrealLua::PropertyHelper::SetPropertyValue_InContainer(params);							
								}
								outValIndex++;								
							}
						}
					}
					//@TODO : this can be merged with the out parms above
					for (FOutParmRec* rec = callEventParams.Stack.OutParms; rec != nullptr; rec = rec->NextOutParm)
					{
						//Copy from this functions stack memory to parent caller stack memory
						rec->Property->CopyCompleteValueToScriptVM_InContainer(rec->PropAddr, functionArgsMemory);
					}
					/*
					if(func->HasAnyFunctionFlags(FUNC_HasOutParms))
					{
						for(FProperty* prop : func->OutParms)
						{
							if (prop->HasAnyPropertyFlags(CPF_OutParm))
							{
								for (FOutParmRec* rec = callEventParams.Stack.OutParms; rec != nullptr; rec = rec->NextOutParm)
								{
									if(rec->Property == prop)
									{
										//Copy from this functions stack memory to parent caller stack memory
										prop->CopyCompleteValueToScriptVM_InContainer(rec->PropAddr, functionArgsMemory);
										break;
									}
								}
							}
						}
					}
					*/
				}
			}
			//unlocks GLuaLock
		}		
		return true;
	}
	
	bool CallUFunctionOverride(FLuaOverrideCallParams& callEventParams)
	{
		return CallUFunctionLuaScript(callEventParams);
	}
	
	bool CallLuaImplementedUFunction(FLuaOverrideCallParams& callEventParams)
	{
		return CallUFunctionLuaScript(callEventParams);
	}

	bool CallTickUFunctionOverride(FLuaOverrideCallParams& callEventParams)
	{
		sol::object& callingObjectRef = callEventParams.CallingObjectReference;
		
		sol::function* mapping = callEventParams.FuncMapping;

		UFunction* func = callEventParams.Function;
		
		verify(func->GetFName() == UnrealLua::PropertyNames::NAME_ReceiveTick && func->NumParms == 1);
		verify(func->ChildProperties->IsA<FFloatProperty>());
				
		//Fast path for ReceiveTick, which should only have a single float as argument
		float dt = *reinterpret_cast<float*>(callEventParams.Stack.Locals);
		
		(void)UnrealLua::LuaScriptCall::CallLuaFunctionSafe(*mapping, callingObjectRef, dt);
			
		return true;
	}

	bool CallWidgetTickUFunctionOverride(FLuaOverrideCallParams& callEventParams)
	{
		sol::object& callingObjectRef = callEventParams.CallingObjectReference;
		
		sol::function* luaFunc = callEventParams.FuncMapping;

		UFunction* func = callEventParams.Stack.CurrentNativeFunction;
		
		verify(func->GetFName() == UnrealLua::PropertyNames::NAME_UserWidgetTick && func->NumParms == 2);
		verify(func->ChildProperties->IsA<FStructProperty>());
		verify(CastField<FStructProperty>(func->ChildProperties)->Struct == FGeometry::StaticStruct());
		verify(func->ChildProperties->Next->IsA<FFloatProperty>());
		
		//UUserWidget::Tick(FGeometry MyGeometry, float InDeltaTime)
		FGeometry* argsPtr = reinterpret_cast<FGeometry*>(callEventParams.Stack.Locals);
		//Get Geometry
		FGeometry geometry = *argsPtr;
		//Move pointer by sizeof(FGeometry) bytes
		argsPtr++;
		//now it points to InDeltaTime
		float dt = *reinterpret_cast<float*>(argsPtr);

		(void)UnrealLua::LuaScriptCall::CallLuaFunctionSafe(*luaFunc, callingObjectRef, &geometry, dt);
			
		return true;
	}

	int SuperCall(lua_State* L)
	{
		if(lua_gettop(L) < 2)
		{
			return 0;
		}

		sol::stack_object self{L, 1};
		
	
		if (!self.valid())
		{
			LUA_LOG_ERROR("Unable to call super function : self is not valid. Please use \":\" instead of \".\" when calling a function or manually pass 'self' as a parameter when using \".\"")
			return 0;
		}

		sol::stack_object funcName{L, 2};

		if(!funcName.valid() || funcName.get_type() != sol::type::string)
		{
			LUA_LOG_ERROR("Unable to call super function : funcname is not a valid string.")
			return 0;
		}
		
		UObject* callTarget = UnrealLua::GetUObject(self);
		if (!callTarget)
		{
			LUA_LOG_ERROR("Unable to call reflected function : Could not find valid UObject, neither as self nor as self[true]. Please use \".\" instead of \":\" when calling a function or manually pass 'self' as a parameter")
			return 0;
		}
		
		FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(callTarget);
		
		
		//Figure out whether it's a UnrealLua-compiled-function supercall
		
		int32 top = sol::stack::top(L);
		//get info about calling function
		lua_Debug ar{};
		lua_getstack(L, 1, &ar);
		lua_getinfo(L, "f", &ar);
		
		verify(lua_isfunction(L, -1));
		TValue* val = UnrealLua::Utility::index2value(L, -1);
		verify(ttisfunction(val));
		LClosure *f = clLvalue(val);
		Proto* funcProto = f->p;
		UFunction* callingUFunction = static_cast<UFunction*>(funcProto->UnrealLuaCompiledFunction);
		lua_settop(L, top);
		
		if (callingUFunction != nullptr)
		{
			UUnrealLuaCompiledUFunction* compiledUFunction = static_cast<UUnrealLuaCompiledUFunction*>(callingUFunction);
			verify(IsValid(compiledUFunction))
			//Call super UFunction
			//UFunction* parentFunction = compiledUFunction->GetSuperFunction();			
			
			const FFunctionDescr* parentDescr = compiledUFunction->GetParentDescr();
			if (!parentDescr)
			{
				return 0;
			}
			
			UnrealLua::LuaScriptCall::SetSuperCall(true);
			
			//FFunctionDescr descr{parentFunction};
			
			int numpushed = parentDescr->PerformCall(callTarget, L);
			
			UnrealLua::LuaScriptCall::SetSuperCall(false);
			
			return numpushed;
		}
		else
		{
			const FHashedFieldMapping* mapping = item.GetPropertyMapping(funcName);
		
			if(!mapping || !mapping->IsFunction())
			{
				return 0;
			}

			const FFunctionDescr* descr = mapping->GetFunction();

			FName funcFName = descr->Func->GetFName();
		
			if(funcFName == UnrealLua::PropertyNames::NAME_ReceiveTick || funcFName == UnrealLua::PropertyNames::NAME_Tick)
			{
				//no need for supercalling tick
				LUA_LOG_WARNING("Super-calling Tick or ReceiveTick not needeed, it will get called automatically after Lua tick calls")
				return 0;
			}
		
			if(descr->Func->HasAnyFunctionFlags(FUNC_Static))
			{
				callTarget = descr->Func->GetOuterUClassUnchecked()->GetDefaultObject();
			}
		
			UnrealLua::LuaScriptCall::SetSuperCall(true);
		
			//FLuaSuperCallInfo superCallInfo{};

			int numPushed = descr->PerformCall(callTarget, L);
		
			//verify(superCallInfo.IsDone());

			UnrealLua::LuaScriptCall::SetSuperCall(false);
		
			return numPushed;	
		}
	}

	void CallMulticastDelegateBoundFunction(sol::table script, const std::string& funcName, UFunction* delegateFunction, void* parms)
	{
		checkNoEntry();
		if(!delegateFunction || !script.valid())
		{
			return;
		}
		sol::protected_function luaFunc = script.raw_get<sol::function>(funcName);

		if(!luaFunc.valid())
		{
			return;
		}

		//@TODO : Get FFunctionDescr instead to save analyzing func props
		const EFunctionFlags funcFlags = delegateFunction->FunctionFlags;
		TArray<FProperty*, TInlineAllocator<8>> inputParms;
		TArray<FProperty*, TInlineAllocator<8>> outParms;
	
		for (TFieldIterator<FProperty> propIt(delegateFunction); propIt; ++propIt)
		{
			FProperty* property = *propIt;
			const uint64 propflags = property->GetPropertyFlags();
			if (propflags & CPF_Parm)
			{
				//native return parameter
				if (funcFlags & EFunctionFlags::FUNC_Native && propflags & CPF_ReturnParm)
				{
					outParms.Emplace(property);	
				}
				if(UnrealLua::PropertyHelper::IsInputParameter(property))
				{
					inputParms.Emplace(property);
				}
				if(UnrealLua::PropertyHelper::IsOutputParameter(property))
				{
					outParms.Emplace(property);	
				}
			}
		}
	
		std::vector<sol::object> args_o{};
		sol::this_state lua = luaFunc.lua_state();
		for(FProperty* prop : inputParms)
		{
			//copy from input parms to lua
			FGetPropertyValueParams getParms{prop, parms, 0, lua};
			args_o.push_back(UnrealLua::PropertyHelper::GetPropertyValue(getParms));
		}
		const sol::protected_function_result result = luaFunc(script, sol::as_args(args_o));

		if(!result.valid())
		{
			if(result.status() != sol::call_status::ok)
			{
				const std::string errStr = result.get<sol::error>().what();
				LUA_LOG_ERROR("Error during Lua function override call %hs : \n%hs", funcName.c_str(), errStr.c_str());
			}
			return;
		}
		//Multicast delegates can't have a return value
	}

	void CallMulticastDelegateBoundFunction(UObject* self, sol::function luafunc, UFunction* func, void* parms)
	{
		if(!func || !IsValid(self) || !luafunc.valid())
		{
			return;
		}
		sol::protected_function luaFunc = luafunc;

		if(!luaFunc.valid())
		{
			return;
		}

		//@TODO : Get FFunctionDescr instead to save analyzing func props
		const EFunctionFlags funcFlags = func->FunctionFlags;
		TArray<FProperty*, TInlineAllocator<8>> inputParms;
		TArray<FProperty*, TInlineAllocator<8>> outParms;
	
		for (TFieldIterator<FProperty> propIt(func); propIt; ++propIt)
		{
			FProperty* property = *propIt;
			const uint64 propflags = property->GetPropertyFlags();
			if (propflags & CPF_Parm)
			{
				//native return parameter
				if (funcFlags & EFunctionFlags::FUNC_Native && propflags & CPF_ReturnParm)
				{
					outParms.Emplace(property);	
				}
				if(UnrealLua::PropertyHelper::IsInputParameter(property))
				{
					inputParms.Emplace(property);
				}
				if(UnrealLua::PropertyHelper::IsOutputParameter(property))
				{
					outParms.Emplace(property);	
				}
			}
		}
	
		std::vector<sol::object> args_o{};
		sol::this_state lua = luaFunc.lua_state();
		for(FProperty* prop : inputParms)
		{
			//copy from input parms to lua
			FGetPropertyValueParams getParms{prop, parms, 0, lua};
			args_o.push_back(UnrealLua::PropertyHelper::GetPropertyValue(getParms));
		}
		const sol::protected_function_result result = luaFunc(self, sol::as_args(args_o));

		if(result.valid())
		{
			if(func->HasAnyFunctionFlags(FUNC_MulticastDelegate))
			{
				verify(func->GetReturnProperty() == nullptr);
				//can't have return values on multicast delegate
				return;
			}
			const int resultCount = result.return_count();
			if(resultCount > 0)
			{
				int outValIndex = 0;
				if(func->GetReturnProperty())
				{
					outValIndex = 1;
					FProperty* prop = func->GetReturnProperty();
					sol::stack_object value = result[outValIndex];
					TSetPropertyValueParams params{prop, parms, 0, value};
					UnrealLua::PropertyHelper::SetPropertyValue_InContainer(params);
				}
				for(FProperty* prop : outParms)
				{
					if(outValIndex < resultCount)
					{
						sol::stack_object value = result[outValIndex];
						TSetPropertyValueParams params{prop, parms, 0, value};
						UnrealLua::PropertyHelper::SetPropertyValue_InContainer(params);							 
					}
					else
					{
						sol::object nil = sol::nil;
						TSetPropertyValueParams params{prop, parms, 0, nil};
						UnrealLua::PropertyHelper::SetPropertyValue_InContainer(params);							
					}
					outValIndex++;							
				}					
			}
		}
		else
		{
			if(result.status() != sol::call_status::ok)
			{
				FName funcName = func->GetFName();
				const std::string errStr = result.get<sol::error>().what();
				LUA_LOG_ERROR("Error during Lua function override call %s : \n%hs", *funcName.ToString(), errStr.c_str());
			}
			return;
		}
	}
}
