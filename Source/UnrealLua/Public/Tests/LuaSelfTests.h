#pragma once
#include "sol/sol.hpp"
#include "LuaCoreDelegates.h"
#include "UObject/ScriptInterface.h"

class ILuaContext;

struct UNREALLUA_API FUnrealLuaTest
{
	FUnrealLuaTest(TFunction<bool(sol::state_view&)> func);
	
	TFunction<bool(sol::state_view&)> func = nullptr;
};

namespace UnrealLua::SelfTest
{
	sol::protected_function_result UNREALLUA_API NotifySelfTestError(lua_State* L, sol::protected_function_result pfr);
	
	struct UNREALLUA_API FLuaKeyScopeGuard
	{
		FLuaKeyScopeGuard(sol::state_view& luav, const char* keyc) 
		: lua(luav), key(keyc)
		{
			
		}
		~FLuaKeyScopeGuard()
		{
			lua[key] = sol::nil;
		}
		sol::state_view& lua;
		const char* key = nullptr;
		TArray<const char*> Keys = {};
	};
	extern FLuaStateViewBroadcastDelegate SelfTestDelegate;
	
	FDelegateHandle AddTestCategoryCallback(TFunction<void(sol::state_view&)> callback);
}

#define UNREALLUA_LUA_SCOPE_GUARD(key) \
	SelfTest::FLuaKeyScopeGuard guard##key##{lua, #key};


namespace UnrealLua::SelfTest
{
	bool UNREALLUA_API PerformSelfTest();
	bool UNREALLUA_API TEST(sol::state_view& lua, const char* content);
}

#define UNREALLUA_TEST_STEP(content) \
		if(!UnrealLua::SelfTest::TEST(lua, content)) \
		{ \
			return false;\
		}


struct UNREALLUA_API FLuaConversionTests
{
	static bool Test(TScriptInterface<ILuaContext>& ctx);
private:
	static bool TestUEnums(sol::state_view& lua);
	static bool TestUObjects(sol::state_view& lua);
	static bool TestScriptStructs(sol::state_view& lua);
	static bool TestStrings(sol::state_view& lua);
	static bool TestNames(sol::state_view& lua);
	static bool TestText(sol::state_view& lua);
	static bool TestArrays(sol::state_view& lua);
	static bool TestSets(sol::state_view& lua);
	static bool TestInstancedStructs(sol::state_view& lua);
	static bool TestSharedStructs(sol::state_view& lua);
	static bool TestMaps(sol::state_view& lua);
	static bool TestDelegates(const TScriptInterface<ILuaContext>& ctx);
	
};