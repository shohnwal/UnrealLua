#include "Config/UnrealLua_CompilerFlags.h"
#include "HAL/FileManager.h"
#include "LuaContext/ScopedLuaContext.h"
#include "Misc/Paths.h"

void FScopedLuaContext::TickFileScanner(float deltaTime)
{
	if constexpr(UnrealLua::Compilation::WITH_AUTO_HOTRELOAD)
	{
		IFileManager& fm = IFileManager::Get();

		if(this->LoadedScriptsArray.IsEmpty())
		{
			return;
		}

		int32 nextIndexToCheck = this->NextIndexToHotReloadCheck;
	
		int32 numFilesToReload = 2;
		int32 numFilesToCheck = 5;

		while(numFilesToReload > 0 && numFilesToCheck > 0)
		{
			numFilesToCheck--;
			if(nextIndexToCheck >= this->LoadedScriptsArray.Num())
			{
				nextIndexToCheck = 0;
			}
			ULoadedLuaScriptCollection* coll = this->LoadedScriptsArray[nextIndexToCheck];
		
			for(FLoadedLuaFileInfo& fileInfo : coll->FileInfo.MainFileInfo)
			{
				FString& filePath = fileInfo.FullPathOnDisk;
				if(fm.FileExists(*filePath))
				{
					//File exists, may not have existed before (FDateTime(-1))
					FDateTime& lastTimeStamp = fileInfo.TimeStamp;
					FDateTime actualTimeStamp = fm.GetTimeStamp(*filePath);

					//If new tilestamp is older than memorized timestamp, reload script
					if(actualTimeStamp > lastTimeStamp)
					{
						numFilesToReload--;
						coll->Reload();
						break;
					}				
				}
				else
				{
					//File does not exist (anymore)

					//If file existed before
					if(fileInfo.TimeStamp > FDateTime(-1))
					{
						numFilesToReload--;
						coll->Reload();
						break;
					}
					fileInfo.TimeStamp = FDateTime(-1);
				}
			}
		
			nextIndexToCheck++;
			if(nextIndexToCheck >= this->LoadedScriptsArray.Num())
			{
				nextIndexToCheck = 0;
			}
			if(nextIndexToCheck == this->NextIndexToHotReloadCheck)
			{
				break;
			}
		}

		this->NextIndexToHotReloadCheck = nextIndexToCheck;
	}
}
