// Fill out your copyright notice in the Description page of Project Settings.


#include "DetailsViews/WeakStructViewDetails.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "BlueprintSupport/WeakStructView.h"

TSharedRef<IPropertyTypeCustomization> FWeakStructViewDetails::MakeInstance()
{
	return MakeShared<FWeakStructViewDetails>();
}


void FWeakStructViewDetails::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	StructProperty = StructPropertyHandle;
	StructProperty->SetInstanceMetaData(TEXT("ShowOnlyInnerProperties"), "true");
	TArray<void*> rawData;
	StructProperty->AccessRawData(rawData);
	FWeakStructView* view = static_cast<FWeakStructView*>(rawData[0]);
	
	if (view != nullptr && view->IsValid())
	{
		ChildStruct = MakeShared<FStructOnScope>(view->GetScriptStruct(), reinterpret_cast<uint8*>(view->GetMemory()));
	}
	else
	{
		ChildStruct = nullptr;
	}

	PropUtils = StructCustomizationUtils.GetPropertyUtilities();

	OnObjectsReinstancedHandle = FCoreUObjectDelegates::OnObjectsReinstanced.AddSP(this, &FWeakStructViewDetails::OnObjectsReinstanced);
	HeaderRow
		.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(250.f)
		.VAlign(VAlign_Center);
	/*
	UUnrealLuaEngineSubsystem* ss = UUnrealLuaEngineSubsystem::Get();
	if (ss)
	{
		UUnrealLuaDebug* debug = ss->GetUnrealLuaDebug();
		if (debug)
		{
			debug->OnWatchedActorChangedNative.AddSP(this, &FWeakStructViewDetails::NotifyWatchedActorChanged);
		}
	}
	*/
}

void FWeakStructViewDetails::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	if (this->ChildStruct.IsValid())
	{
		StructBuilder.AddExternalStructure(this->ChildStruct.ToSharedRef());
	}
}

void FWeakStructViewDetails::OnObjectsReinstanced(const TMap<UObject*, UObject*>& objectMap)
{
	// Force update the details when BP is compiled, since we may cached hold references to the old object or class.
	if (!objectMap.IsEmpty() && PropUtils.IsValid())
	{
		PropUtils->RequestRefresh();
	}
}

/*
void FWeakStructViewDetails::NotifyWatchedActorChanged(bool watchjedIsValid, UUnrealLuaDebug* UnrealLuaDebug)
{
	if (PropUtils.IsValid())
	{
		PropUtils->RequestRefresh();
	}
}
*/

//////////////////////////////////////////////////////////////////
///
