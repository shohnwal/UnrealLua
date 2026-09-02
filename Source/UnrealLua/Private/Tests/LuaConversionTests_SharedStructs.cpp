
#include "Tests/LuaSelfTests.h"
#include "Tests/LuaSelfTestTypes.h"
#include "LuaTypes/LuaSharedStruct.h"
#include "LuaStackHandler/LuaStackHandler.h"

namespace UnrealLua::SelfTests
{
	static const char* key = "testkey";
	
	bool SharedStructTest_CopyToLua(sol::state_view& lua)
	{
		FSharedStruct original_is = FSharedStruct::Make<FTestScriptStruct>({"yay", 123});
		FTestScriptStruct& original_content = original_is.Get<FTestScriptStruct>();

		//Copy to Lua
		lua[key] = original_is;

		lua.safe_script("assert(testkey ~= nil)");
		lua.safe_script("assert(testkey:IsValid())");

		lua.safe_script("assert(utype(testkey) == 'TSharedStruct')");
		lua.safe_script("assert(utype(testkey, true) == 'TSharedStruct<FTestScriptStruct>')");
	
		sol::object checkObj = lua[key];
		verify(checkObj.is<FLuaSharedStruct>());
		verify(checkObj.is<FSharedStruct>());

		//change original content
		original_content.x = 456;
		original_content.msg = "barf!";

		//the copy should have same values
		lua.safe_script("assert(testkey.x == 456)");
		lua.safe_script("assert(testkey.msg == 'barf!')");

		int32 x = lua[key]["x"]; 

		verify(original_content.x == 456);
		verify(x == 456);

		//test copying back into C++
		FSharedStruct ret_is = lua[key];

		//memory should be valid
		verify(ret_is.IsValid());
		
		//metastruct shoujld be the same
		verify(ret_is.GetScriptStruct() == original_is.GetScriptStruct());

		//memory should be same (shared memory)
		verify(ret_is.GetMemory() == original_is.GetMemory());

		//verify that we actually copied out from lua
		const FLuaSharedStruct& luaRef = lua[key].get<FLuaSharedStruct&>();
		//metastruct shoujld be the same
		verify(luaRef.GetScriptStruct() != nullptr);
		verify(luaRef.GetScriptStruct() == ret_is.GetScriptStruct());
		//memory should be valid
		verify(luaRef.GetMemory() != nullptr);
		//memory should be same
		verify(luaRef.GetMemory() == ret_is.GetMemory());
		
		lua[key] = sol::nil;
		lua.collect_garbage();
		return true;
	}
	
	bool SharedStructTest_ReferenceToLua(sol::state_view& lua)
	{
		FSharedStruct original_is = FSharedStruct::Make<FTestScriptStruct>({"yay", 123});
		FTestScriptStruct& original_content = original_is.Get<FTestScriptStruct>();

		//Reference to Lua
		lua[key] = &original_is;
		
		lua.safe_script("assert(utype(testkey) == 'TSharedStruct')");
		lua.safe_script("assert(utype(testkey, true) == 'TSharedStruct<FTestScriptStruct>')");

		lua.safe_script("assert(testkey ~= nil)");
		lua.safe_script("assert(testkey:IsValid())");
	
		sol::object checkObj = lua[key];
		verify(checkObj.is<FLuaSharedStruct>());
		verify(checkObj.is<FSharedStruct>());

		//change original content
		original_content.x = 456;
		original_content.msg = "barf!";

		//the ref should have same values
		//lua.safe_script("print(testkey.x)");
		lua.safe_script("assert(testkey.x == 456)");
		lua.safe_script("assert(testkey.msg == 'barf!')");
		//lua.safe_script("print(type(testkey.msg))");

		int32 x = lua[key]["x"]; 

		verify(original_content.x == x);
		verify(x == 456);

		//test referencing back into C++
		FLuaSharedStruct* ret_is = lua[key].get<FLuaSharedStruct*>();

		verify(ret_is != nullptr);

		//metastruct shoujld be the same
		verify(ret_is->GetScriptStruct() == original_is.GetScriptStruct());
		//memory should be same
		verify(ret_is->GetMemory() == original_is.GetMemory());
		
		lua[key] = sol::nil;
		lua.collect_garbage();
		return true;
	}
	
	bool SharedStructTest_CreateInLua(sol::state_view& lua)
	{
		lua.safe_script("testkey = TSharedStruct(FTestScriptStruct)");
		lua.safe_script("assert(testkey ~= nil)");
		//lua.safe_script("print(utype(testkey, true))");
		lua.safe_script("assert(testkey:IsValid())");
		//lua.safe_script("assert(not testkey:IsReference())");
		lua.safe_script("assert(testkey.x ~= nil)");
		lua.safe_script("assert(testkey.msg ~= nil)");
		
		lua.safe_script("assert(utype(testkey) == 'TSharedStruct')");
		lua.safe_script("assert(utype(testkey, true) == 'TSharedStruct<FTestScriptStruct>')");

		sol::object checkObj = lua[key];
		verify(checkObj.is<FLuaSharedStruct>());
		verify(checkObj.is<FSharedStruct>());

		//->

		FSharedStruct* ssPtr = checkObj.as<FSharedStruct*>();
		verify(ssPtr != nullptr);
		verify(ssPtr->GetScriptStruct() == FTestScriptStruct::StaticStruct());
		FTestScriptStruct& ssPtrData = ssPtr->Get<FTestScriptStruct>();

		FSharedStruct ssCopy = checkObj.as<FSharedStruct>();
		verify(ssCopy.GetScriptStruct() == FTestScriptStruct::StaticStruct());
		verify(ssCopy.GetScriptStruct() == ssPtr->GetScriptStruct());
		verify(ssCopy.GetMemory() == ssPtr->GetMemory());
		FTestScriptStruct& ssCopyData = ssPtr->Get<FTestScriptStruct>();
		
		lua.safe_script("testkey.msg = 'meow'");
		lua.safe_script("assert(testkey.msg == 'meow')");
		verify(ssPtrData.msg == "meow");
		verify(ssCopyData.msg == "meow");
		lua.safe_script("testkey.x = 987");
		lua.safe_script("assert(testkey.x == 987)");
		verify(ssPtrData.x == 987);
		verify(ssCopyData.x == 987);
		
		ssPtrData.x = 741;
		lua.safe_script("assert(testkey.x == 741)");
		verify(ssCopyData.x == 741);
		
		lua["testkey"] = sol::nil;
		lua.collect_garbage();
		return true;
	}

	bool SharedStructTest_CreateAndInitializeInLua(sol::state_view& lua)
	{
		lua.safe_script("testkey = TSharedStruct()");
		lua.safe_script("assert(testkey ~= nil)");
		lua.safe_script("assert(not testkey:IsValid())");
		lua.safe_script("assert(testkey.x == nil)");
		lua.safe_script("assert(testkey.msg == nil)");

		lua.safe_script("assert(utype(testkey) == 'TSharedStruct')");
		lua.safe_script("assert(utype(testkey, true) == 'TSharedStruct<__Garbage__>')");
		
		lua.safe_script("testkey:InitializeAs(FTestScriptStruct)");
		lua.safe_script("assert(testkey:IsValid())");
		lua.safe_script("assert(testkey.x ~= nil)");
		lua.safe_script("assert(testkey.msg ~= nil)");

		lua.safe_script("assert(utype(testkey) == 'TSharedStruct')");
		lua.safe_script("assert(utype(testkey, true) == 'TSharedStruct<FTestScriptStruct>')");

		sol::object checkObj = lua[key];
		verify(checkObj.is<FLuaSharedStruct>());
		verify(checkObj.is<FSharedStruct>());

		FSharedStruct ssCopy = checkObj.as<FSharedStruct>();
		verify(ssCopy.GetScriptStruct() == FTestScriptStruct::StaticStruct());
		FTestScriptStruct& ssCopyData = ssCopy.Get<FTestScriptStruct>();
		
		lua.safe_script("testkey.msg = 'meow'");
		lua.safe_script("assert(testkey.msg == 'meow')");
		verify(ssCopyData.msg == "meow");
		lua.safe_script("testkey.x = 987");
		lua.safe_script("assert(testkey.x == 987)");
		verify(ssCopyData.x == 987);

		lua["testkey"] = sol::nil;

		return true;
	}
	
	bool SharedStructTest_AsProperty(sol::state_view& lua)
	{
		return true;
	}
	
	bool SharedStructTest_AsScriptValue(sol::state_view& lua)
	{
		return true;
	}
}
bool FLuaConversionTests::TestSharedStructs(sol::state_view& lua)
{
	LUA_LOG("Testing Lua SharedStruct")
	
	UnrealLua::SelfTests::SharedStructTest_CopyToLua(lua);
	UnrealLua::SelfTests::SharedStructTest_ReferenceToLua(lua);
	UnrealLua::SelfTests::SharedStructTest_CreateInLua(lua);
	UnrealLua::SelfTests::SharedStructTest_CreateAndInitializeInLua(lua);
	UnrealLua::SelfTests::SharedStructTest_AsProperty(lua);
	UnrealLua::SelfTests::SharedStructTest_AsScriptValue(lua);
	return true;
}

