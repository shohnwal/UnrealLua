// Fill out your copyright notice in the Description page of Project Settings.


#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptBoxSubEditorNewStruct.h"

#include "SlateOptMacros.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "LuaValue/LuaValue.h"
#include "UIWidgets/LuaScriptEditorTextBox/SLuaScriptEditorTextBox.h"
#include "UIWidgets/SMultiTabEdtitableLuaScriptSwitcher.h"
#include "UIWidgets/SMultiLineEditableTextBoxEx.h"
#include "UIWidgets/SPropertySelectionList.h"
#include "UObject/StructOnScope.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Utility/UnrealVersion.h"
#define LOCTEXT_NAMESPACE "UnrealLuaTools"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SLuaScriptBoxSubEditorNewStruct::Construct(const FArguments& InArgs)
{
	SLuaScriptBoxSubEditorNewObjectBase::Construct(SLuaScriptBoxSubEditorNewObjectBase::FArguments()
		.OnSubEditorCommit(InArgs._OnSubEditorCommit)
	);
}

void SLuaScriptBoxSubEditorNewStruct::RebuildObjectList()
{
	LoadedObjectList.Empty();
	
	FARFilter Filter;
	Filter.PackagePaths.Add("/Game"); // Restrict to your content directory
	Filter.ClassPaths.Add(UScriptStruct::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	Filter.WithoutPackageFlags = PKG_UncookedOnly | PKG_EditorOnly | PKG_Developer;
	
	TArray<FAssetData> StructAssets;
	IAssetRegistry::Get()->GetAssets(Filter, StructAssets);

	for (const FAssetData& AssetData : StructAssets)
	{
		this->AllAssetData.Emplace(AssetData.GetFullName(), AssetData.ToSoftObjectPath());
		LoadedObjectList.Add(*AssetData.GetFullName());
	}

	for (TObjectIterator<UScriptStruct> it; it; ++it)
	{
		if (it->IsNative() && !it->IsEditorOnly())
		{
			this->AllAssetData.Emplace(*it->GetFullName(), FSoftObjectPath{*it});
			LoadedObjectList.Add(*it->GetFullName());		
		}
	}
}

void SLuaScriptBoxSubEditorNewStruct::SetSelectedStruct(UStruct* selectedStruct)
{
	FName name = NAME_None;
	if (selectedStruct)
	{
		if (UScriptStruct* ss = Cast<UScriptStruct>(selectedStruct))
		{
			name = *ss->GetFullName();
		}
	}
	this->SetSelectedStruct(name);
}

void SLuaScriptBoxSubEditorNewStruct::SetSelectedStruct(FName selected)
{
	UScriptStruct* ss = nullptr;
	
	if (selected != NAME_None)
	{
		FSoftObjectPath& data = this->AllAssetData.FindChecked(selected);
		ss = Cast<UScriptStruct>(data.TryLoad());		
	}

	this->SelectedStruct.Reset(ss);
	
	this->UpdateSectionVisibility(ss);
	
	this->RebuildTextboxContent();
}

void SLuaScriptBoxSubEditorNewStruct::RebuildTextboxContent()
{
	TSharedPtr<SLuaScriptEditorTextBox> textBox = this->LuaScriptEditorTextBox;
	if (!textBox.IsValid())
	{
		return;
	}
	UScriptStruct* selected = Cast<UScriptStruct>(this->SelectedStruct.Get());

#if UNREALLUA_UE_VERSION_OLDER_THAN(5,8,0)
	FStringBuilderBase builder;
#else
	TStringBuilder<4096> builder;
#endif
	if (selected)
	{
		verify(selected == this->PropertyListWidget->Struct.Get());
		bool multiline = this->AsMultilineCheckboxWidget->IsChecked();
		if (selected->IsNative())
		{
			builder << "F" << selected->GetName();
		}
		else
		{
			//@TODO: Perhaps alink to parent window up to Script Editor could provide info whether this was already imported 
			builder << this->ImportNameTextBox->GetText().ToString();
		}
		
		TArray<FProperty*> selectedProperties = this->PropertyListWidget->GetSelectedProperties();
		if (selectedProperties.Num() > 0)
		{
			builder << '{';
			if (multiline)
			{
				builder << '\n';
			}
			
			FStructOnScope scriptStruct{selected};
				
			for (FProperty* prop : selectedProperties)
			{
				builder << ' ' << prop->GetAuthoredName() << " = ";
				
				void* memLoc = prop->ContainerPtrToValuePtr<void>(scriptStruct.GetStructMemory());
				
				FLuaValue val {prop, memLoc};
				FString defaultValue = val.ToStringForStructBuilderEditor();

				builder << defaultValue << ", ";

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
			builder << '}';
		}
		else
		{
			builder << "()";	
		}
	}
	
	textBox->SetText(FText::AsCultureInvariant(builder.ToString()));
	this->SelectedStructText->SetText(FText::AsCultureInvariant("Create new " + GetNameSafe(selected)));
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE
