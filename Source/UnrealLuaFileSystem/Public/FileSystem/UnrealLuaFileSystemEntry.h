#pragma once
#include "CoreMinimal.h"

class UUnrealLuaFileSystem;
struct FUnrealLuaFileSystemEntry;
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUnrealLuaFileSystemEntryUpdated, TSharedRef<FUnrealLuaFileSystemEntry>);


enum class EUnrealLuaDirectoryType : uint8
{
	None,
	RootDirectory,
	DefaultScriptDirectory,
	CompiledTypesDirectory,
	GameModeDirectory,
};
ENUM_CLASS_FLAGS(EUnrealLuaDirectoryType);

struct UNREALLUAFILESYSTEM_API FUnrealLuaFileSystemEntry : public TSharedFromThis<FUnrealLuaFileSystemEntry>
{
	/** Constructor for FDDFileTreeItem */
	FUnrealLuaFileSystemEntry(const TSharedPtr<FUnrealLuaFileSystemEntry> parentDir, const FString& fileSystemName, const FString& displayName, FDateTime modifiedTime, bool isFile)
		: ParentDir(parentDir)
		, FileSystemName(fileSystemName)
		, DisplayName(displayName)
		, ModifiedTime(modifiedTime)
		, bIsFile(isFile)
	{
		this->RebuildFullPath();
	}
	
	TArray<TSharedPtr<FUnrealLuaFileSystemEntry>>::RangedForIteratorType begin() { return this->AccessSubDirectories().begin(); }
	TArray<TSharedPtr<FUnrealLuaFileSystemEntry>>::RangedForConstIteratorType begin() const { return this->Children.begin(); }
	TArray<TSharedPtr<FUnrealLuaFileSystemEntry>>::RangedForIteratorType end() { return this->AccessSubDirectories().end(); }
	TArray<TSharedPtr<FUnrealLuaFileSystemEntry>>::RangedForConstIteratorType end() const { return this->Children.end(); }
	
	bool SaveFile(const FString& content) const;
	
	void RebuildFullPath()
	{
		FullPath = this->BuildFullPath();
	}
	/** @return Returns the parent or NULL if this is a root */
	const TSharedPtr<FUnrealLuaFileSystemEntry> GetParentFolder() const
	{
		return ParentDir.Pin();
	}
	
	void SetParentDir(TSharedPtr<FUnrealLuaFileSystemEntry> parent)
	{
		if (this->GetParentFolder() == parent || (parent.IsValid() && parent->IsFile()))
		{
			return;
		}
		this->RemoveFromParent();
		this->ParentDir = parent;
		if (parent.IsValid() && parent->IsDirectory())
		{
			parent->AddChild(this->SharedThis(this));
			this->RebuildFullPath();
		}
		this->OnFileUpdated.Broadcast(this->SharedThis(this));
	}

	const FString& GetFileSystemName()
	{
		return this->FileSystemName;
	}
	/** @return the path on hard disk, read-only */
	
	const FString& GetFullPath() const
	{
		return this->FullPath;
	}
	
	/** @return name to display in file tree view! read-only */
	const FString& GetDisplayName() const
	{
		return DisplayName;
	}

	/** @return Returns all subdirectories, read-only */
	const TArray<TSharedPtr<FUnrealLuaFileSystemEntry>>& GetSubDirectoriesAndFiles() const
	{
		return this->Children;
	}
	
	/** @return Returns all subdirectories, read-only, recursive */
	TArray<TSharedPtr<FUnrealLuaFileSystemEntry>> GetAllFilesRecursive(const TArray<FString>& excludedFileStarts = {}, const TArray<FString>& excludedFileEnds = {})
	{
		TArray<TSharedPtr<FUnrealLuaFileSystemEntry>> foundFiles = {};
		if (this->IsFile())
		{
			for (const FString& excludedFileStart : excludedFileStarts)
			{
				if (this->GetFileSystemName().StartsWith(excludedFileStart))
				{
					return {};
				}
			}
			for (const FString& excludedFileEnd : excludedFileEnds)
			{
				if (this->GetFileSystemName().EndsWith(excludedFileEnd))
				{
					return {};
				}
			}	

			foundFiles.Add(this->SharedThis(this));
			return foundFiles;
		}
		
		TArray<TSharedPtr<FUnrealLuaFileSystemEntry>> items = this->Children;
		for (auto item : items)
		{
			foundFiles.Append(item->GetAllFilesRecursive(excludedFileStarts, excludedFileEnds));
		}
		return foundFiles;
	}

	/** @return Returns all subdirectories, read or write */
	TArray<TSharedPtr<FUnrealLuaFileSystemEntry>>& AccessSubDirectories()
	{
		return Children;
	}

	/** Add a subdirectory to this node in the tree! */
	void AddChild(const TSharedPtr<FUnrealLuaFileSystemEntry> NewSubDir)
	{
		if (this->Contains(NewSubDir->FileSystemName))
		{
			return;
		}
		Children.Add(NewSubDir);
		this->Sort();
	}

	bool Contains(FStringView folderName) const
	{
		for (auto dir : Children)
		{
			if (folderName.Equals(dir->FileSystemName))
			{
				return true;
			}
		}
		return false;
	}

	bool IsDirectory() const
	{
		return !this->IsFile();
	}
	
	bool IsFile() const
	{
		return this->bIsFile;
	}
	
	bool IsInDefaultScriptDirectory() const
	{
		TSharedPtr<const FUnrealLuaFileSystemEntry> current = this->SharedThis(this);
		while (current.IsValid())
		{
			if (current->IsDefaultScriptDirectory())
			{
				return true;
			}
			current = current->GetParentFolder();
		}
		return false;
	}

	bool IsDefaultScriptDirectory() const
	{
		return this->DirectoryType == EUnrealLuaDirectoryType::DefaultScriptDirectory;
	}
	
	bool IsRoot() const
	{
		return this->DirectoryType == EUnrealLuaDirectoryType::RootDirectory;
	}

	void Sort();
	
	int32 HasChildren() const
	{
		return this->NumChildren() > 0;
	}
	
	int32 NumChildren() const
	{
		return this->Children.Num();
	}

	bool SaveFile(const FString& content);

	TSharedPtr<FUnrealLuaFileSystemEntry> FindChild(const FStringView& filesystemName) const;
	
	bool CanBeDeleted() const
	{
		return ! this->IsRoot();
	}
	
	bool IsValid() const
	{
		return this->IsRoot() || this->GetParentFolder();
	}

	
	FOnUnrealLuaFileSystemEntryUpdated& GetOnFileUpdatedDelegate ()
	{
		return this->OnFileUpdated;
	}

	void BroadcastUpdate();
	
	void MarkAsRoot(UUnrealLuaFileSystem& filesystem);
	void RemoveRootTag(UUnrealLuaFileSystem& filesystem);

	bool VerifyIntegrity() const;
	
	FDateTime GetLastModifiedFileTime() const
	{
		return ModifiedTime;
	}
	
	void SetModifiedFileTime(FDateTime modifiedTime)
	{
		ModifiedTime = modifiedTime;
		this->OnFileUpdated.Broadcast(this->SharedThis(this));
	}
	
	bool WasExternallyUpdated(FDateTime toCompare) const
	{
		return this->ModifiedTime < toCompare;
	}

	FString LoadFileToString();

	bool HasChild(const FString& name) const;
	TArray<TSharedPtr<FUnrealLuaFileSystemEntry>> GetSubDirectories() const;

	void SetDirectoryType(EUnrealLuaDirectoryType newDirectoryType);
private:
	void RemoveFromParent();
	FString BuildFullPath() const;
	
	/** Parent item or NULL if this is a root  */
	TWeakPtr<FUnrealLuaFileSystemEntry> ParentDir = {};

	/** Full path of this directory in the tree */
	FString FileSystemName = {};

	FString FullPath = {};
	
	/** Display name of the category */
	FString DisplayName = {};
	
	FDateTime ModifiedTime = FDateTime::MinValue();
	
	bool bIsFile = false;
	
	EUnrealLuaDirectoryType DirectoryType = EUnrealLuaDirectoryType::None;
	
	bool bIsInMod = false;
	
	bool bIsModRoot = false;
    
	/** Child categories */
	TArray<TSharedPtr<FUnrealLuaFileSystemEntry>> Children = {};
	
	FOnUnrealLuaFileSystemEntryUpdated OnFileUpdated = {};
};