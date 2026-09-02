// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "IPropertyTypeCustomization.h"
#include "Debug/UnrealLuaDebug.h"

class FStructOnScope;
class IDetailPropertyRow;
class IDetailGroup;
/**
 * Type customization for FInstancedStruct.
 */
class UNREALLUAEDITOR_API FLuaUObjectItemViewDetails : public IPropertyTypeCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

	void NotifyScriptValuesRemoved();
	void NotifyLuaGameSessionActiveChangedNative(UUnrealLuaEngineSubsystem* UnrealLuaEngineSubsystem, bool isActive);
private:
	void OnObjectsReinstanced(const TMap<UObject*, UObject*>& objectMap);
	/** Handle to the struct property being edited */
	TSharedPtr<IPropertyHandle> StructProperty = nullptr;
	TSharedPtr<FStructOnScope> ChildStruct = nullptr;
	FDelegateHandle OnObjectsReinstancedHandle = {};
	TSharedPtr<IPropertyUtilities> PropUtils = nullptr;
};
