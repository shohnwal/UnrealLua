
#include "Tests/LuaSelfTests.h"
#include "Tests/LuaSelfTestTypes.h"
#include "LuaValue/LuaScriptValue.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "UnrealLua.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"

namespace UnrealLua::SelfTests
{
	void UObjectBasics(sol::state_view& lua)
	{
		static const char* key = "testkey";
		UUnrealLuaTestObject* testObj = NewObject<UUnrealLuaTestObject>();
		lua[key] = testObj;

		lua.safe_script("assert(utype(testkey) == 'UUnrealLuaTestObject')");
		lua.safe_script("assert(utype(testkey, true) == 'UUnrealLuaTestObject')");
		
		//lua.safe_script("print(tostring(testkey))");
		//lua.safe_script("print(testkey.String)");
		
		sol::object checkObj = lua[key];
		//verify(checkObj.is<FLuaUObjectWrapper>());
		verify(UnrealLua::IsUObject(checkObj));
		verify(UnrealLua::IsUObject<UUnrealLuaTestObject>(checkObj));
	
 		UUnrealLuaTestObject* testObjRet = lua[key];
		
		verify(testObj == testObjRet);
		verify(testObj->GetClass() == testObjRet->GetClass());

		UObject* testObjRet2 = lua[key];
		verify(testObj == testObjRet2);
		verify(testObj->GetClass() == testObjRet2->GetClass());

		UActorComponent* invalid = lua[key];
		verify(invalid == nullptr);
	
		lua[key] = sol::nil;
		
		lua.collect_garbage();
	}

	void UObjectScriptConstruction(sol::state_view& lua)
	{
		lua.safe_script("obj = NewObject('UUnrealLuaTestObject')");
		//lua.safe_script("print(tostring(obj))");
		lua.safe_script("assert(obj ~= nil)");

		lua.safe_script("assert(utype(obj) == 'UUnrealLuaTestObject')");
		lua.safe_script("assert(utype(obj, true) == 'UUnrealLuaTestObject')");

		
		lua.safe_script("obj = nil");
		
		lua.safe_script("assert(utype(testkey) == 'nil')");
		lua.safe_script("assert(utype(testkey, true) == 'nil')");
		
		lua.safe_script("assert(obj == nil)");
		lua.safe_script("obj = NewObject(UUnrealLuaTestObject)");
		//lua.safe_script("print(tostring(obj))");
		lua.safe_script("assert(obj ~= nil)");
		lua.safe_script("assert(obj.TestEnum == EUnrealLuaTestEnum.Two)");

		//lua.safe_script("print(tostring(obj.TestEnum))");
		//lua.safe_script("print(tostring(EUnrealLuaTestEnum.Two))");
		//lua.safe_script("print(type(obj.TestEnum))");
		//lua.safe_script("print(type(EUnrealLuaTestEnum.Two))");
		lua.safe_script("e1 = obj.TestEnum");
		lua.safe_script("e2 = EUnrealLuaTestEnum.Two");
		lua.safe_script("assert(e1 == e2)");
		lua.safe_script("assert(e1 == EUnrealLuaTestEnum.Two)");
		lua.safe_script("assert(e2 == obj.TestEnum)");
		lua.safe_script("assert(EUnrealLuaTestEnum.Two == e1)");
		lua.safe_script("assert(obj.TestEnum == e2)");

		lua.safe_script("obj.TestEnum = tostring(EUnrealLuaTestEnum.Three)");
		//lua.safe_script("print(tostring(obj.TestEnum))");
		//lua.safe_script("print(#obj.TestEnum)");
		//lua.safe_script("print(#EUnrealLuaTestEnum.Three)");
		lua.safe_script("assert(#obj.TestEnum == #EUnrealLuaTestEnum.Three)");
		lua.safe_script("assert(obj.TestEnum == EUnrealLuaTestEnum.Three)");
		lua.safe_script("assert(#obj.TestEnum == #EUnrealLuaTestEnum[2])");
		lua.safe_script("assert(obj.TestEnum == EUnrealLuaTestEnum[2])");
		
		lua.safe_script("obj = nil");
		
		lua.collect_garbage();
	}

	bool UObjectProperties(sol::state_view& lua)
	{
		lua.safe_script("obj = NewObject('UUnrealLuaTestObject')");
		
		lua.safe_script("assert(utype(obj) == 'UUnrealLuaTestObject')");
		lua.safe_script("assert(utype(obj, true) == 'UUnrealLuaTestObject')");
		UObject* obj_ = lua["obj"];
		verify(IsValid(obj_));
		UUnrealLuaTestObject* obj = lua["obj"];
		verify(IsValid(obj));

		{
			//bool
			lua.safe_script("assert(obj.Bool == true)");
			verify(obj->Bool == true);
			lua.safe_script("obj.Bool = false");
			lua.safe_script("assert(obj.Bool == false)");
			verify(obj->Bool == false);
			lua.safe_script("obj.Bool = true");
			lua.safe_script("assert(obj.Bool == true)");
			verify(obj->Bool == true);
			lua.safe_script("assert(utype(obj.Bool) == 'boolean')");

			lua.collect_garbage();
		}

		{
			//int8 types
			lua.safe_script("assert(obj.Int8 == 127)");
			verify(obj->Int8 == INT8_MAX);
			lua.safe_script("assert(utype(obj.Int8) == 'int64')");
			lua.safe_script("obj.Int8 = -42");
			verify(obj->Int8 == -42);

			lua.safe_script("assert(obj.UInt8 == 255)");
			verify(obj->UInt8 == UINT8_MAX);
			lua.safe_script("assert(utype(obj.UInt8) == 'int64')");
			lua.safe_script("obj.UInt8 = 42");
			verify(obj->UInt8 == 42);
			
			lua.collect_garbage();
		}

		{
			//int16 types
			lua.safe_script("assert(obj.Int16 ==  32767)");
			verify(obj->Int16 == INT16_MAX);
			lua.safe_script("assert(utype(obj.Int16) == 'int64')");
			lua.safe_script("obj.Int16 = -42");
			verify(obj->Int16 == -42);
;
			lua.safe_script("assert(obj.UInt16 == 65535)");
			verify(obj->UInt16 == UINT16_MAX);
			lua.safe_script("assert(utype(obj.UInt16) == 'int64')");
			lua.safe_script("obj.UInt16 = 42");
			verify(obj->UInt16 == 42);

			lua.collect_garbage();
		}
		
		{
			//int32 types
			lua.safe_script("assert(obj.Int32 == 2147483647)");
			verify(obj->Int32 == INT32_MAX);
			lua.safe_script("assert(utype(obj.Int32) == 'int64')");
			lua.safe_script("obj.Int32 = -42");
			verify(obj->Int32 == -42);
			;
			lua.safe_script("assert(obj.UInt32 == 4294967295)");
			verify(obj->UInt32 == UINT32_MAX);
			lua.safe_script("assert(utype(obj.UInt32) == 'int64')");
			lua.safe_script("obj.UInt32 = 42");
			verify(obj->UInt32 == 42);

			lua.collect_garbage();
		}

		{
			//int64 types
			const int64 max = (1LL << std::numeric_limits<double>::digits);// ((1LL << 53));
			const int64 min = -(1LL << std::numeric_limits<double>::digits); //-((1LL << 53));
			verify(obj->Int64 == INT64_MAX);
			
			std::stringstream ss;
			ss << "assert(obj.Int64 == " << max << ")";
			lua.safe_script(ss.str());
			
			lua.safe_script("assert(utype(obj.Int64) == 'int64')");
			obj->Int64 = INT64_MIN;
			verify(obj->Int64 == INT64_MIN);
			
			ss.str("");
			ss.clear();
			ss << "assert(obj.Int64 == " << min << ")";
			lua.safe_script(ss.str());
			
			lua.safe_script("assert(utype(obj.Int64) == 'int64')");
			lua.safe_script("obj.Int64 = -42");
			verify(obj->Int64 == -42);
			
			//lua.safe_script("assert(obj.UInt64 == 18446744073709551615)");
			verify(obj->UInt64 == UINT64_MAX);
			lua.safe_script("assert(utype(obj.UInt64) == 'int64')");
			lua.safe_script("obj.UInt64 = 42");
			verify(obj->UInt64 == 42);

			lua.collect_garbage();
		}

		{
			//Float
			//lua.safe_script("print(obj.Float)");
			verify(obj->Float == PI);
			lua.safe_script("obj.Float = 0");
			lua.safe_script("assert(obj.Float == 0)");
			verify(obj->Float == 0);
			lua.safe_script("obj.Float = 1.23");
			//lua.safe_script("print(obj.Float)");
			verify(obj->Float == 1.23f);
			lua.safe_script("assert(utype(obj.Float) == 'double')");

			lua.collect_garbage();
		}

		{
			//Double
			//lua.safe_script("print(obj.Double)");
			verify(obj->Double == DOUBLE_PI);
			lua.safe_script("obj.Double = 0");
			lua.safe_script("assert(obj.Double == 0)");
			verify(obj->Double == 0);
			lua.safe_script("obj.Double = 3.14");
			//lua.safe_script("print(obj.Double)");
			verify(obj->Double == 3.14);
			lua.safe_script("assert(utype(obj.Double) == 'double')");
			
			lua.collect_garbage();
		}

		{
			//FString
			lua.safe_script("assert(obj.String == 'yay')");
			verify(obj->String == "yay");
			lua.safe_script("obj.String = 'may'");
			lua.safe_script("assert(obj.String == 'may')");
			verify(obj->String == "may");
			lua.safe_script("obj.String = 'OK'");
			lua.safe_script("assert(obj.String == 'OK')");
			lua.safe_script("assert(obj.String ~= 'ok')");
			verify(obj->String == "OK");
			lua.safe_script("assert(utype(obj.String) == 'string')");
			
			lua.collect_garbage();
		}

		{
			//FName
			lua.safe_script("assert(obj.Name == 'nay')");
			verify(obj->Name == "nay");
			lua.safe_script("obj.Name = 'oki'");
			lua.safe_script("assert(obj.Name == 'oki')");
			verify(obj->Name == "oki");
			lua.safe_script("obj.Name = 'OkIdOkI'");
			lua.safe_script("assert(obj.Name == 'OkIdOkI')");
			lua.safe_script("assert(obj.Name ~= 'okidoki')");
			verify(obj->Name == "OkIdOkI");
			lua.safe_script("assert(utype(obj.Name) == 'string')");

			lua.collect_garbage();
		}

		{
			//Object
			lua.safe_script("assert(obj.Object == nil)");
			obj->Object = nullptr;
			lua.safe_script("assert(utype(obj.Object) == 'nil')");
			lua.safe_script("obj.Object = obj");
			lua.safe_script("assert(obj.Object == obj)");
			lua.safe_script("assert(utype(obj.Object) == 'UUnrealLuaTestObject')");
			obj->Object = obj;
			lua.safe_script("obj.Object = nil");
			lua.safe_script("assert(obj.Object == nil)");
			obj->Object = nullptr;
			lua.safe_script("assert(utype(obj.Object) == 'nil')");

			lua.collect_garbage();
		}

		{
			//Enum
			verify(obj->TestEnum == EUnrealLuaTestEnum::Two);
			lua.safe_script("assert(obj.TestEnum == EUnrealLuaTestEnum.Two)");
		
			lua.safe_script("obj.TestEnum = tostring(EUnrealLuaTestEnum.Three)");
			verify(obj->TestEnum == EUnrealLuaTestEnum::Three);
			lua.safe_script("assert(obj.TestEnum == EUnrealLuaTestEnum.Three)");
			lua.safe_script("assert(#obj.TestEnum == #EUnrealLuaTestEnum.Three)");
			lua.safe_script("assert(#obj.TestEnum == #EUnrealLuaTestEnum[2])");
			lua.safe_script("assert(obj.TestEnum == EUnrealLuaTestEnum[2])");

			lua.safe_script("obj.TestEnum = EUnrealLuaTestEnum.Two");
			verify(obj->TestEnum == EUnrealLuaTestEnum::Two);
			lua.safe_script("e1 = obj.TestEnum");
			lua.safe_script("e2 = EUnrealLuaTestEnum.Two");
			//lua.safe_script("print(tostring(e1))");
			//lua.safe_script("print(tostring(e2))");
			lua.safe_script("assert(e1 == EUnrealLuaTestEnum.Two)");
			lua.safe_script("assert(e2 == obj.TestEnum)");
			lua.safe_script("assert(EUnrealLuaTestEnum.Two == e1)");
			lua.safe_script("assert(obj.TestEnum == e2)");
			lua.safe_script("assert(e1 == e2)");
			
			lua.collect_garbage();
		}

		{
			//ScriptStruct

			//Getting FScriptStruct Property gets a reference

			//changing values in reference should also change value in UObject

			//Make copy of reference

			//change value in copy

			//original should still be same

			//assign changed copy back to property

			//property value should have changed

			//reference should also have changed
			
		}

		{
			//InstancedStruct
		}

		{
			//SharedStruct
		}

		{
			//SingleDelegate

			//Already got a separate test for that
		}

		{
			//MultiDelegate
			
			//Already got a separate test for that
		}

		{
			//Array

			//Getting Property should return a reference

			/*
			lua.safe_script("assert(obj.StringArray ~= nil)");
			lua.safe_script("assert(utype(obj.StringArray, true) == 'TArray<FString>')");
			lua.safe_script("assert(obj.StringArray:Num() == 0)");

			verify(obj->StringArray.IsEmpty());
			lua.safe_script("obj.StringArray:Add('meow')");
			verify(!obj->StringArray.IsEmpty());
			lua.safe_script("assert(not obj.StringArray:IsEmpty())");
			lua.safe_script("assert(obj.StringArray:Num() == 1)");
			verify(obj->StringArray.Contains("meow"));
			lua.safe_script("obj.StringArray:Contains('meow')");
			obj->StringArray.Empty();
			lua.safe_script("assert(obj.StringArray:IsEmpty())");
			
			lua.safe_script("arr = TArray(str, {'nya!', 'wuff', 'chirp!'})");
			lua.safe_script("assert(not arr:IsEmpty())");
			lua.safe_script("assert(arr:Contains('nya!'))");
			lua.safe_script("assert(arr:Contains('chirp!'))");
			lua.safe_script("assert(arr:Contains('wuff'))");
			
			verify(obj->StringArray.IsEmpty());
			lua.safe_script("obj.StringArray = arr");
			
			lua.safe_script("assert(obj.StringArray ~= arr)");
			
			verify(!obj->StringArray.IsEmpty());
			verify(obj->StringArray.Num() == 3);
			verify(!obj->StringArray.Contains("meow"));
			verify(obj->StringArray.Contains("nya!"));
			verify(obj->StringArray.Contains("chirp!"));
			verify(obj->StringArray.Contains("wuff"));
			
			lua.safe_script("assert(obj.StringArray:Contains('nya!'))");
			lua.safe_script("assert(obj.StringArray:Contains('chirp!'))");
			lua.safe_script("assert(arr:Contains('nya!'))");
			
			lua.safe_script("for k,v in pairs(obj.StringArray) do print(tostring(k) .. tostring(v)) end");
			*/
			//Adding item to Property should also change reference

			//Adding item in reference should also change Property content

			//make copy

			//changing copy should not affect original value or reference

			//assigning copy back to property should change property and reference
			
			lua["arr"] = sol::nil;
			
			lua.collect_garbage();
			lua.collect_garbage();
			
		}

		{
			//Set
		}

		{
			//Map
		}

		{
			//UScriptStruct
		}

		{
			//UClass
		}

		{
			//TSubclassOf
		}

		{
			//SoftObjectPtr
		}

		{
			//Interface
		}
		lua["obj"] = sol::nil;
		lua.collect_garbage();
		lua.collect_garbage();
		return true;
	}

	bool UFunctionArgs(sol::state_view& lua)
	{
		lua.safe_script("obj = NewObject('UUnrealLuaTestObject')");
		UUnrealLuaTestObject* obj = NewObject<UUnrealLuaTestObject>();
		lua["obj"] = obj;
		//obj->FuncInt32(int32)
		lua.safe_script("assert(obj:FuncInt32(123) == 123)");
		//obj->FuncInt64(int64);
		lua.safe_script("assert(obj:FuncInt64(456) == 456)");
		//obj->FuncFloat(float)
		lua.safe_script("assert(math.abs(obj:FuncFloat(1.23) - 1.23) < 0.1)");
		//obj->FuncDouble(double)
		lua.safe_script("assert(math.abs(obj:FuncDouble(1.23) - 1.23) < 0.1)");
		//obj->FuncFString(FString);
		lua.safe_script("assert(obj:FuncFString('meow') == 'meow')");
		//obj->FuncFName(FName);
		lua.safe_script("assert(obj:FuncFName('barf') == 'barf')");
		//obj->FuncUObject(UObject*)
		lua.safe_script("assert(obj:FuncUObject(obj) == obj)");

		//obj->FuncArrayCopy(TArray<int32>)
		lua.safe_script("arr = TArray(int32)");
		lua.safe_script("arr:Add(123)");
		lua.safe_script("arr:Add(456)");
		lua.safe_script("arr2 = obj:FuncArrayCopy(arr)");
		lua.safe_script("arr:Add(789)");
		lua.safe_script("assert(arr ~= arr2)");
		lua.safe_script("assert(arr:Contains(789))");
		lua.safe_script("assert(not arr2:Contains(789))");
		lua["arr"] = sol::nil;
		lua["arr2"] = sol::nil;
		
		//obj->FuncArrayRef(TArray<int32>&)
		lua.safe_script("arr = TArray(int32)");
		lua.safe_script("arr:Add(123)");
		lua.safe_script("arr:Add(456)");
		lua.safe_script("assert(arr:Num() == 2)");
		lua.safe_script("assert(not arr:Contains(999))");
		lua.safe_script("obj:FuncArrayRef(arr)");
		lua.safe_script("assert(arr:Num() == 3)");
		lua.safe_script("assert(arr:Contains(999))");
		lua["arr"] = sol::nil;

		//obj->FuncSetCopy(TSet<int32>)
		lua.safe_script("set = TSet(int32)");
		lua.safe_script("set:Add(123)");
		lua.safe_script("set:Add(456)");
		lua.safe_script("set2 = obj:FuncSetCopy(set)");
		lua.safe_script("set:Add(789)");
		lua.safe_script("assert(set ~= set2)");
		lua.safe_script("assert(set:Contains(789))");
		lua.safe_script("assert(not set2:Contains(789))");
		lua["set"] = sol::nil;
		lua["set2"] = sol::nil;
		
		//obj->FuncSetRef(TSet<int32>&)
		lua.safe_script("set = TSet(int32)");
		lua.safe_script("set:Add(123)");
		lua.safe_script("set:Add(456)");
		lua.safe_script("assert(set:Num() == 2)");
		lua.safe_script("assert(not set:Contains(999))");
		lua.safe_script("obj:FuncSetRef(set)");
		lua.safe_script("assert(set:Num() == 3)");
		lua.safe_script("assert(set:Contains(999))");
		lua["set"] = sol::nil;
		
		lua.safe_script("result = obj:TestFuncArgs(123, 3.14, true, 'meow', 'barf', obj)");
		lua.safe_script("assert(result == 456)");
		lua["obj"] = sol::nil;
		lua["result"] = sol::nil;
		lua.collect_garbage();

		return true;
	}

	bool UFunctionOutRefs(sol::state_view& lua)
	{
		lua.safe_script("obj = NewObject('UUnrealLuaTestObject')");
		
		lua.safe_script("assert(utype(obj) == 'UUnrealLuaTestObject')");
		lua.safe_script("assert(utype(obj, true) == 'UUnrealLuaTestObject')");
		UUnrealLuaTestObject* obj = lua["obj"];
		verify(IsValid(obj));
		{
			//TArray 1
			lua.safe_script("arr = TArray('int32')");
			lua.safe_script("assert(arr:Num() == 0)");
			lua.safe_script("arr:Add(1)");
			lua.safe_script("arr:Add(2)");
			lua.safe_script("assert(arr:Num() == 2)");

			//TestArrayRef adds two numbers, modifying the passed-in array
			lua.safe_script("success = obj:TestArrayRef(arr)");
			lua.safe_script("assert(success)");
			lua.safe_script("assert(arr:Num() == 4)");
			lua.safe_script("assert(arr:Contains(123))");
			lua.safe_script("assert(arr[2] == 123)");

			lua.safe_script("arr = nil");
			lua.safe_script("success = nil");
			
			lua.collect_garbage();
		}

		{
			//TArray 2
			lua.safe_script("arr = TArray('int32')");
			lua.safe_script("assert(arr:Num() == 0)");
			lua.safe_script("arr:Add(1)");
			lua.safe_script("arr:Add(2)");
			lua.safe_script("assert(arr:Num() == 2)");

			//TestArrayRef adds two numbers, modifying the passed-in array
			//arr2 should refer to the same array, thus sharing content
			lua.safe_script("success, arr2 = obj:TestArrayRef(arr)");
			lua.safe_script("assert(success)");
			lua.safe_script("assert(arr:Num() == 4)");
			lua.safe_script("assert(arr2:Num() == 4)");
			lua.safe_script("assert(arr:Contains(123))");
			lua.safe_script("assert(arr2:Contains(123))");
			lua.safe_script("assert(arr[2] == 123)");
			lua.safe_script("assert(arr2[2] == 123)");
			
			lua.safe_script("arr:Add(999)");
			lua.safe_script("assert(arr:Num() == 5)");
			lua.safe_script("assert(arr2:Num() == 5)");
			lua.safe_script("assert(arr:Contains(999))");
			lua.safe_script("assert(arr2:Contains(999))");

			//even when one arr gets nilled, arr2 should sitll have the content
			lua.safe_script("arr = nil");
			lua.safe_script("assert(arr2:Num() == 5)");
			lua.safe_script("assert(arr2:Contains(1))");
			lua.safe_script("assert(arr2:Contains(123))");
			lua.safe_script("assert(arr2:Contains(999))");
			
			lua.safe_script("arr2 = nil");
			lua.safe_script("success = nil");

			lua.collect_garbage();
		}

		{
			//TMap
			lua.safe_script("map = TMap('int32', 'FString')");
			//lua.safe_script("print(type(map))");
			//lua.safe_script("print(utype(map, true))");
			lua.safe_script("assert(map:Num() == 0)");
			lua.safe_script("map:Add(1, 'meow')");
			lua.safe_script("map:Add(2, 'barf')");
			lua.safe_script("assert(map:Num() == 2)");
			lua.safe_script("assert(map[1] == 'meow')");
			lua.safe_script("assert(map[2] == 'barf')");
			
			lua.safe_script("map = nil");
			lua.safe_script("success = nil");
			
			lua.collect_garbage();
		}
		{
			//TMap 2
			lua.safe_script("map = TMap('int32', 'FString')");
			//lua.safe_script("print(type(map))");
			//lua.safe_script("print(utype(map, true))");
			lua.safe_script("assert(map:Num() == 0)");
			lua.safe_script("map:Add(1, 'meow')");
			lua.safe_script("map:Add(2, 'barf')");
			lua.safe_script("assert(map:Num() == 2)");
			lua.safe_script("assert(map[1] == 'meow')");
			lua.safe_script("assert(map[2] == 'barf')");
			//TestMapRef adds two elements, modifying the passed-in array
			//arr2 should refer to the same array, thus sharing content
			lua.safe_script("success, map2 = obj:TestMapRef(map)");
			lua.safe_script("assert(success)");
			lua.safe_script("assert(map:Num() == 4)");
			lua.safe_script("assert(map2:Num() == 4)");
			lua.safe_script("assert(map:Contains(123))");
			lua.safe_script("assert(map2:Contains(123))");
			lua.safe_script("assert(map[1] == 'meow')");
			lua.safe_script("assert(map2[1] == 'meow')");
			lua.safe_script("assert(map[2] == 'barf')");
			lua.safe_script("assert(map2[2] == 'barf')");
			lua.safe_script("assert(map[123] == 'chirp')");
			lua.safe_script("assert(map2[123] == 'chirp')");
			
			lua.safe_script("map:Add(999, '\?\?\?')");
			lua.safe_script("assert(map:Num() == 5)");
			lua.safe_script("assert(map2:Num() == 5)");
			lua.safe_script("assert(map:Contains(999))");
			lua.safe_script("assert(map2:Contains(999))");
			lua.safe_script("assert(map2[999] == '\?\?\?')");

			//even when one arr gets nilled, arr2 should sitll have the content
			lua.safe_script("map = nil");
			lua.safe_script("assert(map2:Num() == 5)");
			lua.safe_script("assert(map2:Contains(1))");
			lua.safe_script("assert(map2:Contains(123))");
			lua.safe_script("assert(map2:Contains(999))");
			
			lua.safe_script("map2 = nil");
			lua.safe_script("success = nil");
			
			lua.collect_garbage();
		}

		{
			lua.safe_script("vec = FVector(5,6,5)");
			lua.safe_script("assert(utype(vec) == 'FVector')");
			lua.safe_script("assert(vec.X == 5)");
			lua.safe_script("assert(vec.Y == 6)");
			lua.safe_script("assert(vec.Z == 5)");
			lua.safe_script("success = obj:TestVectorRef(vec)");
			lua.safe_script("assert(success)");
			lua.safe_script("assert(vec ~= nil)");
			lua.safe_script("assert(utype(vec) == 'FVector')");
			lua.safe_script("assert(vec.X == 1)");
			lua.safe_script("assert(vec.Y == 2)");
			lua.safe_script("assert(vec.Z == 3)");
			lua.safe_script("vec = nil");
			lua.safe_script("success = nil");
		
			lua.collect_garbage();
		}

		{
			//TSet 1
			lua.safe_script("set = TSet('int32')");
			lua.safe_script("assert(set:Num() == 0)");
			lua.safe_script("set:Add(1)");
			lua.safe_script("set:Add(2)");
			lua.safe_script("assert(set:Num() == 2)");
			lua.safe_script("set:Add(2)");
			lua.safe_script("assert(set:Num() == 2)");

			//TestSetRef adds two numbers, modifying the passed-in set
			lua.safe_script("success = obj:TestSetRef(set)");
			lua.safe_script("assert(success)");
			lua.safe_script("assert(set:Num() == 4)");
			lua.safe_script("assert(set:Contains(123))");
			lua.safe_script("assert(set:Contains(1))");

			lua.safe_script("set = nil");
			lua.safe_script("success = nil");
			
			lua.collect_garbage();
		}

		{
			//TSet 2
			lua.safe_script("set = TSet('int32')");
			lua.safe_script("assert(set:Num() == 0)");
			lua.safe_script("set:Add(1)");
			lua.safe_script("set:Add(2)");
			lua.safe_script("assert(set:Num() == 2)");

			//TestsetayRef adds two numbers, modifying the passed-in setay
			//set2 should refer to the same setay, thus sharing content
			lua.safe_script("success, set2 = obj:TestSetRef(set)");
			lua.safe_script("assert(success)");
			lua.safe_script("assert(set:Num() == 4)");
			lua.safe_script("assert(set2:Num() == 4)");
			lua.safe_script("assert(set:Contains(123))");
			lua.safe_script("assert(set2:Contains(123))");
			
			lua.safe_script("set:Add(999)");
			lua.safe_script("assert(set:Num() == 5)");
			lua.safe_script("assert(set2:Num() == 5)");
			lua.safe_script("assert(set:Contains(999))");
			lua.safe_script("assert(set2:Contains(999))");

			//even when one set gets nilled, set2 should sitll have the content
			lua.safe_script("set = nil");
			lua.safe_script("assert(set2:Num() == 5)");
			lua.safe_script("assert(set2:Contains(1))");
			lua.safe_script("assert(set2:Contains(123))");
			lua.safe_script("assert(set2:Contains(999))");
			
			lua.safe_script("set2 = nil");
			lua.safe_script("success = nil");

			lua.collect_garbage();
		}
	
		{
			lua.safe_script("strct = FTestScriptStruct('yay', 789)");
			//lua.safe_script("print(utype(strct))");
			lua.safe_script("assert(utype(strct) == 'FTestScriptStruct')");
			lua.safe_script("assert(strct.x == 789)");
			lua.safe_script("assert(strct.msg == 'yay')");
			
			lua.safe_script("success, strct2 = obj:TestStructRef(strct)");
			lua.safe_script("assert(success)");
			lua.safe_script("assert(utype(strct2) == 'FTestScriptStruct')");

			lua.safe_script("assert(strct.x == 999)");
			lua.safe_script("assert(strct.msg == 'cat')");
			lua.safe_script("assert(strct2.x == 999)");
			lua.safe_script("assert(strct2.msg == 'cat')");

			lua.safe_script("strct2.x = 555");
			lua.safe_script("strct2.msg = 'doge'");
			lua.safe_script("assert(strct2.x == 555)");
			lua.safe_script("assert(strct2.msg == 'doge')");
			lua.safe_script("assert(strct.x == 555)");
			lua.safe_script("assert(strct.msg == 'doge')");
			
			lua.safe_script("strct = nil");
			lua.safe_script("assert(strct2.x == 555)");
			lua.safe_script("assert(strct2.msg == 'doge')");
			
			lua.safe_script("strct2 = nil");
			lua.safe_script("success = nil");
			
			lua.collect_garbage();
		}
		lua["obj"] = sol::nil;
		obj->ConditionalBeginDestroy();
		lua.collect_garbage();
		return true;
	}

	bool UObjectScriptValues(sol::state_view& lua)
	{
		lua.safe_script("obj = NewObject('UUnrealLuaTestObject')");
		
		lua.safe_script("assert(utype(obj) == 'UUnrealLuaTestObject')");
		lua.safe_script("assert(utype(obj, true) == 'UUnrealLuaTestObject')");
		UUnrealLuaTestObject* obj = lua["obj"];
		verify(IsValid(obj));

		FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(obj);
		
		{
			//bool
			FLuaScriptValue* value = item.GetLuaScriptValue("lua_bool");
			verify(value == nullptr);
			lua.safe_script("assert(obj.lua_bool == nil)");
		
			lua.safe_script("obj.lua_bool = true");
			lua.safe_script("assert(obj.lua_bool == true)");
			lua.safe_script("assert(utype(obj.lua_bool) == 'boolean')");
		
			value = item.GetLuaScriptValue("lua_bool");
			verify(value != nullptr);
			verify(value->HasInitializedValue());
			verify(!value->IsNil());
			verify(value->IsType<bool>());
			verify(value->IsValue<bool>(true));

			lua.safe_script("obj.lua_bool = nil");
			lua.safe_script("assert(obj.lua_bool == nil)");

			value = item.GetLuaScriptValue("lua_bool");
			verify(value == nullptr || value->IsNil());

			lua.safe_script("obj.lua_bool = false");
			lua.safe_script("assert(obj.lua_bool == false)");
			lua.safe_script("assert(utype(obj.lua_bool) == 'boolean')");
			
			value = item.GetLuaScriptValue("lua_bool");
			verify(value != nullptr);
			verify(value->HasInitializedValue());
			verify(!value->IsNil());
			verify(value->IsType<bool>());
			verify(value->IsValue<bool>(false));
			
			lua.safe_script("obj.lua_bool = nil");
			value = item.GetLuaScriptValue("lua_bool");
			verify(value == nullptr || value->IsNil());
		}
		
		{
			//int
			FLuaScriptValue* value = item.GetLuaScriptValue("lua_int");
			verify(value == nullptr);
			lua.safe_script("assert(obj.lua_int == nil)");
		
			lua.safe_script("obj.lua_int = 123");
			lua.safe_script("assert(obj.lua_int == 123)");
			lua.safe_script("assert(utype(obj.lua_int) == 'int64')");
		
			value = item.GetLuaScriptValue("lua_int");
			verify(value != nullptr);
			verify(value->HasInitializedValue());
			verify(!value->IsNil());
			verify(value->IsType<int64>());
			verify(value->IsValue<int64>(123));
			
			lua.safe_script("obj.lua_int = nil");
			lua.safe_script("assert(obj.lua_int == nil)");

			value = item.GetLuaScriptValue("lua_int");
			verify(value == nullptr || value->IsNil());
		}

		{
			//double
			FLuaScriptValue* value = item.GetLuaScriptValue("lua_double");
			verify(value == nullptr);
			lua.safe_script("assert(obj.lua_double == nil)");
		
			lua.safe_script("obj.lua_double = 123.4");
			lua.safe_script("assert(obj.lua_double == 123.4)");
			lua.safe_script("assert(utype(obj.lua_double) == 'double')");
		
			value = item.GetLuaScriptValue("lua_double");
			verify(value != nullptr);
			verify(value->HasInitializedValue());
			verify(!value->IsNil());
			verify(value->IsType<double>());
			verify(value->IsValue<double>(123.4));
			
			lua.safe_script("obj.lua_double = nil");
			lua.safe_script("assert(obj.lua_double == nil)");

			value = item.GetLuaScriptValue("lua_double");
			verify(value == nullptr || value->IsNil());
		}

		{
			//TArray
			FLuaScriptValue* value = item.GetLuaScriptValue("lua_array");
			verify(value == nullptr);
			lua.safe_script("assert(obj.lua_array == nil)");
		
			lua.safe_script("obj.lua_array = TArray('int32')");
		
			value = item.GetLuaScriptValue("lua_array");
			verify(value != nullptr);
			verify(value->HasInitializedValue());
			verify(!value->IsNil());
			verify(value->IsType<FLuaArray>());
			//verify(value->IsValue<FLuaArray>(123.4));
			FLuaArray& arr = value->GetMutable<FLuaArray>();
			verify(arr.Num() == 0);
			
			lua.safe_script("arr = obj.lua_array");
			lua.safe_script("assert(utype(arr, true) == 'TArray<int32>')");
			lua.safe_script("assert(arr:IsEmpty())");
			lua.safe_script("assert(arr:Num() == 0)");
			
			lua.safe_script("arr:Add(123)");
			lua.safe_script("assert(arr:Num() == 1)");
			verify(arr.Num() == 1);
			lua.safe_script("arr:Contains(123)");
			lua.safe_script("assert(arr[0] == 123)");
			int32* val = reinterpret_cast<int32*>(arr.GetData(0));
			verify(val != nullptr);
			verify(*val == 123);			

			lua.safe_script("arr2 = obj.lua_array");
			lua.safe_script("arr2:Contains(123)");
			lua.safe_script("assert(arr2:Num() == 1)");
			lua.safe_script("assert(arr2[0] == 123)");
			
			lua.safe_script("arr2:Add(456)");
			verify(arr.Num() == 2);
			lua.safe_script("arr:Contains(456)");
			lua.safe_script("assert(arr:Num() == 2)");
			lua.safe_script("assert(arr[1] == 456)");

			arr.Clear();
			verify(arr.Num() == 0)
			lua.safe_script("assert(arr:Num() == 0)");
			lua.safe_script("assert(arr2:Num() == 0)");

			lua.safe_script("arr = nil");
			lua.safe_script("arr2 = nil");
			lua.safe_script("obj.lua_array = nil");
			lua.safe_script("assert(obj.lua_array == nil)");

			value = item.GetLuaScriptValue("lua_array");
			verify(value == nullptr || value->IsNil());
		}

		{
			//TArray2
			FLuaScriptValue* value = item.GetLuaScriptValue("lua_array");
			verify(value == nullptr || value->IsNil());
			lua.safe_script("assert(obj.lua_array == nil)");
			lua.safe_script("obj.lua_array = TArray('int32')");
		
			value = item.GetLuaScriptValue("lua_array");
			verify(value != nullptr);
			verify(value->HasInitializedValue());
			verify(!value->IsNil());
			verify(value->IsType<FLuaArray>());
			//verify(value->IsValue<FLuaArray>(123.4));
			FLuaArray& arr = value->GetMutable<FLuaArray>();
			verify(arr.Num() == 0);
			
			lua.safe_script("arr = obj.lua_array");
			lua.safe_script("assert(utype(arr, true) == 'TArray<int32>')");
			lua.safe_script("assert(arr:IsEmpty())");
			lua.safe_script("assert(arr:Num() == 0)");
			
			lua.safe_script("arr:Add(123)");
			lua.safe_script("arr:Add(456)");
			lua.safe_script("arr:Add(789)");
			lua.safe_script("assert(arr:Num() == 3)");
			verify(arr.Num() == 3);
			lua.safe_script("assert(arr:Contains(123))");
			lua.safe_script("assert(arr[0] == 123)");
			int32* val = reinterpret_cast<int32*>(arr.GetData(0));
			verify(val != nullptr);
			verify(*val == 123);			

			//Create new array<int32>
			lua.safe_script("arr2 = TArray('int32')");
			lua.safe_script("arr2:Add(111)");
			lua.safe_script("arr2:Add(222)");
			lua.safe_script("assert(arr2:Num() == 2)");

			//replace array in LuaScriptValue
			lua.safe_script("obj.lua_array = arr2");
			//array should have been replaced
			verify(arr.Num() == 2);
			val = reinterpret_cast<int32*>(arr.GetData(0));
			verify(val != nullptr);
			verify(*val == 111);			

			//FLuaScriptValue should have made a reference to arr, sharing the values
			lua.safe_script("arr2:Add(333)");
			lua.safe_script("assert(arr2:Num() == 3)");
			verify(arr.Num() == 3);
			val = reinterpret_cast<int32*>(arr.GetData(2));
			verify(val != nullptr);
			verify(*val == 333);
			

			lua.safe_script("arr = nil");
			lua.safe_script("arr2 = nil");

			
			lua.safe_script("arr3 = obj.lua_array");
			lua.safe_script("assert(arr3:Num() == 3)");
			lua.safe_script("assert(arr3[2] == 333)");
			
			lua.safe_script("obj.lua_array = nil");
			lua.safe_script("assert(obj.lua_array == nil)");

			lua.safe_script("assert(arr3 ~= nil)");
			lua.safe_script("assert(arr3:Num() == 3)");
			lua.safe_script("assert(arr3[2] == 333)");
			lua.safe_script("arr3:Add(444)");
			lua.safe_script("assert(arr3:Num() == 4)");
			lua.safe_script("assert(arr3[3] == 444)");
			
			lua.safe_script("arr3 = nil");

			value = item.GetLuaScriptValue("lua_array");
			verify(value == nullptr || value->IsNil());
		}

		lua["obj"] = sol::nil;
		obj->ConditionalBeginDestroy();
		
		return true;
	}
}
bool FLuaConversionTests::TestUObjects(sol::state_view& lua)
{
	LUA_LOG("Testing Lua UObject")
	UnrealLua::SelfTests::UObjectBasics(lua);
	lua.collect_garbage();
	UnrealLua::SelfTests::UObjectScriptConstruction(lua);
	lua.collect_garbage();
	UnrealLua::SelfTests::UObjectProperties(lua);
	lua.collect_garbage();
	UnrealLua::SelfTests::UObjectScriptValues(lua);
	lua.collect_garbage();
	UnrealLua::SelfTests::UFunctionOutRefs(lua);
	lua.collect_garbage();
	UnrealLua::SelfTests::UFunctionArgs(lua);
	lua.collect_garbage();
	return true;
}