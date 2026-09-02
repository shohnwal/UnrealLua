#include "Tests/LuaSelfTests.h"
#include "Tests/LuaSelfTestTypes.h"
#include "LuaStackHandler/LuaStackHandler.h"
#include "LuaTypes/LuaMap.h"

bool TestMapsCopy(sol::state_view& lua, const char* key)
{
	TMap<int32, int32> map{{1,2},{3,4},{5,6}};

	lua[key] = map;
	sol::object checkObj = lua[key];
	verify(checkObj.is<FLuaMap>());

	lua.safe_script("assert(utype(testkey) == 'TMap')");
	lua.safe_script("assert(utype(testkey, true) == 'TMap<int32,int32>')");

	//lua.safe_script("print(testkey:Num())");
	lua.safe_script("assert(testkey:Find(1) == 2)");
	lua.safe_script("assert(testkey:Find(5) == 6)");
	lua.safe_script("assert(testkey:Num() == 3)");

	TMap<int32, int32> mapcopy = lua[key];
	verify(mapcopy.OrderIndependentCompareEqual(map));

	lua[key] = sol::nil;
	return true;
}

bool TestMapsPtr(sol::state_view& lua, const char* key)
{
	TMap<int32, int32> map{{1,2},{3,4},{5,6}};

	lua[key] = &map;
	
	lua.safe_script("assert(utype(testkey) == 'TMap')");
	lua.safe_script("assert(utype(testkey, true) == 'TMap<int32,int32>')");
	
	sol::object checkObj = lua[key];
	verify(checkObj.is<FLuaMap>());

	lua.safe_script("assert(testkey:Find(1) == 2)");
	lua.safe_script("assert(testkey:Find(5) == 6)");
	lua.safe_script("assert(testkey:Num() == 3)");

	TMap<int32, int32>* mapcopy = lua[key];
	verify(mapcopy == &map);
	lua[key] = sol::nil;
	return true;
}

bool TestScriptMap(sol::state_view& lua, const char* key)
{
	lua.safe_script("testkey = TMap('int32', 'int32')");
	lua.safe_script("assert(testkey ~= nil)");

	lua.safe_script("assert(utype(testkey) == 'TMap')");
	lua.safe_script("assert(utype(testkey, true) == 'TMap<int32,int32>')");

	TMap<int32, int32>* observer = lua[key];
	
	sol::object checkObj = lua[key];
	verify(checkObj.is<FLuaMap>());

	lua.safe_script("testkey:Add(1,2)");
	//lua.safe_script("print(testkey:Find(1))");
	//lua.safe_script("print(testkey:Find(5))");
	lua.safe_script("testkey:Add(5,6)");
	//lua.safe_script("print(testkey:Find(5))");
	lua.safe_script("assert(testkey:Num() == 2)");


	verify(observer->Num() == 2);
	verify(observer->Contains(1));
	verify(observer->Contains(5));

	lua.safe_script("testkey:Remove(1)");
	//lua.safe_script("print(testkey:Remove(1))");
	lua.safe_script("assert(testkey:Num() == 1)");
	lua.safe_script("testkey:Add(3,4)");
	lua.safe_script("assert(testkey:Num() == 2)");
	lua.safe_script("testkey:Clear()");
	lua.safe_script("assert(testkey:Num() == 0)");
	
	lua[key] = sol::nil;

	lua.safe_script("testkey = TMap('FString', 'FString')");
	lua.safe_script("assert(testkey ~= nil)");

	lua.safe_script("assert(utype(testkey) == 'TMap')");
	lua.safe_script("assert(utype(testkey, true) == 'TMap<FString,FString>')");
	lua.safe_script("testkey:Add('cat','meow')");
	lua.safe_script("testkey:Add('dog','bark')");
	lua.safe_script("assert(testkey:Num() == 2)");
	lua.safe_script("expr = testkey:Find('cat')");
	//lua.safe_script("print(tostring(expr))");
	lua.safe_script("assert(expr == 'meow')");
	lua.safe_script("testkey:Clear()");

	lua["expr"] = sol::nil;
	lua[key] = sol::nil;
	return true;
}

bool TestScriptMapWithObjects(sol::state_view& lua, const char* key)
{
	lua.safe_script("testkey = TMap('FName', 'UObject')");
	lua.safe_script("assert(testkey ~= nil)");
	
	lua.safe_script("assert(utype(testkey) == 'TMap')");
	lua.safe_script("assert(utype(testkey, true) == 'TMap<FName,UObject>')");

	TMap<FName, UObject*>* observer = lua[key];
	
	sol::object checkObj = lua[key];
	verify(checkObj.is<FLuaMap>());

	lua.safe_script("assert(testkey:Num() == 0)");
	verify(observer->Num() == 0);
	lua.safe_script("obj = NewObject(UUnrealLuaTestObject)");
	lua.safe_script("assert(IsValid(obj))");
	//lua.safe_script("print(tostring(obj))");
	//lua.safe_script("print(utype(obj))");
	lua.safe_script("testkey:Add('test',obj)");
	lua.safe_script("obj = nil");
	verify(observer->Num() == 1);
	lua.safe_script("assert(testkey:Num() == 1)");
	lua.safe_script("assert(testkey:Contains('test'))");
	verify(observer->Contains("test"));
	UObject* obj = observer->FindRef("test");
	verify(IsValid(obj));
	verify(obj->IsA<UUnrealLuaTestObject>());
	lua.safe_script("assert(obj == nil)");
	lua.safe_script("obj = testkey:Find('test')");
	//lua.safe_script("print(tostring(obj))");
	//lua.safe_script("print(utype(obj))");
	lua.safe_script("assert(utype(obj) == 'UUnrealLuaTestObject')");
	lua.safe_script("obj = nil");
	verify(observer->Num() == 1);
	verify(observer->Contains(FName{"test"}));

	lua.safe_script("local removed = testkey:Remove('test')");
	//	"print(tostring(removed))");
	lua.safe_script("assert(testkey:Num() == 0)");
	
	lua[key] = sol::nil;
	return true;
}

bool TestScriptMapNumericIterators(sol::state_view& lua, const char* key)
{
	//numeric for indexing retreives values
	//LUA_LOG("\n------------------------------------\nChecking FLuaMap for i=1,#Map iterator\n------------------------------------")
	lua.safe_script("testkey = TMap('FName', 'int32')");
	lua.safe_script("assert(testkey ~= nil)");
	
	lua.safe_script("assert(utype(testkey) == 'TMap')");
	lua.safe_script("assert(utype(testkey, true) == 'TMap<FName,int32>')");

	lua.safe_script("testkey:Add('key1',1)");
	lua.safe_script("testkey:Add('key2',2)");
	lua.safe_script("testkey:Add('key3',3)");

	/*
	lua.safe_script(
R"(for k=1,#testkey do
		local value = testkey[k]
		print(tostring(value))
		end)");
	*/
	lua[key] = sol::nil;
	
	return true;
}

bool TestScriptMapIpairsIterators(sol::state_view& lua, const char* key)
{
	//ipairs lists internal index + value
	
	//LUA_LOG("\n------------------------------------\nChecking FLuaSet ipairs() iterator\n------------------------------------")
	lua.safe_script("testkey = TMap('FName', 'int32')");
	lua.safe_script("assert(testkey ~= nil)");
	
	lua.safe_script("assert(utype(testkey) == 'TMap')");
    lua.safe_script("assert(utype(testkey, true) == 'TMap<FName,int32>')");

	lua.safe_script("testkey:Add('key1',1)");
	lua.safe_script("testkey:Add('key2',2)");
	lua.safe_script("testkey:Add('key3',3)");

	//lua.safe_script("for k,v in ipairs(testkey) do print(k .. v .. tostring(testkey:Find(v))) end");
	lua[key] = sol::nil;
	
	lua.safe_script("testkey = TMap('FName', 'UObject')");
	TMap<FName, UObject*>* observer = lua[key];
	observer->Emplace("key4", NewObject<UUnrealLuaTestObject>());
	observer->Emplace("key5", NewObject<UUnrealLuaTestObject>());
	observer->Emplace("key6", NewObject<UUnrealLuaTestObject>());
	//lua.safe_script("for k,v in ipairs(testkey) do print(k .. ' : ' .. tostring(v) .. tostring(testkey:Find(v))) end");
	
	lua[key] = sol::nil;
	
	return true;
}

bool TestScriptMapPairsIterators(sol::state_view& lua, const char* key)
{
	//pairs retreives key : value pairs
	//LUA_LOG("\n------------------------------------\nChecking FLuaSet pairs() iterator\n------------------------------------")
	
	lua.safe_script("testkey = TMap('FName', 'int32')");
	lua.safe_script("assert(testkey ~= nil)");

	lua.safe_script("assert(utype(testkey) == 'TMap')");
	lua.safe_script("assert(utype(testkey, true) == 'TMap<FName,int32>')");

	lua.safe_script("testkey:Add('key1',1)");
	lua.safe_script("testkey:Add('key2',2)");
	lua.safe_script("testkey:Add('key3',3)");

	//lua.safe_script("for k,v in pairs(testkey) do print(k .. v) end");
	lua[key] = sol::nil;
	
	lua.safe_script("testkey = TMap('FName', 'UObject')");
	TMap<FName, UObject*>* observer = lua[key];
	observer->Emplace("key4", NewObject<UUnrealLuaTestObject>());
	observer->Emplace("key5", NewObject<UUnrealLuaTestObject>());
	observer->Emplace("key6", NewObject<UUnrealLuaTestObject>());
	//lua.safe_script("for k,v in pairs(testkey) do print(k .. ' : ' .. tostring(v)) end");
	
	lua[key] = sol::nil;
	
	return true;
}

bool FLuaConversionTests::TestMaps(sol::state_view& lua)
{
	LUA_LOG("Testing Lua LuaMap")
	static const char* key = "testkey";

	TestMapsCopy(lua, key);
	TestMapsPtr(lua, key);
	TestScriptMap(lua, key);
	TestScriptMapWithObjects(lua, key);
	TestScriptMapNumericIterators(lua, key);
	//TestScriptMapIpairsIterators(lua, key);
	TestScriptMapPairsIterators(lua, key);
	lua.collect_garbage();
	return true;
}

