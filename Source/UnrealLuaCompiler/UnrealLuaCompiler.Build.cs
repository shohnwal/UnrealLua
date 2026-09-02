using UnrealBuildTool;

public class UnrealLuaCompiler : ModuleRules
{
    public UnrealLuaCompiler(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "UnrealLuaVM",
                "UnrealLua",
                "Engine",
                "EnhancedInput",
                "Projects"
            }
        );
        bEnableExceptions = true;
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.Add("UnrealEd");
        }

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore", "UnrealLuaFileSystem",
                
            }
        );
    }
}