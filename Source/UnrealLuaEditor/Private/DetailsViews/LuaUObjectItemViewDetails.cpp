// Fill out your copyright notice in the Description page of Project Settings.


#include "DetailsViews/LuaUObjectItemViewDetails.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
#include "IPropertyUtilities.h"
#include "PropertyCustomizationHelpers.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"
#include "SubSystem/UnrealLuaGameWorldSubsystem.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "UObjectRegistry/LuaUObjectItemView.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<IPropertyTypeCustomization> FLuaUObjectItemViewDetails::MakeInstance()
{
	return MakeShared<FLuaUObjectItemViewDetails>();
}

void FLuaUObjectItemViewDetails::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle,
                                                 class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	UUnrealLuaEngineSubsystem* ss = UUnrealLuaEngineSubsystem::Get();
	if (ss)
	{
		ss->OnLuaGameSessionActiveChangedNative.AddSP(this, &FLuaUObjectItemViewDetails::NotifyLuaGameSessionActiveChangedNative);
	}
	
	StructProperty = StructPropertyHandle;
	
	if (UUnrealLuaEngineSubsystem::IsGameSessionActive())
	{
		StructProperty->SetInstanceMetaData(TEXT("ShowOnlyInnerProperties"), "true");
		TArray<void*> rawData;
		StructProperty->AccessRawData(rawData);
		FLuaUObjectItemView* view = static_cast<FLuaUObjectItemView*>(rawData[0]);
		
		if (view != nullptr && view->IsValid())
		{
			FLuaUObjectItem* item = view->LuaUObjectItem;
			ChildStruct = MakeShared<FStructOnScope>(FLuaUObjectItem::StaticStruct(), reinterpret_cast<uint8*>(item));
			item->OnNumberOfValuesChanged.AddSP(this, &FLuaUObjectItemViewDetails::NotifyScriptValuesRemoved);
		}
		else
		{
			ChildStruct = nullptr;
		}

		PropUtils = StructCustomizationUtils.GetPropertyUtilities();

		OnObjectsReinstancedHandle = FCoreUObjectDelegates::OnObjectsReinstanced.AddSP(this, &FLuaUObjectItemViewDetails::OnObjectsReinstanced);
		HeaderRow
			.NameContent()
			[
				StructPropertyHandle->CreatePropertyNameWidget()
			]
			.ValueContent()
			.MinDesiredWidth(250.f)
			.VAlign(VAlign_Center);
	}
	else
	{
		this->ChildStruct = nullptr;
		HeaderRow.NameContent()
		[
			SNew(STextBlock)
			.Font(StructCustomizationUtils.GetRegularFont())
			.Text(FText::AsCultureInvariant("No LuaUObjectItem"))
		];
	}
}

void FLuaUObjectItemViewDetails::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	if (this->ChildStruct.IsValid())
	{
		StructBuilder.AddExternalStructure(this->ChildStruct.ToSharedRef());
	}
}

void FLuaUObjectItemViewDetails::NotifyScriptValuesRemoved()
{
	if (PropUtils.IsValid())
	{
		StructProperty->RequestRebuildChildren();
		//PropUtils->RequestRefresh();
	}	
}

void FLuaUObjectItemViewDetails::NotifyLuaGameSessionActiveChangedNative(UUnrealLuaEngineSubsystem* UnrealLuaEngineSubsystem, bool isActive)
{
	StructProperty->RequestRebuildChildren();
	if (PropUtils.IsValid())
	{
		PropUtils->ForceRefresh();
	}	
}

void FLuaUObjectItemViewDetails::OnObjectsReinstanced(const TMap<UObject*, UObject*>& objectMap)
{
	// Force update the details when BP is compiled, since we may cached hold references to the old object or class.
	if (!objectMap.IsEmpty() && PropUtils.IsValid())
	{
		PropUtils->RequestRefresh();
	}
}