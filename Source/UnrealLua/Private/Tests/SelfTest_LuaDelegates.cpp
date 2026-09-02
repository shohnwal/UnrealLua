#include "sol/sol.hpp"
#include "UnrealLua.h"
#include "Tests/LuaSelfTests.h"
#include "Tests/LuaSelfTestTypes.h"

namespace UnrealLua::SelfTest
{
	bool TestSingleDelegateWithFunction(sol::state_view& lua)
	{
		UNREALLUA_TEST_STEP("del = Delegate()"
			"assert(del ~= nil)"
			"assert(not del:IsBound())"
		);
		
		UNREALLUA_TEST_STEP("del:Execute(1, 2, 3)");
		
		UNREALLUA_TEST_STEP(R"###(func = function(a, b, c)
			ret1 = a
			ret2 = b
			ret3 = c
			res = a + b + c
			end)###");
		
		UNREALLUA_TEST_STEP("del:Add(func)");
		
		UNREALLUA_TEST_STEP("assert(del:IsBound())");
		
		UNREALLUA_TEST_STEP("del:Execute(1, 2, 3)");
		
		UNREALLUA_TEST_STEP("assert(ret1 == 1)");
		UNREALLUA_TEST_STEP("assert(ret2 == 2)");
		UNREALLUA_TEST_STEP("assert(ret3 == 3)");
		UNREALLUA_TEST_STEP("assert(res == 6)");
		UNREALLUA_TEST_STEP("del = nil; ret1 = nil; ret2 = nil; ret3 = nil; res = nil; func = nil");
		
		return true;
	}
	
	bool TestSingleDelegateWithTable(sol::state_view& lua)
	{
		UNREALLUA_TEST_STEP("del = Delegate()"
			"assert(del ~= nil)"
			"assert(not del:IsBound())"
		);
		
		UNREALLUA_TEST_STEP(R"###(tbl = {})###");
		
		UNREALLUA_TEST_STEP(R"###(local func = function(self, a, b)
			print(tostring(a) .. tostring(b))
			self.str = tostring(a) .. tostring(b)
			self.ret1 = a
			self.ret2 = b
			self.result = a + b
			print(tostring(self.str) .. tostring(self.ret1) .. tostring(self.ret2) .. tostring(self.result))
			end;
			tbl.Callback = func
		)###");
		
		UNREALLUA_TEST_STEP("assert(tbl ~= nil)");
		UNREALLUA_TEST_STEP("assert(tbl.Callback ~= nil)");
		UNREALLUA_TEST_STEP("assert(type(tbl.Callback) == 'function', type(tbl.Callback))");
		
		UNREALLUA_TEST_STEP("del:Add(tbl, 'Callback')");
		
		UNREALLUA_TEST_STEP("assert(del:IsBound())");
		
		UNREALLUA_TEST_STEP("del:Execute(123, 456)");
		
		UNREALLUA_TEST_STEP("assert(tbl.str == '123456')");
		UNREALLUA_TEST_STEP("assert(tbl.ret1 == 123)");
		UNREALLUA_TEST_STEP("assert(tbl.ret2 == 456)");
		UNREALLUA_TEST_STEP("assert(tbl.result == 579)");
		
		UNREALLUA_TEST_STEP("del = nil; tbl = nil");
		
		return true;
	}
	
	bool TestSingleDelegateWithUObject(sol::state_view& lua)
	{
		UNREALLUA_TEST_STEP("del = Delegate()"
			"assert(del ~= nil)"
			"assert(not del:IsBound())"
		);
				
		UUnrealLuaTestObject* testObj = NewObject<UUnrealLuaTestObject>();
		
		lua["obj"] = testObj;
		
		testObj->Bool =  false;
		testObj->Int32 = -1;
		testObj->Float = -1.1f;
		testObj->Object = nullptr;
		testObj->String = "nay!";
		testObj->Name = "dog";
		
		
		UNREALLUA_TEST_STEP("assert(obj.Bool == false)");
		UNREALLUA_TEST_STEP("assert(obj.Int32 == -1)");
		UNREALLUA_TEST_STEP("assert(obj.Object == nil)");
		UNREALLUA_TEST_STEP("assert(obj.String == 'nay!')");
		UNREALLUA_TEST_STEP("assert(obj.Name == 'dog')");

		UNREALLUA_TEST_STEP("assert(obj ~= nil)");
		
		UNREALLUA_TEST_STEP("del:Add(obj, 'TestDelCallbackArgs')");
		
		UNREALLUA_TEST_STEP("assert(del:IsBound())");
		
		UNREALLUA_TEST_STEP("del:Execute(123, 1.23, true, 'meow', 'cat', obj)");
		
		UNREALLUA_TEST_STEP("assert(obj.Bool == true)");
		UNREALLUA_TEST_STEP("assert(obj.Int32 == 123)");
		UNREALLUA_TEST_STEP("assert(obj.Object == obj)");
		UNREALLUA_TEST_STEP("assert(obj.String == 'meow')");
		UNREALLUA_TEST_STEP("assert(obj.Name == 'cat')");
		
		UNREALLUA_TEST_STEP("del = nil; obj = nil");
		
		testObj->ConditionalBeginDestroy();
		
		return true;
	}
	
	bool TestLuaDelegates(sol::state_view& lua)
	{
		TestSingleDelegateWithFunction(lua);
		TestSingleDelegateWithTable(lua);
		TestSingleDelegateWithUObject(lua);
		return true;
	}
}
