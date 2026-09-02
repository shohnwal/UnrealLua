
#include "Tests/LuaSelfTests.h"
#include "Tests/LuaSelfTestTypes.h"
#include "LuaTypes/LuaSet.h"
#include "LuaStackHandler/LuaStackHandler.h"

static bool TestCopy(sol::state_view& lua, const char* key)
{
	TSet<int32> originalSet{1,2,3};
	lua[key] = originalSet;

	lua.safe_script("assert(utype(testkey) == 'TSet')");
	lua.safe_script("assert(utype(testkey, true) == 'TSet<int32>')");
	
	sol::object checkObj = lua[key];
	verify(checkObj.is<FLuaSet>());
	verify(checkObj.is<TSet<int32>>());

	lua.safe_script("testkey:Add(1)");
	lua.safe_script("testkey:Add(3)");
	lua.safe_script("testkey:Add(4)");
	lua.safe_script("testkey:Add(2)");
	
	TSet<int32> retSet = lua[key];
	verify(retSet.Difference(originalSet).Num() == 1);
	verify(retSet.Num() == 4);
	
	lua.safe_script("testkey:Remove(1)");

	TSet<int32> retSet2 = lua[key];
	verify(retSet2.Difference(retSet).Num() == 0);
	verify(retSet.Difference(retSet2).Num() == 1);

	lua[key] = sol::nil;

	return true;
}

static bool TestPtr(sol::state_view& lua, const char* key)
{
	TSet<int32> originalSet{1,2,3};
	lua[key] = &originalSet;

	lua.safe_script("assert(utype(testkey) == 'TSet')");
	lua.safe_script("assert(utype(testkey, true) == 'TSet<int32>')");
	
	sol::object checkObj = lua[key];
	verify(checkObj.is<FLuaSet>());
	verify(checkObj.is<TSet<int32>>());

	lua.safe_script("testkey:Add(1)");
	lua.safe_script("testkey:Add(3)");
	lua.safe_script("testkey:Add(4)");
	lua.safe_script("testkey:Add(2)");
	
	TSet<int32> retSet = lua[key];
	verify(retSet.Difference(originalSet).IsEmpty());
	verify(retSet.Num() == 4);
	
	lua[key] = sol::nil;

	return true;
}

static bool TestPtr2(sol::state_view& lua, const char* key)
{
	TSet<int32> originalSet{1,2,3};
	lua[key] = &originalSet;
	
	lua.safe_script("assert(utype(testkey) == 'TSet')");
	lua.safe_script("assert(utype(testkey, true) == 'TSet<int32>')");
	
	sol::object checkObj = lua[key];
	verify(checkObj.is<FLuaSet>());
	verify(checkObj.is<TSet<int32>>());
	
	lua.safe_script("testkey:Add(4)");

	TSet<int32>* retSet = lua[key];
	verify(retSet == &originalSet)
	retSet->Add(5);
	verify(originalSet.Contains(5));
	
	lua[key] = sol::nil;

	return true;
}

static bool TestLuaOps(sol::state_view& lua, const char* key)
{
	lua.safe_script("testkey = TSet('int32')");

	lua.safe_script("assert(utype(testkey) == 'TSet')");
	lua.safe_script("assert(utype(testkey, true) == 'TSet<int32>')");
	
	TSet<int32>* observer = lua[key];
	verify(observer != nullptr);
	TSet<float>* invalid = lua[key];
	verify(invalid == nullptr);
	
	//lua.safe_script("print(type(testkey))");
	lua.safe_script("assert(not testkey:Contains(4))");
	lua.safe_script("testkey:Add(4)");
	lua.safe_script("assert(testkey:Contains(4))");
	lua.safe_script("testkey:Remove(4)");
	lua.safe_script("assert(not testkey:Contains(4))");
	lua.safe_script("testkey:Add(1)");
	lua.safe_script("testkey:Add(2)");
	lua.safe_script("assert(testkey:Num() == 2)");
	lua.safe_script("testkey:Clear()");
	lua.safe_script("assert(testkey:Num() == 0)");
	lua.safe_script("testkey:Add(7)");
	lua.safe_script("testkey:Add(8)");
	lua.safe_script("testkey:Add(9)");
	lua.safe_script("assert(testkey:Num() == 3)");
	lua.safe_script("testkey:Empty()");
	lua.safe_script("assert(testkey:Num() == 0)");
	lua.safe_script("testkey:Add(123)");
	lua.safe_script("testkey:Add(456)");
	lua.safe_script("testkey:Add(789)");
	lua.safe_script("assert(testkey:Find(456) ~= -1)");

	UUnrealLuaTestObject* obj = NewObject<UUnrealLuaTestObject>();
	lua["obj"] = obj;
	lua.safe_script("assert(testkey:Num() == 3)");
	lua.safe_script("testkey:Add(obj)");
	lua.safe_script("assert(testkey:Num() == 3)");
	
	TSet<float> invalid2 = lua[key];
	verify(invalid2.Num() == 0);
	TSet<UObject*> invalid3 = lua[key];
	verify(invalid3.Num() == 0);
	
	lua.safe_script("a = testkey:Any()");
	//lua.safe_script("print(a)");
	lua.safe_script("a, b = testkey:Any(2)");
	//lua.safe_script("print(a)");
	//lua.safe_script("print(b)");
	lua.safe_script("a,b,c = testkey:Any(3)");
	//lua.safe_script("print(a)");
	//lua.safe_script("print(b)");
	//lua.safe_script("print(c)");
	lua.safe_script("a = nil");
	lua.safe_script("b = nil");
	lua.safe_script("c = nil");
	
	lua.safe_script("local f = function(item)\n"
				 "return (item < 700)\n"
				 "end\n"
				 "testkey:RemoveAll(f)");
	lua.safe_script("assert(testkey:Num() == 1)");
	
	lua.safe_script("testkey:Add(123)");
	lua.safe_script("testkey:Add(456)");
	lua.safe_script("testkey:KeepAll(function(item) return item < 700 end)");
	lua.safe_script("assert(testkey:Num() == 2)");
	lua.safe_script("assert(not testkey:IsEmpty())");

	verify(observer->Num() == 2)
	verify(!observer->IsEmpty())
	
	lua.safe_script("testkey:Empty()");
	lua.safe_script("assert(testkey:Num() == 0)");
	lua.safe_script("assert(testkey:IsEmpty())");

	verify(observer->Num() == 0)
	verify(observer->IsEmpty())
	
	lua.safe_script("testkey:Add(1)");
	lua.safe_script("testkey:Add(2)");
	lua.safe_script("assert(not testkey:IsEmpty())");
	lua.safe_script("assert(testkey:Num() == 2)");
	
	verify(observer->Num() == 2)
	verify(observer->Contains(2))
	verify(observer->Contains(1))
	observer = nullptr;

	lua[key] = sol::nil;
	lua["a"] = sol::nil;
	lua["b"] = sol::nil;
	lua["c"] = sol::nil;
	lua["obj"] = sol::nil;

	return true;
}

static bool TestNumericIterators(sol::state_view& lua, const char* key)
{
	lua.safe_script("testkey = TSet('int32')");

	TSet<int32>* observer = lua[key];
	verify(observer != nullptr);
	
	lua.safe_script("assert(not testkey:Contains(4))");
	lua.safe_script("testkey:Add(1)");
	lua.safe_script("testkey:Add(2)");
	lua.safe_script("testkey:Add(3)");
	lua.safe_script("testkey:Add(4)");
	/*
	lua.safe_script(
R"(for k=1,#testkey do
		local item = testkey[k]
		print(item)
		end)");
	*/
	lua[key] = sol::nil;
	
	return true;
}

static bool TestIpairsIterators(sol::state_view& lua, const char* key)
{
	lua.safe_script("testkey = TSet('int32')");
	lua.safe_script("assert(not testkey:Contains(4))");
	lua.safe_script("testkey:Add(1)");
	lua.safe_script("testkey:Add(2)");
	lua.safe_script("testkey:Add(3)");
	lua.safe_script("testkey:Add(4)");
	//lua.safe_script("for k,v in ipairs(testkey) do print(tostring(k) .. tostring(v)) end");
	lua[key] = sol::nil;

	lua.safe_script("testkey = TSet('UObject')");
	TSet<UObject*>* observer = lua[key];
	observer->Emplace(NewObject<UUnrealLuaTestObject>());
	observer->Emplace(NewObject<UUnrealLuaTestObject>());
	observer->Emplace(NewObject<UUnrealLuaTestObject>());
	observer->Emplace(NewObject<UUnrealLuaTestObject>());
	//lua.safe_script("for k,v in ipairs(testkey) do print(tostring(k) .. ' : ' .. tostring(v)) end");
	lua[key] = sol::nil;

	return true;
}

static bool TestPairsIterators(sol::state_view& lua, const char* key)
{
	lua.safe_script("testkey = TSet('int32')");
	lua.safe_script("assert(not testkey:Contains(4))");
	lua.safe_script("testkey:Add(1)");
	lua.safe_script("testkey:Add(2)");
	lua.safe_script("testkey:Add(3)");
	lua.safe_script("testkey:Add(4)");
	//lua.safe_script("for k,v in pairs(testkey) do print(k .. v) end");
	lua[key] = sol::nil;

	lua.safe_script("assert(utype(testkey) == 'nil')");
	
	lua.safe_script("testkey = TSet('UObject')");

	lua.safe_script("assert(utype(testkey) == 'TSet')");
	lua.safe_script("assert(utype(testkey, true) == 'TSet<UObject>')");
	
	TSet<UObject*>* observer = lua[key];
	observer->Emplace(NewObject<UUnrealLuaTestObject>());
	observer->Emplace(NewObject<UUnrealLuaTestObject>());
	observer->Emplace(NewObject<UUnrealLuaTestObject>());
	observer->Emplace(NewObject<UUnrealLuaTestObject>());
	//lua.safe_script("for k,v in pairs(testkey) do print(k .. ' : ' .. tostring(v)) end");
	lua[key] = sol::nil;
	
	return true;
}

bool FLuaConversionTests::TestSets(sol::state_view& lua)
{
	LUA_LOG("Testing Lua Set")
	static const char* key = "testkey";
	TestCopy(lua, key);
	TestPtr(lua,key);
	TestPtr2(lua,key);
	TestLuaOps(lua, key);
	TestNumericIterators(lua, key);
	TestPairsIterators(lua, key);
	TestIpairsIterators(lua, key);
	lua.collect_garbage();
	return true;
}