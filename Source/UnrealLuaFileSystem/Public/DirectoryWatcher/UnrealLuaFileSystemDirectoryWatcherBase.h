#pragma once
#include "CoreMinimal.h"

DECLARE_DELEGATE_OneParam(FUnrealLuaFileSystemDirectoryChanged, const TArray<struct FUnrealLuaFileChangeData>& /*FileChanges*/);


struct FUnrealLuaFileChangeData
{
	enum EFileChangeAction
	{
		FCA_Unknown,
		FCA_Added,
		FCA_Modified,
		FCA_Removed,
		FCA_RescanRequired,
	};

	FUnrealLuaFileChangeData(const FString& InFilename, EFileChangeAction InAction, bool isDirectory)
		: Filename(InFilename)
		, Action(InAction)
		, bIsDirectory(isDirectory)
	{
		FPaths::MakeStandardFilename(Filename);
	}

	/**
	 * If the Action references a specific file, the name of the file.
	 * Applies To: FCA_Added, FCA_Modified, FCA_Removed, FCA_RescanRequired.
	 * For all other actions value will be emptystring.
	 */
	FString Filename;
	/**
	 * If the Action references a timestamp, the timezone UTC UnixTimeStamp (e.g. FDateTime::ToUnixTimeStamp) of the Action.
	 * Applies to: FCA_RescanRequired.
	 * For all other actions value will be 0.
	 */
	int64 TimeStamp = 0;
	/** The reported Action. */
	EFileChangeAction Action = EFileChangeAction::FCA_Unknown;
	
	bool bIsDirectory = false;
};

struct FUnrealLuaFileSystemDirectoryWatcherBase
{
	
	static TSharedPtr<FUnrealLuaFileSystemDirectoryWatcherBase> Create(const FString& filePath, const FUnrealLuaFileSystemDirectoryChanged& onFileChangedCallback);
	
	virtual ~FUnrealLuaFileSystemDirectoryWatcherBase() = default;
	FUnrealLuaFileSystemDirectoryWatcherBase(const FString& filePath, const FUnrealLuaFileSystemDirectoryChanged& onFileChangedCallback);
	
	
	virtual bool Init();
	virtual void Deinitialize();
	virtual void Tick();
protected:
	
	FString InputFilePath = {};
	
	FString WatchDirectoryFullPath = {};
	
	bool bEndWatchRequestInvoked = false;
	
	TArray<FUnrealLuaFileChangeData> FileChanges = {};
	
	FUnrealLuaFileSystemDirectoryChanged OnDirectoryChanged = {};
};
