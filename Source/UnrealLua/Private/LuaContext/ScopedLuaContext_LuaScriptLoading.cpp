#include "Utility/LuaLogMacros.h"
#include "Config/UnrealLuaConstants.h"
#include "LuaContext/ScopedLuaContext.h"
#include "LuaContext/LuaScripts/LoadedLuaScriptResult.h"
#include "UnrealLua.h"
#include "sol/sol.hpp"
#include "Config/UnrealLuaConfig.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"

//OnScriptLoaded : called when lua script is loaded and patched from hdd
//OnScriptInstanced : called when a new instance of a loaded lua script is created

static TAutoConsoleVariable<bool> CVarLogLuaScriptLoading(TEXT("lua.LogScriptLoading"), 1, TEXT("Whether UnrealLua should log when attempting to load scripts from disk"));

ULoadedLuaScriptCollection* FScopedLuaContext::GetOrCreateLuaScriptCollection(const FName& fileName)
{
	ULoadedLuaScriptCollection* collection = this->LoadedScripts.FindRef(fileName);
	if(collection != nullptr)
	{
		return collection;
	}
	//attempt to load lua script
	FLoadLuaScriptResult newScriptTemplate;
	if(fileName == NAME_None)
	{
		//dummy collection for LuaScriptable objects that have no valid file name given
		newScriptTemplate = FLoadLuaScriptResult{"", true, this->LuaState.create_table(),"", {}, {}, {}};
	}
	else
	{
		const FString scriptPath = fileName.ToString();
		bool bIsAbsolutePath = scriptPath.StartsWith(UnrealLua::Paths::FullProjectDir);
		bool bAllowModding = !bIsAbsolutePath;
		newScriptTemplate = this->LoadLuaScriptFromDisk(fileName.ToString(), bIsAbsolutePath, bAllowModding);
	}
	if(!newScriptTemplate.IsValid())
	{
		//dummy collection for LuaScriptable objects that have no valid file name given
		newScriptTemplate = FLoadLuaScriptResult{"", true, this->LuaState.create_table(),"", {}, {}, {}};
	}

	//there is a valid Lua script, create a new collection
	ULoadedLuaScriptCollection* newColl = NewObject<ULoadedLuaScriptCollection>(GetTransientPackage());
	this->LoadedScripts.Add(fileName, newColl);
	this->LoadedScriptsArray.Add(newColl);
	newColl->Initialize(this, fileName, newScriptTemplate);
	return newColl;
}

FLuaScriptInstanceHandle FScopedLuaContext::GetLuaScriptHandle(const FLuaScriptSettings& scriptSettings)
{
	FScopeLock locked{&this->LuaStateLock};
	if(!this->IsLuaLoaded())
	{
		return {};
	}
	ULoadedLuaScriptCollection* collection = this->GetOrCreateLuaScriptCollection(*scriptSettings.ScriptPathOverride);

	if(!collection)
	{
		return {};
	}
	
	return FLuaScriptInstanceHandle{collection};
}

sol::table FScopedLuaContext::ImportLuaScript(const std::string_view path, bool bAllowModding, bool bTrackScript)
{
	return this->ImportLuaScript(std::string(path), bAllowModding, bTrackScript);
}

//used by lua runtime via "require" and by "mixin" and from Dynamic Enum Handler
sol::table FScopedLuaContext::ImportLuaScript(const std::string& path, bool bAllowModding, bool bTrackScript)
{
	if(!bTrackScript)
	{
		return this->LoadLuaScriptFromDisk(path, bAllowModding).FinalResult;
	}
	const FString fileNameString{path.c_str()};
	return this->ImportLuaScript(fileNameString, bAllowModding, true);
}

sol::table FScopedLuaContext::ImportLuaScript(const FString& filePath, bool bAllowModding, bool bTrackScript)
{
	if(!bTrackScript)
	{
		const std::string path = StringCast<char>(*filePath).Get();
		return this->ImportLuaScript(path, bAllowModding, false);
	}
	ULoadedLuaScriptCollection* collection = this->GetOrCreateLuaScriptCollection(*filePath);
	if(collection)
	{
		return collection->GetLuaScriptAsTable(bTrackScript);	
	}
	return sol::nil;
}

void FScopedLuaContext::MixinScript(sol::stack_object mixinPath, sol::this_state lua)
{
	if(this->ImportStack.IsEmpty())
	{
		return;
	}
	if (!mixinPath.is<std::string>())
	{
		return;
	}
	
	std::string_view mixinPathStr = mixinPath.as<std::string_view>();
	if (mixinPathStr.empty())
	{
		//use the same path as the current master import path
		return;
	}
	
	sol::table targetTable = this->ImportStack.ImportStacks.Last()->importedTable;
	verify(targetTable.valid())
	std::string path{};
	if (mixinPathStr.starts_with('+'))
	{
		//Append mixin path to master import path, without the +
		// during loading of 
		// /Lua/Default/Blueprint/MyCharacter.lua
		// with a mixin
		// mixin "+Tick"
		// -> 
		// /Lua/Default/Blueprint/MyCharacterTick.lua
		
		path = this->ImportStack.ImportStacks.Last()->ImportPath;
		//remove the '+'
		mixinPathStr.remove_prefix(1);
		//append the mixin as file name
		path += mixinPathStr.data();
	}
	else if (mixinPathStr.starts_with('.'))
	{
		//use the same path as the current master import path

		// Replace master import path file name with mixin file name, without the .
		// during loading of 
		// /Lua/Default/Blueprint/MyCharacter.lua
		// with a mixin
		// mixin ".Tick"
		// -> 
		// /Lua/Default/Blueprint/Tick.lua
		
		path = this->ImportStack.ImportStacks.Last()->ImportPath;
		size_t slashIndex = path.find_last_of('/');
		if (slashIndex == std::string::npos)
		{
			return;
		}
		//cut off the entire last part until the last slash
		path = path.substr(0, slashIndex+1);
		//remove the '.'
		mixinPathStr.remove_prefix(1);
		//add the mixin as file name
		path += mixinPathStr.data();
		
	}
	else
	{
		//full path
		path = mixinPathStr.data();	
	}
	int32 numPreImport = this->ImportStack.Num();
	sol::table mixinTable =  this->ImportLuaScript(path, true, false);
	int32 numPostImport = this->ImportStack.Num();
	verify(numPreImport == numPostImport);
	
	if (!mixinTable.valid())
	{
		return;
	}
	
	LUA_LOG("Mixing in %hs for %hs", path.data(), this->ImportStack.ImportStacks.Last()->ImportPath.data())
	mixinTable.for_each_stack([&targetTable](sol::stack_object key, sol::stack_object value)
	{
		targetTable[key] = value;
	});
}

sol::protected_function_result FScopedLuaContext::RunScript(sol::stack_object runPath, sol::variadic_args args)
{
	if (!runPath.is<std::string>())
	{
		return {};
	}
	return RunScript(runPath.as<std::string>(), args);
}

sol::protected_function_result FScopedLuaContext::RunScript(const std::string& mixinPath, sol::variadic_args args)
{
	if(CVarLogLuaScriptLoading.GetValueOnGameThread())
	{
		LUA_LOG("Running Lua script file %hs from disk", mixinPath.data())	
	}
	
	//LUA_LOG("Attempting to load Lua file %s", *fileNameStr)

	sol::protected_function_result results{};

	bool ranAnyfile = false;
	
	for (const FLuaPathElement& currentPathElement : this->LuaPath)
	{
		const FString& currentPath = currentPathElement.GetFString();
		std::string fullPath = StringCast<char>(*currentPath).Get() + mixinPath;
		//when file valid AND execution valid : done
		
		//LUA_LOG("Looking in path %s", UTF8_TO_TCHAR(fullPath.c_str()))
		sol::load_result file = this->LuaState.load_file(fullPath);
		if(file.valid())
		{
			//LUA_LOG("Trying to execute loaded primary script file %s", *fileNameStr)
			results = file();
			//sol::protected_function_result execFileContent = file();
			if(results.valid())
			{
				ranAnyfile = true;
				LUA_LOG("Successfully executed script file %hs", fullPath.data())
				break;
			}
			else
			{
				sol::error err = results.get<sol::error>();
				LUA_LOG_ERROR("Error while executing script file %hs : %hs", fullPath.data(), err.what())
				results = sol::protected_function_result{this->LuaState.lua_state(),  -1, 0, 1, sol::call_status::runtime};
			}
		}
		else
		{
			switch(file.status())
			{
			case sol::load_status::syntax:
				{
					sol::error err = file.get<sol::error>();
					LUA_LOG_ERROR("LUA_ERRSYNTAX %hs : %hs", fullPath.data(), err.what())						
				}
				break;
			case sol::load_status::file:
				//LUA_LOG("LUA_ERRFILE... I guess file could not be found?")
				break;
			case sol::load_status::gc:
				LUA_LOG("LUA_ERRGCMM")
			case sol::load_status::memory:
				LUA_LOG("LUA_ERRMEM")
				break;
			default : break;
			}
		}
	}		
	return results;
}

sol::protected_function_result FScopedLuaContext::RunScriptFile(FString pathToFile, sol::variadic_args args)
{
	if(pathToFile.IsEmpty())
	{
		return {};
	}

	//pathToFile.RemoveFromStart(TEXT("/"));
	//pathToFile.RemoveFromEnd(TEXT(".lua"));
	
	std::string subFolderPath = StringCast<char>(*pathToFile).Get();
	return this->RunScript(subFolderPath, args);
}

sol::protected_function_result FScopedLuaContext::RunSingleScriptFile(const std::string_view& fullPath, sol::variadic_args args)
{
	sol::protected_function_result results{};
	if (fullPath.empty())
	{
		return results;
	}
	sol::load_result file = this->LuaState.load_file(fullPath.data());
	if(file.valid())
	{
		//LUA_LOG("Trying to execute loaded primary script file %s", *fileNameStr)
		results = file(args);
		//sol::protected_function_result execFileContent = file();
		if(results.valid())
		{
			LUA_LOG("Successfully executed script file %hs", fullPath.data())
		}
		else
		{
			sol::error err = results.get<sol::error>();
			LUA_LOG_ERROR("Error while executing script file %hs : %hs", fullPath.data(), err.what())
		}
	}
	else
	{
		switch(file.status())
		{
		case sol::load_status::syntax:
			{
				sol::error err = file.get<sol::error>();
				LUA_LOG_ERROR("LUA_ERRSYNTAX %hs : %hs", fullPath.data(), err.what())
				sol::stack::push<std::string>(this->LuaState.lua_state(), err.what());
				results = sol::protected_function_result{this->LuaState.lua_state(), -1, 0, 1, sol::call_status::syntax};
			}
			break;
		case sol::load_status::file:
			//LUA_LOG("LUA_ERRFILE... I guess file could not be found?")
			break;
		case sol::load_status::gc:
			LUA_LOG("LUA_ERRGCMM")
		case sol::load_status::memory:
			LUA_LOG("LUA_ERRMEM")
			break;
		default : break;
		}
	}
	return results;
}

sol::protected_function_result FScopedLuaContext::RunString(const TCHAR* stringToRun, const TArray<sol::object>& args)
{
	auto casted = StringCast<char>(stringToRun);
	return this->RunString(std::string_view{casted.Get()}, args);
}

sol::protected_function_result FScopedLuaContext::RunString(const std::string_view& stringToRun,const TArray<sol::object>& args)
{
	sol::protected_function_result results{};
	if (stringToRun.empty())
	{
		return results;
	}
	/*
	results = this->LuaState.safe_script(stringToRun,sol::script_pass_on_error, sol::detail::default_chunk_name(), sol::load_mode::text );
	if (!results.valid())
	{
		sol::error err = results.get<sol::error>();
		LUA_LOG_ERROR("Error while executing script string : %hs", err.what())
	}
	*/
	sol::load_result loadResult = this->LuaState.load(stringToRun, sol::detail::default_chunk_name(), sol::load_mode::text);
	if(loadResult.valid())
	{
		sol::protected_function loader{loadResult};
		//LUA_LOG("Trying to execute loaded primary script file %s", *fileNameStr)
		results = loader(sol::as_args(args));
		//sol::protected_function_result execFileContent = file();
		if(results.valid())
		{
			LUA_LOG("Successfully ran code %hs", stringToRun.data())
		}
		else
		{
			sol::error err = results.get<sol::error>();
			LUA_LOG_ERROR("Error while executing script string : %hs", err.what())
		}
	}
	else
	{
		switch(loadResult.status())
		{
		case sol::load_status::syntax:
			{
				sol::error err = loadResult.get<sol::error>();
				LUA_LOG_ERROR("LUA_ERRSYNTAX while running code : %hs", err.what())
				sol::stack::push<std::string>(this->LuaState.lua_state(), err.what());
				results = sol::protected_function_result{this->LuaState.lua_state(), -1, 0, 1, sol::call_status::syntax};
			}
			break;
		case sol::load_status::file:
			//LUA_LOG("LUA_ERRFILE... I guess file could not be found?")
			break;
		case sol::load_status::gc:
			LUA_LOG("LUA_ERRGCMM")
		case sol::load_status::memory:
			LUA_LOG("LUA_ERRMEM")
			break;
		default : break;
		}
	}
	
	return results;
}

bool FScopedLuaContext::IsInitialized() const
{
	return bIsInitialized;
}

FLoadLuaScriptResult FScopedLuaContext::LoadLuaScriptFromDisk(const FString& scriptPath, bool bIsAbsolutePath, bool bAllowModding, const FLuaPath* luaPathOverride, ELuaPathFlags requiredFlags, ELuaPathFlags excludedLocationFlags)
{
	if(bIsAbsolutePath)
	{
		return this->FullPathLoadLuaScriptFromDisk(scriptPath);		
	}
	else
	{
		return this->LoadLuaScriptFromDisk(scriptPath, bAllowModding, luaPathOverride, requiredFlags, excludedLocationFlags);
	}
}

FLoadLuaScriptResult FScopedLuaContext::LoadLuaScriptFromDisk(const FString& filePath, bool bAllowModding, const FLuaPath* luaPathOverride, ELuaPathFlags requiredFlags, ELuaPathFlags excludedLocationFlags)
{
	const std::string pathStr = StringCast<char>(*filePath).Get();
	return LoadLuaScriptFromDisk(pathStr, bAllowModding, luaPathOverride, requiredFlags, excludedLocationFlags);
}

FLoadLuaScriptResult FScopedLuaContext::LoadLuaScriptFromDisk(const std::string& opath, bool bAllowMods, const FLuaPath* luaPathOverride, ELuaPathFlags requiredFlags, ELuaPathFlags excludedLocationFlags)
{
	std::string path = opath;
	
	if(path.empty())
	{
		return {};
	}

	FString originalFileRequestPath = path.c_str();
	

	if(path.starts_with("/"))
	{
		path.erase(0,1);
	}
	if(path.length() > 3 && path.ends_with(".lua"))
	{
		path = path.substr(0, path.length() - 4);
	}
	if(path.empty())
	{
		return {};
	}

	sol::table script = this->LuaState.create_table();
	FLuaScriptImportStackItem importStackItem{ .importedTable = script, .ImportPath = path };
	int32 numPreLoadFromDisk = this->ImportStack.Num();
	this->ImportStack.ImportStacks.Emplace(&importStackItem);
	
	const FString fileNameStr{path.data()};

	bool bLogLoading = CVarLogLuaScriptLoading.GetValueOnGameThread();
	if(bLogLoading)
	{
		LUA_LOG("Loading Lua script file %s from disk", *fileNameStr)	
	}
	
	FLoadLuaScriptResult result {
		originalFileRequestPath, false, sol::nil,fileNameStr, {}, {}
	};

	FDateTime now = FDateTime::UtcNow();
	IFileManager& fmg = IFileManager::Get();

	///////////////////////////////////////
	
	const FLuaPath& usedPaths = luaPathOverride != nullptr ? *luaPathOverride : this->LuaPath; 
	
	for (const FLuaPathElement& currentLuaPathEle : usedPaths)
	//for(int32 index = 0; index < this->LuaPath.Paths.Num(); index++)
	{
		if (currentLuaPathEle.HasAnyFlags(excludedLocationFlags))
		{
			continue;
		}
		if (!currentLuaPathEle.HasAnyFlags(requiredFlags))
		{
			continue;
		}
		const FString& currentLuaPathStr = currentLuaPathEle.GetFString();
    	FString luaFolderFilePath = fileNameStr + ".lua";
    	FString fullPathStr = currentLuaPathStr + luaFolderFilePath;
		if(bLogLoading)
		{
			//LUA_LOG("Looking in path %s", *fullPathStr)
		}
    	if(fmg.FileExists(*fullPathStr))
    	{
    		std::string currentLuaPath = StringCast<char>(*currentLuaPathStr).Get();
    		std::string fullPath = currentLuaPath + path + ".lua";
    		if(bLogLoading)
    		{
    			//LUA_LOG("Found file in path %hs", fullPath.c_str())
    		}
    		sol::load_result file = this->LuaState.load_file(fullPath, sol::load_mode::text);

    		if(file.valid())
    		{
    			sol::string_view strv = file.get<sol::string_view>();
    			if (strv.starts_with("---@"))
    			{
    				if (strv.starts_with("---@SERVER"))
    				{
    					
    				}
    				else if (strv.starts_with("---@CLIENT"))
    				{
    					
    				}
    			}
    			//LUA_LOG("Trying to execute loaded primary script file %s", *fileNameStr)
    			sol::table execFileContent = file(script).get<sol::table>();
    			//sol::protected_function_result execFileContent = file();
    			if(execFileContent.valid())
    			{
    				result.MainFileInfo.Emplace(FLoadedLuaFileInfo{fullPathStr, fmg.GetTimeStamp(*fullPathStr)});
    				//LUA_LOG("Successfully executed script file %s", *fileNameStr)
    				script = execFileContent;
    				//result.LoadedChunks.Emplace(MoveTemp(file));
    				break;	
    			}
    			else
    			{
    				//File existed, but didn't return a valid table
    				//Still keep this file in mind, might become valid table returner later	
    				result.MainFileInfo.Emplace(FLoadedLuaFileInfo{fullPathStr, FDateTime(-1), "No table returned"});
    			}
    		}
    		else
    		{
    			FString errMsg;
    			switch(file.status())
    			{
    			case sol::load_status::syntax:
    				{
    					LUA_LOG_ERROR("Error while loading lua script file : Loaded file %s had syntax issues:", *fileNameStr)
    					sol::error err = file.get<sol::error>();
    					LUA_LOG_ERROR("LUA_ERRSYNTAX : %s", UTF8_TO_TCHAR(err.what()))
    					errMsg = FString::Printf(TEXT("LUA_ERRSYNTAX, Main file %s : %hs"), *fullPathStr, err.what());
    					result.ErrorMessages.Emplace(errMsg);
    				}
    				break;
    			case sol::load_status::file:
    				{
    					errMsg = FString::Printf(TEXT("LUA_ERRFILE %s"), *fileNameStr);
					}
    				break;
    			case sol::load_status::gc:
    				{
    					errMsg = FString::Printf(TEXT("LUA_ERRGCMM %s"), *fileNameStr);
						LUA_LOG("Lua GC error : %s", *errMsg)
    				}
    				break;
    			case sol::load_status::memory:
    				{
    					errMsg = FString::Printf(TEXT("LUA_ERRMEM %s"), *fileNameStr);
    					LUA_LOG("Lua memory issue : %s", *errMsg)
					}
    				break;
    			default :
    				{
    					errMsg = "Unknown file error";
    				}
    				break;
    			}
    			result.MainFileInfo.Emplace(FLoadedLuaFileInfo{fullPathStr, fmg.GetTimeStamp(*fullPathStr), errMsg});
    		}	
    	}
		else
		{
			result.MainFileInfo.Emplace(FLoadedLuaFileInfo{fullPathStr, FDateTime(-1), "File not found"});
		}
    }
	
	FLuaScriptImportStackItem* last = this->ImportStack.ImportStacks.Last();
	verify(last == &importStackItem);
	int32 numPreMods = this->ImportStack.Num();
	verify(numPreLoadFromDisk == numPreMods - 1);


	///////////////////////////////////////
	/// Apply mods
	///////////////////////////////////////

	int32 modCount = 0;
	int32 modErrors = 0;

	if(bAllowMods)
	{
		std::string modExtension = StringCast<char>(*UUnrealLuaConfig::GetLuaScriptModFileExtension()).Get();
		//loop from least important ("/Lua/Path" to most important "/<Mod>/<GameMode>/Path
		//for(int32 index = this->LuaPath.Paths.Num()-1; index >= 0; index--)
		for (const FLuaPathElement& currentLuaPathEle : usedPaths)
		{
			if (excludedLocationFlags != ELuaPathFlags::None || requiredFlags != ELuaPathFlags::None)
			{
				if (currentLuaPathEle.HasAnyFlags(excludedLocationFlags) || !currentLuaPathEle.HasAnyFlags(requiredFlags))
				{
					continue;
				}
			}
			const FString& currentLuaPathStr = currentLuaPathEle.GetFString();
			FString pathStr = currentLuaPathStr;
			FString fullPathStr = pathStr + fileNameStr + UUnrealLuaConfig::GetLuaScriptModFileExtension();
			if(fmg.FileExists(*fullPathStr))
			{
				std::string currentLuaPath = StringCast<char>(*currentLuaPathStr).Get();
				std::string fullPath = currentLuaPath + path + modExtension;
				sol::load_result file = this->LuaState.load_file(fullPath);
				if(file.valid())
				{
					//LUA_LOG("Found mod file, executing it")
					file(script);
					///.LoadedChunks.Emplace(MoveTemp(file));
					result.ModFileInfos.Emplace(fullPathStr, fmg.GetTimeStamp(*fullPathStr));
					modCount++;
				}
				else
				{
					switch(file.status())
					{
					case sol::load_status::syntax:
						{
							sol::error err = file.get<sol::error>();
							LUA_LOG_ERROR("LUA_ERRSYNTAX : %s", UTF8_TO_TCHAR(err.what()))
							result.ErrorMessages.Emplace(FString::Printf(TEXT("LUA_ERRSYNTAX, Mod file %s : %hs"), *fullPathStr, err.what()));
							modErrors++;
						}
						break;
					case sol::load_status::file:
						//LUA_LOG("LUA_ERRFILE... I guess file could not be found?")
							break;
					case sol::load_status::gc:
						LUA_LOG("LUA_ERRGCMM")
						modErrors++;
						break;
					case sol::load_status::memory:
						LUA_LOG("LUA_ERRMEM")
						modErrors++;
						break;
					default : break;
					}
				}
			}
		}
		
		if(!script.valid())
		{
			LUA_LOG_ERROR("Error while modding loaded lua script file : script %s got invalidated!", *fileNameStr)
		}
	}
	FLuaScriptImportStackItem* popped = this->ImportStack.ImportStacks.Pop();
	verify(popped == &importStackItem);
	int32 numPostLoadFromDisk = this->ImportStack.Num();
	verify(numPreLoadFromDisk == numPostLoadFromDisk);
	
	if(!script.valid())
	{
		LUA_LOG_ERROR("Unable to load lua script file : script %s not valid!", *fileNameStr)
		return result;
	}

	///////////////////////////////////////
	/// Mix ins from mods
	/// ///////////////////////////////////


	UnrealLua::LuaScriptCall::CallLuaFunctionSafeByName(script, UnrealLua::scriptLoading::ScriptLoadedFromDisk, script);

	result.FinalResult = script;
	
	return result;

}

sol::table FScopedLuaContext::ModTable(const std::string& opath, sol::table script, const FLuaPath* luaPathOverride, ELuaPathFlags requiredFlags, ELuaPathFlags excludedLocationFlags)
{
	std::string path = opath;
	FString originalFileRequestPath = path.c_str();
	IFileManager& fmg = IFileManager::Get();
    if(path.starts_with("/"))
    {
    	path.erase(0,1);
    }
    if(path.length() > 3 && path.ends_with(".lua"))
    {
    	path = path.substr(0, path.length() - 4);
    }
    if(path.empty())
    {
    	return {};
    }
	const FString fileNameStr{path.data()};
	FLoadLuaScriptResult result {
		originalFileRequestPath, false, sol::nil,fileNameStr, {}, {}
	};
	
	int32 modCount = 0;
	int32 modErrors = 0;
	std::string modExtension = StringCast<char>(*UUnrealLuaConfig::GetLuaScriptModFileExtension()).Get();

	//loop from least important ("/Lua/Path" to most important "/<Mod>/<GameMode>/Path
	//for(int32 index = this->LuaPath.Paths.Num()-1; index >= 0; index--)
	const FLuaPath& usedPaths = luaPathOverride != nullptr ? *luaPathOverride : this->LuaPath; 
	for (const FLuaPathElement& currentLuaPathEle : usedPaths)
	{
		if (excludedLocationFlags != ELuaPathFlags::None || requiredFlags != ELuaPathFlags::None)
		{
			if (currentLuaPathEle.HasAnyFlags(excludedLocationFlags) || !currentLuaPathEle.HasAnyFlags(requiredFlags))
			{
				continue;
			}
		}
		const FString& currentLuaPathStr = currentLuaPathEle.GetFString();
		FString pathStr = currentLuaPathStr;
		FString fullPathStr = pathStr + fileNameStr + UUnrealLuaConfig::GetLuaScriptModFileExtension();
		if(fmg.FileExists(*fullPathStr))
		{
			std::string currentLuaPath = StringCast<char>(*currentLuaPathStr).Get();
			std::string fullPath = currentLuaPath + path + modExtension;
			sol::load_result file = this->LuaState.load_file(fullPath);
			if(file.valid())
			{
				//LUA_LOG("Found mod file, executing it")
				file(script);
				///.LoadedChunks.Emplace(MoveTemp(file));
				result.ModFileInfos.Emplace(originalFileRequestPath, fmg.GetTimeStamp(*fullPathStr));
				modCount++;
			}
			else
			{
				switch(file.status())
				{
				case sol::load_status::syntax:
					{
						sol::error err = file.get<sol::error>();
						LUA_LOG_ERROR("LUA_ERRSYNTAX : %s", UTF8_TO_TCHAR(err.what()))
						result.ErrorMessages.Emplace(FString::Printf(TEXT("LUA_ERRSYNTAX, Mod file %s : %hs"), *fullPathStr, err.what()));
						modErrors++;
					}
					break;
				case sol::load_status::file:
					//LUA_LOG("LUA_ERRFILE... I guess file could not be found?")
						break;
				case sol::load_status::gc:
					LUA_LOG("LUA_ERRGCMM")
					modErrors++;
					break;
				case sol::load_status::memory:
					LUA_LOG("LUA_ERRMEM")
					modErrors++;
					break;
				default : break;
				}
			}
		}
	}
	
	if(!script.valid())
	{
		LUA_LOG_ERROR("Error while modding loaded lua script file : script %s got invalidated!", *fileNameStr)
		return {};	
	}
	return result.FinalResult;
}


FLoadLuaScriptResult FScopedLuaContext::FullPathLoadLuaScriptFromDisk(const FString& filePath)
{
	std::string str{StringCast<char>(*filePath).Get()};
	return this->FullPathLoadLuaScriptFromDisk(str);
}

FLoadLuaScriptResult FScopedLuaContext::FullPathLoadLuaScriptFromDisk(const std::string& filePath)
{
	std::string path = filePath;
	
	if(path.empty())
	{
		return {};
	}

	FString originalFileRequestPath = path.c_str();
	
	if(!path.ends_with(".lua"))
	{
		return {};
	}
	
	const FString fileNameStr{path.data()};

	std::string currentLuaPath = "";
	sol::table script{};
	
	FLoadLuaScriptResult result {
		originalFileRequestPath, true, sol::nil, fileNameStr, {}, {}
	};
	
	IFileManager& fmg = IFileManager::Get();
	
	if(fmg.FileExists(*fileNameStr))
	{
		sol::load_result file = this->LuaState.load_file(path);
		
		if(file.valid())
		{
			//sol::environment my_env(this->LuaState, sol::create);
			//verify(my_env.set_on(file));
			sol::table execFileContent = file(/*my_env*/);
			//sol::protected_function_result execFileContent = file();
			if(execFileContent.valid())
			{
				result.MainFileInfo.Emplace(FLoadedLuaFileInfo{fileNameStr, fmg.GetTimeStamp(*fileNameStr)});
				//LUA_LOG("Successfully executed script file %s", *fileNameStr)
				script = execFileContent;
				//result.LoadedChunks.Emplace(MoveTemp(file));
			}
			else
			{
				//LUA_LOG_ERROR("Error while loading lua script file : Executed file %s did not return a value, trying next path", *fileNameStr)
			}
		}
		else
		{
			switch(file.status())
			{
			case sol::load_status::syntax:
				{
					LUA_LOG_ERROR("Error while loading lua script file : Loaded file %s had syntax issues:", *fileNameStr)
					sol::error err = file.get<sol::error>();
					LUA_LOG_ERROR("LUA_ERRSYNTAX : %s", UTF8_TO_TCHAR(err.what()))
					result.ErrorMessages.Emplace(FString::Printf(TEXT("LUA_ERRSYNTAX, Main file %s : %hs"), *originalFileRequestPath, err.what()));
				}
				break;
			case sol::load_status::file:
				//LUA_LOG("LUA_ERRFILE... I guess file could not be found?")
					break;
			case sol::load_status::gc:
				LUA_LOG("LUA_ERRGCMM %s", *fileNameStr)
			case sol::load_status::memory:
				LUA_LOG("LUA_ERRMEM %s", *fileNameStr)
				break;
			default : break;
			}
		}	
	}
	if(!script.valid())
	{
		LUA_LOG_ERROR("Unable to load lua script file : script %s not valid!", *fileNameStr)
		//result.LoadedChunks.Empty();
		
		return result;
	}
	result.FinalResult = script;
	
	return result;

}

bool FScopedLuaContext::ReloadAllScripts()
{
	for(TTuple<FName, TObjectPtr<ULoadedLuaScriptCollection>> it : this->LoadedScripts)
	{
		it.Value->Reload();
	}
	return true;
}


bool FScopedLuaContext::ReloadScript(const FName& fileName)
{
	ULoadedLuaScriptCollection* coll = this->LoadedScripts.FindRef(fileName);
	if(coll)
	{
		return coll->Reload();
	}
	return false;
}

void FScopedLuaContext::ReloadScriptByFullFileName(const FString& fullFileName)
{
	if (!fullFileName.EndsWith(".lua"))
	{
		return;
	}
	TArray<ULoadedLuaScriptCollection*> toReload{};
	
	bool isModFile = fullFileName.EndsWith(".mod.lua");
	for (TTuple<FName, TObjectPtr<ULoadedLuaScriptCollection>> pair : this->LoadedScripts)
	{
		ULoadedLuaScriptCollection* coll = pair.Value;
		if (isModFile)
		{
			for (const FLoadedLuaFileInfo& fileinfo : coll->FileInfo.ModFileInfos)
			{
				if (fileinfo.IsValid() && fileinfo.FullPathOnDisk == fullFileName)
				{
					toReload.Emplace(coll);
				}
			}
		}
		else
		{
			for (const FLoadedLuaFileInfo& fileinfo : coll->FileInfo.MainFileInfo)
			{
				if (fileinfo.IsValid() && fileinfo.FullPathOnDisk == fullFileName)
				{
					toReload.Emplace(coll);
				}
			}	
		}
	}
	
	for (ULoadedLuaScriptCollection* coll : toReload)
	{
		coll->Reload();
	}
}

