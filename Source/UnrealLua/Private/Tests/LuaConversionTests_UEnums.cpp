
#include "Utility/LuaLogMacros.h"
#include "Tests/LuaSelfTests.h"
#include "Tests/LuaSelfTestTypes.h"

namespace UnrealLua::SelfTests
{
	bool UEnumBasics(sol::state_view& lua)
	{
		SelfTest::FLuaKeyScopeGuard guard(lua, "uenum");
		UNREALLUA_TEST_STEP("assert(EUnrealLuaTestEnum ~= nil)")
		//UNREALLUA_TEST_STEP("print(tostring(EUnrealLuaTestEnum))");
		//UNREALLUA_TEST_STEP("print(utype(EUnrealLuaTestEnum))");
		UNREALLUA_TEST_STEP("assert(tostring(EUnrealLuaTestEnum) == 'EUnrealLuaTestEnum')");
		UNREALLUA_TEST_STEP("assert(tostring(EUnrealLuaTestEnum.One) == 'EUnrealLuaTestEnum::One', tostring(EUnrealLuaTestEnum.One))");
		UNREALLUA_TEST_STEP("assert(tostring(EUnrealLuaTestEnum.DoesntExist) == 'nil')");
		UNREALLUA_TEST_STEP("assert(tostring(EUnrealLuaTestEnum[1]) == 'EUnrealLuaTestEnum::Two')");
		UNREALLUA_TEST_STEP("assert(tostring(EUnrealLuaTestEnum[1000000]) == 'nil')");
		
		UNREALLUA_TEST_STEP("uenum = EUnrealLuaTestEnum");
		UNREALLUA_TEST_STEP("assert(uenum ~= nil)");
		UNREALLUA_TEST_STEP("assert(tostring(uenum) == 'EUnrealLuaTestEnum')");
		UNREALLUA_TEST_STEP("assert(tostring(uenum.One == 'EUnrealLuaTestEnum::One'))");
		UNREALLUA_TEST_STEP("assert(tostring(uenum[1]) == 'EUnrealLuaTestEnum::Two')");
		
		UNREALLUA_TEST_STEP("assert(tostring(uenum.One == EUnrealLuaTestEnum.One))");
		UNREALLUA_TEST_STEP("assert(uenum[1] == EUnrealLuaTestEnum[1])");
		UNREALLUA_TEST_STEP("assert(EUnrealLuaTestEnum.DoesntExist == nil)");

		UNREALLUA_TEST_STEP("uenum = nil");
		return true;
	}
}

bool FLuaConversionTests::TestUEnums(sol::state_view& lua)
{
	LUA_LOG("Testing Lua Enum")
	return UnrealLua::SelfTests::UEnumBasics(lua);
}
