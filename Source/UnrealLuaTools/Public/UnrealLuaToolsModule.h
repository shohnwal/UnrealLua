#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FUnrealLuaToolsModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    
    static void NotifyAllModuleLoadingPhasesComplete();
};
