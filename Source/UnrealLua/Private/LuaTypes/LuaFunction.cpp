// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaValue/LuaFunction.h"

#include "LuaContext/ScopedLuaContext.h"
#include "LuaValue/LuaValue.h"
#include "sol/sol.hpp"

FLuaFunction::FLuaFunction(sol::function func)
	: Func(func)
{
}

void FLuaFunction::Invalidate()
{
	this->Func = sol::nil;
}

bool FLuaFunction::IsValid() const
{
	return this->Func.valid();
}

bool FLuaFunction::CallFunction(const TArray<FLuaValue>& args, TArray<FLuaValue>& outResults) const
{
	outResults = {};
	if (!this->IsValid())
	{
		return false;
	}
	sol::protected_function_result result = this->Func(sol::as_args(args));
	if (!result.valid())
	{
		return false;
	}
	for (sol::stack_object res : result)
	{
		outResults.Emplace(res);
	}
	return true;
}

bool FLuaFunction::CallFunction(const TArray<FLuaValue>& args)
{
	if (!this->IsValid())
	{
		return false;
	}
	sol::protected_function_result result = this->Func(sol::as_args(args));
	if (!result.valid())
	{
		return false;
	}
	return true;
}

FLuaFunctionHandle FLuaFunctionHandle::MakeHandle(sol::function func)
{
	verify(func.valid());
	lua_State* L = func.lua_state();
	return FScopedLuaContext::GetLuaContextFromLuaState(L)->CreateFunctionHandleForLuaFunction(func);	
}

bool FLuaFunctionHandle::operator==(const sol::function& func) const
{
	return this->GetFunction() == func;
}

FLuaFunctionHandle::FLuaFunctionHandle(const TSharedPtr<FLuaFunction>& coSharedPtr)
	: FunctionWrapper(coSharedPtr)
{
}

FLuaFunctionHandle::FLuaFunctionHandle(FLuaFunctionHandle&& other) noexcept: FunctionWrapper(other.FunctionWrapper)
{
	other.FunctionWrapper = nullptr;
}

bool FLuaFunctionHandle::CallFunction(const TArray<FLuaValue>& args, TArray<FLuaValue>& outResults) const
{
	if (!this->IsValid())
	{
		return false;
	}
	return this->FunctionWrapper->CallFunction(args, outResults);
}

bool FLuaFunctionHandle::CallFunction(const TArray<FLuaValue>& args) const
{
	if (!this->IsValid())
	{
		return false;
	}
	return this->FunctionWrapper->CallFunction(args);
}

bool FLuaFunctionHandle::IsValid() const
{
	return this->FunctionWrapper.IsValid() && this->FunctionWrapper->IsValid();
}

sol::protected_function FLuaFunctionHandle::GetFunction() const
{
	if(!this->IsValid())
	{
		return sol::nil;
	}
	return this->FunctionWrapper->Func;
}

sol::object FLuaFunctionHandle::GetFunctionAsObject() const
{
	if(!this->IsValid())
	{
		return sol::nil;
	}
	return sol::make_object(this->FunctionWrapper->Func.lua_state(), this->FunctionWrapper->Func);
}

void FLuaFunctionHandle::Invalidate()
{
	this->FunctionWrapper.Reset();
}

FWeakLuaFunctionHandle::FWeakLuaFunctionHandle(TSharedPtr<FLuaFunction>& coWrapper)
	: LuaFunctionWrapper(coWrapper)
{
	
}

void FWeakLuaFunctionHandle::Invalidate()
{
	if (!this->LuaFunctionWrapper.IsValid())
    {
    	return;
    }
    this->LuaFunctionWrapper.Pin()->Invalidate();
    this->LuaFunctionWrapper.Reset();
}
