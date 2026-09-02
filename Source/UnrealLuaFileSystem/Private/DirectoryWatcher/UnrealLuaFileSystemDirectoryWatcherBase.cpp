
#include "DirectoryWatcher/UnrealLuaFileSystemDirectoryWatcherBase.h"

#include "DirectoryWatcher/UnrealLuaFileSystemDirectoryWatcherLinux.h"

FUnrealLuaFileSystemDirectoryWatcherBase::FUnrealLuaFileSystemDirectoryWatcherBase(const FString& filePath, const FUnrealLuaFileSystemDirectoryChanged& onFileChangedCallback)
	: InputFilePath(filePath), OnDirectoryChanged(onFileChangedCallback)
{
}

TSharedPtr<FUnrealLuaFileSystemDirectoryWatcherBase> FUnrealLuaFileSystemDirectoryWatcherBase::Create(const FString& filePath, const FUnrealLuaFileSystemDirectoryChanged& onFileChangedCallback)
{
#if PLATFORM_LINUX
	return MakeShared<FUnrealLuaFileSystemDirectoryWatcherLinux>(filePath, onFileChangedCallback);
#endif
	return {};
}

bool FUnrealLuaFileSystemDirectoryWatcherBase::Init()
{
	return false;
}

void FUnrealLuaFileSystemDirectoryWatcherBase::Deinitialize()
{
	
}


void FUnrealLuaFileSystemDirectoryWatcherBase::Tick()
{
	// Trigger all listening delegates with the files that have changed
	if (FileChanges.Num() > 0)
	{
		TArray<FUnrealLuaFileChangeData> FileChangeCache;

		for (const FUnrealLuaFileChangeData& FileChangeData : FileChanges)
		{
			FileChangeCache.Add(FileChangeData);			
		}
		this->OnDirectoryChanged.ExecuteIfBound(FileChangeCache);
	}
	FileChanges.Empty();
}
