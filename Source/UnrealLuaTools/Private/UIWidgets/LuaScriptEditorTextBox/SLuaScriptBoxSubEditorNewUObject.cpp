// Fill out your copyright notice in the Description page of Project Settings.


#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptBoxSubEditorNewUObject.h"

#include "SlateOptMacros.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "GameFramework/Actor.h"
#include "LuaValue/LuaValue.h"
#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptEditorTextBox.h"
#include "UIWidgets/SMultiLineEditableTextBoxEx.h"
#include "UIWidgets/SPropertySelectionList.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Utility/UnrealVersion.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"

#define LOCTEXT_NAMESPACE "UnrealLuaTools"
BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SLuaScriptBoxSubEditorNewUObject::Construct(const FArguments& InArgs)
{
	SLuaScriptBoxSubEditorNewObjectBase::Construct(SLuaScriptBoxSubEditorNewObjectBase::FArguments()
		.OnSubEditorCommit(InArgs._OnSubeditorCommit)
		.OnCancelEdit(InArgs._OnCancelEdit)
	);
}

void SLuaScriptBoxSubEditorNewUObject::RebuildObjectList()
{
	LoadedObjectList.Empty();
	
	FARFilter Filter;
	Filter.PackagePaths.Add("/Game"); // Restrict to your content directory
	Filter.ClassPaths.Add(UObject::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	Filter.WithoutPackageFlags = PKG_UncookedOnly | PKG_EditorOnly | PKG_Developer;
	
	TArray<FAssetData> StructAssets;
	IAssetRegistry::Get()->GetAssets(Filter, StructAssets);

	for (const FAssetData& AssetData : StructAssets)
	{
		// UScriptStruct or UUserDefinedStruct asset found
		//UScriptStruct* StructAsset = Cast<UScriptStruct>(AssetData.GetAsset());
		//LoadedObjectList.Add(TStrongObjectPtr<UObject>(StructAsset));
		this->AllAssetData.Emplace(AssetData.GetFullName(), AssetData.ToSoftObjectPath());
		LoadedObjectList.Add(*AssetData.GetFullName());
	}

	for (TObjectIterator<UClass> it; it; ++it)
	{
		if (it->IsNative() && !it->IsEditorOnly())
		{
			//LoadedObjectList.Add(TStrongObjectPtr<UObject>(*it));	
			this->AllAssetData.Emplace(*it->GetFullName(), FSoftObjectPath{*it});
			LoadedObjectList.Add(*it->GetFullName());		
		}
	}
}

void SLuaScriptBoxSubEditorNewUObject::SetSelectedStruct(UStruct* selectedStruct)
{
	this->SelectedStruct.Reset(selectedStruct);
	
	this->UpdateSectionVisibility(selectedStruct);

	this->RebuildTextboxContent();
}

void SLuaScriptBoxSubEditorNewUObject::SetSelectedStruct(FName selectedStructFullPath)
{
	UClass* ss = nullptr;
	
	if (selectedStructFullPath != NAME_None)
	{
		FSoftObjectPath& data = this->AllAssetData.FindChecked(selectedStructFullPath);
		ss = Cast<UClass>(data.TryLoad());
		if (!ss)
		{
			ss = Cast<UClass>(StaticLoadClass(UObject::StaticClass(), nullptr, data.ToString() + "_C"));
		}	
	}
	
	this->SetSelectedStruct(ss);
}

void SLuaScriptBoxSubEditorNewUObject::RebuildTextboxContent()
{
	UClass* selected = Cast<UClass>(this->SelectedStruct.Get());

#if UNREALLUA_UE_VERSION_OLDER_THAN(5,8,0)
	FStringBuilderBase builder;
#else
	TStringBuilder<4096> builder;
#endif
	if (selected)
	{
		verify(selected == this->PropertyListWidget->Struct.Get());
		bool isActor = selected->IsChildOf(AActor::StaticClass());
		
		if (isActor)
		{
			//AActor* UUnrealLuaGameplayStatics::SpawnActor(UObject* worldContext, TSubclassOf<AActor> actorClass, FTransform spawnTransform, AActor* owner, APawn* instigator, ESpawnActorCollisionHandlingMethod spawnmethod, FLuaValue init)
			builder << "World:SpawnActor( ";
		}
		else
		{
			//UObject* UUnrealLuaGameplayStatics::NewObject(TSubclassOf<UObject> clazz, UObject* outer, FName name, UObject* templat, FLuaValue init)
			builder << "NewObject( ";
		}
		
		//ClassName
		char prefix = isActor ? 'A' : 'U';
		bool multiline = this->AsMultilineCheckboxWidget->IsChecked();
		if (selected->IsNative())
		{
			builder << prefix << selected->GetName();
		}
		else
		{
			//@TODO: Perhaps alink to parent window up to Script Editor could provide info whether this was already imported 
			builder << this->ImportNameTextBox->GetText().ToString();
		}

		
		UObject* cdo = selected->GetDefaultObject<UObject>();
		//init
		TArray<FProperty*> selectedProperties = this->PropertyListWidget->GetSelectedProperties();
		if (selectedProperties.Num() > 0)
		{
			builder << ", {";
			if (multiline)
			{
				builder << '\n';
			}
				
			for (FProperty* prop : selectedProperties)
			{
				builder << ' ' << prop->GetAuthoredName() << " = ";
				
				void* memLoc = prop->ContainerPtrToValuePtr<void>(cdo);
				
				FLuaValue val {prop, memLoc};
				FString defaultValue = val.ToStringForStructBuilderEditor();
			
				builder << defaultValue << ",";
				
				if (multiline)
				{
					builder << '\n';
				}
			}
			//@TODO: improve. Ugly hack, remove last comma, and possibly line break
			builder.RemoveSuffix(multiline ? 2 : 1);
			if (multiline)
			{
				builder << '\n';
			}
			builder << "}, ";
		}
		else
		{
			builder << ", { }";	
		}
		
		//outer
		builder << ", nil --[[outer]] ";
		
		builder << ")";
	}
	
	this->LuaScriptEditorTextBox->SetText(FText::AsCultureInvariant(builder.ToString()));
	this->SelectedStructText->SetText(FText::AsCultureInvariant("Create new " + GetNameSafe(selected)));
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
#undef LOCTEXT_NAMESPACE
