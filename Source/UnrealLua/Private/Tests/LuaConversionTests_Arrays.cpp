
#include "Components/ActorComponent.h"
#include "Tests/LuaSelfTests.h"
#include "Tests/LuaSelfTestTypes.h"
#include "LuaTypes/LuaArray.h"
#include "LuaStackHandler/LuaStackHandler.h"


static bool TestNumericIterators(sol::state_view& lua, const char* key)
{
	lua.safe_script("testkey = TArray('int32')");

	lua.safe_script("assert(utype(testkey) == 'TArray')");
	lua.safe_script("assert(utype(testkey, true) == 'TArray<int32>')");
	sol::object arrayMaybe = lua[key]; 
	verify(arrayMaybe.is<FLuaArray>());
	TArray<int32>* observer = lua[key];
	verify(observer != nullptr);
	
	lua.safe_script("assert(not testkey:Contains(4))");
	lua.safe_script("testkey:Add(1)");
	lua.safe_script("testkey:Add(2)");
	lua.safe_script("testkey:Add(3)");
	lua.safe_script("testkey:Add(4)");
	/*
	UNREALLUA_TEST_STEP(
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
	lua.safe_script("testkey = TArray('int32')");
	
	lua.safe_script("assert(utype(testkey) == 'TArray')");
	lua.safe_script("assert(utype(testkey, true) == 'TArray<int32>')");
	
	lua.safe_script("assert(not testkey:Contains(4))");
	lua.safe_script("testkey:Add(1)");
	lua.safe_script("testkey:Add(2)");
	lua.safe_script("testkey:Add(3)");
	lua.safe_script("testkey:Add(4)");
	//lua.safe_script("for k,v in ipairs(testkey) do print(k .. v) end");
	lua[key] = sol::nil;

	lua.safe_script("testkey = TArray('UObject')");
	TArray<UObject*>* observer = lua[key];
	observer->Emplace(NewObject<UUnrealLuaTestObject>());
	observer->Emplace(NewObject<UUnrealLuaTestObject>());
	observer->Emplace(NewObject<UUnrealLuaTestObject>());
	observer->Emplace(NewObject<UUnrealLuaTestObject>());
	//lua.safe_script("for k,v in ipairs(testkey) do print(k .. ' : ' .. tostring(v)) end");
	lua[key] = sol::nil;
	
	return true;
}

static bool TestPairsIterators(sol::state_view& lua, const char* key)
{
	lua.safe_script("testkey = TArray('int32')");
	
	lua.safe_script("assert(utype(testkey) == 'TArray')");
	lua.safe_script("assert(utype(testkey, true) == 'TArray<int32>')");
	
	lua.safe_script("assert(not testkey:Contains(4))");
	lua.safe_script("testkey:Add(1)");
	lua.safe_script("testkey:Add(2)");
	lua.safe_script("testkey:Add(3)");
	lua.safe_script("testkey:Add(4)");
	//lua.safe_script("for k,v in pairs(testkey) do print(k .. v) end");
	lua[key] = sol::nil;
	
	lua.safe_script("testkey = TArray('UObject')");
	TArray<UObject*>* observer = lua[key];
	observer->Emplace(NewObject<UUnrealLuaTestObject>());
	observer->Emplace(NewObject<UUnrealLuaTestObject>());
	observer->Emplace(NewObject<UUnrealLuaTestObject>());
	observer->Emplace(NewObject<UUnrealLuaTestObject>());
	//lua.safe_script("for k,v in pairs(testkey) do print(k .. ' : ' .. utype(v)) end");
	lua[key] = sol::nil;
	
	return true;
}

bool FLuaConversionTests::TestArrays(sol::state_view& lua)
{
	LUA_LOG("Testing Lua Arrays")
	static const char* key = "testkey";
	{
		lua.safe_script("arr = TArray(int32, {333, 666, 999})");
		lua.safe_script("assert(utype(arr, true) == 'TArray<int32>')");
		lua.safe_script("assert(arr:Num() == 3)");
		//lua.safe_script("for k,v in pairs(arr) do print(tostring(k) .. tostring(v)) end");
		
		lua.safe_script("arr = TArray(FString, {'meow', 'nya', 'barf'})");
		lua.safe_script("assert(utype(arr, true) == 'TArray<FString>')");
		lua.safe_script("assert(arr:Num() == 3)");
		//lua.safe_script("for k,v in pairs(arr) do print(tostring(k) .. tostring(v)) end");
		
		lua["arr"] = sol::nil;
	}
	
	TestNumericIterators(lua, key);
	TestIpairsIterators(lua, key);
	TestPairsIterators(lua, key);
	{
		TArray<int32> originalArr1{1,2,3};
		lua[key] = originalArr1;

		lua.safe_script("assert(utype(testkey) == 'TArray')");
		lua.safe_script("assert(utype(testkey, true) == 'TArray<int32>')");
		
		sol::object checkObj = lua[key];
		verify(checkObj.is<FLuaArray>());

		TArray<int32> retArr1_1 = lua[key];
		verify(retArr1_1 == originalArr1);
		
		lua.safe_script("testkey:Add(5)");

		retArr1_1.Add(5);
		TArray<int32> retArr1_2 = lua[key];
		verify(retArr1_1 == retArr1_2);

		lua[key] = sol::nil;
	}
	{
		TArray<int32> originalArr2{1,2,3};
		lua[key] = originalArr2;
		
		lua.safe_script("assert(utype(testkey) == 'TArray')");
		lua.safe_script("assert(utype(testkey, true) == 'TArray<int32>')");
		
		sol::object checkObj = lua[key];
		verify(checkObj.is<FLuaArray>());
		
		TArray<int32>* retArrPtr2 = lua[key];
		
		verify(retArrPtr2 != nullptr)
		verify(originalArr2 == *retArrPtr2);

		lua.safe_script("testkey:Add(5)");
		
		verify(retArrPtr2->Contains(5))
		verify(!originalArr2.Contains(5))
		
		lua[key] = sol::nil;
	}
	{
        TArray<int32> originalArr3{1,2,3};
        lua[key] = &originalArr3;
		
		lua.safe_script("assert(utype(testkey) == 'TArray')");
		lua.safe_script("assert(utype(testkey, true) == 'TArray<int32>')");
        
        sol::object checkObj = lua[key];
        verify(checkObj.is<FLuaArray>());
        
        TArray<int32>* retArrPtr3 = lua[key];
        
        verify(retArrPtr3 != nullptr)
        verify(originalArr3 == *retArrPtr3);

		lua.safe_script("testkey:Add(5)");
		
        verify(retArrPtr3->Contains(5))
		verify(originalArr3.Contains(5))
		
        lua[key] = sol::nil;
	}

	{
		lua.safe_script("testkey = TArray('int32')");
		//lua.safe_script("print(type(testkey))");
		
		lua.safe_script("assert(utype(testkey) == 'TArray')");
		lua.safe_script("assert(utype(testkey, true) == 'TArray<int32>')");
		
		lua.safe_script("testkey:Add(1)");
		lua.safe_script("testkey:Add(2)");
		lua.safe_script("testkey:Add(3)");
		//lua.safe_script("local item = testkey:Get(1)"
		//					"print(type(item))"
		//					"print(item)");
		//UNREALLUA_TEST_STEP("local item2 = testkey[2]"
		//					"print(type(item2))"
		//					"print(item2)");

		TArray<int32> retArr5_1 = lua[key];
		TArray<int32> retArr5_2 = {1,2,3};
		verify(retArr5_1 == retArr5_2);

		TArray<int32>* arrPtr5 = lua[key];
		arrPtr5->Add(4);
		lua.safe_script("assert(testkey:Contains(4))");
		lua.safe_script("assert(testkey:Find(4) == 4)");
		lua.safe_script("assert(testkey:Find(999) == -1)");

		lua.safe_script("testkey:RemoveAll(function(item) return item < 3 end)");
		//lua.safe_script("print(tostring(testkey:Contains(3)))");

		
		lua[key] = sol::nil;
	}

	{
		lua.safe_script("testkey = TArray('double')");
		//lua.safe_script("print(type(testkey))");

		lua.safe_script("assert(utype(testkey) == 'TArray')");
		lua.safe_script("assert(utype(testkey, true) == 'TArray<double>')");
		
		lua.safe_script("testkey:Add(1.2)");
		lua.safe_script("testkey:Add(2.3)");
		lua.safe_script("testkey:Add(3.4)");
		//lua.safe_script("local item = testkey:Get(1)"
		//					"print(type(item))"
		//					"print(item)");
		//lua.safe_script("local item2 = testkey[2]"
		//					"print(type(item2))"
		//					"print(item2)");

		TArray<double> retArr6_1 = lua[key];
		TArray<double> retArr6_2 = {1.2,2.3,3.4};
		verify(retArr6_1 == retArr6_2);

		TArray<double>* arrPtr6 = lua[key];
		
		verify(arrPtr6 != nullptr);
		
		arrPtr6->Add(4.5);
		
		lua.safe_script("assert(testkey:Contains(4.5))");
		lua.safe_script("assert(testkey:Find(4.5) == 4)");
		lua.safe_script("assert(testkey:Find(999.9) == -1)");
		//lua.safe_script("assert(testkey:Contains(0))");
		
		TArray<double> retArr6_3 = lua[key];
		
		verify(*arrPtr6 == retArr6_3);
		verify(*arrPtr6 != retArr6_2);
		
		lua[key] = sol::nil;
	}
	{
		/*
		TArray<UObject*> originalArr7{};
		originalArr7.Add(NewObject<UTestObject>());
		originalArr7.Add(NewObject<UTestObject>());
		originalArr7.Add(NewObject<UTestObject>());

		lua[key] = &originalArr7;
		*/
		lua.safe_script("testkey = TArray('UObject')");
		
		lua.safe_script("assert(utype(testkey) == 'TArray')");
		lua.safe_script("assert(utype(testkey, true) == 'TArray<UObject>')");
		
		TArray<UObject*>* ptrArr7 = lua[key];
		verify(ptrArr7 != nullptr);
		verify(ptrArr7->Num() == 0);
		
		lua["testobj"] = NewObject<UUnrealLuaTestObject>();
		lua.safe_script("testkey:Add(testobj)");
		lua.safe_script("assert(testkey:Num() == 1)");
		lua.safe_script("testkey:Add(testobj)");
		lua.safe_script("assert(testkey:Num() == 2)");
		//lua.safe_script("print(tostring(testkey[1]))");
		
		verify(ptrArr7->Num() == 2);
		ptrArr7->Empty();
		lua.safe_script("assert(testkey:Num() == 0)");

		lua[key] = sol::nil;
		lua["testobj"] = sol::nil;
	}
	{
	
		TArray<UObject*> originalArr8{};
		originalArr8.Add(NewObject<UUnrealLuaTestObject>());
		originalArr8.Add(NewObject<UUnrealLuaTestObject>());
		originalArr8.Add(NewObject<UUnrealLuaTestObject>());

		lua[key] = &originalArr8;
		
		lua.safe_script("assert(utype(testkey) == 'TArray')");
		lua.safe_script("assert(utype(testkey, true) == 'TArray<UObject>')");

		lua.safe_script("assert(testkey:Num() == 3)");

		UUnrealLuaTestObject* obj4 = NewObject<UUnrealLuaTestObject>(); 
		lua["testobj"] = obj4; 
		lua.safe_script("testkey:Add(testobj)");
		lua.safe_script("assert(testkey:Num() == 4)");
		//lua.safe_script("print(tostring(testkey[3]))");

		TArray<UObject*>* ptrArr7 = lua[key];
		verify(ptrArr7 != nullptr);
		verify(ptrArr7->Num() == 4);
		verify((*ptrArr7)[3] == obj4);
		lua[key] = sol::nil;
		lua["testobj"] = sol::nil;
	}

	{
	
		TArray<UActorComponent*> originalArr8{};
		lua[key] = &originalArr8;
		
		lua.safe_script("assert(utype(testkey) == 'TArray')");
		lua.safe_script("assert(utype(testkey, true) == 'TArray<UActorComponent>')");
		
		lua.safe_script("assert(testkey:Num() == 0)");
		UUnrealLuaTestObject* obj4 = NewObject<UUnrealLuaTestObject>(); 

		lua["testobj"] = obj4; 
		lua.safe_script("testkey:Add(testobj)");
		lua.safe_script("assert(testkey:Num() == 0)");

		lua[key] = sol::nil;
		lua["testobj"] = sol::nil;
	}

	{
	
		lua.safe_script("testkey = TArray(TSubclassOf(UObject))");

		lua.safe_script("assert(utype(testkey) == 'TArray')");
		lua.safe_script("assert(utype(testkey, true) == 'TArray<TSubclassOf<UObject>>')");
		
		TArray<TSubclassOf<UObject>>* ptrArr7 = lua[key];
		verify(ptrArr7 != nullptr);
		verify(ptrArr7->Num() == 0);
		
		lua.safe_script("testkey:Add('UButton')");
		lua.safe_script("assert(testkey:Num() == 1)");
		lua.safe_script("testkey:Add('UActorComponent')");
		lua.safe_script("assert(testkey:Num() == 2)");
		//lua.safe_script("print(tostring(testkey[1]))");

		//LUA_LOG("From ptrArr : %s", *(*ptrArr7)[1]->GetName())
		lua[key] = sol::nil;
	}
	lua.collect_garbage();
	return true;
}
