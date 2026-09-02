
#include "LuaContext/ScopedLuaContext.h"
#include "LuaValue/LuaFunction.h"
#include "Utility/LuaLogMacros.h"

FLuaTableHandle FScopedLuaContext::CreateNewLuaTable()
{
	if (!this->IsLuaLoaded())
	{
		return {};
	}
	sol::table newTable = this->LuaState.create_table();
	return this->CreateLuaTableHandleForTable(newTable);
}

FLuaTableHandle FScopedLuaContext::CreateLuaTableHandleForTable(const sol::table& table)
{
	if (!table)
	{
		return {};
	}
	TSharedPtr<FLuaTable> tableSharedPtr = MakeShared<FLuaTable>(table);
	FWeakLuaTableHandle& newTableHandle = this->ExternalLuaTables.Emplace_GetRef(tableSharedPtr);
	return FLuaTableHandle{tableSharedPtr};
}

FLuaFunctionHandle FScopedLuaContext::CreateNewFunctionFromString(const FString& funcString)
{
	if (!this->IsLuaLoaded())
    {
    	return {};
    }
    
    FString funcStr = funcString;
    if (!funcString.StartsWith("return "))
    {
    	funcStr = "return " + funcString;
    }
    if (!funcStr.StartsWith("return function(") || !funcStr.EndsWith("end"))
    {
    	return {};
    }
    auto casted = StringCast<char>(*funcStr);
    std::string_view strv = casted.Get();
    
    sol::protected_function_result result = this->LuaState.safe_script(strv,sol::detail::default_chunk_name(), sol::load_mode::text);
    if (!result.valid())
    {
    	sol::error err = result;
    	sol::string_view errStrv = err.what();
    	LUA_LOG_ERROR("Can't create lua function : %hs", errStrv.data())
    	return {};
    }
	if (result.return_count() < 1)
	{
		LUA_LOG_ERROR("Can't create lua function from string %s :\nScript did not return anything",*funcStr)
		return {};		
	}
	sol::object obj = result.get<sol::object>();
	if (obj.get_type() != sol::type::function)
	{
		LUA_LOG_ERROR("Can't create lua function from string %s :\nScript did not return a Lua function",*funcStr)
		return {};
	}
    sol::function func = obj.as<sol::function>();
    return this->CreateFunctionHandleForLuaFunction(func);
}

FLuaFunctionHandle FScopedLuaContext::CreateFunctionHandleForLuaFunction(const sol::function& func)
{
	if (!func.valid())
	{
		return {};
	}
	sol::protected_function pfunc{func};
	TSharedPtr<FLuaFunction> funcSharedPtr = MakeShared<FLuaFunction>(pfunc);
	FWeakLuaFunctionHandle& newHandle = this->ExternalLuaFunctions.Emplace_GetRef(funcSharedPtr);
	return FLuaFunctionHandle{funcSharedPtr};
}

FLuaCoroutineHandle FScopedLuaContext::CreateNewCoroutineFromString(const FString& funcString)
{
	if (!this->IsLuaLoaded())
	{
		LUA_LOG_ERROR("Can't create coroutine from string %s :\nLua is not loaded",*funcString)
		return {};
	}
	
	FString funcStr = funcString;
	if (!funcString.StartsWith("return "))
	{
		funcStr = "return " + funcString;
	}
	if (!funcStr.StartsWith("return function(") || !funcStr.EndsWith("end"))
	{
		LUA_LOG_ERROR("coroutine template does either not begin with 'return function(' or not end with 'end' : \n%s", *funcStr);
		return {};
	}
	auto casted = StringCast<char>(*funcStr);
	std::string_view strv = casted.Get();
	
	sol::thread newThread = sol::thread::create(this->LuaState.lua_state());
	sol::state_view lua{newThread.state()};
	sol::protected_function_result result = lua.safe_script(strv,sol::detail::default_chunk_name(), sol::load_mode::text);
	if (!result.valid())
	{
		sol::error err = result;
		sol::string_view errStrv = err.what();
		LUA_LOG_ERROR("Can't create coroutine from string %s :\n%hs",*funcStr, errStrv.data())
		return {};
	}
	if (result.return_count() < 1)
	{
		LUA_LOG_ERROR("Can't create coroutine from string %s :\nScript did not return anything",*funcStr)
		return {};		
	}
	sol::object obj = result.get<sol::object>();
	if (obj.get_type() != sol::type::function)
	{
		LUA_LOG_ERROR("Can't create coroutine from string %s :\nScript did not return a Lua function",*funcStr)
		return {};
	}
	sol::function func = result.get<sol::function>();
	return this->CreateCoroutineHandleForLuaFunction(newThread, func);
}

FLuaCoroutineHandle FScopedLuaContext::CreateCoroutineHandleForLuaFunction(sol::function& func)
{
	sol::thread newthread = sol::thread::create(func.lua_state());
	return CreateCoroutineHandleForLuaFunction(newthread, func);
}

FLuaCoroutineHandle FScopedLuaContext::CreateCoroutineHandleForLuaFunction(sol::thread& t, sol::function& func)
{
	if (!func.valid())
	{
		return {};
	}
	
	TSharedPtr<FLuaCoroutine> coSharedPtr = MakeShared<FLuaCoroutine>(t, func);
	FWeakLuaCoroutineHandle& newCoHandle = this->ExternalLuaCoroutines.Emplace_GetRef(coSharedPtr);
	verify(newCoHandle.LuaCoroutineWrapper.Pin()->GetCoroutineStatus() == ELuaCoroutineCallStatus::Yielded);
	return FLuaCoroutineHandle{coSharedPtr};
}

sol::table& FScopedLuaContext::GetRegistryTable()
{
	return this->RegistryTable;
}

const sol::table& FScopedLuaContext::GetRegistryTable() const
{
	return this->RegistryTable;
}

ULuaStateInputHandler* FScopedLuaContext::GetInputHandler() const
{
	return this->PlayerInputHandler;
}
