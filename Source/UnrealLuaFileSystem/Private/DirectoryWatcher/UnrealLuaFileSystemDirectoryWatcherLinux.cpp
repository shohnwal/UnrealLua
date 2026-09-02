// Fill out your copyright notice in the Description page of Project Settings.


#if PLATFORM_LINUX
#include "DirectoryWatcher/UnrealLuaFileSystemDirectoryWatcherLinux.h"

#include <sys/inotify.h>

bool FUnrealLuaFileSystemDirectoryWatcherLinux::Init()
{
	//UE_LOG(LogTemp, Error, TEXT("FUnrealLuaFileSystemDirectoryWatcherLinux::Init : %s"), *this->InputFilePath);
	if (this->InputFilePath.Len() == 0)
	{
		// Verify input
		return false;
	}
	this->WatchDirectoryFullPath = FPaths::ConvertRelativePathToFull(this->InputFilePath);
	//UE_LOG(LogTemp, Error, TEXT("FUnrealLuaFileSystemDirectoryWatcherLinux: Full path is %s"), *this->WatchDirectoryFullPath);
	if (GFileDescriptor == -1)
	{
		GFileDescriptor = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
		if (GFileDescriptor == -1)
		{
			if (errno == EMFILE)
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to init inotify (ran out of inotify instances)"));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to init inotify (errno=%d, %s)"), errno, UTF8_TO_TCHAR(strerror(errno)));
			}
			return false;
		}
	}
	
	// Find all subdirs and add inotify watch requests
	//UE_LOG(LogTemp, Error, TEXT("FUnrealLuaFileSystemDirectoryWatcherLinux: init success, now start watching subdirectories"));
	this->WatchDirectoryTree(WatchDirectoryFullPath, nullptr);
	
	return true;
}

void FUnrealLuaFileSystemDirectoryWatcherLinux::Deinitialize()
{
	//UE_LOG(LogTemp, Error, TEXT("FUnrealLuaFileSystemDirectoryWatcherLinux::Deinitialize : %s"), *this->InputFilePath);
	for (auto MapIt = GWatchDescriptorsToWatchInfo.CreateIterator(); MapIt; ++MapIt)
	{

		int WatchDescriptor = MapIt->Key;

		// Remove this entry
		MapIt.RemoveCurrent();

		// If this was last watch descriptor for this directory, rm the inotify watch.
		if (!GWatchDescriptorsToWatchInfo.Contains(WatchDescriptor))
		{
			UE_LOG(LogTemp, VeryVerbose, TEXT("- inotify_rm_watch(%d)"), WatchDescriptor);

			inotify_rm_watch(GFileDescriptor, WatchDescriptor);
		}
	}


	PathNameHashSet.Empty();

	if (GWatchDescriptorsToWatchInfo.IsEmpty() && (GFileDescriptor != -1))
	{
		close(GFileDescriptor);
		GFileDescriptor = -1;
	}
	
}

// To see inotify watch events:
//   TestPAL dirwatcher -LogCmds="LogDirectoryWatcher VeryVerbose"

#define EVENT_SIZE     ( sizeof(struct inotify_event) )
#define EVENT_BUF_LEN  ( 1024 * ( EVENT_SIZE + 16 ) )

void FUnrealLuaFileSystemDirectoryWatcherLinux::Tick()
{
	uint8_t Buffer[EVENT_BUF_LEN] __attribute__ ((aligned(__alignof__(struct inotify_event))));
	
	if (GFileDescriptor == -1)
	{
		return;
	}

	// Loop while events can be read from inotify file descriptor
	for (;;)
	{
		// Read event stream
		ssize_t Len = read(GFileDescriptor, Buffer, EVENT_BUF_LEN);

		// If the non-blocking read() found no events to read, then it returns -1 with errno set to EAGAIN.
		if (Len == -1 && errno != EAGAIN)
		{
			UE_LOG(LogTemp, Error, TEXT("FUnrealLuaLinuxFileSystemDirectoryWatcher::ProcessAllINotifyChanges() read() error (errno = %d, %s)"),
				errno, ANSI_TO_TCHAR(strerror(errno)));
			break;
		}

		if (Len <= 0)
		{
			break;
		}

		// Loop over all events in the buffer
		uint8_t* Ptr = Buffer;
		while (Ptr < Buffer + Len)
		{
			const struct inotify_event* Event;

			Event = reinterpret_cast<const struct inotify_event *>(Ptr);
			Ptr += EVENT_SIZE + Event->len;

			// Skip if overflowed
			if ((Event->wd != -1) && (Event->mask & IN_Q_OVERFLOW) == 0)
			{
				TArray<FWatchInfo> WatchInfos;
				GWatchDescriptorsToWatchInfo.MultiFind(Event->wd, WatchInfos);

				for (FWatchInfo& WatchInfo : WatchInfos)
				{
					this->ProcessNotifyChanges(WatchInfo.FolderName, Event);
				}
			}
		}
	}
	
	FUnrealLuaFileSystemDirectoryWatcherBase::Tick();
}

void FUnrealLuaFileSystemDirectoryWatcherLinux::ProcessNotifyChanges(const FString& folderName, const struct inotify_event* event)
{
	//UE_LOG(LogTemp, Error, TEXT("FUnrealLuaFileSystemDirectoryWatcherLinux:ProcessNotifyChanges %s"), *folderName);
	if (bEndWatchRequestInvoked)
	{
		return;
	}

	int WatchDescriptor = event->wd;
	bool bIsDir = (event->mask & IN_ISDIR) != 0;
	FUnrealLuaFileChangeData::EFileChangeAction Action = FUnrealLuaFileChangeData::FCA_Unknown;
	FString AffectedFile = folderName / UTF8_TO_TCHAR(event->name);

	UE_LOG(LogTemp, VeryVerbose, TEXT("Event: WatchDescriptor %d, mask 0x%08x, EventPath: '%s' Event Name: '%s' Len: %u %s"),
		WatchDescriptor, event->mask, *folderName, UTF8_TO_TCHAR(event->name), event->len, *INotifyFlagsToStr(event->mask));

	if ((event->mask & IN_CREATE) || (event->mask & IN_MOVED_TO))
	{
		// IN_CREATE: File/directory created in watched directory
		// IN_MOVED_TO: Generated for the directory containing the new filename when a file is renamed
		if (bIsDir)
		{
			// If a directory was created/moved, watch it and add changes to FileChanges.
			// Leave Action as FCA_Unknown so nothing gets added down below.
			WatchDirectoryTree(AffectedFile, &FileChanges);
		}
		else
		{
			Action = FUnrealLuaFileChangeData::FCA_Added;
		}
	}
	else if (event->mask & IN_MODIFY)
	{
		// IN_MODIFY: File was modified
		// If a directory was modified, we expect to get events from already watched files in it
		Action = FUnrealLuaFileChangeData::FCA_Modified;
	}
	// Check if the file/directory itself has been deleted (IGNORED can also be sent on delete)
	else if ((event->mask & IN_DELETE_SELF) || (event->mask & IN_UNMOUNT))
	{
		// IN_DELETE_SELF: Watched file/directory was itself deleted.
		//   In addition, an IN_IGNORED event will subsequently be generated for the watch descriptor
		// IN_UNMOUNT: Filesystem containing watched object was unmounted.
		//   In addition, an IN_IGNORED event will subsequently be generated for the watch descriptor

		// If a directory was deleted, we expect to get events from already watched files in it

		// NOTE: This code should ever get called - we only watch directories.
		checkf(bIsDir, TEXT("Watched item was file?"));

		if (bIsDir)
		{
			UnwatchDirectoryTree(AffectedFile);
			Action = FUnrealLuaFileChangeData::FCA_Removed;
		}
	}
	else if (event->mask & IN_IGNORED)
	{
		// IN_IGNORED: Watch was removed explicitly (inotify_rm_watch) or
		//   automatically (file was deleted, or filesystem was unmounted).
		PathNameHashSet.Remove(GetPathNameHash(this->InputFilePath));
		GWatchDescriptorsToWatchInfo.Remove(WatchDescriptor);
	}
	else if ((event->mask & IN_DELETE) || (event->mask & IN_MOVED_FROM))
	{
		// IN_DELETE: File/directory deleted from watched directory
		// IN_MOVED_FROM: Generated for the directory containing the old filename when a file is renamed

		// If a directory was deleted/moved, unwatch it
		if (bIsDir)
		{
			UnwatchDirectoryTree(AffectedFile);
		}

		Action = FUnrealLuaFileChangeData::FCA_Removed;
	}

	if (Action != FUnrealLuaFileChangeData::FCA_Unknown)
	{
		FileChanges.Emplace(FUnrealLuaFileChangeData(AffectedFile, Action, bIsDir));
	}
}

void FUnrealLuaFileSystemDirectoryWatcherLinux::WatchDirectoryTree(const FString& directoryFullPath,	TArray<FUnrealLuaFileChangeData>* FileChangesPtr)
{
	if (bEndWatchRequestInvoked || (GFileDescriptor == -1))
	{
		UE_LOG(LogTemp, Error, TEXT("FUnrealLuaFileSystemDirectoryWatcherLinux: no #1, can't watch directory tree %s because end request was made or file descriptor is invalid"), *directoryFullPath);
		return;
	}

	// If this isn't our root watch directory or under it, don't watch
	if (!directoryFullPath.StartsWith(this->WatchDirectoryFullPath, ESearchCase::CaseSensitive))
	{
		UE_LOG(LogTemp, Error, TEXT("FUnrealLuaFileSystemDirectoryWatcherLinux: no #2 : %s does not start with %s"), *directoryFullPath, *this->WatchDirectoryFullPath);
		return;
	}

	if (FileChangesPtr)
	{
		FileChangesPtr->Emplace(FUnrealLuaFileChangeData(directoryFullPath, FUnrealLuaFileChangeData::FCA_Added, true));
	}

	bool bWatchSubtree = true;
	if (!bWatchSubtree && (directoryFullPath != WatchDirectoryFullPath))
	{
		UE_LOG(LogTemp, Error, TEXT("FUnrealLuaFileSystemDirectoryWatcherLinux: no #3 : directoryFullPath and WatchDirectoryFullPath are the same"));
		return;
	}

	UE_LOG(LogTemp, VeryVerbose, TEXT("Watching tree '%s'"), *directoryFullPath);

	TArray<FString> AllFiles;
	if (bWatchSubtree)
	{
		IPlatformFile::GetPlatformPhysical().IterateDirectoryRecursively(*directoryFullPath,
			[&AllFiles, FileChangesPtr](const TCHAR* Name, bool bIsDirectory)
				{
					if (bIsDirectory)
					{
						AllFiles.Add(Name);
					}

					if (FileChangesPtr)
					{
						FileChangesPtr->Emplace(FUnrealLuaFileChangeData(Name, FUnrealLuaFileChangeData::FCA_Added, bIsDirectory));
					}
					return true;
				});
	}

	// Add root path
	AllFiles.Add(directoryFullPath);

	for (const FString& FolderName: AllFiles)
	{
		uint32 PathNameHash = GetPathNameHash(FolderName);

		// Check if we're already watching this directory
		if (!PathNameHashSet.Contains(PathNameHash))
		{
			// If we watch a directory twice, it'll return the same Watch Descriptor
			int32 NotifyFilter = IN_CREATE | IN_MOVE | IN_MODIFY | IN_DELETE | IN_ONLYDIR;
			int32 WatchDescriptor = inotify_add_watch(GFileDescriptor, TCHAR_TO_UTF8(*FolderName), NotifyFilter);
			
			if (WatchDescriptor == -1)
			{
				// ENOSPC: The user limit on the total number of inotify watches was reached or the kernel failed to allocate a needed resource.
				if (errno == ENOSPC)
				{
					UE_LOG(LogTemp, Warning, TEXT("inotify_add_watch cannot watch folder %s (Out of inotify watches)"),
					*FolderName);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("inotify_add_watch cannot watch folder %s (errno = %d, %s)"),
							*FolderName, errno, UTF8_TO_TCHAR(strerror(errno)));
				}
			}
			else
			{
				UE_LOG(LogTemp, VeryVerbose, TEXT("+ Added WatchDescriptor %d for '%s'"), WatchDescriptor, *FolderName);

				// Set the inotify watch descriptor -> folder name mapping
				FWatchInfo WatchInfo{ FolderName };
				GWatchDescriptorsToWatchInfo.Add(WatchDescriptor, WatchInfo);

				// Add hashed directory path
				PathNameHashSet.Add(PathNameHash);
			}
		}
	}
	//UE_LOG(LogTemp, Error, TEXT("FUnrealLuaFileSystemDirectoryWatcherLinux: added all subfiles for watching"));
}


void FUnrealLuaFileSystemDirectoryWatcherLinux::UnwatchDirectoryTree(const FString& RootAbsolutePath)
{
	checkf(IsInGameThread(), TEXT("INotify operations only support on main thread"));

	UE_LOG(LogTemp, VeryVerbose, TEXT("Unwatching tree '%s'"), *RootAbsolutePath);

	for (auto MapIt = GWatchDescriptorsToWatchInfo.CreateIterator(); MapIt; ++MapIt)
	{
		int WatchDescriptor = MapIt->Key;
		const FWatchInfo& WatchInfo = MapIt->Value;

		if (WatchInfo.FolderName.StartsWith(RootAbsolutePath, ESearchCase::CaseSensitive))
		{
			UE_LOG(LogTemp, VeryVerbose, TEXT("- Removing WatchDescriptor %d for '%s'"), WatchDescriptor, *WatchInfo.FolderName);

			PathNameHashSet.Remove(GetPathNameHash(WatchInfo.FolderName));

			// Safe version of:
			//   GWatchDescriptorsToWatchInfo.Remove(WatchDescriptor);
			MapIt.RemoveCurrent();

			// If that was the last reference to this watch descriptor, remove the inotify watch
			if (!GWatchDescriptorsToWatchInfo.Contains(WatchDescriptor))
			{
				// delete the descriptor
				int RetVal = inotify_rm_watch(GFileDescriptor, WatchDescriptor);

				UE_LOG(LogTemp, VeryVerbose, TEXT("- inotify_rm_watch(%d): %d"), WatchDescriptor, RetVal ? errno : 0);

				// This function may be called when root path has been deleted, and inotify_rm_watch() will fail
				// with an EINVAL when removing a watch on a deleted file.
				if (RetVal == -1 && errno != EINVAL)
				{
					UE_LOG(LogTemp, Warning, TEXT("inotify_rm_watch cannot remove descriptor %d for folder '%s' (errno = %d, %s)"),
							WatchDescriptor, *WatchInfo.FolderName, errno, ANSI_TO_TCHAR(strerror(errno)));
				}
			}
		}
	}
}


#endif
