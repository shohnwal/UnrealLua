// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "UObject/Object.h"

class FStructOnScope;
class UUnrealLuaDebug;
/**
 * 
 */
class UNREALLUAEDITOR_API FWeakStructViewDetails : public IPropertyTypeCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
	void OnObjectsReinstanced(const TMap<UObject*, UObject*>& objectMap);
	//void NotifyWatchedActorChanged(bool watchjedIsValid, UUnrealLuaDebug* UnrealLuaDebug);
	/** Handle to the struct property being edited */
	TSharedPtr<IPropertyHandle> StructProperty;
	TSharedPtr<FStructOnScope> ChildStruct;
	FDelegateHandle OnObjectsReinstancedHandle;
	TSharedPtr<IPropertyUtilities> PropUtils;	
};
