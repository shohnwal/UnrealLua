using System.IO;
using UnrealBuildTool;

public class UnrealLuaVM : ModuleRules
{
    public UnrealLuaVM(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.NoPCHs;
        CppStandard = CppStandardVersion.Latest;
        
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Core"
            }
        );

        //Lua.h files open to public
        string luaHeadersPath = Path.Combine(ModuleDirectory, "Public/LuaVM/");
        PublicIncludePaths.Add(luaHeadersPath);
        PublicIncludePaths.Add(luaHeadersPath + "Utility");
        
        
        //sol.h files open to public
        string solHealersPath = Path.Combine(ModuleDirectory, "Public/sol/");
        PublicIncludePaths.Add(solHealersPath);
        
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicAdditionalLibraries.Add(Path.Combine(luaHeadersPath, "lua-static.lib"));
            //PublicDefinitions.Add(new string("LUA_USE_WINDOWS"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            PublicAdditionalLibraries.Add(Path.Combine(luaHeadersPath, "liblua.a"));
            //PublicDefinitions.Add(new string("LUA_USE_LINUX"));
        }
        else
        {
            throw new System.Exception("Unsupported platform: " + Target.Platform.ToString());
        }
        
        //By default, Lua 5.5 is used. Set to true to use LuaJit 2.1 instead
        //However, note that not all tests or features might be compatible with LuaJit,
        //such as ipairs or pairs with TArray<>, TSet<> and TMap<>
        //Also:
        //- Testing has shown that UnrealLua using Lua's C-API to access UProperties and UFunctions does not really give LuaJit
        //  a good moment to gain any big advantage over normal Lua 5.5
        //- We are using Unreal's memory pooling system to malloc userdata memory, so LuaJit has no advantage here either
        //- enabling Jit has even shown to cause slowdowns by a few fps, so using normal Lua is recommended

        bool bUseLuaJit = false;
        if (bUseLuaJit)
        {
            PublicDefinitions.Add(new string("SOL_LUAJIT"));
        }
    }
}