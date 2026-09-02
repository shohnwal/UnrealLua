// Fill out your copyright notice in the Description page of Project Settings.


#include "DetailsViews/WatchedActorViewDetails.h"

#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "Debug/UnrealLuaDebug.h"
#include "DetailsViews/LuaUObjectItemViewDetails.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"

TSharedRef<IPropertyTypeCustomization> FWatchedActorViewDetails::MakeInstance()
{
	return MakeShared<FWatchedActorViewDetails>();
}


void FWatchedActorViewDetails::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle,
												 class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	UUnrealLuaEngineSubsystem* ss = UUnrealLuaEngineSubsystem::Get();
	if (ss)
	{
		UUnrealLuaDebug* debug = ss->GetUnrealLuaDebug();
		if (debug)
		{
			debug->OnWatchedActorChangedNative.AddSP(this, &FWatchedActorViewDetails::NotifyWatchedActorChanged);
		}
	}

	
	StructProperty = StructPropertyHandle;

	TArray<void*> rawData;
	StructProperty->AccessRawData(rawData);
	ChildStruct = MakeShared<FStructOnScope>(FUnrealLuaDebugActorWatcher::StaticStruct(), static_cast<uint8*>(rawData[0]));
}


void FWatchedActorViewDetails::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	uint32 NumChildren = 0;
	StructPropertyHandle->GetNumChildren(NumChildren);
	for (uint32 i = 0; i < NumChildren; i++)
	{
		StructBuilder.AddProperty(StructPropertyHandle->GetChildHandle(i).ToSharedRef());
	}
}


void FWatchedActorViewDetails::NotifyWatchedActorChanged(bool watchjedIsValid, UUnrealLuaDebug* UnrealLuaDebug)
{
	StructProperty->RequestRebuildChildren();
}
