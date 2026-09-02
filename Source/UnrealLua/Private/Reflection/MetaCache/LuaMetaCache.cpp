
#include "Reflection/MetaCache/LuaMetaCache.h"

#include "Utility/LuaLogMacros.h"
#include "Config/UnrealLuaConfig.h"
#include "Async/Async.h"
#include "Config/UnrealLuaConstants.h"
#include "Framework/Notifications/NotificationManager.h"
#include "GameFramework/Actor.h"
#include "Misc/FileHelper.h"
#include "Reflection/PropertyHelper.h"
#include "StructUtils/InstancedStruct.h"
#include "StructUtils/SharedStruct.h"
#include "UObject/Package.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "UnrealLua"

namespace UnrealLua::MetaCache
{
	const char* LuaMetaFolderSuffix = "Lua/.Meta";
	
	FBooleanMulticastDelegate OnProcessUpdate = {};

	FString GetLuaMetaFolderDir()
	{
		return FPaths::ProjectContentDir() + LuaMetaFolderSuffix;
	}

	
	void CreateUClassMetaData(UClass* uclass, const FString& metaLuaDirectory, IFileManager& files, bool bClearCache)
	{
		
		if(uclass->HasAnyClassFlags(EClassFlags::CLASS_NewerVersionExists))
		{
			return;
		}
		
		FString className = uclass->GetFName().ToString();
		if(className.StartsWith("SKEL_") || className.StartsWith("PLACEHOLDER-CLASS_") || className.StartsWith("TRASH_") || className.StartsWith("REINST_"))
		{
			return;
		}

		UPackage* package = uclass->GetPackage();
		const bool bIsEditorOnlyPackage = package->HasAllPackagesFlags(PKG_EditorOnly) || uclass->IsEditorOnly();
		const bool bIsUncookedOrDev = package->HasAnyPackageFlags(PKG_UncookedOnly | PKG_Developer);
		const bool bIsTransient = false;//package->HasAnyFlags(RF_Transient);
		if(bIsEditorOnlyPackage || bIsUncookedOrDev || bIsTransient)
		{
			return;
		}
		/*
		if(package == GetTransientPackage())
		{
			return;
		}
		*/
		if(package->GetName() == "/Engine/PythonTypes")
		{
			return;
		}
	
		FWideStringBuilderBase content;
		content << "---@meta\n\n";
	
		FName packageName = uclass->GetOuterUPackage()->GetFName();
	
		bool bIsNative = uclass->HasAnyClassFlags(EClassFlags::CLASS_Native);
		bool bIsActor = uclass->IsChildOf(AActor::StaticClass()); 
		bool bIsInterface = uclass->IsChildOf(UInterface::StaticClass()); 
		UClass* superClass = bIsInterface ? nullptr : uclass->GetSuperClass();

	
		FString prefixedClassname = (bIsActor ? "A" : bIsInterface ? "I" : "U") + className;
		// Lua/__Meta   /Script/CoreUObject      /  AActor.lua
		FString filePath = metaLuaDirectory + packageName.ToString() + "/" + prefixedClassname + ".lua";

		//if file exists for this class and we are not clearing cache, ignore
		if(files.FileExists(*filePath) && !bClearCache)
		{
			return;
		}
		
		bool bSuperIsActor = superClass ? superClass->IsChildOf(AActor::StaticClass()) : false;
		FString prefixedSuperClassName = superClass ? (bSuperIsActor ? "A" : "U") + superClass->GetName() : bIsInterface ? "UInterface" : ""; 
	
		FString interfaces = "";

		if(!bIsInterface)
		{
			for(auto& inf : uclass->Interfaces)
			{
				UClass* iclass = inf.Class;
				if(iclass)
				{
					FString iname = "I" + iclass->GetName();
					if(!interfaces.IsEmpty())
					{
						interfaces += ", ";
					}
					interfaces += iname;
				}
			}
		}

		FString inheritancePostfix = superClass || bIsInterface ? ": " + prefixedSuperClassName : "";
		if(!interfaces.IsEmpty())
		{
			if(inheritancePostfix.IsEmpty())
			{
				inheritancePostfix += ": ";
			}
			else
			{
				inheritancePostfix += ", ";
			}
			inheritancePostfix += interfaces;
		}
	
		content << "---@class " << prefixedClassname << inheritancePostfix << "\n";
	
		for(TFieldIterator<FProperty> it(uclass, EFieldIteratorFlags::SuperClassFlags::ExcludeSuper); it; ++it )
		{
			FString propName = it->GetName();
			//@TODO : also needs prefix
			content << "---@field " << propName << " " << UnrealLua::PropertyHelper::GetPropertyTypeName(*it, true) << "\n";
			//@TODO : add type name... for ints use integer, for float and double use number
		}

		//BP Libraries are globally accessible via class name (which gets the FLuaUClass)
		//Also, by doing this we say that all UClasses are globally accessible
		//when calling something on it, the CDO will be used

		//DONT do empty line between fields and class table, it will disassociate the type from the global
		
		//content << "---@type " + prefixedClassname << "\n";
		content << prefixedClassname << " = {}\n\n";
	
		for (TFieldIterator<UFunction> it(uclass, EFieldIteratorFlags::ExcludeSuper); it; ++it)
		{
#if WITH_EDITOR
			FText txt = it->GetToolTipText();
			FString tooltipStr = txt.ToString();
			tooltipStr.ReplaceInline(TEXT("@param"), TEXT("\t@param"));
			tooltipStr.ReplaceInline(TEXT("@return"), TEXT("\t@return"));
			int32 idx = tooltipStr.Find(TEXT("\t@"));
				
			content << "--[[\n" << tooltipStr << "\n]]\n";
#endif
			FString funcName = it->GetName();
			content << "---@param self " << prefixedClassname <<"\n";
		
			FString paramNames = "self";
			
			for(TFieldIterator<FProperty> prop(*it); prop; ++prop)
			{
				FString propName = prop->GetName();
			
				if(prop->HasAllPropertyFlags(CPF_Parm) && !prop->HasAnyPropertyFlags(CPF_ReturnParm))
				{
					FString outParmPostFix = "";
					if(prop->HasAnyPropertyFlags(CPF_ConstParm))
					{
						outParmPostFix += " const";
					}
					else if(prop->HasAnyPropertyFlags(CPF_OutParm))
					{
						outParmPostFix += " out";
					}
					if(prop->HasAnyPropertyFlags(CPF_ReferenceParm))
					{
						outParmPostFix += " ref";
					}
					if(!outParmPostFix.IsEmpty())
					{
						outParmPostFix = outParmPostFix.TrimStart();
						outParmPostFix = FString::Printf(TEXT(" (%s)"),*outParmPostFix);
					}
					content << "---@param " << propName << " " << UnrealLua::PropertyHelper::GetPropertyTypeName(*prop, true) << "?" << outParmPostFix << "\n";
					paramNames += ", " + propName;
				}
			}

			if(FProperty* retprop = it->GetReturnProperty())
			{
				FString propName = retprop->GetName();
				content << "---@return " << UnrealLua::PropertyHelper::GetPropertyTypeName(retprop, true) << " " << "\n";
			}

			for(TFieldIterator<FProperty> prop(*it); prop; ++prop)
			{
				FString propName = prop->GetName();
			
				if(prop->HasAllPropertyFlags(CPF_OutParm) && !prop->HasAllPropertyFlags(CPF_ConstParm) && !prop->HasAnyPropertyFlags(CPF_ReturnParm))
				{
					content << "---@return " << UnrealLua::PropertyHelper::GetPropertyTypeName(*prop, true) << " " << propName << "\n";
				}
			}
		
			content << "function " << prefixedClassname << "." << funcName << "(" << paramNames << ") end\n\n";
		}

		if(uclass == UObject::StaticClass())
		{
			content << UnrealLua::MetaCache::FUObjectPropertyWrapperAdditions; 
		}
		FFileHelper::SaveStringToFile(content.ToString(), *filePath, FFileHelper::EEncodingOptions::ForceUTF8);
	};

	void CreateUScriptStructMetaData(UScriptStruct* strct, const FString& metaLuaDirectory, IFileManager& files, bool bClearCache)
	{
		UScriptStruct* iss = FInstancedStruct::StaticStruct();
		UScriptStruct* sss = FSharedStruct::StaticStruct();
		
		UPackage* package = strct->GetPackage();
		const bool bIsEditorOnlyPackage = package->HasAllPackagesFlags(PKG_EditorOnly) || strct->IsEditorOnly();
		const bool bIsUncookedOrDev = package->HasAnyPackageFlags(PKG_UncookedOnly | PKG_Developer);
		const bool bIsTransient = false;//package->HasAnyFlags(RF_Transient);
		if(bIsEditorOnlyPackage || bIsUncookedOrDev || bIsTransient)
		{
			return;
		}
		if(strct == iss || strct == sss)
		{
			return;
		}
		/*
		if(package == GetTransientPackage())
		{
			return;
		}
		*/
		if(package->GetName() == "/Engine/PythonTypes")
		{
			return;
		}
		FWideStringBuilderBase content;
		content << "---@meta\n\n";
		FString structName = strct->GetFName().ToString();

		FName packageName = strct->GetPackage()->GetFName();
		UStruct* superStruct = strct->GetSuperStruct();
		FString prefixedStructName = "F" + structName;
		
		// Lua/__Meta   /Script/CoreUObject      /  AActor.lua
		FString filePath = metaLuaDirectory + packageName.ToString() + "/" + prefixedStructName + ".lua";
		//if file exists for this class and we are not clearing cache, ignore
		if(files.FileExists(*filePath) && !bClearCache)
		{
			return;
		}
			
		FString prefixedSuperClassName = superStruct ? "F" + superStruct->GetName() : ""; 
		FString inheritancePostfix = superStruct ? ": " + prefixedSuperClassName : ": struct";
		content << "---@class " << prefixedStructName << inheritancePostfix << "\n";

		//simple call constructor
		content << "---@operator call: " << prefixedStructName << "\n";

		//overload for table constructors
		content << "---@overload fun(init: " << prefixedStructName << "): " << prefixedStructName << "\n";
		content << "---@operator call:" << prefixedStructName << "\n";

		//overload for single fields in order
		content << "---@overload fun(";
		for(TFieldIterator<FProperty> it(strct, EFieldIteratorFlags::SuperClassFlags::ExcludeSuper); it; ++it )
		{
			FString propName = it->GetName();
			content << propName << ": " << UnrealLua::PropertyHelper::GetPropertyTypeName(*it, true) << "?";
			if(it->Next != nullptr)
			{
				content << ", ";
			}
		}
		content << "): " << prefixedStructName << "\n";
		content << "---@operator call:" << prefixedStructName << "\n";
			
		for(TFieldIterator<FProperty> it(strct, EFieldIteratorFlags::SuperClassFlags::ExcludeSuper); it; ++it )
		{
			FString propName = it->GetName();
			content << "---@field " << propName << " " << UnrealLua::PropertyHelper::GetPropertyTypeName(*it, true) << "?\n";
		}

		//Structs can be called globally for construction

		//DONT do empty line between fields and class table, it will disassociate the type from the global
		
		//content << "---@type " + prefixedStructName << "\n";
		content << prefixedStructName << " = {}";
		
		FFileHelper::SaveStringToFile(content.ToString(), *filePath, FFileHelper::EEncodingOptions::ForceUTF8);
	};

	void CreateUEnumMetaData(UEnum* uenum, const FString& metaLuaDirectory, IFileManager& files, bool bClearCache)
	{
		UPackage* package = uenum->GetPackage();
		const bool bIsEditorOnlyPackage = package->HasAllPackagesFlags(PKG_EditorOnly) || uenum->IsEditorOnly();
		const bool bIsUncookedOrDev = package->HasAnyPackageFlags(PKG_UncookedOnly | PKG_Developer);
		const bool bIsTransient = false;//package->HasAnyFlags(RF_Transient);
		if(bIsEditorOnlyPackage || bIsUncookedOrDev || bIsTransient)
		{
			return;
		}
		/*
		if(package == GetTransientPackage())
		{
			return;
		}
		*/
		if(package->GetName() == "/Engine/PythonTypes")
		{
			return;
		}

		// Ignore abstract, hidedropdown, and deprecated.
		if (uenum->HasAnyEnumFlags(EEnumFlags::NewerVersionExists))
		{
			return;
		}
			
		FString enumName = uenum->GetFName().ToString();
		FName packageName = uenum->GetPackage()->GetFName();

		FString filePath = metaLuaDirectory + packageName.ToString() + "/" + enumName + ".lua";
		//if file exists for this class and we are not clearing cache, ignore
		if(files.FileExists(*filePath) && !bClearCache)
		{
			return;
		}
		int32 numEnumEntries = uenum->NumEnums();
		FName MaxEnumItem = *uenum->GenerateFullEnumName(*(uenum->GenerateEnumPrefix() + TEXT("_MAX")));
		int64 MAX_Value = uenum->GetIndexByName(MaxEnumItem, EGetByNameFlags::CaseSensitive);

		FWideStringBuilderBase content;
		content << "---@meta\n\n";

		content << "---@enum " << enumName << "\n";
		content << enumName << " = {\n";

		//struct FEnumIndexValuePair { int32 index; uint64 value; FName name; };

		//TArray<FEnumIndexValuePair> enumIndicesToValues;

		for(int32 i = 0; i < numEnumEntries; i++)
		{
			int64 value = uenum->GetValueByIndex(i);
			if(value == MAX_Value)
			{
				break;
			}
			FString name = uenum->GetNameStringByIndex(i);
			//enumIndicesToValues.Emplace(i, value, name);
				
			content << "  " << name << " = " << value;
			content << ((i < numEnumEntries - 1) ? ",\n" : "\n");
		}
		content << "}";
			
		FFileHelper::SaveStringToFile(content.ToString(), *filePath, FFileHelper::EEncodingOptions::ForceUTF8);
			
	};

	static bool bIsWorkingOnMetadata = false;
	
	void CreateClassMetaDatabase(bool bForce)
	{
		//only set if in editor or LuaConfig has DevMode=true
		if(!bForce/* && !FLuaConfig::bLuaDevMode*/)
		{
			return;
		}
		if(bIsWorkingOnMetadata)
		{
			LUA_LOG("Already working on creating Lua metadata, be patient!")
			return;
		}
		bIsWorkingOnMetadata = true;
		OnProcessUpdate.Broadcast(false);
		bool clearCache = false;//UUnrealLuaConfig::ShouldClearLuaMetaCache();

		LUA_LOG("Creating Lua metadata")
		FNotificationInfo Info(LOCTEXT("LuaMetaAnalysis_Notification", "Creating Lua metadata."));
		//Set a default expire duration
		Info.ExpireDuration = 600.0f;
		Info.bUseThrobber = true;
	
		//And call Add Notification, this is pretty much it!
		auto infoHandle = FSlateNotificationManager::Get().AddNotification(Info);

		AsyncTask(ENamedThreads::AnyThread, [infoHandle, clearCache]()
		{
			IFileManager& files = IFileManager::Get();
			const FString metaFolder = GetLuaMetaFolderDir();

			if(clearCache)
			{
				files.DeleteDirectory(*metaFolder);
			}

			/*
			if(files.DirectoryExists(*metaFolderBase))
			{
				return;
			}
			*/
			//files.DeleteDirectory(*metaFolderBase, false, true);
			if(!files.DirectoryExists(*metaFolder))
			{
				files.MakeDirectory(*metaFolder);	
			}
			

			auto loadIntrinsics = [&files, &metaFolder, clearCache](const char* file, const char* hardcoded)
			{
				FString filePath = metaFolder + "/" + file + ".lua";
				if(!files.FileExists(*filePath) || clearCache)
				{
					FWideStringBuilderBase content;
					content << hardcoded;
					FFileHelper::SaveStringToFile(content.ToString(), *filePath, FFileHelper::EEncodingOptions::ForceUTF8);
				}				
			};

			loadIntrinsics("Primitives", UnrealLua::MetaCache::PrimitivesDef);
			loadIntrinsics("Structs", UnrealLua::MetaCache::StructUtilsDef);
			loadIntrinsics("Delegates", UnrealLua::MetaCache::FDelegatesDef);
			loadIntrinsics("TSubclassOf", UnrealLua::MetaCache::TSubclassOfDef);
			loadIntrinsics("TWeakObjectPtr", UnrealLua::MetaCache::TWeakObjectPtrDef);
			loadIntrinsics("TScriptInterface", UnrealLua::MetaCache::TScriptInterfaceDef);
			loadIntrinsics("TArray", UnrealLua::MetaCache::TArrayDef);
			loadIntrinsics("TMap", UnrealLua::MetaCache::TMapDef);
			loadIntrinsics("TSet", UnrealLua::MetaCache::TSetDef);
			loadIntrinsics("GlobalFunctions", UnrealLua::MetaCache::GlobalFuncs);

			for (UEnum* uenum : TObjectRange<UEnum>())
			{
				if (!uenum->IsNative())
				{
					continue;
				}

				UnrealLua::MetaCache::CreateUEnumMetaData(uenum, metaFolder, files, clearCache);
			}
		
			for (UScriptStruct* strct : TObjectRange<UScriptStruct>())
			{
				UnrealLua::MetaCache::CreateUScriptStructMetaData(strct, metaFolder, files, clearCache);
			}
		
			for (UClass* clazz : TObjectRange<UClass>())
			{
				UnrealLua::MetaCache::CreateUClassMetaData(clazz, metaFolder, files, clearCache);
			}
			
			AsyncTask(ENamedThreads::GameThread, [infoHandle]()
			{
				infoHandle->SetExpireDuration(0.5f);
				infoHandle->ExpireAndFadeout();
				LUA_LOG("Finished creating Lua metadata")
				FNotificationInfo Info(LOCTEXT("LuaMetaAnalysisFinished_Notification", "Finished creating Lua metadata"));
		
				//Set a default expire duration
				Info.ExpireDuration = 5.0f;
				
				//And call Add Notification, this is pretty much it!
				FSlateNotificationManager::Get().AddNotification(Info);

				bIsWorkingOnMetadata = false;
				
				OnProcessUpdate.Broadcast(true);
			});
		});
	}
}
#undef LOCTEXT_NAMESPACE