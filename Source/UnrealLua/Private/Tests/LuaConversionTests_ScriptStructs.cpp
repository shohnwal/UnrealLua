
#include "Config/UnrealLua_CompilerFlags.h"
#include "Tests/LuaSelfTests.h"
#include "Tests/LuaSelfTestTypes.h"
#include "LuaTypes/LuaScriptStruct.h"
#include "LuaStackHandler/LuaStackHandler.h"

namespace UnrealLua::Test
{
	bool TestScriptStructCopy(sol::state_view& lua)
	{
		static const char* key = "testkey";

		FTestScriptStruct ss{"yay", 123};
		//will create a copy
		lua[key] = ss;

		lua.safe_script("assert(utype(testkey) == 'FTestScriptStruct')");
		lua.safe_script("assert(utype(testkey, true) == 'FTestScriptStruct')");
		lua.safe_script("assert(testkey.x == 123)");
		lua.safe_script("assert(testkey.msg == 'yay')");
		
		if constexpr(UnrealLua::Compilation::WITH_SCRIPTSTRUCT_FUNCTION_LIBS)
		{
			//lua.safe_script("print(utype(testkey.TestFunc))");
			lua.safe_script("assert(utype(testkey.TestFunc) == 'UFunction')");
			lua.safe_script("assert(testkey:TestFunc(testkey) == true)");
			lua.safe_script("assert(testkey.msg == 'meow')");
			//lua.safe_script("print(tostring(testkey.x))");
			lua.safe_script("assert(testkey.x == 999)");
			//revert values back to normal for more tests below
			lua.safe_script("testkey.x = 123");
			lua.safe_script("assert(testkey.x == 123)");
			lua.safe_script("testkey.msg = 'yay'");
			lua.safe_script("assert(testkey.msg == 'yay')");
		}
	
		sol::object checkObj = lua[key];
		verify(checkObj.is<FLuaScriptStruct>());
		verify(checkObj.is<FTestScriptStruct>())

		//lua.safe_script("print(type(testkey))");
		lua.safe_script("assert(type(testkey) == 'userdata')");
		lua.safe_script("assert(testkey.x == 123)");
		lua.safe_script("assert(testkey.msg == 'yay')");
		lua.safe_script("testkey.msg = 'nay'");

		const FLuaScriptStruct& ssRef = checkObj.as<FLuaScriptStruct&>();
		verify(ssRef.OwnsMemory());
		verify(ssRef.GetScriptStruct() == FTestScriptStruct::StaticStruct());
		verify(ssRef.Data != nullptr);

		FTestScriptStruct ret = lua[key];
		verify(ret.x == ss.x);
		verify(ret.msg == "nay");
		verify(ss.msg == "yay");
		verify(ret.msg != ss.msg);
		verify(ret.StaticStruct() == ss.StaticStruct())
		ret.x = 456;
		verify(ret.x != ss.x);

		lua[key] = sol::nil;
		return true;
	}
	
	bool TestScriptStructRef(sol::state_view& lua)
	{
		static const char* key = "testkey";

		FTestScriptStruct ss{"yay", 123};
		lua[key] = &ss;

		lua.safe_script("assert(utype(testkey) == 'FTestScriptStruct')");
		lua.safe_script("assert(utype(testkey, true) == 'FTestScriptStruct')");
	
		sol::object checkObj = lua[key];
		verify(checkObj.is<FLuaScriptStruct>());
		verify(checkObj.is<FTestScriptStruct>())

		lua.safe_script("assert(type(testkey) == 'userdata')");
		lua.safe_script("assert(testkey.x == 123)");
		lua.safe_script("assert(testkey.msg == 'yay')");

		const FLuaScriptStruct& ssRef = checkObj.as<FLuaScriptStruct&>();
		verify(!ssRef.OwnsMemory());
		verify(ssRef.GetScriptStruct() == FTestScriptStruct::StaticStruct());
		verify(ssRef.Data != nullptr);

		FTestScriptStruct ret = lua[key];
		verify(ret.x == ss.x);
		verify(ret.msg == ss.msg);
		verify(ret.StaticStruct() == ss.StaticStruct())
		ret.x = 456;
		verify(ret.x != ss.x);

		FTestScriptStruct* retptr = lua[key];
		verify(retptr->x == ss.x);
		verify(retptr->msg == ss.msg);
		verify(retptr->StaticStruct() == ss.StaticStruct())
		retptr->x = 789;
		verify(retptr->x == ss.x);
		verify(retptr == &ss);
		//verify(retptr->x == retref.x);

		lua[key] = sol::nil;
	
		return true;
	
	}
}
bool FLuaConversionTests::TestScriptStructs(sol::state_view& lua)
{
	LUA_LOG("Testing Lua ScriptStruct")
	
	bool success = UnrealLua::Test::TestScriptStructCopy(lua);
	lua.collect_garbage();
	success &= UnrealLua::Test::TestScriptStructRef(lua);
	lua.collect_garbage();
	return success; 
}