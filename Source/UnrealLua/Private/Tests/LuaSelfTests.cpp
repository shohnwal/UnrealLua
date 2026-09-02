#include "Tests/LuaSelfTests.h"

#include "Utility/LuaLogMacros.h"
#include "Engine/Engine.h"
#include "Interface/LuaContext.h"
#include "LuaContext/ScopedLuaContext.h"
#include "LuaContext/StandaloneLuaContext.h"
#include "Misc/MessageDialog.h"
#include "Tests/LuaSelfTestTypes.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"

namespace UnrealLua::SelfTest
{
	static FLuaStateViewBroadcastDelegate UnrealLuaSelfTestDelegate = {};

	EAppReturnType::Type DisplayErrorBox(const FString& error)
	{
		const char* title = "UnrealLua Self Test Error";
		
		FWideStringBuilderBase errorStr{};
		
		errorStr << "Error during UnrealLua self test:\n";
		errorStr << error;
		
		const char* instructions = R"###(Please notify the plugin developer.
		Press 'ok' to close program)###";
		
		FString errorDetails = errorStr.ToString();
		
		FText txt = FText::FromString(FString::Printf(TEXT("%hs\n--------------\n%s\n--------------\n%hs"), title, *errorDetails, instructions));
		const EAppReturnType::Type clicked = FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, txt);

		return clicked;    
	}

	sol::protected_function_result NotifySelfTestError(lua_State* L, sol::protected_function_result pfr)
	{
		if (!pfr.valid())
		{
			sol::error err = pfr;
			std::string_view strv = err.what();
			EAppReturnType::Type clicked = DisplayErrorBox(strv.data());
			verify(clicked == EAppReturnType::Ok);
			RequestEngineExit("Error during UnrealLua self test");
		}
		return pfr;
	}

	FDelegateHandle AddTestCategoryCallback(TFunction<void(sol::state_view&)> callback)
	{
		return UnrealLuaSelfTestDelegate.AddLambda(callback);
	}
}

bool UTestScriptStructLibrary::TestFunc(FTestScriptStruct& strct)
{
	//LUA_LOG("TestFunc : %s %d",*strct.msg, strct.x)
	strct.msg = "meow";
	strct.x = 999;
	return true;
}

void UUnrealLuaTestObject::BeginDestroy()
{
	//LUA_LOG("Destroying TestObject %s", *this->GetName())
	UObject::BeginDestroy();
}

void UUnrealLuaTestObject::OnClusterMarkedAsPendingKill()
{
	//LUA_LOG("Cluster marked as pending kill %s", *this->GetName())
	UObject::OnClusterMarkedAsPendingKill();
}

void ULuaScriptableTestObject::OnMultiDelegate(bool b, int32 i, float f, FString str, FTestScriptStruct strct, UObject* obj)
{
	//LUA_LOG("%s received multicast delegate data : %d, %d, %f, %s, {%d, %s}, %s", *GetNameSafe(this), (int32)b, i, f, *str, strct.x, *strct.msg, *GetNameSafe(obj));
	msg = str;
}

void ULuaScriptableTestObject::OnSingleDelegate(bool b, int32 i, float f, FString str, FTestScriptStruct strct, UObject* obj)
{
	//LUA_LOG("%s received single delegate data : %d, %d, %f, %s, {%d, %s}, %s", *GetNameSafe(this), (int32)b, i, f, *str, strct.x, *strct.msg, *GetNameSafe(obj));
	msg = str;
}


bool UnrealLua::SelfTest::PerformSelfTest()
{
	LUA_LOG("===Performing Lua self check===");
	
	UUnrealLuaEngineSubsystem::Get()->UObjectRegistry->UClassOverrideRegistry.DisableUFunctionOverriding();

	UStandaloneLuaContext* textContext = NewObject<UStandaloneLuaContext>(GetTransientPackage());
	textContext->InitializeLuaStateAndLoadGameMode(ELuaContextType::SelfTest, "SelfTextContext", NAME_None);
	bool allOk = textContext->GetScopedLuaContext().PerformSelfTest(textContext);
	//this will call UUnrealLuaEngineSubsystem::Get()->NotifyEndGameSession(textContext);
	textContext->ConditionalBeginDestroy();
	
	UUnrealLuaEngineSubsystem::Get()->UObjectRegistry->UClassOverrideRegistry.EnableUFunctionOverriding();
	
	LUA_LOG("===Lua self check concluded===");
	return allOk;
}

bool UnrealLua::SelfTest::TEST(sol::state_view& lua, const char* content)
{
	//sol::protected_function_result res = lua.safe_script(content, UnrealLua::SelfTest::NotifySelfTestError);
	sol::protected_function_result res = lua.safe_script(content);
	if (!res.valid())
	{
		sol::error err = res;
		std::string_view what = err.what();
		LUA_LOG_ERROR("Error during self test: %hs", what.data())
		return false;
	}
	return true;
}

namespace UnrealLua::SelfTest
{
	bool TestLuaDelegates(sol::state_view& lua);
}

bool FLuaConversionTests::Test(TScriptInterface<ILuaContext>& ctx)
{
	verify(UUnrealLuaEngineSubsystem::IsGameSessionActive())
	sol::state_view lua = ctx->GetScopedLuaContext().GetLuaState();

	UnrealLua::SelfTest::UnrealLuaSelfTestDelegate.Broadcast(lua);
	if(!TestUEnums(lua))
	{
		return false;
	}
	if(!TestScriptStructs(lua))
	{
   		return false;
	}
	if(!TestSharedStructs(lua))
	{
		return false;
	}
	if(!TestInstancedStructs(lua))
	{
		return false;
	}
	if(!TestStrings(lua))
	{
		return false;
	}
	if(!TestNames(lua))
	{
		return false;
	}
	if(!TestText(lua))
	{
		return false;
	}
	if(!TestArrays(lua))
	{
		return false;
	}
	if(!TestSets(lua))
	{
		return false;
	}
	if(!TestMaps(lua))
	{
		return false;
	}
	if(!TestDelegates(ctx))
	{
		return false;
	}
	if(!TestUObjects(lua))
	{
		return false;
	}
	//@TODO : test instanced structs/shared structs
	//@TODO : Setting entire map/array/set
	//@TODO : UPROPERTY getting of Structs/shared structs (should get a reference) instanced structs (should get copy)
	//@TODO : Function arg passing (by value, by ref)
	//@TODO : Function result passing (UObject/FStruct-out params and return)
	
	if (!UnrealLua::SelfTest::TestLuaDelegates(lua))
	{
		return false;
	}
	return true;
}

