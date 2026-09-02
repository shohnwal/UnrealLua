// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaContext/LuaContextTypes.h"

const char* enumFileContent = R"###("-- Extendible UEnum defining LuaContextID
--With .lua file extension:
--Create new Enum or add to existing one
--String determines subpath to folder containing the file
--Expects table with <string,number> pairs as return,
--will round to int64 for values
UENUM("ELuaContextID", { Package = "/Script/UnrealLua", Extend = true, Default = "Default" }) 
"UnrealTypes/Default/LuaContextID.lua"

local AdditionalIDs = 
{
	Default = 0,
}
return AdditionalIDs
)###";

void UnrealLua::LuaContext::GenerateLuaContextUTypeFiles()
{
	
}