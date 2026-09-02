// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IDirectoryWatcher.h"
#include "StructUtils/InstancedStruct.h"
#include "Subsystems/EngineSubsystem.h"
#include "UnrealLuaTools/Public/UIWidgets/FileBrowser/SLuaScriptEditorFileBrowser.h"
#include "UnrealLuaFileSystem.generated.h"

struct FUnrealLuaFileChangeData;
struct FUnrealLuaFileSystemDirectoryWatcherBase;
struct FUnrealLuaFileSystemEntryHandle;
struct FUnrealLuaFileSystemEntry;
/**
 * 
 */

UCLASS()
class UNREALLUAFILESYSTEM_API UUnrealLuaFileSystem : public UEngineSubsystem
{
public:
	GENERATED_BODY()
	
	static UUnrealLuaFileSystem* Get();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	void NotifyEndFrame();
	
	void InitializeRoot();
	void ScanForFiles(bool fullRescan = false);
	void CreateMissingFiles();
	void AssignDirectoryFlags();
	TSharedPtr<FUnrealLuaFileSystemEntry> GetRoot() const;
	const TArray<TSharedPtr<FUnrealLuaFileSystemEntry>>& GetRootArrayRef() const;
	
	bool CreateNewFile(const TSharedRef<FUnrealLuaFileSystemEntry>& parentDirectoryPtr, const FString& inFileName, bool modFile = false);
	TSharedPtr<FUnrealLuaFileSystemEntry> CreateNewFile(const FString& filePath);
	bool CreateNewDirectory(const TSharedRef<FUnrealLuaFileSystemEntry>& parentDirectoryPtr, const FString& directoryName);
	bool DeleteFileOrDirectory(const TSharedRef<FUnrealLuaFileSystemEntry>& toDelete);
	
	bool LoadFileToString(const FString& filePath, FString& outContent);
	
	TWeakPtr<FUnrealLuaFileSystemEntry> FindFileDescriptorForFilePath(const FString& filePath);
	TWeakPtr<FUnrealLuaFileSystemEntry> FindFileDescriptorForFilePath(const TArray<FStringView>& filePathElements);
	
	TSharedPtr<FUnrealLuaFileSystemEntry> GetDefaultScriptDirectory();
	TSharedPtr<FUnrealLuaFileSystemEntry> GetUnrealTypesCompilerDirectory();
	
	FSimpleMulticastDelegate& GetOnFileSystemChangedDelegate();

	bool SaveFile(const TSharedRef<const FUnrealLuaFileSystemEntry>& targetFile, const FString& content);
	bool SaveFile(const FString& path, const FString& content);

	bool MoveItemToDirectory(const TWeakPtr<FUnrealLuaFileSystemEntry>& toMove, const TWeakPtr<FUnrealLuaFileSystemEntry>& targetDirectory);
	
	const FString& GetLuaRootDirPath();
	const FString& GetDefaultScriptFileDirectoryLocation();
private:
	void Tick();
	
	TWeakPtr<FUnrealLuaFileSystemEntry> FindFileDescriptorForFilePathInternal(const TArray<FStringView>& filePathElements);
	TSharedPtr<FUnrealLuaFileSystemEntry> CreateOrFindFileForFilePathInternal(const TArray<FStringView>& filePathElements);
	FSimpleMulticastDelegate OnFileSystemChanged;
	
	void ParseIntoStringViewArray(const FString& string, TArray<FStringView>& outArray, const TCHAR* delim, bool cullEmpty = true);
		
	void RemoveRoot(const TSharedRef<FUnrealLuaFileSystemEntry>& rootToRemove);

	void NotifyFileChanged(const TArray<FUnrealLuaFileChangeData>& fileChangeDatas);
	
	void AddFileByFilesytemCommand(const FUnrealLuaFileChangeData& fileChangeData);
	void RemoveFileByFilesytemCommand(const FUnrealLuaFileChangeData& fileChangeData);
	void FileModifiedByFilesytemCommand(const FUnrealLuaFileChangeData& fileChangeData);
	void RescanFileByFilesystemCommand(const FUnrealLuaFileChangeData& fileChangeData);
	
	void AddDirectoryByFilesystemCommand(const FUnrealLuaFileChangeData& fileChangeData);
	void RemoveDirectoryByFilesystemCommand(const FUnrealLuaFileChangeData& fileChangeData);
	void ModifyDirectoryByFilesystemCommand(const FUnrealLuaFileChangeData& fileChangeData);
	void RescanDirectoryByFilesystemCommand(const FUnrealLuaFileChangeData& fileChangeData);
	
	TSharedPtr<FUnrealLuaFileSystemEntry> Root = {};
	TArray<TSharedPtr<FUnrealLuaFileSystemEntry>> RootArray = {};
	
	TMap<FString, TSharedPtr<FUnrealLuaFileSystemEntry>> ModRoots = {};
	TMap<FString, TArray<TSharedPtr<FUnrealLuaFileSystemEntry>>> ModArrays = {};
	
	friend struct FUnrealLuaFileSystemEntry;
	
	UPROPERTY()
	FString ProjectContentDir = "";
		
	UPROPERTY()
	FString FullProjectContentDir = "";
	
	UPROPERTY(VisibleAnywhere)
	FString RootLuaPath = "";
	
	UPROPERTY(VisibleAnywhere)
	FString RootLuaDisplayName = "Lua";
	
	TSharedPtr<FUnrealLuaFileSystemDirectoryWatcherBase> FileWatcher = {};
	
	TArray<TSharedPtr<FUnrealLuaFileSystemEntry>> WatchedItems = {};
};
