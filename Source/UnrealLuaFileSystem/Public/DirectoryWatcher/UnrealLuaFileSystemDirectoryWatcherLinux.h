// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include <sys/inotify.h>

#if PLATFORM_LINUX
#include "CoreMinimal.h"
#include "UnrealLuaFileSystemDirectoryWatcherBase.h"

/**
 * 
 */

struct UNREALLUAFILESYSTEM_API FUnrealLuaFileSystemDirectoryWatcherLinux : public FUnrealLuaFileSystemDirectoryWatcherBase
{
	FUnrealLuaFileSystemDirectoryWatcherLinux(const FString& filePath, const FUnrealLuaFileSystemDirectoryChanged& onFileChangedCallback)
		: FUnrealLuaFileSystemDirectoryWatcherBase(filePath, onFileChangedCallback)
	{}

	virtual ~FUnrealLuaFileSystemDirectoryWatcherLinux() override = default;
	
	virtual bool Init() override;
	virtual void Deinitialize() override;
	virtual void Tick() override;
	
	void ProcessNotifyChanges(const FString& folderName, const struct inotify_event* event);
	
	void WatchDirectoryTree(const FString & directoryFullPath, TArray<FUnrealLuaFileChangeData>* FileChangesPtr);
	void UnwatchDirectoryTree(const FString& RootAbsolutePath);

	static uint32 GetPathNameHash(const FString& Key)
	{
		const TCHAR* Str = &Key[0];
		uint32 StrLen = sizeof(TCHAR) * Key.Len();

		return CityHash64(reinterpret_cast<const char*>(Str), StrLen);
	}
	
	/** Set of hashed directory names we're watching */
	TSet<uint32> PathNameHashSet;
	
	int GFileDescriptor = -1;
	
	
	struct FWatchInfo
	{
		FString FolderName;
	};
	
	static FString INotifyFlagsToStr(uint32 INotifyFlags)
	{
#if UE_BUILD_SHIPPING
		return FString();
#else
		FString Ret = TEXT("[");

#define _XTAG(_x) if (INotifyFlags & _x) Ret += FString(TEXT(" ")) + TEXT(#_x)
		_XTAG(IN_ACCESS);
		_XTAG(IN_MODIFY);
		_XTAG(IN_ATTRIB);
		_XTAG(IN_CLOSE_WRITE);
		_XTAG(IN_CLOSE_NOWRITE);
		_XTAG(IN_OPEN);
		_XTAG(IN_MOVED_FROM);
		_XTAG(IN_MOVED_TO);
		_XTAG(IN_CREATE);
		_XTAG(IN_DELETE);
		_XTAG(IN_DELETE_SELF);
		_XTAG(IN_MOVE_SELF);
		_XTAG(IN_UNMOUNT);
		_XTAG(IN_Q_OVERFLOW);
		_XTAG(IN_IGNORED);
		_XTAG(IN_ISDIR);
#undef _XTAG

		Ret += TEXT(" ]");
		return Ret;
#endif
	}
	
	TMultiMap<int32, FWatchInfo> GWatchDescriptorsToWatchInfo;
};
#endif