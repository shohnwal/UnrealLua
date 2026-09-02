#include "UnrealLuaModule.h"
#include "LuaCoreDelegates.h"

#define LOCTEXT_NAMESPACE "FUnrealLuaModule"

void FUnrealLuaModule::StartupModule()
{
	UE_LOG(LogTemp, Warning, TEXT("Startup UnrealLua Module"));
}

void FUnrealLuaModule::ShutdownModule()
{
	UE_LOG(LogTemp, Warning, TEXT("Shutdown UnrealLua Module"));
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FLuaCoreDelegates::OnRegisterLuaUsertypes.Clear();
}

bool FUnrealLuaModule::SupportsDynamicReloading()
{
	return false;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUnrealLuaModule, UnrealLua)
