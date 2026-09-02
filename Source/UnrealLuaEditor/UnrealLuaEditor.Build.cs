using UnrealBuildTool;

public class UnrealLuaEditor : ModuleRules
{
    public UnrealLuaEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Latest;
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", "CoreUObject",
                "UnrealLuaTools",
                "UnrealLua"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "BlueprintGraph", 
                "ToolMenus", 
                "ContentBrowser", 
                "Engine",
                "Slate",
                "SlateCore",
                "UnrealEd",
            }
        );
    }
}
