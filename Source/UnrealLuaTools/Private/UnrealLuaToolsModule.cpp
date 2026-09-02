#include "UnrealLuaToolsModule.h"

#include "Misc/CoreDelegates.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"
#include "Utility/WidgetStyles.h"

#define LOCTEXT_NAMESPACE "FUnrealLuaToolsModule"

void FUnrealLuaToolsModule::StartupModule()
{
	FCoreDelegates::OnAllModuleLoadingPhasesComplete.AddStatic(&FUnrealLuaToolsModule::NotifyAllModuleLoadingPhasesComplete);
}

void FUnrealLuaToolsModule::ShutdownModule()
{
    
}

void FUnrealLuaToolsModule::NotifyAllModuleLoadingPhasesComplete()
{
	UnrealLuaTools::SlateStyles::Initialize();
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FUnrealLuaToolsModule, UnrealLuaTools)