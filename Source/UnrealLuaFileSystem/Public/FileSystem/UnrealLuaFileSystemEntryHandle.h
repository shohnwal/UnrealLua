// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"

#include "UnrealLuaFileSystemEntryHandle.generated.h"


struct FUnrealLuaFileSystemEntry;
/**
 * 
 */
USTRUCT()
struct UNREALLUAFILESYSTEM_API FUnrealLuaFileSystemEntryHandle
{
	GENERATED_BODY()
	
	FUnrealLuaFileSystemEntryHandle() = default;
	FUnrealLuaFileSystemEntryHandle(const TWeakPtr<FUnrealLuaFileSystemEntry>& fileSystemEntry);
	FUnrealLuaFileSystemEntryHandle(const FUnrealLuaFileSystemEntryHandle& other) = default;
	FUnrealLuaFileSystemEntryHandle(FUnrealLuaFileSystemEntryHandle&& other) noexcept = default;
	
	FUnrealLuaFileSystemEntryHandle& operator=(FUnrealLuaFileSystemEntryHandle&& other) noexcept = default;
	FUnrealLuaFileSystemEntryHandle& operator=(const FUnrealLuaFileSystemEntryHandle& other) noexcept = default;
	
	~FUnrealLuaFileSystemEntryHandle() = default;
	
	bool SaveFile(const FString& content) const;
	
	FUnrealLuaFileSystemEntryHandle GetParentFolder() const;
	bool IsValid() const;
	const FString& GetFileSystemName() const;
	const FString& GetFullPath() const;
	const FString& GetDisplayName() const;
	const TArray<TSharedPtr<FUnrealLuaFileSystemEntry>>& GetSubDirectories() const;
	bool Contains(FStringView fileSystemItemName) const;
	bool IsDirectory() const;
	bool IsFile() const;
	bool IsRoot() const;
	int32 HasChildren() const;
	int32 NumChildren() const;
private:
	friend class UUnrealLuaFileSystem;
	TWeakPtr<FUnrealLuaFileSystemEntry> FileSystemEntry = {};
};
