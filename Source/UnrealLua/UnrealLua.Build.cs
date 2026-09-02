// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;
using EpicGames.Core;

public class UnrealLua : ModuleRules
{
    public UnrealLua(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseSharedPCHs;
        CppStandard = CppStandardVersion.Latest;
        //PCHUsage = PCHUsageMode.NoSharedPCHs;
        PrivatePCHHeaderFile = "UnrealLuaPch.h";
        //PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        
        if (Target.Version.MajorVersion >= 5) //Unreal 5
        {
            if (Target.Version.MinorVersion >= 6)  //Unreal 5.6 and later
            {
                CppCompileWarningSettings.UndefinedIdentifierWarningLevel = UnrealBuildTool.WarningLevel.Error;
            }
            else //Unreal 5.5 and earlier
            {
                UndefinedIdentifierWarningLevel = UnrealBuildTool.WarningLevel.Default;    
            }
        }
        else //Unreal 4
        {
            bEnableUndefinedIdentifierWarnings = false;
        }
        
        /*
            This would only be needed for the dynamic_cast<> in LuaLightUserdata.cpp
            However it also breaks UE in Linux: downloadable UE-Linux binaries are npt
            compiled with RTTI enabled and Linux does not allow mixed RTTI between
            modules. Getting UE to use RTTI would require recompiling the entire engine,
            which is not user-friendly. Sol2 does not need RTTI 
            https://sol2.readthedocs.io/en/latest/rtti.html
            So unless we absolutely NEED to use the lightuserdata-approach, which would require
            any Linux user of UnrealLua to compile the entire engine with RTTI enabled,
            we should keep this off.
            
            If we turn this on now, clang would complain with "missing typeinfo for AActor"-errors, etc
        */
        //bUseRTTI = true;
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", 
                "CoreUObject", 
                "InputCore",                 
                "EnhancedInput", 
                "DeveloperSettings", 
                "Engine",
                "Slate", 
                "SlateCore", 
                "UMG",
                "NetCore",
                //"AssetRegistry",
                //"Launch",
                "UnrealLuaVM",
            }
        );
        
        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "EngineSettings",
            "ApplicationCore",
            "UnrealLuaFileSystem"
        });

        if (Target.Version.MajorVersion == 5 && Target.Version.MinorVersion < 8)
        {
            PublicDependencyModuleNames.Add("StructUtils");
        }
        
        if (Target.bBuildEditor)
        {
            //PrivateDependencyModuleNames.Add("ScriptDisassembler");
            //PrivateDependencyModuleNames.Add("BlueprintGraph");
           // PrivateDependencyModuleNames.Add("KismetCompiler");
            PrivateDependencyModuleNames.Add("UnrealEd"); //For EnumCompiler
            PublicDependencyModuleNames.Add("DeveloperToolSettings");
            //PublicDependencyModuleNames.Add("EditorSubsystem");
        }
        

        

    }
}
