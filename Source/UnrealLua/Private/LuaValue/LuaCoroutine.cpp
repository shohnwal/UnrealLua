// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaValue/LuaCoroutine.h"
#include "LuaValue/LuaValue.h"
#include "UnrealLua.h"

FLuaCoroutine::FLuaCoroutine(sol::thread& t, sol::function& func)
	: Thread(t), Coroutine(func), Func(func)
{
}

void FLuaCoroutine::Invalidate()
{
	this->Coroutine = sol::nil;
	this->Func = sol::nil;
	this->Thread.reset();
}

bool FLuaCoroutine::IsValid() const
{
	return this->Coroutine.valid();
}

ELuaCoroutineCallStatus FLuaCoroutine::GetCoroutineStatus() const
{
	if(!this->IsValid())
	{
		return ELuaCoroutineCallStatus::Invalid;
	}
	return TranslateLuaCallStatusToUnrealCallStatus(this->Coroutine.status());
}

ELuaCoroutineCallStatus FLuaCoroutine::CallCoroutine(const TArray<FLuaValue>& args, TArray<FLuaValue>& outResults)
{
	outResults = {};
	if (!this->IsValid())
	{
		return ELuaCoroutineCallStatus::Invalid;
	}
	if(!this->Coroutine.runnable())
	{
		return TranslateLuaCallStatusToUnrealCallStatus(this->Coroutine.status());
	}
	sol::protected_function_result result = this->Coroutine(sol::as_args(args));
	if (!result.valid())
	{
		return ELuaCoroutineCallStatus::Invalid;
	}
	for (sol::stack_object res : result)
	{
		outResults.Emplace(res);
	}
	return TranslateLuaCallStatusToUnrealCallStatus(this->Coroutine.status());
}

FLuaCoroutineCallResult FLuaCoroutine::CallCoroutine(const TArray<FLuaValue>& args)
{
	if (!this->IsValid())
	{
		return {ELuaCoroutineCallStatus::Invalid};
	}
	if(!this->Coroutine.runnable())
	{
		return {TranslateLuaCallStatusToUnrealCallStatus(this->Coroutine.status())};
	}
	sol::protected_function_result result = this->Coroutine(sol::as_args(args));
	if (!result.valid())
	{
		return {ELuaCoroutineCallStatus::Invalid};
	}
	return {TranslateLuaCallStatusToUnrealCallStatus(this->Coroutine.status()), MoveTemp(result)};
}


ELuaCoroutineCallStatus FLuaCoroutine::TranslateLuaCallStatusToUnrealCallStatus(sol::call_status status)
{
	switch(status)
	{
	case sol::call_status::ok:
		return ELuaCoroutineCallStatus::Finished;
	case sol::call_status::yielded:
		return ELuaCoroutineCallStatus::Yielded;
	default:
		return ELuaCoroutineCallStatus::Error;
	}
}

FLuaCoroutineHandle::FLuaCoroutineHandle(const TSharedPtr<FLuaCoroutine>& coSharedPtr)
	: CoroutineWrapper(coSharedPtr)
{
}

ELuaCoroutineCallStatus FLuaCoroutineHandle::CallCoroutine(const TArray<FLuaValue>& args, TArray<FLuaValue>& outResults) const
{
	if (!this->IsValid())
	{
		return ELuaCoroutineCallStatus::Invalid;
	}
	return this->CoroutineWrapper->CallCoroutine(args, outResults);
}

FLuaCoroutineCallResult FLuaCoroutineHandle::CallCoroutine(const TArray<FLuaValue>& args) const
{
	if (!this->IsValid())
    {
    	return {ELuaCoroutineCallStatus::Invalid};
    }
    return this->CoroutineWrapper->CallCoroutine(args);
}

ELuaCoroutineCallStatus FLuaCoroutineHandle::GetCoroutineStatus() const
{
	if(!this->IsValid())
	{
		return ELuaCoroutineCallStatus::Invalid;
	}
	return this->CoroutineWrapper->GetCoroutineStatus();
}

void FLuaCoroutineHandle::Invalidate()
{
	this->CoroutineWrapper.Reset();
}

bool FLuaCoroutineHandle::IsValid() const
{
	return this->CoroutineWrapper.IsValid() && this->CoroutineWrapper->IsValid();
}

sol::coroutine FLuaCoroutineHandle::GetCoroutine() const
{
	if(!this->IsValid())
	{
		return sol::nil;
	}
	return this->CoroutineWrapper->Coroutine;
}

FWeakLuaCoroutineHandle::FWeakLuaCoroutineHandle(TSharedPtr<FLuaCoroutine>& coWrapper)
	: LuaCoroutineWrapper(coWrapper)
{
}

void FWeakLuaCoroutineHandle::Invalidate()
{
	if (!this->LuaCoroutineWrapper.IsValid())
	{
		return;
	}
	this->LuaCoroutineWrapper.Pin()->Invalidate();
	this->LuaCoroutineWrapper.Reset();
}
