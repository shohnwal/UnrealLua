
#include "Replication/LuaRPCFunctions.h"

#include "LuaCoreDelegates.h"
#include "Utility/LuaLogMacros.h"
#include "LuaCallHelpers/LuaScriptRPCCalls.h"
#include "UnrealLua.h"

static const FDelegateHandle fLuaRpcFuncLuaTypeHandle = FLuaCoreDelegates::OnRegisterLuaUsertypes.AddStatic(&FLuaRPCFunction::RegisterUsertype);

void FLuaRPCFunction::RegisterUsertype(sol::state_view& lua)
{
	lua.new_usertype<FLuaRPCFunction>(
		"LuaRPCFunction",
		"new", sol::no_constructor
	);
}

FLuaRPCFunction::FLuaRPCFunction()
	: LuaFunc(), FuncName()
{
	
}

FLuaRPCFunction::FLuaRPCFunction(sol::function func, const sol::string_view& funcName)
	: LuaFunc(func), FuncName(MakeUnique<FString>(funcName.data()))
{
}

FLuaRPCFunction::FLuaRPCFunction(FLuaRPCFunction&& other)
	: LuaFunc(MoveTemp(other.LuaFunc)), FuncName(MoveTemp(other.FuncName))
{
}

FLuaRPCFunction::FLuaRPCFunction(const FLuaRPCFunction& other)
	: LuaFunc(other.LuaFunc), FuncName(MakeUnique<FString>(*other.FuncName.Get()))
{
}

void FLuaRPCFunction::operator()(sol::stack_object self, sol::variadic_args args)
{
	if(!this->LuaFunc.valid() || !this->FuncName.IsValid() || this->FuncName.Get()->IsEmpty())
	{
		return;
	}
	if (!self.valid())
	{
		LUA_LOG_ERROR("Unable to call reflected function : self is not valid. Please use \":\" instead of \".\" when calling a function or manually pass 'self' as a parameter when using \".\"")
		return;
	}

	UObject* obj = UnrealLua::LightUserdata::GetUObject(self);
	if (!obj)
	{
		LUA_LOG_ERROR("Unable to call reflected function : Could not find valid UObject, neither as self nor as self[true]. Please use \".\" instead of \":\" when calling a function or manually pass 'self' as a parameter")
		return;
	}
	UnrealLua::LuaScriptCall::RPCCallOnObject(obj, *this->FuncName.Get(), args);
}

sol::object FLuaRPCFunction::GetValue(sol::this_state lua) const
{
	return sol::object(lua, sol::in_place_type<FLuaRPCFunction>,*this);
}
