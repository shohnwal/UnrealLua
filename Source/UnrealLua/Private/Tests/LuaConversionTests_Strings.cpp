
#include "Tests/LuaSelfTests.h"
#include "LuaStackHandler/LuaStackHandler.h"

bool FLuaConversionTests::TestStrings(sol::state_view& lua)
{
	static const char* key = "testkey";

	FString str{"test"};
	lua[key] = str;

	lua.safe_script("assert(utype(testkey) == 'string')");
	lua.safe_script("assert(utype(testkey, true) == 'string')");
	
	sol::object checkObj = lua[key];
	verify(checkObj.get_type() == sol::type::string);

	FString result = lua[key];
	verify(result == str);

	lua.safe_script("testkey = testkey .. 'yay'");
	/*
	lua.safe_script("print(testkey)" 
		 "testkey = testkey .. \"yay\""
		 "print(testkey)"
		 );
	*/
	FString result2 = lua[key];
	str += "yay";
	verify(result2 == str);
	
	lua[key] = sol::nil;

	lua.collect_garbage();
	
	return true;
}

bool FLuaConversionTests::TestNames(sol::state_view& lua)
{
	static const char* key = "testkey";

	FName str{NAME_Actor};
	lua[key] = str;

	lua.safe_script("assert(utype(testkey) == 'string')");
	lua.safe_script("assert(utype(testkey, true) == 'string')");
	
	sol::object checkObj = lua[key];
	verify(checkObj.get_type() == sol::type::string);

	FName result = lua[key];
	verify(result == str);

	/*
	lua.safe_script("print(testkey)" 
		 "testkey = testkey .. \"yay\""
		 "print(testkey)"
		 );
	*/
	lua.safe_script("testkey = testkey .. 'yay'");
	
	FName result2 = lua[key];
	str = *(str.ToString() + "yay");
	verify(result2 == str);

	lua[key] = sol::nil;

	lua.collect_garbage();

	return true;
}

bool FLuaConversionTests::TestText(sol::state_view& lua)
{
	static const char* key = "testkey";

	FText str = FText::FromString("test");
	lua[key] = str;

	lua.safe_script("assert(utype(testkey) == 'string')");
	lua.safe_script("assert(utype(testkey, true) == 'string')");
	
	sol::object checkObj = lua[key];
	verify(checkObj.get_type() == sol::type::string);

	FText result = lua[key];
	verify(result.EqualTo(str));

	/*
	lua.safe_script("print(testkey)" 
		 "testkey = testkey .. \"yay\""
		 "print(testkey)"
		 );
	*/
	lua.safe_script("testkey = testkey .. 'yay'");
	
	FText result2 = lua[key];
	str = FText::FromString(*(str.ToString() + "yay"));
	verify(result2.EqualTo(str));

	lua[key] = sol::nil;

	lua.collect_garbage();

	return true;
}

