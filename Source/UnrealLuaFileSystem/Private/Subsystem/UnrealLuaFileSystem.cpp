// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystem/UnrealLuaFileSystem.h"

#include "DirectoryWatcher/UnrealLuaFileSystemDirectoryWatcherBase.h"
#include "FileSystem/UnrealLuaFileSystemEntry.h"
#include "FileSystem/UnrealLuaFileSystemEntryHandle.h"
#include "String/ParseTokens.h"
#include "Utility/LuaFIleSystemLogMacros.h"

namespace UnrealLua::FileSystem
{
	UUnrealLuaFileSystem* FileSystem = nullptr;
	
	const FString DefaultLuaScriptFolder = "DefaultScript";
	const FString LuaGameModeFolder = "GameMode";
	const FString DefaultLibsFolder = "Libs";
	const FString DefaultUnrealTypesFolder = "UnrealTypes";
}

UUnrealLuaFileSystem* UUnrealLuaFileSystem::Get()
{
	return UnrealLua::FileSystem::FileSystem;
}

void UUnrealLuaFileSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	LUA_FILES_LOG("UUnrealLuaFileSystem::Initialize")
	Super::Initialize(Collection);
	UnrealLua::FileSystem::FileSystem = this;

	this->ProjectContentDir = FPaths::ProjectContentDir();
	this->FullProjectContentDir = FPaths::ConvertRelativePathToFull(this->ProjectContentDir);
	this->RootLuaPath = FPaths::Combine(this->ProjectContentDir, "Lua");
	
	FUnrealLuaFileSystemDirectoryChanged onFileChangedDel;
	onFileChangedDel.BindUObject(this, &UUnrealLuaFileSystem::NotifyFileChanged	);
	this->FileWatcher = FUnrealLuaFileSystemDirectoryWatcherBase::Create(this->RootLuaPath, onFileChangedDel);
	this->FileWatcher->Init();
	
	this->InitializeRoot();
	
	this->ScanForFiles(true);
	
	this->CreateMissingFiles();
	
	this->AssignDirectoryFlags();
	
	FCoreDelegates::OnEndFrame.AddUObject(this, &UUnrealLuaFileSystem::NotifyEndFrame);
	
}

void UUnrealLuaFileSystem::Deinitialize()
{
	LUA_FILES_LOG("UUnrealLuaFileSystem::Deinitialize")
	if (this->FileWatcher.IsValid())
	{
		this->FileWatcher->Deinitialize();
		this->FileWatcher = nullptr;	
	}
	UnrealLua::FileSystem::FileSystem = nullptr;
	Super::Deinitialize();
}

void UUnrealLuaFileSystem::NotifyEndFrame()
{
	if (this->FileWatcher.IsValid())
	{
		this->FileWatcher->Tick();
	}
	this->Tick();
}

void UUnrealLuaFileSystem::InitializeRoot()
{
	

	verify(!this->Root.IsValid())
	verify(this->RootArray.IsEmpty());
	if (!IFileManager::Get().DirectoryExists(*this->RootLuaPath))
	{
		bool success = IFileManager::Get().MakeDirectory(*this->RootLuaPath, true);
	}
	
	int32 numToChop = this->ProjectContentDir.Len();
	
	FString rootDirDisplayName = "Lua";
	
	FDateTime now = FDateTime::UtcNow().ToUnixTimestamp();
	
	TSharedPtr<FUnrealLuaFileSystemEntry> RootDir = MakeShareable(new FUnrealLuaFileSystemEntry(NULL, *this->RootLuaPath, *rootDirDisplayName, now, false));
	this->Root = RootDir;
	this->Root->MarkAsRoot(*this);
	this->RootArray.Add(RootDir);
}

void UUnrealLuaFileSystem::ScanForFiles(bool fullRescan)
{
	LUA_FILES_LOG("UUnrealLuaFileSystem::ScanForFiles")
	verify(this->Root.IsValid())
	verify(this->Root->IsRoot())
	verify(!this->RootArray.IsEmpty())

	verify(!this->ProjectContentDir.IsEmpty())
	verify(this->ProjectContentDir == FPaths::ProjectContentDir())
	verify(this->Root->GetDisplayName() == "Lua"); 
	
	int32 numToChop = this->ProjectContentDir.Len();
	
	//TSharedPtr<FUnrealLuaFileSystemEntry> RootDir = MakeShareable(new FUnrealLuaFileSystemEntry(NULL, *rootDir, *rootDirDisplayName, false, true));
	//this->Root = RootDir;
	//this->RootArray.Add(RootDir);
	
	class FLocalDirIterator : public IPlatformFile::FDirectoryVisitor
	{
		TSharedPtr<FUnrealLuaFileSystemEntry> CurrentDirectory;

	public:
		FLocalDirIterator(TSharedPtr<FUnrealLuaFileSystemEntry> root)
			: CurrentDirectory(root)
		{}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			FStringView FilenameOrDirectoryStrv(FilenameOrDirectory);
			FDateTime now = FDateTime::UtcNow().ToUnixTimestamp();
			if (!bIsDirectory)
			{
				if (FilenameOrDirectoryStrv.EndsWith(FString(TEXT(".lua"))))
				{
					int32 index;
					FilenameOrDirectoryStrv.FindLastChar(TCHAR('/'), index);
					FStringView fileSystemName = FilenameOrDirectoryStrv.RightChop(index + 1);
					TSharedPtr<FUnrealLuaFileSystemEntry> newFileItem = MakeShared<FUnrealLuaFileSystemEntry>(this->CurrentDirectory, fileSystemName.GetData(), fileSystemName.GetData(), now, true);
					this->CurrentDirectory->AddChild( newFileItem );
					(void)newFileItem->VerifyIntegrity();
				}
				else
				{
					//file doesn't end with .lua -> ignore for now
				}
			}
			else
			{
				int32 index;
				FilenameOrDirectoryStrv.FindLastChar(TCHAR('/'), index);
				FStringView folderName = FilenameOrDirectoryStrv.RightChop(index + 1);
				
				TSharedPtr<FUnrealLuaFileSystemEntry> newDirectory = MakeShared<FUnrealLuaFileSystemEntry>(this->CurrentDirectory,folderName.GetData(), folderName.GetData(), now, false);
				CurrentDirectory->AddChild(newDirectory);
				(void)newDirectory->VerifyIntegrity();
				
				FLocalDirIterator DirIterator(newDirectory);
				IFileManager::Get().IterateDirectory(FilenameOrDirectory, DirIterator);		
			}
			CurrentDirectory->Sort();
			return true;
		}
	};

	FLocalDirIterator DirIterator(this->Root);
	IFileManager::Get().IterateDirectory(*this->RootLuaPath, DirIterator);
}

void UUnrealLuaFileSystem::CreateMissingFiles()
{
	verify(this->Root.IsValid())
	verify(this->Root->IsRoot())
	verify(!this->RootArray.IsEmpty())
	
	//Must be kept in sync with UnrealLua/Config/UnrealLuaConstants.h

	
	if (!this->GetRoot()->HasChild(UnrealLua::FileSystem::LuaGameModeFolder))
	{
		verify(this->CreateNewDirectory(this->Root.ToSharedRef(), UnrealLua::FileSystem::LuaGameModeFolder));
	}
	if (!this->GetRoot()->HasChild(UnrealLua::FileSystem::DefaultLuaScriptFolder))
	{
		verify(this->CreateNewDirectory(this->Root.ToSharedRef(), UnrealLua::FileSystem::DefaultLuaScriptFolder));
	}
	
	if (!this->GetRoot()->HasChild(UnrealLua::FileSystem::DefaultLibsFolder))
	{
		verify(this->CreateNewDirectory(this->Root.ToSharedRef(), UnrealLua::FileSystem::DefaultLibsFolder));
	}
	if (!this->GetRoot()->HasChild(UnrealLua::FileSystem::DefaultUnrealTypesFolder))
	{
		verify(this->CreateNewDirectory(this->Root.ToSharedRef(), UnrealLua::FileSystem::DefaultUnrealTypesFolder));
	}
}

void UUnrealLuaFileSystem::AssignDirectoryFlags()
{
	verify(this->Root->IsRoot())
	this->GetRoot()->FindChild(UnrealLua::FileSystem::LuaGameModeFolder)->SetDirectoryType(EUnrealLuaDirectoryType::GameModeDirectory);
	this->GetRoot()->FindChild(UnrealLua::FileSystem::DefaultUnrealTypesFolder)->SetDirectoryType(EUnrealLuaDirectoryType::CompiledTypesDirectory);
	this->GetRoot()->FindChild(UnrealLua::FileSystem::DefaultLuaScriptFolder)->SetDirectoryType(EUnrealLuaDirectoryType::DefaultScriptDirectory);
}

TSharedPtr<FUnrealLuaFileSystemEntry> UUnrealLuaFileSystem::GetRoot() const
{
	return this->Root;
}

const TArray<TSharedPtr<FUnrealLuaFileSystemEntry>>& UUnrealLuaFileSystem::GetRootArrayRef() const
{
	return this->RootArray;
}

bool UUnrealLuaFileSystem::CreateNewFile(const TSharedRef<FUnrealLuaFileSystemEntry>& parentDirectory, const FString& inFileName, bool modFile)
{
	if (!parentDirectory->IsDirectory())
	{
		UE_LOG(LogTemp, Log, TEXT("Can't create new file, parent is not a directory"))
		return false;
	}
	
	if (inFileName.IsEmpty())
	{
		//LUA_LOG_ERROR("Can't create new file, file name is empty")
		UE_LOG(LogTemp, Log, TEXT("Can't create new file, file name is empty"))
		return false;
	}
	for (TCHAR c : inFileName)
	{
		if ( !FChar::IsAlnum(c) && c != TCHAR('_') && c != TCHAR('.') )
		{
			//LUA_LOG_ERROR("Can't create new file %s : only alphanumeric characters, underscore '_' and colon '.' allowed", *fileName)
			UE_LOG(LogTemp, Log, TEXT("Can't create new file %s : only alphanumeric characters, underscore '_' and colon '.' allowed"),  *inFileName);
			return false;
		}		
	}

	FString fileName = inFileName;
	if (modFile)
	{
		if (!fileName.EndsWith(".mod.lua"))
		{
			if (fileName.EndsWith(".lua"))
			{
				fileName.LeftChopInline(4);
			}
			fileName.Append(".mod.lua");
		}		
	}
	else
	{
		if (!fileName.EndsWith(".lua"))
		{
			fileName.Append(".lua");
		}	
	}
	
	if (parentDirectory->Contains(fileName))
	{
		UE_LOG(LogTemp, Log, TEXT("File %s already exists in folder %s"), *fileName, *parentDirectory->GetFullPath());
		return false;
	}
	
	
	FString fullPath = FPaths::Combine(parentDirectory->GetFullPath(), fileName);
	
	if (!fullPath.StartsWith(this->ProjectContentDir))
	{
		//LUA_LOG_ERROR("Can not save new file to disk: path %s is not inside project directory!", *fullPath);
		UE_LOG(LogTemp, Log, TEXT("Can not create new file to disk: path %s is not inside project directory!"), *fullPath);
		return false;
	}
	
	FString fileContent = "";
	
	bool success = FFileHelper::SaveStringToFile(*fileContent, *fullPath, FFileHelper::EEncodingOptions::ForceUTF8);
	
	if (!success)
	{
		UE_LOG(LogTemp, Log, TEXT("Failed to create new file %s"), *fullPath)
		return false;
	}
	//UE_LOG(LogTemp, Log, TEXT("create file %s success!"), *fullPath)
	
	if (parentDirectory->Contains(fileName))
	{
		//file adding already handled. This is fine.
		//LUA_FILES_LOG("File %s added by UnrealLua file system, but already in the Unreal file system", *fullPath);
		verify(parentDirectory->FindChild(fileName)->IsFile())
		
		return success;
	}
	
	FDateTime now = FDateTime::UtcNow().ToUnixTimestamp();
	
	TSharedPtr<FUnrealLuaFileSystemEntry> newDirectory = MakeShared<FUnrealLuaFileSystemEntry>(parentDirectory, fileName, fileName, now, true);

	verify(fullPath.StartsWith(this->ProjectContentDir))

	parentDirectory->AddChild( newDirectory );

	(void)newDirectory->VerifyIntegrity();	

	//UE_LOG(LogTemp, Error, TEXT("File %s added"), *newDirectory->GetFullPath());

	this->OnFileSystemChanged.Broadcast();

	return true;
}

TSharedPtr<FUnrealLuaFileSystemEntry> UUnrealLuaFileSystem::CreateNewFile(const FString& filePath)
{
	if (filePath.IsEmpty())
	{
		return nullptr;
	}
	//UE_LOG(LogTemp, Log, TEXT("input filePath is %s"), *filePath);
	FString relativePath = IFileManager::Get().ConvertToRelativePath(*filePath);
	//UE_LOG(LogTemp, Log, TEXT("relative filePath is %s"), *relativePath);

	if (!relativePath.StartsWith(this->RootLuaPath))
	{
		//second chance, perhaps it's just a short path, leaving off the ../Content/Lua-prefix
		FString firstPart = filePath.Left(filePath.Find(TEXT("/")));
		TSharedPtr<FUnrealLuaFileSystemEntry> found = this->GetRoot()->FindChild(firstPart);
		if (found)
		{
			relativePath = this->RootLuaPath / filePath;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Can not find file descriptor: path %s is not inside lua project directory %s"), *relativePath, *this->RootLuaPath);
			return {};			
		}
	}

	verify(relativePath.RemoveFromStart(this->RootLuaPath))
	
	if (relativePath.IsEmpty())
	{
		return this->Root;
	}
	
	TArray<FStringView> filePathElements{};
	ParseIntoStringViewArray(relativePath, filePathElements, TEXT("/"));
	
	return this->CreateOrFindFileForFilePathInternal(filePathElements);
}

bool UUnrealLuaFileSystem::CreateNewDirectory(const TSharedRef<FUnrealLuaFileSystemEntry>& parentDirectory, const FString& directoryName)
{
	if (!parentDirectory->IsDirectory())
	{
		return false;
	}

	if (directoryName.IsEmpty())
	{
		//LUA_LOG_ERROR("Can't create new directory, directory name is empty")
		return false;
	}
	for (TCHAR c : directoryName)
	{
		if ( !FChar::IsAlnum(c) && c != TCHAR('_') && c != TCHAR('.'))
		{
			//LUA_LOG_ERROR("Can't create new directory %s : only alphanumeric characters, underscore '_' and colon '.' allowed", *directoryName)
			return false;
		}		
	}

	if (parentDirectory->Contains(directoryName))
	{
		if (parentDirectory->FindChild(directoryName)->IsDirectory())
		{
			//already a directory, this is fine
			return true;
		}
		else
		{
			//type conflict! This is a file
			return false;
		}
	}

	FString fullParentPath = parentDirectory->GetFullPath();

	if (!fullParentPath.StartsWith(this->ProjectContentDir))
	{
		//LUA_LOG_ERROR("Can not save new directory to disk: path %s is not inside project directory!", *fullPath);
		return false;
	}
	
	FString fullPath = FPaths::Combine(parentDirectory->GetFullPath(), directoryName);
	
	FDateTime now = FDateTime::UtcNow().ToUnixTimestamp();
	//
	//TSharedPtr<FUnrealLuaFileSystemEntry> newDirectory = MakeShared<FUnrealLuaFileSystemEntry>(parentDirectory, directoryName, directoryName, now, false);
	//
	//FString fullPath = newDirectory->GetFullPath();
	//
	//verify(fullPath.StartsWith(this->ProjectContentDir))
	
	bool success = IFileManager::Get().MakeDirectory(*fullPath, true);
	
	if (success)
	{
		if (parentDirectory->Contains(directoryName))
		{
			//file adding already handled. This is fine.
			//LUA_FILES_LOG("Directory %s added by UnrealLua file system, but already in the Unreal file system", *fullPath);
			verify(parentDirectory->FindChild(directoryName)->IsDirectory())
			
			return success;
		}
		
		TSharedPtr<FUnrealLuaFileSystemEntry> newDirectory = MakeShared<FUnrealLuaFileSystemEntry>(parentDirectory, directoryName, directoryName, now, false);

		verify(fullPath.StartsWith(this->ProjectContentDir))

		parentDirectory->AddChild( newDirectory );
	
		if (parentDirectory == this->Root)
		{
			if (directoryName == UnrealLua::FileSystem::DefaultLuaScriptFolder)
			{
				newDirectory->SetDirectoryType(EUnrealLuaDirectoryType::DefaultScriptDirectory);
			}
			else if (directoryName == UnrealLua::FileSystem::DefaultUnrealTypesFolder)
			{
				newDirectory->SetDirectoryType(EUnrealLuaDirectoryType::CompiledTypesDirectory);
			}
			else if (directoryName == UnrealLua::FileSystem::LuaGameModeFolder)
			{
				newDirectory->SetDirectoryType(EUnrealLuaDirectoryType::GameModeDirectory);
			}
		}
	
		(void)newDirectory->VerifyIntegrity();	
	
		//UE_LOG(LogTemp, Error, TEXT("Directory %s added"), *newDirectory->GetFullPath());
	
		this->OnFileSystemChanged.Broadcast();
	}
	//if (success)
	//{
	//	parentDirectory->AddChild( newDirectory );
	//	
	//	(void)newDirectory->VerifyIntegrity();
	//}
	//
	//this->OnFileSystemChanged.Broadcast();
	return success;
}

bool UUnrealLuaFileSystem::DeleteFileOrDirectory(const TSharedRef<FUnrealLuaFileSystemEntry>& toDelete)
{
	const FString filePath = toDelete->GetFullPath();
	
	FString fullFilePath = FPaths::ConvertRelativePathToFull(filePath);
	if (!fullFilePath.StartsWith(this->FullProjectContentDir))
	{
		//LUA_LOG_ERROR("Can not delete file: path %s is not inside project directory %s!", *filePath, *projectContentDir);
		return false;
	}
	
	if (!toDelete->CanBeDeleted())
	{
		return false;
	}

	bool success = false;
	if (toDelete->IsFile())
	{
		//UE_LOG(LogTemp, Log, TEXT("Deleting file %s"), *fullFilePath)
		if (IFileManager::Get().Delete(*fullFilePath, false, false))
		{
			success = true;
		}	
	}
	else if (toDelete->IsDirectory())
	{
		//UE_LOG(LogTemp, Log, TEXT("Deleting directory %s"), *fullFilePath)
		if (IFileManager::Get().DeleteDirectory(*fullFilePath, false, true))
		{
			success = true;
		}
	}
	//if (success)
	//{
	//	toDelete->SetParentDir(nullptr);
	//				
	//	if (toDelete->IsRoot())
	//	{
	//		toDelete->RemoveRootTag(*this);
	//		verify(!toDelete->IsRoot());
	//		verify(!this->Root.IsValid())
	//		verify(!this->RootArray.IsEmpty())
	//	}
	//	
	//	toDelete->BroadcastUpdate();
	//	
	//	int32 totalRemoved = 1;
	//	int32 removed = 0;
	//	do 
	//	{
	//		removed = this->RootArray.RemoveAll([](const TSharedPtr<FUnrealLuaFileSystemEntry>& item )
	//		{
	//			if (!item->IsValid())
	//			{
	//				item->BroadcastUpdate();
	//				return true;
	//			}
	//			return false;
	//		});
	//		totalRemoved += removed;
	//	} while (removed > 0);
	//	
	//	UE_LOG(LogTemp, Log, TEXT("Invalidated %d files and directories"), totalRemoved)
	//}
	//
	//this->OnFileSystemChanged.Broadcast();
	return success;
}

bool UUnrealLuaFileSystem::LoadFileToString(const FString& filePath, FString& outContent)
{
	outContent = {};
	TWeakPtr<FUnrealLuaFileSystemEntry> found = this->FindFileDescriptorForFilePath(filePath);
	if (!found.IsValid())
	{
		return false;
	}
	TSharedPtr<FUnrealLuaFileSystemEntry> foundFile = found.Pin();
	if (!foundFile->IsFile())
	{
		return false;
	}
	outContent = foundFile->LoadFileToString();
	return true;
}

TWeakPtr<FUnrealLuaFileSystemEntry> UUnrealLuaFileSystem::FindFileDescriptorForFilePath(const FString& filePath)
{
	if (filePath.IsEmpty())
	{
		return nullptr;
	}
	//UE_LOG(LogTemp, Log, TEXT("input filePath is %s"), *filePath);
	FString relativePath = IFileManager::Get().ConvertToRelativePath(*filePath);
	//UE_LOG(LogTemp, Log, TEXT("relative filePath is %s"), *relativePath);

	if (!relativePath.StartsWith(this->RootLuaPath))
	{
		//second chance, perhaps it's just a short path, leaving off the ../Content/Lua-prefix
		FString firstPart = filePath.Left(filePath.Find(TEXT("/")));
		TSharedPtr<FUnrealLuaFileSystemEntry> found = this->GetRoot()->FindChild(firstPart);
		if (found)
		{
			relativePath = this->RootLuaPath / filePath;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Can not find file descriptor: path %s is not inside lua project directory %s"), *relativePath, *this->RootLuaPath);
			return {};			
		}
	}

	verify(relativePath.RemoveFromStart(this->RootLuaPath))
	
	if (relativePath.IsEmpty())
	{
		return this->Root;
	}
	
	TArray<FStringView> filePathElements{};
	ParseIntoStringViewArray(relativePath, filePathElements, TEXT("/"));
	
	return this->FindFileDescriptorForFilePathInternal(filePathElements);
}

TWeakPtr<FUnrealLuaFileSystemEntry> UUnrealLuaFileSystem::FindFileDescriptorForFilePath(const TArray<FStringView>& filePathElements)
{
	return this->FindFileDescriptorForFilePathInternal(filePathElements);
}

TSharedPtr<FUnrealLuaFileSystemEntry> UUnrealLuaFileSystem::GetDefaultScriptDirectory()
{
	return this->Root->FindChild(UnrealLua::FileSystem::DefaultLuaScriptFolder);
}

TSharedPtr<FUnrealLuaFileSystemEntry> UUnrealLuaFileSystem::GetUnrealTypesCompilerDirectory()
{
	return this->Root->FindChild(UnrealLua::FileSystem::DefaultUnrealTypesFolder);
}

FSimpleMulticastDelegate& UUnrealLuaFileSystem::GetOnFileSystemChangedDelegate()
{
	return this->OnFileSystemChanged;
}

bool UUnrealLuaFileSystem::SaveFile(const TSharedRef<const FUnrealLuaFileSystemEntry>& targetFile, const FString& content)
{
	if (!targetFile->IsFile())
	{
		return false;
	}
	const FString& fullPath =  targetFile->GetFullPath();
	
	UE_LOG(LogTemp, Log, TEXT("Attempting to save file %s"), *fullPath);
	
	if (!fullPath.StartsWith(this->ProjectContentDir))
	{
		UE_LOG(LogTemp, Log, TEXT("Can not save file to disk: path %s is not inside project directory!"), *fullPath);
		//LUA_LOG_ERROR("Can not save new file to disk: path %s is not inside project directory!", *fullPath);
		return false;
	}
	
	if (FFileHelper::SaveStringToFile(*content, *fullPath, FFileHelper::EEncodingOptions::ForceUTF8))
	{
		UE_LOG(LogTemp, Log, TEXT("Success!"), *fullPath);
		//this->OnFileSystemChanged.Broadcast();
		//
		//targetFile->BroadcastUpdate();
		
		return true;
	}
	UE_LOG(LogTemp, Log, TEXT("Saving file failed!"), *fullPath);
	return false;
}

bool UUnrealLuaFileSystem::SaveFile(const FString& filePath, const FString& content)
{
	if (filePath.IsEmpty())
	{
		return false;
	}
	//UE_LOG(LogTemp, Log, TEXT("input filePath is %s"), *filePath);
	FString relativePath = IFileManager::Get().ConvertToRelativePath(*filePath);
	//UE_LOG(LogTemp, Log, TEXT("relative filePath is %s"), *relativePath);

	if (!relativePath.StartsWith(this->RootLuaPath))
	{
		//second chance, perhaps it's just a short path, leaving off the ../Content/Lua-prefix
		FString firstPart = filePath.Left(filePath.Find(TEXT("/")));
		TSharedPtr<FUnrealLuaFileSystemEntry> found = this->GetRoot()->FindChild(firstPart);
		if (found)
		{
			relativePath = this->RootLuaPath / filePath;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Can not find file descriptor to save file: path %s is not inside lua project directory %s"), *relativePath, *this->RootLuaPath);
			return {};			
		}
	}

	verify(relativePath.RemoveFromStart(this->RootLuaPath))
	
	if (relativePath.IsEmpty())
	{
		return false;
	}
	
	TArray<FStringView> filePathElements{};
	ParseIntoStringViewArray(relativePath, filePathElements, TEXT("/"));
	
	auto file = this->FindFileDescriptorForFilePathInternal(filePathElements);
	if (!file.IsValid())
	{
		file = this->CreateOrFindFileForFilePathInternal(filePathElements);
	}
	if (file.IsValid())
	{
		file.Pin()->SaveFile(content);
		return true;
	}
	return false;
}

bool UUnrealLuaFileSystem::MoveItemToDirectory(const TWeakPtr<FUnrealLuaFileSystemEntry>& toMove, const TWeakPtr<FUnrealLuaFileSystemEntry>& targetDirectory)
{
	if (!toMove.IsValid() || !targetDirectory.IsValid() || toMove == targetDirectory)
	{
		return false;
	}
	TSharedPtr<FUnrealLuaFileSystemEntry> toMovePtr = toMove.Pin();
	TSharedPtr<FUnrealLuaFileSystemEntry> targetDirectoryPtr = targetDirectory.Pin();
	if (toMovePtr->IsRoot() || !targetDirectoryPtr->IsDirectory() || toMovePtr->GetParentFolder() == targetDirectory)
	{
		return false;
	}
	
	if (!toMovePtr->GetFullPath().StartsWith(this->ProjectContentDir))
	{
		UE_LOG(LogTemp, Log, TEXT("Can not move file : source path %s is not inside project directory!"), *toMovePtr->GetFullPath());
		return false;
	}
	if (!targetDirectoryPtr->GetFullPath().StartsWith(this->ProjectContentDir))
	{
		UE_LOG(LogTemp, Log, TEXT("Can not move file %s : target path %s is not inside project directory!"), *toMovePtr->GetFileSystemName(), *targetDirectoryPtr->GetFullPath());
		return false;
	}
	FString toMoveCurrentFullPath = toMovePtr->GetFullPath();

	TSharedPtr<FUnrealLuaFileSystemEntry> oldParent = toMovePtr->GetParentFolder();
	
	verify(oldParent.IsValid());
	
	toMovePtr->SetParentDir(targetDirectoryPtr);
	
	verify(toMovePtr->GetParentFolder() == targetDirectoryPtr);
	
	verify(toMovePtr->GetFullPath().StartsWith(targetDirectoryPtr->GetFullPath()));
	
	FString toMoveNewFullPath = toMovePtr->GetFullPath();
	
	UE_LOG(LogTemp, Log, TEXT("Moving %s\nto\n%s"), *toMoveCurrentFullPath, *toMoveNewFullPath);
	bool success = IFileManager::Get().Move(*toMoveNewFullPath, *toMoveCurrentFullPath, true);

	if (!success)
	{
		//@TODO : undo file change
		toMovePtr->SetParentDir(oldParent);
		verify(toMovePtr->GetParentFolder() == oldParent);
	
		verify(toMovePtr->GetFullPath().StartsWith(oldParent->GetFullPath()));
	}
	toMovePtr->BroadcastUpdate();
	
	this->OnFileSystemChanged.Broadcast();
	
	
	return success;
}

const FString& UUnrealLuaFileSystem::GetLuaRootDirPath()
{
	return this->RootLuaPath;
}

void UUnrealLuaFileSystem::Tick()
{
	//const float dt = FApp::GetDeltaTime();
	//static float timer = 0.0f;
	//timer += dt;
	//if (timer < 0.5f)
	//{
	//	return;
	//}
	//timer = 0.0f;
	//
	//TArray<TSharedPtr<FUnrealLuaFileSystemEntry>> toUpdate;
	//
	//FDateTime now = FDateTime::UtcNow().ToUnixTimestamp();
	//for (TSharedPtr<FUnrealLuaFileSystemEntry>& tickedItem : this->WatchedItems)
	//{
	//	FDateTime directoryModifiedFromFileSystem = IFileManager::Get().GetTimeStamp(*tickedItem->GetFullPath());
	//	if (directoryModifiedFromFileSystem == FDateTime::MinValue())
	//	{
	//		//failure, perhaps item does no longer exist?
	//		
	//		if (!IFileManager::Get().DirectoryExists(*tickedItem->GetFullPath()))
	//		{
	//			this->NotifyItemNoLongerExists(tickedItem);
	//			toUpdate.Add(tickedItem);
	//			continue;
	//		}
	//		
	//	}
	//	if (directoryModifiedFromFileSystem > tickedItem->GetLastModifiedFileTime())
	//	{
	//		///item got modified
	//	}
	//	
	//	TArray<FString> files;
	//	IFileManager::Get().FindFiles(files, *tickedItem->GetFullPath(), true, true);
	//	
	//	for (auto file : files)
	//	{
	//		if (TSharedPtr<FUnrealLuaFileSystemEntry> child = tickedItem->FindChild(file))
	//		{
	//			FDateTime fileModified = IFileManager::Get().GetTimeStamp(*child->GetFullPath());
	//			if (child->WasExternallyUpdated(fileModified))
	//			{
	//				toUpdate.Add(child);
	//			}
	//		}
	//		else
	//		{
	//			if (IFileManager::Get().DirectoryExists(*tickedItem->GetFullPath()))
	//			{
	//				//Add directory
	//			}
	//			else
	//			{
	//				
	//			}
	//			this->AddNewlyDiscoveredFile(file, tickedItem);
	//		}
	//	}
	//}
}

TWeakPtr<FUnrealLuaFileSystemEntry> UUnrealLuaFileSystem::FindFileDescriptorForFilePathInternal(const TArray<FStringView>& filePathElements)
{
	TSharedPtr<FUnrealLuaFileSystemEntry> current = this->Root;
	
	for (int32 index = 0; index < filePathElements.Num(); index++)
	{
		const FStringView& element = filePathElements[index];
		current = current->FindChild(element);
		if (!current.IsValid())
		{
			return {};
		}
		if (current->IsFile())
		{
			if (index == filePathElements.Num() - 1)
			{
				break;
			}
			else
			{
				//LUA_LOG_WARNING("Found file in the middle of a full path: %s is inside %s", *element, *relativePath);
				UE_LOG(LogTemp, Error, TEXT("Found file in the middle of a full path: %s is inside %s"),element.GetData(), *current->GetFullPath());
				checkNoEntry()
			}
		}
	}

	return current;
}

TSharedPtr<FUnrealLuaFileSystemEntry> UUnrealLuaFileSystem::CreateOrFindFileForFilePathInternal(const TArray<FStringView>& filePathElements)
{
	TSharedPtr<FUnrealLuaFileSystemEntry> current = this->Root;
	
	for (int32 index = 0; index < filePathElements.Num(); index++)
	{
		bool shouldBeFile = index == filePathElements.Num() - 1;
		const FStringView& element = filePathElements[index];
		TSharedPtr<FUnrealLuaFileSystemEntry> child = current->FindChild(element); 
		if (!child.IsValid())
		{
			FString childName {element};
			if (shouldBeFile)
			{
				if (this->CreateNewFile(current.ToSharedRef(), childName))
				{
					return current->FindChild(childName);	
				}
				return nullptr;
			}
			else if (!this->CreateNewDirectory(current.ToSharedRef(), childName))
			{
				return nullptr;
			}
			child = current->FindChild(childName);
		}
		else
		{
			if (shouldBeFile)
			{
				verify(child->IsFile());
				return child;
			}
		}
		current = child;
	}
	return nullptr;
}

void UUnrealLuaFileSystem::ParseIntoStringViewArray(const FString& string, TArray<FStringView>& outArray, const TCHAR* pchDelim, bool bInCullEmpty)
{
	//UE_LOG(LogTemp, Log, TEXT("ParseIntoStringViewArray: %s"), *string);
	FStringView strv = string;
	int32 index = INDEX_NONE;
		
	while (strv.FindChar('/', index))
	{
		FStringView currentWord = strv.Mid(0, index);
		if (!currentWord.IsEmpty())
		{
			//UE_LOG(LogTemp, Log, TEXT("Adding word %s"), *FString(currentWord))
			outArray.Emplace(currentWord);
		}
		strv = strv.Mid(index + 1);
	}
	if (!strv.IsEmpty())
	{
		//UE_LOG(LogTemp, Log, TEXT("Adding last strv %s"), *FString(strv))
		outArray.Emplace(strv);
	}
}

void UUnrealLuaFileSystem::RemoveRoot(const TSharedRef<FUnrealLuaFileSystemEntry>& rootToRemove)
{
	verify(rootToRemove->IsRoot())
	if (this->Root == rootToRemove)
	{
		this->Root.Reset();
		this->RootArray.Empty();
	}
}

void UUnrealLuaFileSystem::NotifyFileChanged(const TArray<FUnrealLuaFileChangeData>& fileChangeDatas)
{
	for (const FUnrealLuaFileChangeData& fileChangeData : fileChangeDatas)
	{

		//UE_LOG(LogTemp, Error, TEXT("UUnrealLuaFileSystem::NotifyFileChanged : handling file change %s!"), *fileChangeData.Filename);
		if (fileChangeData.bIsDirectory)
		{
			switch (fileChangeData.Action)
			{
			case FUnrealLuaFileChangeData::FCA_Unknown:
				checkNoEntry()
				//UE_LOG(LogTemp, Error, TEXT("Directory %s unknown in %s"), *itemName, *parentFolder);
				break;
			case FUnrealLuaFileChangeData::FCA_Added:
				this->AddDirectoryByFilesystemCommand(fileChangeData);
				break;
			case FUnrealLuaFileChangeData::FCA_Modified:
				this->ModifyDirectoryByFilesystemCommand(fileChangeData);
				break;
			case FUnrealLuaFileChangeData::FCA_Removed:
				this->RemoveDirectoryByFilesystemCommand(fileChangeData);

				break;
			case FUnrealLuaFileChangeData::FCA_RescanRequired:
				this->RescanDirectoryByFilesystemCommand(fileChangeData);
				break;
			default: ;
			}			
		}
		else
		{
			switch (fileChangeData.Action)
			{
			case FUnrealLuaFileChangeData::FCA_Unknown:
				checkNoEntry()
				//UE_LOG(LogTemp, Error, TEXT("File %s unknown in %s"), *itemName, *parentFolder);
				break;
			case FUnrealLuaFileChangeData::FCA_Added:
				this->AddFileByFilesytemCommand(fileChangeData);
				break;
			case FUnrealLuaFileChangeData::FCA_Modified:
				this->FileModifiedByFilesytemCommand(fileChangeData);
				break;
			case FUnrealLuaFileChangeData::FCA_Removed:
				this->RemoveFileByFilesytemCommand(fileChangeData);
				break;
			case FUnrealLuaFileChangeData::FCA_RescanRequired:
				this->RescanFileByFilesystemCommand(fileChangeData);
				break;
			default: ;
			}			
		}
	}
}

void UUnrealLuaFileSystem::AddFileByFilesytemCommand(const FUnrealLuaFileChangeData& fileChangeData)
{
	
	FString parentFolder;
	FString itemName;
	fileChangeData.Filename.Split(TEXT("/"), &parentFolder, &itemName, ESearchCase::CaseSensitive, ESearchDir::FromEnd );
	
	TSharedPtr<FUnrealLuaFileSystemEntry> parentDirectory = this->FindFileDescriptorForFilePath(parentFolder).Pin();
	
	if (!parentDirectory)
	{
		return;
	}
	
	if (parentDirectory->Contains(itemName))
	{
		//file adding already handled. This is fine.
		//LUA_FILES_LOG("File %s added, but already in the file system", *fileChangeData.Filename);
		verify(parentDirectory->FindChild(itemName)->IsFile())
		
		this->OnFileSystemChanged.Broadcast();
		
		return;
	}
	
	FDateTime now = FDateTime::UtcNow().ToUnixTimestamp();
	//LUA_LOG("Creating new item %s in folder %s", *fileName, *parentDirectory->GetFullPath())
	TSharedPtr<FUnrealLuaFileSystemEntry> newFileItem = MakeShared<FUnrealLuaFileSystemEntry>(parentDirectory, itemName, itemName, now, true);
	
	parentDirectory->AddChild( newFileItem );
	
	(void)newFileItem->VerifyIntegrity();
	
	//LUA_FILES_LOG("File %s added", *newFileItem->GetFullPath());
	
	this->OnFileSystemChanged.Broadcast();
	
}

void UUnrealLuaFileSystem::RemoveFileByFilesytemCommand(const FUnrealLuaFileChangeData& fileChangeData)
{
	FString parentFolder;
	FString itemName;
	fileChangeData.Filename.Split(TEXT("/"), &parentFolder, &itemName, ESearchCase::CaseSensitive, ESearchDir::FromEnd );
	
	//UE_LOG(LogTemp, Error, TEXT("File %s removed in %s"), *itemName, *parentFolder);
	
	TSharedPtr<FUnrealLuaFileSystemEntry> parentDirectory = this->FindFileDescriptorForFilePath(parentFolder).Pin();

	if (!parentDirectory)
	{
		return;
	}
	
	//UE_LOG(LogTemp, Error, TEXT("Directory %s removing from %s"), *itemName, *parentFolder);
	
	TSharedPtr<FUnrealLuaFileSystemEntry> toDelete = parentDirectory->FindChild(itemName);
	if (toDelete)
	{
		verify(toDelete->IsFile())
		
		toDelete->SetParentDir(nullptr);
					
		if (toDelete->IsRoot())
		{
			toDelete->RemoveRootTag(*this);
			verify(!toDelete->IsRoot());
			verify(!this->Root.IsValid())
			verify(!this->RootArray.IsEmpty())
		}
		
		toDelete->BroadcastUpdate();
		
		int32 totalRemoved = 1;
		int32 removed = 0;
		do 
		{
			removed = this->RootArray.RemoveAll([](const TSharedPtr<FUnrealLuaFileSystemEntry>& item )
			{
				if (!item->IsValid())
				{
					item->BroadcastUpdate();
					return true;
				}
				return false;
			});
			totalRemoved += removed;
		} while (removed > 0);
		
		//UE_LOG(LogTemp, Log, TEXT("Invalidated %d files and directories"), totalRemoved)
	}
	
	//UE_LOG(LogTemp, Log, TEXT("Removed file callback success!"))
	
	this->OnFileSystemChanged.Broadcast();
}

void UUnrealLuaFileSystem::FileModifiedByFilesytemCommand(const FUnrealLuaFileChangeData& fileChangeData)
{
	FString parentFolder;
	FString itemName;
	fileChangeData.Filename.Split(TEXT("/"), &parentFolder, &itemName, ESearchCase::CaseSensitive, ESearchDir::FromEnd );
	
	//UE_LOG(LogTemp, Error, TEXT("File %s modified in %s"), *itemName, *parentFolder);
	
	TSharedPtr<FUnrealLuaFileSystemEntry> fileItem = this->FindFileDescriptorForFilePath(fileChangeData.Filename).Pin();
	
	if (!fileItem)
	{
		return;
	}
	//UE_LOG(LogTemp, Error, TEXT("File %s modified in %s, broadcsating"), *itemName, *parentFolder);
	fileItem->BroadcastUpdate();
}

void UUnrealLuaFileSystem::RescanFileByFilesystemCommand(const FUnrealLuaFileChangeData& fileChangeData)
{
	FString parentFolder;
	FString itemName;
	fileChangeData.Filename.Split(TEXT("/"), &parentFolder, &itemName, ESearchCase::CaseSensitive, ESearchDir::FromEnd );
	
	//UE_LOG(LogTemp, Error, TEXT("File %s rescan required in %s"), *itemName, *parentFolder);
}

void UUnrealLuaFileSystem::AddDirectoryByFilesystemCommand(const FUnrealLuaFileChangeData& fileChangeData)
{
	FDateTime now = FDateTime::UtcNow().ToUnixTimestamp();
	
	FString parentFolder;
	FString itemName;
	fileChangeData.Filename.Split(TEXT("/"), &parentFolder, &itemName, ESearchCase::CaseSensitive, ESearchDir::FromEnd );
	
	TSharedPtr<FUnrealLuaFileSystemEntry> parentDirectory = this->FindFileDescriptorForFilePath(parentFolder).Pin();
	
	if (!parentDirectory)
	{
		return;
	}
	
	if (parentDirectory->Contains(itemName))
	{
		//file adding already handled. This is fine.
		LUA_FILES_LOG("Directory %s added by file system command, but already in the Unreal file system", *fileChangeData.Filename);
		
		verify(parentDirectory->FindChild(itemName)->IsDirectory())
		
		return;
	}
	
	
	TSharedPtr<FUnrealLuaFileSystemEntry> newDirectory = MakeShared<FUnrealLuaFileSystemEntry>(parentDirectory, itemName, itemName, now, false);

	FString fullPath = newDirectory->GetFullPath();

	verify(fullPath.StartsWith(this->ProjectContentDir))

	parentDirectory->AddChild( newDirectory );
	
	if (parentDirectory == this->Root)
	{
		if (itemName == UnrealLua::FileSystem::DefaultLuaScriptFolder)
		{
			newDirectory->SetDirectoryType(EUnrealLuaDirectoryType::DefaultScriptDirectory);
		}
		else if (itemName == UnrealLua::FileSystem::DefaultUnrealTypesFolder)
		{
			newDirectory->SetDirectoryType(EUnrealLuaDirectoryType::CompiledTypesDirectory);
		}
		else if (itemName == UnrealLua::FileSystem::LuaGameModeFolder)
		{
			newDirectory->SetDirectoryType(EUnrealLuaDirectoryType::GameModeDirectory);
		}
	}
	
	(void)newDirectory->VerifyIntegrity();	
	
	//UE_LOG(LogTemp, Error, TEXT("Directory %s added"), *newDirectory->GetFullPath());
	
	this->OnFileSystemChanged.Broadcast();
}

void UUnrealLuaFileSystem::RemoveDirectoryByFilesystemCommand(const FUnrealLuaFileChangeData& fileChangeData)
{
	FString parentFolder;
	FString itemName;
	fileChangeData.Filename.Split(TEXT("/"), &parentFolder, &itemName, ESearchCase::CaseSensitive, ESearchDir::FromEnd );
	
	TSharedPtr<FUnrealLuaFileSystemEntry> parentDirectory = this->FindFileDescriptorForFilePath(parentFolder).Pin();

	if (!parentDirectory)
	{
		return;
	}
	
	//UE_LOG(LogTemp, Error, TEXT("Directory %s removing from %s"), *itemName, *parentFolder);
	
	TSharedPtr<FUnrealLuaFileSystemEntry> toDelete = parentDirectory->FindChild(itemName);
	if (toDelete)
	{
		toDelete->SetParentDir(nullptr);
					
		if (toDelete->IsRoot())
		{
			toDelete->RemoveRootTag(*this);
			verify(!toDelete->IsRoot());
			verify(!this->Root.IsValid())
			verify(!this->RootArray.IsEmpty())
		}
		
		toDelete->BroadcastUpdate();
		
		int32 totalRemoved = 1;
		int32 removed = 0;
		do 
		{
			removed = this->RootArray.RemoveAll([](const TSharedPtr<FUnrealLuaFileSystemEntry>& item )
			{
				if (!item->IsValid())
				{
					item->BroadcastUpdate();
					return true;
				}
				return false;
			});
			totalRemoved += removed;
		} while (removed > 0);
		
		//UE_LOG(LogTemp, Log, TEXT("Invalidated %d files and directories"), totalRemoved)
	}
	
	//UE_LOG(LogTemp, Log, TEXT("Removed directory callback success!"))
	
	this->OnFileSystemChanged.Broadcast();
}

void UUnrealLuaFileSystem::ModifyDirectoryByFilesystemCommand(const FUnrealLuaFileChangeData& fileChangeData)
{
	FString parentFolder;
	FString itemName;
	fileChangeData.Filename.Split(TEXT("/"), &parentFolder, &itemName, ESearchCase::CaseSensitive, ESearchDir::FromEnd );
	
	//UE_LOG(LogTemp, Error, TEXT("Directory %s modified in %s"), *itemName, *parentFolder);
	
}

void UUnrealLuaFileSystem::RescanDirectoryByFilesystemCommand(const FUnrealLuaFileChangeData& fileChangeData)
{
	FString parentFolder;
	FString itemName;
	fileChangeData.Filename.Split(TEXT("/"), &parentFolder, &itemName, ESearchCase::CaseSensitive, ESearchDir::FromEnd );
	//UE_LOG(LogTemp, Error, TEXT("Directory %s rescan required in "), *itemName, *parentFolder);
}
