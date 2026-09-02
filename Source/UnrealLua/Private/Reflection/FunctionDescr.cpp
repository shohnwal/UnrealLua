// Fill out your copyright notice in the Description page of Project Settings.


#include "Reflection/FunctionDescr.h"
#include "Reflection/PropertyHelper.h"
#include "CoreUObject.h"
#include "sol/sol.hpp"
//#include "UObject/PropertyIterator.h"
#include "Reflection/PropertyHelperTypes.h"
#include "LuaCoreDelegates.h"
#include "Config/UnrealLua_CompilerFlags.h"
//#include "LuaCallHelpers/LuaScriptOverrideCalls.h"
#include "StringHandling/UnrealLuaStringCache.h"
#include "Utility/LuaJitFakeTypes.h"
#include "UnrealLua.h"
#include "UnrealOverrides/UnrealLuaCompiledUFunction.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"

extern "C"
{
	#include "lstate.h"
}

namespace UnrealLua::LuaScriptCall
{
	FFunctionDescr* CurrentFunc = nullptr;
}

static const FDelegateHandle fFunctionDescrLuaTypeHandle = FLuaCoreDelegates::OnRegisterLuaUsertypes.AddStatic(&FFunctionDescr::RegisterUsertype);

#define IsRealOutParam(propflag) ((propflag&CPF_OutParm) && !(propflag&CPF_ConstParm) && !(propflag&CPF_BlueprintReadOnly))

void FFunctionDescr::RegisterUsertype(sol::state_view& lua)
{
	lua.new_usertype<FFunctionDescr>(
		"FunctionDescr",
		"new", sol::no_constructor
//		sol::meta_function::call, sol::c_call<decltype(&FFunctionDescr::operator()), &FFunctionDescr::operator()>
	);
}

/*
 * Possibilities:
 * 1) uobj.DoStuff(x,y,z)			first param is function argument (if any)
 * 2) uobj:DoStuff(x,y,z)			first param is uobj
 * 3) script.DoStuff(x,y,z)			first param is argument (if any)
 * 4) script:DoStuff(x,y,z)			
 * 5) local func = uobj.Func
 *    func(x,y.z)
 * 6) local funcself = self.Func
 *    funcself(x,y.z)
 */
	
/*
if(args.size == 0)
{
	
}
sol::object self = args[0];
*/

sol::variadic_results FFunctionDescr::operator()(sol::stack_object self, sol::variadic_args args)
{
	if (!self.valid())
	{
		LUA_LOG_ERROR("Unable to call reflected function : self is not valid. Please use \":\" instead of \".\" when calling a function or manually pass 'self' as a parameter when using \".\"")
		return {};
	}

	if(this->bIsUnrealLuaCompiledFunction)
	{
		return Cast<UUnrealLuaCompiledUFunction>(this->Func)->PerformDirectLuaCall(self, args);
	}
	
	UObject* obj = nullptr;
	if(this->Func->HasAllFunctionFlags(FUNC_Static))
	{
		obj = this->Func->GetOuterUClassUnchecked()->GetDefaultObject();
	}
	else
	{
		obj = UnrealLua::LightUserdata::GetUObject(self);
	}
	
	if (!obj)
	{
		LUA_LOG_ERROR("Unable to call reflected function : Could not find valid UObject, neither as self nor as self[true]. Please use \".\" instead of \":\" when calling a function or manually pass 'self' as a parameter")
		return {};	
	}

	return this->PerformCall(obj, args);
}

//called by UnrealLua::ScriptCall::SuperCall
int FFunctionDescr::PerformCall(UObject* obj, lua_State* L) const
{
	int numPushed = 0;
	
	UFunction* function = this->Func;

	FProperty* returnParm = this->ReturnParm; 
	bool bHasAnyOutput = returnParm != nullptr || !this->OutParms.IsEmpty();
	
	if constexpr(UnrealLua::Compilation::WITH_UFUNCTION_CHAINING)
	{
		//if a function has no output/return params, return calling object
		//This allows function chaining self:DoThis(x):DoThat(y,z):AndThat(w)
		if(!bHasAnyOutput) //@TODO : THis should also not get pushed for tick or super calls...
		{
			//@TODO : This can be filled out with the already known stack_object "self" from UnrealLua::ScriptCall::SuperCall
			numPushed += UnrealLua::UObjectRegistry::PushUObjectAsLightUserdata(L, obj);
		}		
	}

	if (function->NumParms == 0)
	{
		this->PerformCallInternal(obj, function, nullptr);
	}
	else
	{
		void* functionArgsMemory = (uint8*)FMemory_Alloca_Aligned(function->ParmsSize, function->GetMinAlignment());
		
		function->InitializeStruct(functionArgsMemory);

		FUFunctionCallInputLuaObjectRecord inputRecord{};
		
		inputRecord.Reserve(this->InputParms.Num());

		this->PrepareCallInternal(functionArgsMemory, L, inputRecord);
		
		this->PerformCallInternal(obj, function, functionArgsMemory);

		numPushed += this->EvaluateReturnValues(functionArgsMemory, L, inputRecord);
	}
	return numPushed;	
}


sol::variadic_results FFunctionDescr::PerformCall(UObject* obj, const sol::variadic_args& args) const
{
	sol::variadic_results results{};
	
	UFunction* function = this->Func;

	FProperty* returnParm = this->ReturnParm; 
	bool bHasAnyOutput = returnParm != nullptr || !this->OutParms.IsEmpty();
	
	results.reserve(this->OutParms.Num() + (!bHasAnyOutput && UnrealLua::Compilation::WITH_UFUNCTION_CHAINING) + static_cast<bool>(returnParm != nullptr));
	
	if constexpr(UnrealLua::Compilation::WITH_UFUNCTION_CHAINING)
	{
		//if a function has no output/return params, return calling object
		//This allows function chaining self:DoThis(x):DoThat(y,z):AndThat(w)
		if(!bHasAnyOutput)
		{
			//@TODO : This can be filled out with the already known stack_object "self" from FFunctionDescr::operator()
			results.push_back(UnrealLua::UObjectRegistry::GetUObjectAsLightUserdata(args.lua_state(), obj));
		}		
	}

	if (this->Func->NumParms == 0)
	{
		this->PerformCallInternal(obj, function, nullptr);
		//obj->ProcessEvent(function, nullptr);
	}
	else
	{
		void* functionArgsMemory = (uint8*)FMemory_Alloca_Aligned(function->ParmsSize, function->GetMinAlignment());
		
		FMemory::Memzero( functionArgsMemory, function->ParmsSize );
		//function->InitializeStruct(functionArgsMemory);

		FUFunctionCallInputLuaObjectRecord inputRecord{};
		
		inputRecord.Reserve(this->InputParms.Num());

		this->PrepareCallInternal(functionArgsMemory, args, inputRecord);
		
		this->PerformCallInternal(obj, function, functionArgsMemory);
		//obj->ProcessEvent(function, functionArgsMemory);

		this->EvaluateReturnValues(functionArgsMemory, results, args.lua_state(), inputRecord);
	}
	return results;
}

sol::variadic_results FFunctionDescr::PerformCall(UObject* obj, const std::vector<sol::object>& args, sol::this_state lua) const
{
	sol::variadic_results results{};

	FProperty* returnParm = this->ReturnParm;
	UFunction* function = this->Func;
	
	//bool bHasAnyOutput = function->ReturnValueOffset != MAX_uint16 || Func->HasAnyFunctionFlags(EFunctionFlags::FUNC_HasOutParms);
	bool bHasAnyOutput = returnParm != nullptr || !this->OutParms.IsEmpty();

	results.reserve(this->OutParms.Num() + (!bHasAnyOutput && UnrealLua::Compilation::WITH_UFUNCTION_CHAINING) + static_cast<bool>(returnParm != nullptr));
	
	if constexpr(UnrealLua::Compilation::WITH_UFUNCTION_CHAINING)
	{
		//if a function has no output/return params, return calling object
		//This allows function chaining self:DoThis(x):DoThat(y,z):AndThat(w)
		if(!bHasAnyOutput)
		{
			results.push_back(UnrealLua::UObjectRegistry::GetUObjectAsLightUserdata(lua, obj));
		}		
	}

	if (this->Func->NumParms == 0)
	{
		this->PerformCallInternal(obj, function, nullptr);
		//obj->ProcessEvent(function, nullptr);
	}
	else
	{
		void* functionArgsMemory = (uint8*)FMemory_Alloca_Aligned(function->ParmsSize, function->GetMinAlignment());

		FMemory::Memzero( functionArgsMemory, function->ParmsSize );
		//function->InitializeStruct(functionArgsMemory);

		FUFunctionCallInputLuaObjectRecord inputRecord{};
		
		inputRecord.Reserve(this->InputParms.Num());
		
		this->PrepareCallInternal(functionArgsMemory, args, inputRecord);
		
		this->PerformCallInternal(obj, function, functionArgsMemory);
		//obj->ProcessEvent(function, functionArgsMemory);

		this->EvaluateReturnValues(functionArgsMemory, results, lua, inputRecord);
	}
	return results;
}

void FFunctionDescr::PerformCall_NoReturnValues(UObject* obj, const TArray<FLuaValue>& args) const
{
	FProperty* returnParm = this->ReturnParm;
	UFunction* function = this->Func;
	
	//bool bHasAnyOutput = function->ReturnValueOffset != MAX_uint16 || Func->HasAnyFunctionFlags(EFunctionFlags::FUNC_HasOutParms);
	bool bHasAnyOutput = returnParm != nullptr || !this->OutParms.IsEmpty();

	if (this->Func->NumParms == 0)
	{
		this->PerformCallInternal(obj, function, nullptr);
		//obj->ProcessEvent(function, nullptr);
	}
	else
	{
		void* functionArgsMemory = (uint8*)FMemory_Alloca_Aligned(function->ParmsSize, function->GetMinAlignment());

		FMemory::Memzero( functionArgsMemory, function->ParmsSize );
		//function->InitializeStruct(functionArgsMemory);

		FUFunctionCallInputLuaObjectRecord inputRecord{};
		
		inputRecord.Reserve(this->InputParms.Num());
		
		this->PrepareCallInternal(functionArgsMemory, args, inputRecord);
		
		this->PerformCallInternal(obj, function, functionArgsMemory);
		//obj->ProcessEvent(function, functionArgsMemory);
	}
}

void FFunctionDescr::PrepareCallInternal(void* funcMemory, const TArray<FLuaValue>& args, const FUFunctionCallInputLuaObjectRecord& inputRecord) const
{
	int32 numArgs = args.Num();
	int32 index = 0;
	for (FProperty* prop : this->InputParms)
	{
		if(index >= numArgs)
		{
			break;
		}
		const FLuaValue& arg{args[index]};
		arg.WriteValueToPropertyMemoryAddress_WithPropertyTypeCheck(prop, prop->ContainerPtrToValuePtr<void>(funcMemory));
		index++;
	}
}

namespace UnrealLua::UFunctionCall
{
	void PrepareCallWithTableInternal(const FFunctionDescr* self , void* funcMemory, sol::stack_table argtbl, lua_State* L, FUFunctionCallInputLuaObjectRecord& inputRecord)
	{
		int32 numNumericArgs = argtbl.size(); 
		if(numNumericArgs > 0)
		{
			int32 index = 1;
			for (FProperty* prop : self->InputParms)
			{
				if(index > numNumericArgs)
				{
					break;
				}
				sol::object arg = argtbl[index];
				TSetPropertyValueParams parms{prop, funcMemory, 0, arg, &inputRecord};
				UnrealLua::PropertyHelper::SetPropertyValue_InContainer(parms);
				index++;					
			}
		}
		
		argtbl.for_each([self, funcMemory, &inputRecord](sol::object key_o, sol::object val_o)
		{
			if(key_o.get_type() != sol::type::string)
			{
				return;
			}
			const FName key = UnrealLua::StringCache::GetFNameForStringLuaObject(key_o);
			FProperty* prop = self->Func->FindPropertyByName(key);
			if(prop)
			{
				TSetPropertyValueParams parms{prop, funcMemory, 0, val_o, &inputRecord};
				UnrealLua::PropertyHelper::SetPropertyValue_InContainer(parms);
			}
		});
	}

	void PrepareCallWithArgsInternal(const FFunctionDescr* self , void* funcMemory, sol::variadic_args args, lua_State* L, FUFunctionCallInputLuaObjectRecord& inputRecord)
	{
		int32 numArgs = args.size();
		int32 index = 0;
		for (FProperty* prop : self->InputParms)
		{
			if(index >= numArgs)
			{
				break;
			}
			sol::stack_object arg{args[index]};
			TSetPropertyValueParams parms{prop, funcMemory, 0, arg, &inputRecord};
			UnrealLua::PropertyHelper::SetPropertyValue_InContainer(parms);
			index++;
		}
	}
}

void FFunctionDescr::PrepareCallInternal(void* funcMemory, lua_State* L, FUFunctionCallInputLuaObjectRecord& inputRecord) const
{
	//If there is only one argument given and it's a Lua table
	//self:DoFunc({ Name = "value", key = 123 })
	//self:DoFunc({ "value", 1, 123, true})
	
	int argSlots = lua_gettop(L);

	//see int UnrealLua::SuperCall(lua_State* L) at LuaScriptOverrideCalls.cpp
	//slot 1 is self UObject
	//slot 2 is func name string
	//any further func args are at index 3+
	
	if(argSlots > 2)
	{
		sol::stack_object firstArg{L, 3};
		if(argSlots == 3 && firstArg.valid() && firstArg.get_type() == sol::type::table)
		{
			UnrealLua::UFunctionCall::PrepareCallWithTableInternal(this, funcMemory, firstArg.as<sol::stack_table>(), L, inputRecord);
		}
		else
		{
			sol::variadic_args args{L, 3};
			UnrealLua::UFunctionCall::PrepareCallWithArgsInternal(this, funcMemory, args, L, inputRecord);			
		}
	}
}

void FFunctionDescr::PrepareCallInternal(void* funcMemory, const sol::variadic_args& args, FUFunctionCallInputLuaObjectRecord& inputRecord) const
{
	//If there is only one argument given and it's a Lua table
	//self:DoFunc({ Name = "value", key = 123 })
	//self:DoFunc({ "value", 1, 123, true})

	int32 numArgs = args.size();
	if(numArgs == 1 && args[0].get_type() == sol::type::table)
	{
		UnrealLua::UFunctionCall::PrepareCallWithTableInternal(this, funcMemory, args[0].as<sol::stack_table>(), args.lua_state(), inputRecord);
	}
	else
	{
		UnrealLua::UFunctionCall::PrepareCallWithArgsInternal(this, funcMemory, args, args.lua_state(), inputRecord);
	}	
}

void FFunctionDescr::PrepareCallInternal(void* funcMemory, const std::vector<sol::object>& args, FUFunctionCallInputLuaObjectRecord& inputRecord) const
{
	//If there is only one argument given and it's a Lua table
	//self:DoFunc({ Name = "value", key = 123 })
	//self:DoFunc({ "value", 1, 123, true})
	int32 numArgs = args.size();
	if(numArgs == 1 && args[0].get_type() == sol::type::table)
	{
		//If there is only one Struct input parameter
		//void UObject::Func(FSomeStructType val)
		//use the table to initialize the struct
		if(this->InputParms.Num() == 1 && CastField<FStructProperty>(this->InputParms[0]))
		{
			sol::object arg = args[0];
			TSetPropertyValueParams parms{this->InputParms[0], funcMemory, 0, arg, &inputRecord};
			UnrealLua::PropertyHelper::SetPropertyValue_InContainer(parms);
		}
		//Python-like named parameter initialization
		else
		{
			sol::table argtbl = args[0].as<sol::table>();
			verify(argtbl.valid())
			argtbl.for_each([this, funcMemory, &inputRecord](const sol::object key_o, const sol::object val_o)
			{
				if(key_o.get_type() != sol::type::string)
				{
					return;
				}
				const FName key = UnrealLua::StringCache::GetFNameForStringLuaObject(key_o);
				FProperty* prop = this->Func->FindPropertyByName(key);
				if(prop)
				{
					TSetPropertyValueParams parms{prop, funcMemory, 0, val_o, &inputRecord};
					UnrealLua::PropertyHelper::SetPropertyValue_InContainer(parms);
				}
			});					
		}
	}
	else
	{
		int32 index = 0;
		for (FProperty* prop : this->InputParms)
		{
			sol::object arg{sol::nil};
			if(index < numArgs)
			{
				arg = args[index];
			}
			TSetPropertyValueParams parms{prop, funcMemory, 0, arg, &inputRecord};
			UnrealLua::PropertyHelper::SetPropertyValue_InContainer(parms);
			index++;
		}
	}	
}

void FFunctionDescr::PerformCallInternal(UObject* obj, UFunction* function, void* functionArgsMemory) const
{
	//Ugly hack to support BlueprintNative_Implementation
	
	//If UFunction 
	//- exec func is not the generic ProcessInternal
	//- is not a native function, but still marked as an event
	//- and the Blueprint script is empty
	//- and if the outer class is native
	//then the NativeFunc ptr is some BlueprintNative_Implementation
	if ((function->FunctionFlags & FUNC_Native) == 0 && (function->Script.Num() == 0))
	{
		bool hasSpecializedNativeFunc = function->GetNativeFunc() != UnrealLua::NativeFunctions::UObject_ProcessInternal;/* outerClass->NativeFunctionLookupTable.ContainsByPredicate(
		[funcName = function->GetFName()](const FNativeFunctionLookup& item)
		{
			return item.Name == funcName;
		});*/
		if (hasSpecializedNativeFunc)
		{
			UClass* outerClass = function->GetOuterUClassUnchecked();
			if (function->HasAllFunctionFlags(FUNC_BlueprintEvent) &&  outerClass->HasAllClassFlags(EClassFlags::CLASS_Native))
			{
				FFrame newFrame{obj, function, functionArgsMemory, nullptr};
				void* ResultParm = nullptr;
				if (this->ReturnParm)
				{
					ResultParm = this->ReturnParm->ContainerPtrToValuePtr<void>(functionArgsMemory);
				}
				function->Invoke(obj, newFrame, ResultParm);
			}	
		}
	}
	else
	{
		obj->ProcessEvent(function, functionArgsMemory);
	}
}

int FFunctionDescr::EvaluateReturnValues(void* funcMemory, lua_State* lua, FUFunctionCallInputLuaObjectRecord& inputRecord) const
{
#ifdef SOL_LUAJIT
	int64 numExpectedReturns = UnrealLua::LuaFakeTypes::results_wanted(reinterpret_cast<UnrealLua::LuaFakeTypes::lua_State*>(luaState));
#else
	int64 numExpectedReturns = get_nresults(lua->ci->callstatus); //lua->ci->nresults;
#endif
	//@TODO : Still need to handle out params
	int numPushed = 0;

	//If Lua does not expect any returns and there is no out values to process, just skip returns
	if(numExpectedReturns == 0 && inputRecord.IsEmpty())
	{
		return numPushed;
	}

	int64 returnValuesLeftToProcess = numExpectedReturns;

	//First return value in Lua will always be the return value of the C function
	if (this->ReturnParm != nullptr && numExpectedReturns > 0)
	{
		returnValuesLeftToProcess--;

		//Return value does need to use input record
		FPushPropertyValueParams parms{ this->ReturnParm, funcMemory, 0, lua, nullptr};
		numPushed = UnrealLua::PropertyHelper::GetPropertyValue_InContainer(parms);
	}
	for (FProperty* prop : OutParms)
	{
		if(returnValuesLeftToProcess == 0 && inputRecord.IsEmpty())
		{
			break;
		}
		returnValuesLeftToProcess--;
		//This pushes it into the out array, but does not modify the original parameters
		FPushPropertyValueParams parms{prop, funcMemory, 0, lua, &inputRecord};
		numPushed = UnrealLua::PropertyHelper::GetPropertyValue_InContainer(parms);
	}
	return numPushed;
}

void FFunctionDescr::EvaluateReturnValues(void* funcMemory, sol::variadic_results& results, sol::this_state lua, FUFunctionCallInputLuaObjectRecord& inputRecord) const
{
	lua_State* luaState = lua.lua_state();
#ifdef SOL_LUAJIT
	int64 numExpectedReturns = UnrealLua::LuaFakeTypes::results_wanted(reinterpret_cast<UnrealLua::LuaFakeTypes::lua_State*>(luaState));
#else
	int64 numExpectedReturns = get_nresults(lua.lua_state()->ci->callstatus);
#endif
		
	//If Lua does not expect any returns and there is no out values to process, just skip returns
	if(numExpectedReturns == 0 && inputRecord.IsEmpty())
	{
		return;
	}

	int64 returnValuesLeftToProcess = numExpectedReturns;
	if (this->ReturnParm != nullptr)
	{
		returnValuesLeftToProcess--;
		
		//Return value does need to use input record
		FGetPropertyValueParams parms{ this->ReturnParm, funcMemory, 0, lua, nullptr};
		results.push_back(UnrealLua::PropertyHelper::GetPropertyValue_InContainer(parms));
	}
	for (FProperty* prop : OutParms)
	{
		if(returnValuesLeftToProcess == 0 && inputRecord.IsEmpty())
		{
			break;
		}
		returnValuesLeftToProcess--;
		
		FGetPropertyValueParams parms{prop, funcMemory, 0,  lua, &inputRecord};
		results.push_back(UnrealLua::PropertyHelper::GetPropertyValue_InContainer(parms));
	}
}

FFunctionDescr::FFunctionDescr() 
	: Func(nullptr)
{ 
	//FUNC_LOG("new blank ffunctiondescr created"); 
}

FFunctionDescr::FFunctionDescr(FFunctionDescr && other) noexcept
	: InputParms(MoveTemp(other.InputParms)), OutParms(MoveTemp(other.OutParms)), ReturnParm(other.ReturnParm), Func(other.Func), bIsUnrealLuaCompiledFunction(other.bIsUnrealLuaCompiledFunction)
{
	other.Func = nullptr;
}

FFunctionDescr::FFunctionDescr(UFunction* function)
	: Func(function)
{
	const EFunctionFlags funcFlags = function->FunctionFlags;
	for (TFieldIterator<FProperty> propIt(function); propIt; ++propIt)
	{
		FProperty* property = *propIt;
		const uint64 propflags = property->GetPropertyFlags();
		if (propflags & CPF_Parm)
		{
			//native return parameter
			if (funcFlags & EFunctionFlags::FUNC_Native && propflags & CPF_ReturnParm)
			{
				ReturnParm = property;
			}
			if(UnrealLua::PropertyHelper::IsInputParameter(property))
			{
				InputParms.Emplace(property);
			}
			if(UnrealLua::PropertyHelper::IsOutputParameter(property))
			{
				OutParms.Emplace(property);	
			}
		}
	}
	this->bIsUnrealLuaCompiledFunction = Cast<UUnrealLuaCompiledUFunction>(function);
}

/*
sol::object FFunctionDescr::GetThisPropertyReference(UObject* owner, sol::this_state lua) const
{
	return sol::make_object(lua, this);
}
*/

/*
void FFunctionDescr::SetThisPropertyReference(UObject* owner, const sol::object& value, sol::this_state lua)
{
	//can't modify func	
}
*/
