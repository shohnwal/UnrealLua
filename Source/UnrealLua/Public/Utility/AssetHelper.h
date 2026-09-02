#pragma once
#include "CoreMinimal.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Engine/AssetManager.h"
#include "Runtime/CoreUObject/Public/AssetRegistry/AssetData.h"
#include "Engine/ObjectLibrary.h"


class UObjectLibrary;

struct UNREALLUA_API FAssetHelper
{
	static UNREALLUA_API FString ParseToFullPath(const FString& path);

	inline static bool bHasScannedForWidgetBlueprints = false;
	/** Note: Class names etc have been changed for public use. */
	template <class T> static void RegisterWidgetBlueprintsPrimaryAssets( const FString Path /*= "/Game"*/, const bool bShowAssetManagerDump /*= false*/ )
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

		if ( Path.IsEmpty() )
		{
			return;
		}

		TArray<FString> ContentPaths;
		ContentPaths.Add(Path);
	  
		/** Scan ContenPaths recursivley */
		AssetRegistry.ScanPathsSynchronous(ContentPaths, true);

		TSet<FTopLevelAssetPath> DerivedNames;
		{
			TArray<FTopLevelAssetPath> BaseNames; //The base Class Names
			BaseNames.Add(T::StaticClass()->GetStructPathName());
			
			TSet<FTopLevelAssetPath> Excluded;
	    
			/** Get a list of class names derived from BaseNames */
			AssetRegistry.GetDerivedClassNames(BaseNames, Excluded, DerivedNames);
		}

		/** Create a FARFilter to search for WidgetBlueprint type classes */
		FARFilter Filter;
		//Filter.ClassNames.Add(FName("WidgetBlueprint"));
		Filter.ClassPaths.Add(UWidgetBlueprintGeneratedClass::StaticClass()->GetStructPathName());
		Filter.bRecursivePaths = true;
		Filter.bRecursiveClasses = true;

		TArray<FAssetData> AssetList;
	  
		/** Get a list of FAssetData the matches the search criteria of the FARFilter */
		AssetRegistry.GetAssets(Filter, AssetList);

		for ( FAssetData Asset : AssetList )
		{
			/** Get the the class this blueprint generates (this is stored as a full path e.g "WidgetBlueprintGeneratedClass'/Game/Assets/NewWidgetBlueprint.NewWidgetBlueprint_C'") */
			FAssetTagValueRef GeneratedClassPathPtr = Asset.TagsAndValues.FindTag(TEXT("GeneratedClass"));

			if ( GeneratedClassPathPtr.IsSet() )
			{
				/** Convert path to just the name part e.g "/Game/Assets/NewWidgetBlueprint.NewWidgetBlueprint_C" */
				const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(GeneratedClassPathPtr.AsString());
				const FString ClassName = FPackageName::ObjectPathToObjectName(ClassObjectPath);
				FTopLevelAssetPath cp = FTopLevelAssetPath{ClassObjectPath};
				/** Check if this class is in the derived set */
				if ( !DerivedNames.Contains(cp) )
				{
					continue;
				}

				/** The SoftObjectPath to our class */
				FSoftObjectPath SoftPath(ClassObjectPath);

				/** Try to cast the UObject returned from SoftPath.TryLoad() to UWidgetBlueprintGeneratedClass */
				if ( UWidgetBlueprintGeneratedClass* Cls = Cast<UWidgetBlueprintGeneratedClass>(SoftPath.TryLoad()) )
				{
					/** Cast the UWidgetBlueprintGeneratedClass' default object to UMyBaseUUserWidget */
					if ( T* UIAsset = Cast<T>(Cls->GetDefaultObject()) )
					{
						/** Register the specific WidgetBlueprint Asset as a PrimaryAsset using the UMyBaseUUserWidget->GetPrimaryAssetId() function.
			  *   This is neccessary as Asset->GetPrimaryAssetId() will not return the proper PrimaryAssetId as it will auto generate to
			  *   "MyBaseUUserWidget:AssetName".
			  */
						UAssetManager::Get().RegisterSpecificPrimaryAsset(UIAsset->GetPrimaryAssetId(), Asset);
					}
				}
			}
		}

		bHasScannedForWidgetBlueprints = true;
	}

	template <class T> static void GetWidgetBlueprintsPrimaryAssets( const FString Path /*= "/Game"*/, const bool bShowAssetManagerDump /*= false*/ , TArray<FSoftClassPath>& outAssets)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

		if ( Path.IsEmpty() )
		{
		    return;
		}

		TArray<FString> ContentPaths;
		ContentPaths.Add(Path);
	  
	  /** Scan ContenPaths recursivley */
		AssetRegistry.ScanPathsSynchronous(ContentPaths, true);

		TSet<FTopLevelAssetPath> DerivedNames;
		{
			TArray<FTopLevelAssetPath> BaseNames; //The base Class Names
			BaseNames.Add(T::StaticClass()->GetStructPathName());
			
	    TSet<FTopLevelAssetPath> Excluded;
	    
	    /** Get a list of class names derived from BaseNames */
			AssetRegistry.GetDerivedClassNames(BaseNames, Excluded, DerivedNames);
		}

	  /** Create a FARFilter to search for WidgetBlueprint type classes */
		FARFilter Filter;
		//Filter.ClassNames.Add(FName("WidgetBlueprint"));
		FTopLevelAssetPath ClassPathName = UClass::TryConvertShortTypeNameToPathName<UStruct>(TEXT("WidgetBlueprint"), ELogVerbosity::Warning, TEXT("Compiling Asset Registry Filter"));
		Filter.ClassPaths.Add(ClassPathName);
		Filter.bRecursivePaths = true;
		Filter.bRecursiveClasses = true;

		TArray<FAssetData> AssetList;
	  
	  /** Get a list of FAssetData the matches the search criteria of the FARFilter */
		AssetRegistry.GetAssets(Filter, AssetList);

		for ( FAssetData Asset : AssetList )
		{
			/** Get the the class this blueprint generates (this is stored as a full path e.g "WidgetBlueprintGeneratedClass'/Game/Assets/NewWidgetBlueprint.NewWidgetBlueprint_C'") */
			FAssetTagValueRef GeneratedClassPathPtr = Asset.TagsAndValues.FindTag(TEXT("GeneratedClass"));

			if ( GeneratedClassPathPtr.IsSet() )
			{
				/** Convert path to just the name part e.g "/Game/Assets/NewWidgetBlueprint.NewWidgetBlueprint_C" */
				const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(GeneratedClassPathPtr.AsString());
				const FString ClassName = FPackageName::ObjectPathToObjectName(ClassObjectPath);


				FTopLevelAssetPath cp = FTopLevelAssetPath{ClassObjectPath};
				/** Check if this class is in the derived set */
				if ( !DerivedNames.Contains(cp) )
				{
					continue;
				}

	      /** The SoftObjectPath to our class */
				//FSoftObjectPath SoftPath(ClassObjectPath);
				outAssets.Emplace(FSoftClassPath{ClassObjectPath});
				//outAssetMap.Emplace(SoftPath.GetLongPackageFName(), SoftPath);
				/*
				// Try to cast the UObject returned from SoftPath.TryLoad() to UWidgetBlueprintGeneratedClass
				if ( UWidgetBlueprintGeneratedClass* Cls = Cast<UWidgetBlueprintGeneratedClass>(SoftPath.TryLoad()) )
				{
				//Cast the UWidgetBlueprintGeneratedClass' default object to UMyBaseUUserWidget
					if ( T* UIAsset = Cast<T>(Cls->GetDefaultObject()) )
					{
						outAssets.Emplace(SoftPath);
					}
				}
				*/
			}
		}
	}
	/*
	template <class T> T* GetWidgetFromAssetData( const FAssetData AssetData, UGameInstance* GameInstance) const
	{
		FAssetTagValueRef GenClassTag = AssetData.TagsAndValues.FindTag(FName("GeneratedClass"));

		/ Check and see if the "GeneratedClass" tag was found /
		if ( GenClassTag.IsSet() )
		{
			const FString ObjPath = FPackageName::ExportTextPathToObjectPath(GenClassTag.AsString());

			const FSoftObjectPath SoftObjectPath(GenClassTag.AsString());

			/ Try and load the class and cast it to UWidgetBlueprintGeneratedClass  /
			if ( UWidgetBlueprintGeneratedClass* BpClass = Cast<UWidgetBlueprintGeneratedClass>(SoftObjectPath.TryLoad()) )
			{
				/ Try to CreateWidget of type T with a valid UGameInstance and the UWidgetBlueprintGenerated class above /
				if( T* Object = CreateWidget<T>(GameInstance, BpClass) )
				{
					/ If our Object widget is valid, we return it /
					return Object;
				}
			}
		}

		return nullptr;
	}
*/
};

template<typename U>
struct UNREALLUA_API FAssetCollector
{
	FAssetCollector(const TArray<FString>& paths, bool bBlueprintOnly)
	: ObjectLibrary(nullptr), Paths(paths), bBlueprintOnly(bBlueprintOnly), AssetDataList({}), AssetMap{}
	{
		this->ObjectLibrary = UObjectLibrary::CreateLibrary(U::StaticClass(), true, true);
		this->ObjectLibrary->LoadBlueprintAssetDataFromPaths(Paths, true);
		
		this->ObjectLibrary->GetAssetDataList(AssetDataList);
		
		for(const auto& item : AssetDataList)
		{
			AssetMap.Emplace(item.PackageName, item);
		}
		
	}
	TObjectPtr<UObjectLibrary> ObjectLibrary;
	TArray<FString> Paths;
	bool bBlueprintOnly;
	
	TArray<FAssetData> AssetDataList;
	TMap<FName, FAssetData> AssetMap;
};