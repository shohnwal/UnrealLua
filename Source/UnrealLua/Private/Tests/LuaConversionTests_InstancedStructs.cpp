
#include "Tests/LuaSelfTests.h"
#include "Tests/LuaSelfTestTypes.h"
#include "LuaTypes/LuaInstancedStruct.h"
#include "UnrealLua.h"
#include "UObject/Package.h"

namespace UnrealLua::SelfTests
{
	static const char* key = "testkey";
	
	bool InstancedStructTest_CopyToLua(sol::state_view& lua)
	{
		FInstancedStruct original_is = FInstancedStruct::Make<FTestScriptStruct>({"yay", 123});
		FTestScriptStruct& original_content = original_is.GetMutable<FTestScriptStruct>();

		//Copy to Lua
		lua[key] = original_is;

		lua.safe_script("assert(not testkey:IsReference())");
		
		lua.safe_script("assert(utype(testkey) == 'TInstancedStruct')");
		lua.safe_script("assert(utype(testkey, true) == 'TInstancedStruct<FTestScriptStruct>', 'was ' .. utype(testkey, true))");
		lua.safe_script("assert(utype(testkey, 0) == 'FTestScriptStruct')");
	
		sol::object checkObj = lua[key];
		verify(checkObj.is<FLuaInstancedStruct>());
		verify(checkObj.is<FInstancedStruct>());

		//change original content
		original_content.x = 456;
		original_content.msg = "barf!";

		//the copy should have different values
		//lua.safe_script("print(testkey.x)");
		lua.safe_script("assert(testkey.x == 123)");
		lua.safe_script("assert(testkey.msg == 'yay')");
		//lua.safe_script("print(type(testkey.msg))");

		int32 x = lua[key]["x"]; 

		verify(original_content.x != x);
		verify(x == 123);

		//test copying back into C++
		FInstancedStruct ret_is = lua[key];

		//metastruct shoujld be the same
		verify(ret_is.GetScriptStruct() == original_is.GetScriptStruct());

		//but memory should be different
		verify(ret_is.GetMemory() != original_is.GetMemory());

		//but memory should be valid
		verify(ret_is.GetMemory() != nullptr);

		//verify that we actually copied out from lua
		FLuaInstancedStruct& luaRef = lua[key].get<FLuaInstancedStruct&>();
		//metastruct shoujld be the same
		verify(luaRef.GetScriptStruct() != nullptr);
		verify(luaRef.GetScriptStruct() == ret_is.GetScriptStruct());
		//memory should be valid
		verify(luaRef.GetMemory() != nullptr);
		//but memory should be different
		verify(luaRef.GetMemory() != ret_is.GetMemory());
		
		lua[key] = sol::nil;
		lua.collect_garbage();
		return true;
	}

	bool InstancedStructTest_ReferenceToLua(sol::state_view& lua)
	{
		FInstancedStruct original_is = FInstancedStruct::Make<FTestScriptStruct>({"yay", 123});
		FTestScriptStruct& original_content = original_is.GetMutable<FTestScriptStruct>();

		//Reference to Lua
		lua[key] = &original_is;
		
		lua.safe_script("assert(testkey:IsReference())");

		lua.safe_script("assert(utype(testkey) == 'TInstancedStruct')");
		lua.safe_script("assert(utype(testkey, true) == 'TInstancedStruct<FTestScriptStruct>', 'was ' .. utype(testkey, true))");
	
		sol::object checkObj = lua[key];
		verify(checkObj.is<FLuaInstancedStruct>());
		verify(checkObj.is<FInstancedStruct>());

		//change original content
		original_content.x = 456;
		original_content.msg = "barf!";

		//the ref should have same values
		lua.safe_script("assert(testkey:IsReference())");
		//lua.safe_script("print(testkey.x)");
		lua.safe_script("assert(testkey.x == 456)");
		lua.safe_script("assert(testkey.msg == 'barf!')");
		//lua.safe_script("print(type(testkey.msg))");

		int32 x = lua[key]["x"]; 

		verify(original_content.x == x);
		verify(x == 456);

		//test referencing back into C++
		FInstancedStruct* ret_is = lua[key].get<FInstancedStruct*>();

		verify(ret_is != nullptr);

		//metastruct shoujld be the same
		verify(ret_is->GetScriptStruct() == original_is.GetScriptStruct());
		//memory should be same
		verify(ret_is->GetMemory() == original_is.GetMemory());
		
		lua[key] = sol::nil;
		lua.collect_garbage();
		return true;
	}

	bool InstancedStructTest_CreateInLua(sol::state_view& lua)
	{
		lua.safe_script("testkey = TInstancedStruct(FTestScriptStruct)");
		
		lua.safe_script("assert(testkey ~= nil)");
		lua.safe_script("assert(not testkey:IsReference())");
		lua.safe_script("assert(testkey.x ~= nil)");
		lua.safe_script("assert(testkey.msg ~= nil)");
		
		lua.safe_script("assert(utype(testkey) == 'TInstancedStruct')");
		lua.safe_script("assert(utype(testkey, true) == 'TInstancedStruct<FTestScriptStruct>', 'was ' .. utype(testkey, true))");

		sol::object checkObj = lua[key];
		verify(checkObj.is<FLuaInstancedStruct>());
		verify(checkObj.is<FInstancedStruct>());

		FInstancedStruct* is = checkObj.as<FInstancedStruct*>();
		verify(is != nullptr);
		verify(is->GetScriptStruct() == FTestScriptStruct::StaticStruct());
		FTestScriptStruct& ts = is->GetMutable<FTestScriptStruct>();
		
		lua.safe_script("testkey.msg = 'meow'");
		lua.safe_script("assert(testkey.msg == 'meow')");
		verify(ts.msg == "meow");
		lua.safe_script("testkey.x = 987");
		lua.safe_script("assert(testkey.x == 987)");
		verify(ts.x == 987);

		ts.x = 741;
		lua.safe_script("assert(testkey.x == 741)");

		//test copy back into C++
		FInstancedStruct ret_is = lua[key].get<FInstancedStruct>();
		lua.safe_script("assert(not testkey:IsReference())");

		verify(ret_is.IsValid());
		verify(ret_is.GetScriptStruct() == FTestScriptStruct::StaticStruct());
		verify(ret_is.GetScriptStruct() == is->GetScriptStruct());
		verify(ret_is.GetMemory() != is->GetMemory());

		lua["testkey"] = sol::nil;
		lua.collect_garbage();
		return true;
	}

	bool InstancedStructTest_AsProperty(sol::state_view& lua)
	{
		UUnrealLuaTestObject* obj = NewObject<UUnrealLuaTestObject>(GetTransientPackage());

		obj->TestInstancedStruct.InitializeAs<FTestScriptStruct>();
		FTestScriptStruct& data = obj->TestInstancedStruct.GetMutable<FTestScriptStruct>();
		data.msg = "meow";
		data.x = 123;
		
		lua["testKey"] = obj;

		//In lua
		//- InitializeAs
		//- Get properties
		//- modify properties
		lua.safe_script("assert(utype(testKey) == 'UUnrealLuaTestObject')");
		lua.safe_script("ref = testKey.TestInstancedStruct");
		lua.safe_script("assert(ref ~= nil)");
		lua.safe_script("assert(ref:IsReference())");
		lua.safe_script("assert(ref.msg == 'meow')");

		lua.safe_script("assert(utype(ref) == 'TInstancedStruct')");
		lua.safe_script("assert(utype(ref, true) == 'TInstancedStruct<FTestScriptStruct>')");
		
		lua.safe_script("assert(ref.x == 123)");
		lua.safe_script("ref.x = 456");
		verify(data.x == 456);
		lua.safe_script("ref.msg = 'wuff'");
		lua.safe_script("assert(ref.msg == 'wuff')");
		verify(data.msg == "wuff");

		lua.safe_script("copy = ref:Copy()");
		lua.safe_script("assert(ref:IsReference())");
		lua.safe_script("assert(not copy:IsReference())");
		lua.safe_script("assert(copy.x == 456)");
		lua.safe_script("assert(copy.msg == 'wuff')");
		lua.safe_script("assert(utype(ref.msg) == 'string')");
		lua.safe_script("ref.x = 789");
		lua.safe_script("assert(utype(ref.x) == 'int64')");
		verify(data.x == 789);
		lua.safe_script("assert(copy.x == 456)");

		
		lua["ref"] = sol::nil;
		lua["copy"] = sol::nil;
		obj->ConditionalBeginDestroy();
		lua.collect_garbage();
		return true;
	}
	
	bool InstancedStructTest_AsScriptValue(sol::state_view& lua)
	{
		UUnrealLuaTestObject* obj = NewObject<UUnrealLuaTestObject>(GetTransientPackage());

		lua["testKey"] = obj;

		lua.safe_script("inst = TInstancedStruct(FTestScriptStruct)");
		lua.safe_script("assert(inst ~= nil)");
		lua.safe_script("assert(not inst:IsReference())");

		lua.safe_script("assert(utype(inst) == 'TInstancedStruct')");
		lua.safe_script("assert(utype(inst, true) == 'TInstancedStruct<FTestScriptStruct>')");
		
		lua.safe_script("assert(inst.x == 0)");
		lua.safe_script("assert(inst.msg == '')");
		lua.safe_script("inst.x = 123");
		lua.safe_script("inst.msg = 'meow'");
		lua.safe_script("assert(inst.x == 123)");
		lua.safe_script("assert(inst.msg == 'meow')");

		//obj shouldn't have that key/property yet
		lua.safe_script("assert(testKey.ScriptInst == nil)");
		//Assign inst to it, this will create a copy for the scriptproperty
		lua.safe_script("testKey.ScriptInst = inst");
		lua.safe_script("assert(testKey.ScriptInst ~= nil)");
		lua.safe_script("assert(not inst:IsReference())");

		//Get a reference to the new scriptproperty
		lua.safe_script("ref = testKey.ScriptInst");
		//lua.safe_script("print(type(ref))");
		//values should still be the same as the original
		lua.safe_script("assert(ref.x == 123)");
		lua.safe_script("assert(ref.x == inst.x)");
		lua.safe_script("assert(ref.msg == 'meow')");
		lua.safe_script("assert(ref.msg == inst.msg)");

		//assign new values to the scriptproperty-instanced struct
		lua.safe_script("ref.x = 456");
		lua.safe_script("ref.msg = 'wuff'");
		//those should be different now than the original instancedstruct
		lua.safe_script("assert(ref.x ~= inst.x)");
		lua.safe_script("assert(inst.x == 123)");
		lua.safe_script("assert(ref.x == 456)");
		
		lua.safe_script("assert(ref.msg ~= inst.msg)");
		lua.safe_script("assert(inst.msg == 'meow')");
		lua.safe_script("assert(ref.msg == 'wuff')");

		//lua.safe_script("print(utype(inst))");
		//lua.safe_script("print(utype(inst, true))");
		lua.safe_script("testKey.TestInstancedStruct = inst");
		verify(obj->TestInstancedStruct.IsValid());
		verify(obj->TestInstancedStruct.GetPtr<FTestScriptStruct>() != nullptr);
		verify(obj->TestInstancedStruct.GetPtr<FTestScriptStruct>()->x  == 123);
		verify(obj->TestInstancedStruct.GetPtr<FTestScriptStruct>()->msg  == "meow");

		//make sure it actually copied the entire struct from Lua to BP and didn't to a weird reference
		lua.safe_script("inst.msg = 'chirp'");
		lua.safe_script("inst.x = 111");
		lua.safe_script("assert(inst.x == 111)"); 
		lua.safe_script("assert(inst.msg == 'chirp', 'string was ' .. tostring(inst.msg))");
		verify(obj->TestInstancedStruct.GetPtr<FTestScriptStruct>()->msg  == "meow");
		verify(obj->TestInstancedStruct.GetPtr<FTestScriptStruct>()->x  == 123);
		
		lua.safe_script("testKey.TestInstancedStruct = ref");
		verify(obj->TestInstancedStruct.IsValid());
		verify(obj->TestInstancedStruct.GetPtr<FTestScriptStruct>() != nullptr);
		verify(obj->TestInstancedStruct.GetPtr<FTestScriptStruct>()->x  == 456);
		verify(obj->TestInstancedStruct.GetPtr<FTestScriptStruct>()->msg  == "wuff");
		
		lua["testKey"] = sol::nil;
		lua["ref"] = sol::nil;
		lua["inst"] = sol::nil;

		obj->ConditionalBeginDestroy();
		lua.collect_garbage();
		return true;
	}
}

bool FLuaConversionTests::TestInstancedStructs(sol::state_view& lua)
{
	LUA_LOG("Testing Lua InstancedStruct")
	UnrealLua::SelfTests::InstancedStructTest_CopyToLua(lua);
	UnrealLua::SelfTests::InstancedStructTest_ReferenceToLua(lua);
	UnrealLua::SelfTests::InstancedStructTest_CreateInLua(lua);
	UnrealLua::SelfTests::InstancedStructTest_AsProperty(lua);
	UnrealLua::SelfTests::InstancedStructTest_AsScriptValue(lua);
	return true;
}
