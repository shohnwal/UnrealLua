#include "LuaCallHelpers/LuaScriptRPCCalls.h"

#include "UnrealLua.h"
#include "Utility/LuaLogMacros.h"
#include "BlueprintSupport/UnrealLuaUtility.h"
#include "LuaTypes/LuaUClass.h"
#include "sol/sol.hpp"

namespace UnrealLua::LuaScriptCall
{
	UObject* GetScriptOwner(sol::object self)
	{
		if (!self.valid())
		{
			LUA_LOG_ERROR("Unable to get UObject as script owner. Please use \":\" instead of \".\" when calling a function or manually pass 'self' as a parameter when using \".\"")
			return nullptr;
		}
		UObject* obj = nullptr;
		if (UnrealLua::IsUObject(self))
		{
			obj = UnrealLua::LightUserdata::GetUObject(self);
		}
		else if(self.is<FLuaUClass>())
		{
			FLuaUClass& clazz = self.as<FLuaUClass>();
			UClass* loadedClass = clazz.TryLoadClass();
			if(loadedClass)
			{
				obj = loadedClass->GetDefaultObject();
			}
		}
		return obj;	
	}

	void RPCCall(sol::object self, sol::object funcName, sol::variadic_args args)
	{
		if(!self.valid() || funcName.get_type() != sol::type::string)
		{
			return;
		}
		UObject* obj = GetScriptOwner(self);
		if(!obj)
		{
			return;
		}
		const std::string& str = funcName.as<std::string&>();
		FString wstr{str.c_str()};
		RPCCallOnObject(obj, wstr, args);
	}

	void RPCCallOnObject(UObject* self, const FString& funcName, sol::variadic_args& args)
	{
		if(!IsValid(self))
		{
			LUA_LOG_WARNING("Unable to call RPCCallOnObject on target %s : object is not valid", *GetFullNameSafe(self))
			return;
		}
		TArray<FLuaValue> argsArray;
		for(int i = 0; i < args.size(); i++)
		{
			sol::object luaobj = args[i];
			argsArray.Emplace(luaobj);
		}
		UUnrealLuaUtility::LuaRPC_Internal(self, funcName, argsArray);		
	}
}
