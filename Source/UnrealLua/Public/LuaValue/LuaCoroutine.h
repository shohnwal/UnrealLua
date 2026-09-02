
#pragma once
#include "CoreMinimal.h"
#include "sol/sol.hpp"
#include "LuaCoroutine.generated.h"
struct FWeakLuaTableHandle;
struct FScopedLuaContext;
class ULuaScriptInstance;
class ULuaContext;
struct FLuaValue;
/**
 * 
 */
struct FLuaScriptInstance;

UENUM(BlueprintType)
enum class ELuaCoroutineCallStatus : uint8
{
	Finished,
	Yielded,
	Error,
	Invalid
/*
	ok = LUA_OK,
	yielded = LUA_YIELD,
	runtime = LUA_ERRRUN,
	memory = LUA_ERRMEM,
	handler = LUA_ERRERR,
	gc = LUA_ERRGCMM,
	syntax = LUA_ERRSYNTAX,
	file = LUA_ERRFILE,
	invalid = UINT8_MAX
*/
};

struct UNREALLUA_API FLuaCoroutineCallResult
{
	ELuaCoroutineCallStatus CoroutineCallStatus = ELuaCoroutineCallStatus::Invalid;
	sol::protected_function_result result = {};	
};

struct UNREALLUA_API FLuaCoroutine
{
	FLuaCoroutine(sol::thread& t, sol::function& func);
	void Invalidate();
	bool IsValid() const;
	ELuaCoroutineCallStatus GetCoroutineStatus() const;
	ELuaCoroutineCallStatus CallCoroutine(const TArray<FLuaValue>& args, TArray<FLuaValue>& outResults);
	FLuaCoroutineCallResult CallCoroutine(const TArray<FLuaValue>& Args);
	
	sol::thread Thread = {};
	sol::coroutine Coroutine = sol::nil;
	sol::function Func = sol::nil;
private:
	static ELuaCoroutineCallStatus TranslateLuaCallStatusToUnrealCallStatus(sol::call_status status);
};


//A class holding a Lua table and a reference to the Lua state
USTRUCT(BlueprintType)
struct UNREALLUA_API FLuaCoroutineHandle
{
	GENERATED_BODY()
	FLuaCoroutineHandle()
	{
	}
	
	explicit FLuaCoroutineHandle(const TSharedPtr<FLuaCoroutine>& coSharedPtr);

	explicit FLuaCoroutineHandle(FLuaCoroutineHandle&& other) noexcept
		: CoroutineWrapper(other.CoroutineWrapper)
	{
		other.CoroutineWrapper = nullptr;
	}


	FLuaCoroutineHandle& operator=(const FLuaCoroutineHandle& other)
	{
		this->CoroutineWrapper = other.CoroutineWrapper;
		return *this;
	}

	ELuaCoroutineCallStatus CallCoroutine(const TArray<FLuaValue>& args, TArray<FLuaValue>& outResults) const;
	FLuaCoroutineCallResult CallCoroutine(const TArray<FLuaValue>& Args) const;
	ELuaCoroutineCallStatus GetCoroutineStatus() const;
	void Invalidate();

	FLuaCoroutineHandle(const FLuaCoroutineHandle& other)
		: CoroutineWrapper(other.CoroutineWrapper)
	{}

	bool IsValid() const;

	sol::coroutine GetCoroutine() const;
	TSharedPtr<FLuaCoroutine> CoroutineWrapper = nullptr;
};


struct UNREALLUA_API FWeakLuaCoroutineHandle
{
	FWeakLuaCoroutineHandle(TSharedPtr<FLuaCoroutine>& coWrapper);
	void Invalidate();
	
	TWeakPtr<FLuaCoroutine> LuaCoroutineWrapper{};
};
