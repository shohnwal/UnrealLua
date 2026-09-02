#include "Utility/LuaFileLister.h"

#include "LuaContext/LuaPath.h"
#include "LuaContext/ScopedLuaContext.h"
#include "HAL/FileManager.h"
#include "Interface/LuaContext.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"


FLuaFileLister::FLuaFileLister(const TArray<FString>& excludeStartsWith, const TArray<FString>& excludeEndsWith)
	: LuaSubPath(""), StartsWithFilter(excludeStartsWith), EndsWithFilter(excludeEndsWith), LuaPaths(), UniqueFileNames(), FoundFullFilePaths()
{
	this->LuaPaths.SetupPackagePaths({}, "", ELuaPathFlags::Root);
	this->FindFiles(true);
}

FLuaFileLister::FLuaFileLister(FScopedLuaContext& ctx, const FString& appendedPath, bool bRecursiveSearch, const TArray<FString>& excludeStartsWith, const TArray<FString>& excludeEndsWith)
	: LuaSubPath(appendedPath), StartsWithFilter(excludeStartsWith), EndsWithFilter(excludeEndsWith), LuaPaths(ctx.LuaPath), UniqueFileNames(), FoundFullFilePaths()
{
	this->FindFiles(bRecursiveSearch);
}

FLuaFileLister::FLuaFileLister(ILuaContext& ctx, const FString& appendedPath, bool bRecursiveSearch, const TArray<FString>& excludeStartsWith, const TArray<FString>& excludeEndsWith)
	: LuaSubPath(appendedPath), StartsWithFilter(excludeStartsWith), EndsWithFilter(excludeEndsWith), LuaPaths(ctx.GetScopedLuaContext().LuaPath), UniqueFileNames(), FoundFullFilePaths()
{
	this->FindFiles(bRecursiveSearch);
}

FLuaFileLister::FLuaFileLister(const FLuaPath& paths, const FString& appendedPath, bool bRecursiveSearch, const TArray<FString>& excludeStartsWith, const TArray<FString>& excludeEndsWith)
	: LuaSubPath(appendedPath), StartsWithFilter(excludeStartsWith), EndsWithFilter(excludeEndsWith), LuaPaths(paths), UniqueFileNames(), FoundFullFilePaths()
{
	this->FindFiles(bRecursiveSearch);
}

void FLuaFileLister::RemoveAllUniqueFilenames(const TFunction<bool(FString&)>& func)
{
	for(TSet<FString>::TIterator it = this->UniqueFileNames.CreateIterator(); it; ++it)
	{
		if(func(*it))
		{
			it.RemoveCurrent();
		}
	}
}

void FLuaFileLister::FindFiles(bool bRecursiveSearch)
{
	LuaSubPath.RemoveFromStart("/");
	
	//Subpath must end with a slash
	if(!LuaSubPath.IsEmpty() && !LuaSubPath.EndsWith("/"))
	{
		LuaSubPath.Append("/");
	}

	IFileManager& fileMngr = IFileManager::Get();
	
	const FString contentDir = FPaths::ProjectContentDir();
	const FString luaContentDir = contentDir + "Lua/";
	
	for (const FLuaPathElement& ele : this->LuaPaths)
	{
		//This is a full system path to a Lua path folder, ending in a slash
		const FString& luaPath = ele.GetFString();

		//full system path + lua directory path, ending in a slash
		const FString fullDirectoryPath = luaPath + LuaSubPath;
		
		if (bRecursiveSearch)
		{
			//FindFilesRecursive stores full paths
			TArray<FString> FoundFiles{};
			fileMngr.FindFilesRecursive(FoundFiles, *fullDirectoryPath, TEXT("*.lua"), true, false, true);
			
			for (auto& filePath : FoundFiles)
			{
				FString fileName = FPaths::GetCleanFilename(filePath);
				if (this->DoesFileNamePassFilter(fileName))
				{
					this->UniqueFileNames.Add(fileName);
					this->FoundFullFilePaths.Add(filePath);
				}
			}
		}
		else
		{
			//FindFilesRecursive stores only file names
			TArray<FString> FoundFiles{};
			fileMngr.FindFiles(FoundFiles, *fullDirectoryPath, TEXT(".lua"));
			
			for (const auto& fileName : FoundFiles)
			{
				FString cleanFileName = FPaths::GetCleanFilename(fileName);
				if (this->DoesFileNamePassFilter(cleanFileName))
				{
					this->UniqueFileNames.Add(cleanFileName);
					this->FoundFullFilePaths.Add(fullDirectoryPath + cleanFileName);
				}
			}
		}
	}
}

bool FLuaFileLister::DoesFileNamePassFilter(const FString& fileName) const
{
	for (const FString& filterItem : this->StartsWithFilter)
	{
		if (fileName.StartsWith(filterItem))
		{
			return false;
		}
	}
		
	for (const FString& filterItem : this->EndsWithFilter)
	{
		if (fileName.EndsWith(filterItem))
		{
			return false;
		}	
	}
	return true;
}
