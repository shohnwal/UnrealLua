#include "LuaContext/LuaPath.h"


#include "Utility/LuaLogMacros.h"
#include "Config/UnrealLuaConstants.h"
#include "HAL/IConsoleManager.h"
#include "Misc/Paths.h"
#include "Mods/UnrealLuaMod.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"

static TAutoConsoleVariable<float> CVarLogLuaPaths(TEXT("lua.LogLuaPaths"), 1, TEXT("Whether UnrealLua should log used lua paths"));

FLuaPathElement::FLuaPathElement()
	: Path(), path()
{
	
}

FLuaPathElement::FLuaPathElement(const FString& pathStr, ELuaPathFlags flags)
{
	Set(pathStr);
	this->Locationflags = flags;
}

void FLuaPathElement::Set(const FString& str)
{
	this->Path = str;
	this->path = StringCast<char>(*str).Get();
}

bool FLuaPathElement::HasAnyFlags(ELuaPathFlags flags) const
{
	return EnumHasAnyFlags(this->Locationflags, flags);
}


FLuaPath::FLuaPath()
    : Paths({})
{
}

FLuaPath::FLuaPath(const TArray<UUnrealLuaMod*>& mods, const FName& gamemode, const ELuaPathFlags allowedPathLocation)
{
	this->SetupPackagePaths(mods, gamemode, allowedPathLocation);
}

void FLuaPath::SetPackagePath(const FString& path)
{
	this->Paths.Empty();
	FLuaPathElement element;
	element.Set(path);
	this->Paths.Add(element);
}

void FLuaPath::SetPackagePath(const TArray<FString>& paths)
{
	this->Paths.Empty();
	for (const FString& path : paths)
	{
		FLuaPathElement element;
		element.Set(path);
		this->Paths.Add(element);	
	}
}

void FLuaPath::SetupPackagePaths(const TArray<UUnrealLuaMod*>& mods, const FName& gamemode, const ELuaPathFlags allowedPathLocation)
{
	if(CVarLogLuaPaths.GetValueOnAnyThread())
	{
		LUA_LOG("Lua Context : Setting up package paths")	
	}
	
	this->UsedFlags = allowedPathLocation;
	
	this->Paths.Empty();
	
	const bool searchForMapName = false;
	const FString mapName = "";
	const FString gamemodename = *gamemode.ToString();
	
	// Content/Lua/
	const FString projectContentLuaFolder = FPaths::ProjectContentDir() + "Lua/";

	//TODO :Check each mod ini for bUseLuaScripts = true

	// /GameMode/
	const FString gameModeFolder = UnrealLua::scriptLoading::LuaGameModeFolder;

	/*
		SearchOrder = {
			Mod-GameMode-Map,
			GameMode-Map,
			Default-Map,
			Mod-GameMode,
			GameMode,
			Mod-Default,
			Default,
			Mod-Libs
			Libs,
			Mod,
			Root
		}	
		
		SearchHierarchy = {
			Map,
			GameMode,
			Mod
		}

	*/
	//Most specific paths go to the front of the array:
	//<Mod>/Lua/GameMode/<gamemode>/<MapName>/
	//Lua/GameMode/<gamemode>/<MapName>/
	//<Mod>/Lua/GameMode/<DefaultGameMode>/<MapName>/
	//<Mod>/Lua/GameMode/<gamemode>/
	//Lua/GameMode/<gamemode>/
	//<Mod>/Lua/GameMode/<DefaultGameMode>
	//Lua/GameMode/<DefaultGameMode>
	//<Mod>/Lua/Libs
	//Lua/Libs
	//<Mod>/Lua/
	//Lua/
	
	if(gamemode != NAME_None && gamemode != UnrealLua::scriptLoading::DefaultGameMode)
	{
		const ELuaPathFlags modsNamedGameMode = ELuaPathFlags::Mod | ELuaPathFlags::NamedGameMode;
		if (EnumHasAllFlags(allowedPathLocation, modsNamedGameMode))
		{
			//<Mod>/Lua/GameMode/<gamemode>/
			for (const UUnrealLuaMod* mod : mods)
			{
				FString modLuaDir = mod->GetDirectory() + "/Lua/" + gameModeFolder + "/" + gamemodename + "/";
				this->AddPath({modLuaDir, modsNamedGameMode});
			}			
		}
		const ELuaPathFlags baseGameNamedGameMode = ELuaPathFlags::BaseGame | ELuaPathFlags::NamedGameMode;
		if (EnumHasAllFlags(allowedPathLocation, baseGameNamedGameMode))
		{
			///Lua/GameMode/<gamemode>/
			this->AddPath({projectContentLuaFolder + gameModeFolder + "/" + gamemodename + "/", baseGameNamedGameMode});		
		}
	}
	
	const ELuaPathFlags modsDefaultGameMode = ELuaPathFlags::Mod | ELuaPathFlags::DefaultGameMode;
	if (EnumHasAllFlags(allowedPathLocation, modsDefaultGameMode))
	{
		for (const UUnrealLuaMod* mod : mods)
		{
			//<Mod>/Lua/GameMode/Default/
			FString modCommonDir = mod->GetDirectory() + "/Lua/" + gameModeFolder + "/" + UnrealLua::scriptLoading::DefaultGameMode + "/";
			this->AddPath({modCommonDir, modsDefaultGameMode});
		}
	}
	
	const ELuaPathFlags baseGameDefaultGameMode = ELuaPathFlags::BaseGame | ELuaPathFlags::DefaultGameMode;
	if (EnumHasAllFlags(allowedPathLocation, baseGameDefaultGameMode))
	{
		//Lua/GameMode/Default/
		this->AddPath({projectContentLuaFolder + gameModeFolder + "/" + UnrealLua::scriptLoading::DefaultGameMode + "/", baseGameDefaultGameMode});
	}
	
	const ELuaPathFlags modLibs = ELuaPathFlags::Mod | ELuaPathFlags::Lib;
	if (EnumHasAllFlags(allowedPathLocation, modLibs))
	{
		for (const UUnrealLuaMod* mod : mods)
		{
			FString modLibsDir = mod->GetDirectory() + "/Lua/Libs/";
			this->AddPath({modLibsDir, modLibs});
		}
	}
	
	const ELuaPathFlags baseGameLibs = ELuaPathFlags::BaseGame | ELuaPathFlags::Lib;
	if (EnumHasAllFlags(allowedPathLocation, baseGameLibs))
	{
		this->AddPath({projectContentLuaFolder + "Libs/", baseGameLibs});
	}
	
	const ELuaPathFlags modutypes = ELuaPathFlags::Mod | ELuaPathFlags::UnrealTypes;
	if (EnumHasAllFlags(allowedPathLocation, modutypes))
	{
		for (const UUnrealLuaMod* mod : mods)
		{
			FString modRootDir = mod->GetDirectory() + "Lua/UnrealTypes/";
			this->AddPath({modRootDir, modutypes});
		}
	}
	const ELuaPathFlags baseGameUtypes = ELuaPathFlags::BaseGame | ELuaPathFlags::UnrealTypes;
	if (EnumHasAllFlags(allowedPathLocation, baseGameUtypes))
	{
		this->AddPath({projectContentLuaFolder + "UnrealTypes/", baseGameUtypes});
	}
	
	const ELuaPathFlags modsRoot = ELuaPathFlags::Mod | ELuaPathFlags::Root;
	if (EnumHasAllFlags(allowedPathLocation, modsRoot))
	{
		for (const UUnrealLuaMod* mod : mods)
		{
			FString modRootDir = mod->GetDirectory() + "/Lua/";
			this->AddPath({modRootDir,
				modsRoot});
		}
	}
	
	//Most general path goes to the end of the array
	//Lua base folder : Lua/ 
	const ELuaPathFlags baseGameRoot = ELuaPathFlags::BaseGame | ELuaPathFlags::Root;
	if (EnumHasAllFlags(allowedPathLocation, baseGameRoot))
	{
		this->AddPath({projectContentLuaFolder, ELuaPathFlags::BaseGame | ELuaPathFlags::Root});
	}
	
	if(CVarLogLuaPaths.GetValueOnAnyThread())
	{
		LUA_LOG("Lua Paths are : ")
		for (FLuaPathElement& element : this->Paths)
		{
			LUA_LOG("Path : %s", *element.GetFString())
		}
	}
}

void FLuaPath::AddPath(const FString& path, const ELuaPathFlags pathFlags)
{
	this->Paths.Add({path, pathFlags});
}

void FLuaPath::AddPath(const FLuaPathElement& path)
{
	this->Paths.Add(path);
}
