
#include "Config/UnrealLuaConfig.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealLuaCompiler.h"
#include "Utility/LuaLogMacros.h"

namespace UnrealLua::Compiler
{
	void LoadMissingTypesFromBackups(UUnrealLuaCompiler* compiler)
	{
		/*
		for (TTuple<FString, TUniquePtr<IStructPrototypeBase>>& item : compiler->UnrealLuaUTypePrototypes)
		{
			IStructPrototypeBase* proto = item.Value.Get();
			verify(proto->HasFinishedCompilation())
			verify(!proto->GetIsError());
			
		}
		
		FLuaPath backupPath{{}, NAME_None, ELuaPathFlags::UnrealTypes | ELuaPathFlags::BaseGame};
		FLuaFileLister backupFileLister{backupPath, ".CompiledBackup/", false, {"_", "."}, {UUnrealLuaConfig::GetLuaScriptModFileExtension()}};
		*/
	}
	
	
	void BackupCompiledPrototypes(UUnrealLuaCompiler* compiler)
	{
		LUA_LOG("Creating backups from compiled prototypes")
		
		verify(!compiler->HasErrors())
		
		if (compiler->UnrealLuaUTypePrototypes.IsEmpty())
		{
			return;
		}
		
		for (TTuple<FString, TUniquePtr<IStructPrototypeBase>>& item : compiler->UnrealLuaUTypePrototypes)
		{
			IStructPrototypeBase* proto = item.Value.Get();
			verify(proto->HasFinishedCompilation())
			verify(!proto->GetIsError());
		}
		
		FString compiledBackupPath = FPaths::ProjectContentDir() + "Lua/UnrealTypes/.CompiledBackup/";
		
		IFileManager& fileManager = IFileManager::Get();
		
		if (fileManager.DirectoryExists(*compiledBackupPath))
		{
			if (fileManager.IsReadOnly(*compiledBackupPath))
			{
				LUA_LOG_WARNING("Unable to backup compiled files to %s, directory is read-only!", *compiledBackupPath);
				return;
			}			
		}

		
		//Move previous backups to old folder, just in case
		
		LUA_LOG("Preparing compiled type backup files");
		bool success = true;
		
		TMap<FString, FString> fileNameToContentMap = {};
				
		for (TTuple<FString, TUniquePtr<IStructPrototypeBase>>& item : compiler->UnrealLuaUTypePrototypes)
		{
			const IStructPrototypeBase* proto = item.Value.Get();
			
			const TArray<FLuaCompilerSourceFileLine>& lines = compiler->FileContents.FindChecked(proto->FileName).GetLines();
			
			FStringBuilderBase contentBuilder;
			
			//Look for additional annotations on top of the type
			
			//scan upwards until there isn't a line beginning with "--" (Lua comment)
			int32 upperLineIndex = proto->DefinedLine;
			verify(lines[upperLineIndex].HasDefinedType());
			while (upperLineIndex > 0)
			{
				upperLineIndex--;
				FStringView currentContentLine = lines[upperLineIndex].Line;
				currentContentLine.TrimStartAndEndInline();
				if (!currentContentLine.StartsWith(TEXT("--")))
				{
					upperLineIndex++;
					break;
				}
			}
			
			for (int32 lineIndex = upperLineIndex; lineIndex < lines.Num(); ++lineIndex)
			{
				const FString* currentContentLine = &lines[lineIndex].Line;
				if (lines[lineIndex].HasDefinedType() && lineIndex != proto->DefinedLine)
				{
					//Another type started, we're done
					break;
				}
				contentBuilder << *currentContentLine << "\n";
			}
			
			UField* compiledField = proto->GetCompiledField();
			FString fullClassPPath = compiledField->GetPathName();
			fullClassPPath.ReplaceCharInline('/', '.');
			
			FString fullFilePath = compiledBackupPath + fullClassPPath;
			fullFilePath.Append(".compiled.lua");
			
			if (fileNameToContentMap.Contains(fullFilePath))
			{
				success = false;
				LUA_LOG_ERROR("Duplicate compiled Lua file name!: %s", *fullFilePath);
				break;
			}
			
			fileNameToContentMap.Emplace(fullFilePath, contentBuilder.ToString());
		}
				
		if (success)
		{
			//Successfully saved all data in temporary buffer
			LUA_LOG("Cleaning backup directories %s", *compiledBackupPath);
			fileManager.DeleteDirectory(*compiledBackupPath);
			if (fileManager.MakeDirectory(*compiledBackupPath))
			{
				for (const TTuple<FString, FString>& item : fileNameToContentMap)
				{
					//LUA_LOG("Saving prototype backup %s", *item.Key);
					const FString& filePath= item.Key;
					const FString& fileContent = item.Value;
					if (!FFileHelper::SaveStringToFile(*fileContent, *filePath))
					{
						LUA_LOG_ERROR("Could not save compiled type backup for filepath %s!", *filePath);
						break;
					}				
				}
			}
			else
			{
				LUA_LOG_ERROR("Unable to create backup directory %s for Lua-compiled types!\nIs the project directory read-only?",*compiledBackupPath)
			}

		}
		else
		{
			
		}
	}

}
