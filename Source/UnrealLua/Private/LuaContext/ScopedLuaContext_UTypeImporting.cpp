
#include "Utility/LuaLogMacros.h"
#include "LuaContext/ScopedLuaContext.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/UserDefinedEnum.h"
#include "LuaCallHelpers/LuaScriptOverrideCalls.h"
#include "LuaCallHelpers/LuaScriptRPCCalls.h"
#include "LuaTypes/LuaUsertypes.h"
#include "BlueprintSupport/UnrealLuaGameplayStatics.h"
#include "LuaTypes/LuaPrimitives.h"
#include "Reflection/FunctionDescr.h"
#include "Input/LuaStateInputHandler.h"
#include "UnrealLua.h"
#include "LuaContext/LuaImportRegistry.h"
#include "sol/sol.hpp"

struct FLuaUEnumMapping;

void FScopedLuaContext::SetupUnrealTypes()
{
	LUA_LOG("Lua Context : Setting up global utility functions")

	
	this->RegistryTable["require"] = [this](std::string str, bool bAllowModding = true, bool bTrackTable = false){ return this->ImportLuaScript(str, bAllowModding, bTrackTable); };

	this->RegistryTable["IsValid"] = FScopedLuaContext::IsUObjectValid;
	this->RegistryTable["super"] = sol::resolve<int(lua_State*)>(UnrealLua::LuaScriptCall::SuperCall);
	this->RegistryTable["Super"] = sol::resolve<int(lua_State*)>(UnrealLua::LuaScriptCall::SuperCall);
	this->RegistryTable["rpc"] = UnrealLua::LuaScriptCall::RPCCall;
	this->RegistryTable["Rpc"] = UnrealLua::LuaScriptCall::RPCCall;
	this->RegistryTable["is"] = FLuaUsertypes::Is;
	this->RegistryTable["Is"] = FLuaUsertypes::Is;
	this->RegistryTable["mixin"] = [this](sol::stack_object mixinPath, sol::this_state lua){return this->MixinScript(mixinPath, lua);};
	this->RegistryTable["Mixin"] = [this](sol::stack_object mixinPath, sol::this_state lua){return this->MixinScript(mixinPath, lua);};
	this->RegistryTable["run"] = [this](sol::stack_object runPath, sol::variadic_args args){ return this->RunScript(runPath, args);};
	this->RegistryTable["Run"] = [this](sol::stack_object runPath, sol::variadic_args args){ return this->RunScript(runPath, args);};
	
	this->RegistryTable["NewObject"] = [](sol::variadic_args args)
	{
		UUnrealLuaGameplayStatics* statics = UUnrealLuaGameplayStatics::StaticClass()->GetDefaultObject<UUnrealLuaGameplayStatics>();
		UFunction* newObjectFunc = statics->FindFunctionChecked("NewObject"); 
		FFunctionDescr f{newObjectFunc};
		return f.PerformCall(statics, args);
	};

	this->RegistryTable["RenameObject"] = [](sol::variadic_args args)
	{
		UUnrealLuaGameplayStatics* statics = UUnrealLuaGameplayStatics::StaticClass()->GetDefaultObject<UUnrealLuaGameplayStatics>();
		UFunction* renameObjectFunc = statics->FindFunctionChecked("RenameObject"); 
		FFunctionDescr f{renameObjectFunc};
		return f.PerformCall(statics, args);
	};
	
	this->RegistryTable["SpawnActor"] = [](sol::variadic_args args)
    {
    	UUnrealLuaGameplayStatics* statics = UUnrealLuaGameplayStatics::StaticClass()->GetDefaultObject<UUnrealLuaGameplayStatics>();
    	UFunction* spawnActorFunc = statics->FindFunctionChecked("SpawnActor"); 
    	FFunctionDescr f{spawnActorFunc};
    	return f.PerformCall(statics, args);
    };

	this->RegistryTable["CreateWidget"] = [](sol::variadic_args args)
	{
		UUnrealLuaGameplayStatics* statics = UUnrealLuaGameplayStatics::StaticClass()->GetDefaultObject<UUnrealLuaGameplayStatics>();
		UFunction* spawnActorFunc = statics->FindFunctionChecked("CreateWidget"); 
		FFunctionDescr f{spawnActorFunc};
		return f.PerformCall(statics, args);
	};
	
	this->RegistryTable["Input"] = Cast<UObject>(this->PlayerInputHandler.Get());
	
	this->RegistryTable["utype"] = &UnrealLua::LuaTypes::TypeInfo::UType_Stack;
	this->RegistryTable["Utype"] = &UnrealLua::LuaTypes::TypeInfo::UType_Stack;

	FLuaImportRegistry& importRegistry = FLuaImportRegistry::Get();
	
	importRegistry.InitializeLuaContext(*this);
}
