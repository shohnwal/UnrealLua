// Fill out your copyright notice in the Description page of Project Settings.


#include "FileSystem/UnrealLuaFileSystemEntryHandle.h"

#include "FileSystem/UnrealLuaFileSystemEntry.h"

namespace UnrealLua::FileSystem
{
	const FString EmptyString = {};
}
FUnrealLuaFileSystemEntryHandle::FUnrealLuaFileSystemEntryHandle(const TWeakPtr<FUnrealLuaFileSystemEntry>& fileSystemEntry)
	: FileSystemEntry(fileSystemEntry)
{
}

bool FUnrealLuaFileSystemEntryHandle::SaveFile(const FString& content) const
{
	if (!this->FileSystemEntry.IsValid())
	{
		return false;
	}
	return this->FileSystemEntry.Pin()->SaveFile(content);
}

FUnrealLuaFileSystemEntryHandle FUnrealLuaFileSystemEntryHandle::GetParentFolder() const
{
	if (!this->FileSystemEntry.IsValid())
	{
		return {};
	}
	TSharedPtr<FUnrealLuaFileSystemEntry> entry = this->FileSystemEntry.Pin();
	return FUnrealLuaFileSystemEntryHandle{entry->GetParentFolder()};
}

bool FUnrealLuaFileSystemEntryHandle::IsValid() const
{
	return this->FileSystemEntry.IsValid();
}

const FString& FUnrealLuaFileSystemEntryHandle::GetFileSystemName() const
{
	if (!this->FileSystemEntry.IsValid())
	{
		return UnrealLua::FileSystem::EmptyString;
	}
	return this->FileSystemEntry.Pin()->GetFileSystemName();
}

const FString& FUnrealLuaFileSystemEntryHandle::GetFullPath() const
{
	if (!this->FileSystemEntry.IsValid())
	{
		return UnrealLua::FileSystem::EmptyString;
	}
	return this->FileSystemEntry.Pin()->GetFullPath();
}

const FString& FUnrealLuaFileSystemEntryHandle::GetDisplayName() const
{
	if (!this->FileSystemEntry.IsValid())
	{
		return UnrealLua::FileSystem::EmptyString;
	}
	return this->FileSystemEntry.Pin()->GetDisplayName();	
}

const TArray<TSharedPtr<FUnrealLuaFileSystemEntry>>& FUnrealLuaFileSystemEntryHandle::GetSubDirectories() const
{
	return this->FileSystemEntry.Pin()->GetSubDirectoriesAndFiles();
}

bool FUnrealLuaFileSystemEntryHandle::Contains(FStringView fileSystemItemName) const
{
	if (!this->FileSystemEntry.IsValid())
	{
		return false;
	}
	return this->FileSystemEntry.Pin()->Contains(fileSystemItemName);	
}

bool FUnrealLuaFileSystemEntryHandle::IsDirectory() const
{
	if (!this->FileSystemEntry.IsValid())
	{
		return false;
	}
	return this->FileSystemEntry.Pin()->IsDirectory();
}

bool FUnrealLuaFileSystemEntryHandle::IsFile() const
{
	if (!this->FileSystemEntry.IsValid())
	{
		return false;
	}
	return this->FileSystemEntry.Pin()->IsFile();
}

bool FUnrealLuaFileSystemEntryHandle::IsRoot() const
{
	if (!this->FileSystemEntry.IsValid())
	{
		return false;
	}
	return this->FileSystemEntry.Pin()->IsRoot();
}

int32 FUnrealLuaFileSystemEntryHandle::HasChildren() const
{
	if (!this->FileSystemEntry.IsValid())
	{
		return false;
	}
	return this->FileSystemEntry.Pin()->HasChildren();
}

int32 FUnrealLuaFileSystemEntryHandle::NumChildren() const
{
	if (!this->FileSystemEntry.IsValid())
	{
		return 0;
	}
	return this->FileSystemEntry.Pin()->NumChildren();
}
