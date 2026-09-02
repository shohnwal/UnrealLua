#include "Tests/LuaSelfTests.h"
#include "Interface/LuaContext.h"
#include "LuaContext/ScopedLuaContext.h"
#include "Utility/LuaLogMacros.h"


bool FLuaConversionTests::TestDelegates(const TScriptInterface<ILuaContext>& ctx)
{
	LUA_LOG("Testing Lua Delegates")
	UObject* contextObj = ctx.GetObject();
	sol::state_view lua = ctx->GetScopedLuaContext().GetLuaState();

	lua.safe_script(R"###(_delHost = NewObject("UUnrealLuaTestObject"))###");
	lua.safe_script(R"###(assert(_delHost ~= nil))###");
	//lua.safe_script(R"###(print(tostring(_delHost)))###");
	lua.safe_script(R"###(assert(utype(_delHost) == "UUnrealLuaTestObject"))###");
	
	lua.safe_script(R"###(_delSub = NewObject("ULuaScriptableTestObject"))###");
	lua.safe_script(R"###(assert(_delSub ~= nil))###");
	lua.safe_script(R"###(assert(utype(_delSub) == "ULuaScriptableTestObject"))###");

	lua.safe_script(R"###(_delSub2 = NewObject("ULuaScriptableTestObject"))###");
	lua.safe_script(R"###(assert(_delSub2 ~= nil))###");
	lua.safe_script(R"###(assert(utype(_delSub2) == "ULuaScriptableTestObject"))###");

	lua.safe_script(R"###(_delHost.SingleDelegate:Bind(_delSub, "OnSingleDelegate"))###");
	lua.safe_script(R"###(assert(_delHost.SingleDelegate:IsBound()))###");
	lua.safe_script(R"###(_delHost.SingleDelegate:Execute(true, 123, 3.45, "meow", {"hmm", 987}, _delHost ))###");
	lua.safe_script(R"###(_delHost.SingleDelegate:Unbind())###");
	lua.safe_script(R"###(assert(not _delHost.SingleDelegate:IsBound()))###");

	lua.safe_script(R"###(_delHost.MultiDelegate:Add(_delSub, "OnMultiDelegate"))###");
	lua.safe_script(R"###(_delHost.MultiDelegate:Add(_delSub2, "OnMultiDelegate"))###");
	lua.safe_script(R"###(_delHost.MultiDelegate:Broadcast(true, 123, 3.45, "meow", {"hmm", 987}, _delHost ))###");
	lua.safe_script(R"###(assert(_delSub.msg == "meow"))###");
	lua.safe_script(R"###(assert(_delSub2.msg == "meow"))###");
	lua.safe_script(R"###(_delHost.MultiDelegate:Remove(_delSub, "OnMultiDelegate"))###");
	lua.safe_script(R"###(_delHost.MultiDelegate:Broadcast(true, 123, 3.45, "barf", {"hmm", 987}, _delHost ))###");
	lua.safe_script(R"###(assert(_delSub.msg == "meow"))###");
	lua.safe_script(R"###(assert(_delSub2.msg == "barf"))###");
	lua.safe_script(R"###(_delHost.MultiDelegate:Remove(_delSub2, "OnMultiDelegate"))###");

	//lua.safe_script(R"###(print(tostring(_delHost)))###");
	lua.safe_script(R"###(_delHost = nil)###");
	lua.safe_script(R"###(assert(_delHost == nil))###");
	lua.safe_script(R"###(_delSub = nil)###");
	lua.safe_script(R"###(assert(_delSub == nil))###");
	lua.safe_script(R"###(_delSub2 = nil)###");
	lua.safe_script(R"###(assert(_delSub2 == nil))###");

	lua.collect_garbage();

	return true;
}
