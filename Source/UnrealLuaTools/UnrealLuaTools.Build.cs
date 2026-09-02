using UnrealBuildTool;

public class UnrealLuaTools : ModuleRules
{
    public UnrealLuaTools(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Latest;
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", "CoreUObject", "UnrealLua", "UnrealLuaFileSystem"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "InputCore",
                "ApplicationCore",
                "Engine",
                "Slate",
                "SlateCore",
                "UMG"
            }
        );
        
        if(Target.bCompileAgainstEditor)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[] { "UnrealEd" }
            );
        }
    }
}
