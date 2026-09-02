
#include "FileSystem/UnrealLuaFileSystemEntry.h"

#include "Subsystem/UnrealLuaFileSystem.h"

bool FUnrealLuaFileSystemEntry::SaveFile(const FString& content)
{
	if (!this->IsFile())
	{
		return false;
	}
	UUnrealLuaFileSystem::Get()->SaveFile(this->SharedThis(this), content);
	return true;
}

FString FUnrealLuaFileSystemEntry::BuildFullPath() const
{
	//FString projectDir = FPaths::ProjectContentDir(); 
	//FString rootDir = projectDir + "Lua";
	
	FString currentStr = this->FileSystemName;
	TArray<	TSharedPtr<FUnrealLuaFileSystemEntry>> chain{};
		
	TSharedPtr<FUnrealLuaFileSystemEntry> parentDirectory = this->ParentDir.Pin();
	while (parentDirectory.IsValid())
	{
		chain.Add(parentDirectory);
		parentDirectory = parentDirectory->GetParentFolder();
	}
		
	FStringBuilderBase builder;
		
	//builder << rootDir;
	for (int32 index = chain.Num() - 1; index >= 0; --index)
	{
		TSharedPtr<FUnrealLuaFileSystemEntry> item = chain[index];
		builder << item->GetFileSystemName();
		builder << "/";
	}
	builder << this->FileSystemName;
	
	return builder.ToString();
}

bool FUnrealLuaFileSystemEntry::SaveFile(const FString& content) const
{
	if (!this->IsFile())
	{
		return false;
	}
	return UUnrealLuaFileSystem::Get()->SaveFile(this->SharedThis(this), content);
}

void FUnrealLuaFileSystemEntry::Sort()
{
	this->Children.Sort([](const TSharedPtr<FUnrealLuaFileSystemEntry>& a, const TSharedPtr<FUnrealLuaFileSystemEntry>& b)
	{
		if (a->IsFile() && !b->IsFile())
		{
			return false;
		}
		if (!a->IsFile() && b->IsFile())
		{
			return true;
		}
		return a->DisplayName < b->DisplayName;
	});
}

void FUnrealLuaFileSystemEntry::BroadcastUpdate()
{
	this->GetOnFileUpdatedDelegate().Broadcast(this->SharedThis(this));
}

void FUnrealLuaFileSystemEntry::MarkAsRoot(UUnrealLuaFileSystem& filesystem)
{
	this->DirectoryType = EUnrealLuaDirectoryType::RootDirectory;
}

void FUnrealLuaFileSystemEntry::RemoveRootTag(UUnrealLuaFileSystem& filesystem)
{
	filesystem.RemoveRoot(this->SharedThis(this));
	this->SetDirectoryType(EUnrealLuaDirectoryType::None);
}

bool FUnrealLuaFileSystemEntry::VerifyIntegrity() const
{
	verify(this->IsFile() != this->IsDirectory());			//either we are a file or a directory
	verify(!this->FileSystemName.IsEmpty())					//must have a directory or file name
	verify(!this->DisplayName.IsEmpty())					//must have a display name
	verify(!this->FullPath.IsEmpty());						//most have a full path
	verify(this->IsRoot() != this->ParentDir.IsValid())		//Either we are root or we have a parent
	if (this->IsRoot())
	{
		verify(this->IsDirectory())							//root must be a directory
	}

	if (!this->IsRoot())
	{
		bool foundRoot = false;
	
		auto parent = this->GetParentFolder();
		while (parent.IsValid())
		{
			verify(parent.Get() != this)
			if (parent->IsRoot())
			{
				foundRoot = true;
				break;
			}
			parent = parent->GetParentFolder();
		}
		verify(foundRoot)
	}
	
	// check that all children have this as a parent
	for (auto child: this->Children)
	{
		verify(child->GetParentFolder() == this->SharedThis(this));
	}
	
	return true;
}

FString FUnrealLuaFileSystemEntry::LoadFileToString()
{
	FString fileContent;
	FFileHelper::LoadFileToString(fileContent, *this->GetFullPath());
	return fileContent;
}

bool FUnrealLuaFileSystemEntry::HasChild(const FString& name) const
{
	return this->FindChild(name) != nullptr;
}

TArray<TSharedPtr<FUnrealLuaFileSystemEntry>> FUnrealLuaFileSystemEntry::GetSubDirectories() const
{
	TArray<TSharedPtr<FUnrealLuaFileSystemEntry>> result;
	for (auto item : this->GetSubDirectoriesAndFiles())
	{
		if (item->IsDirectory())
		{
			result.Emplace(item);
		}
	}
	return result;
}

void FUnrealLuaFileSystemEntry::SetDirectoryType(EUnrealLuaDirectoryType newDirectoryType)
{
	this->DirectoryType = newDirectoryType;
}

void FUnrealLuaFileSystemEntry::RemoveFromParent()
{
	if (this->ParentDir.IsValid())
	{
		this->ParentDir.Pin()->Children.Remove(this->SharedThis(this));
		this->ParentDir.Reset();
	}
	this->FullPath.Empty();
}

TSharedPtr<FUnrealLuaFileSystemEntry> FUnrealLuaFileSystemEntry::FindChild(const FStringView& filesystemName) const
{
	const TSharedPtr<FUnrealLuaFileSystemEntry>* found = this->Children.FindByPredicate(
	[&filesystemName](const TSharedPtr<FUnrealLuaFileSystemEntry>& item)
	{
		return filesystemName.Equals(item->FileSystemName);
	});
	return found ? *found : nullptr;
}
