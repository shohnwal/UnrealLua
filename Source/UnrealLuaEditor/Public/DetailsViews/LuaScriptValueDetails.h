// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "Input/Reply.h"
#include "LuaValue/LuaScriptValue.h"

/**
 * 
 */

class STextBlock;
class FStructOnScope;
struct FLuaValue;

class UNREALLUAEDITOR_API FLuaScriptValueDetails : public IPropertyTypeCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	//virtual ~FLuaScriptValueDetails() override;

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
	void NotifyLuaScriptValueChanged(FLuaValue val);
	FReply OnEditScriptValueButtonClicked() const;
	void OnObjectsReinstanced(const TMap<UObject*, UObject*>& objectMap);
	/** Handle to the struct property being edited */
	TSharedPtr<IPropertyHandle> StructProperty = nullptr;
	TSharedPtr<FStructOnScope> ChildStruct = nullptr;
	FDelegateHandle OnObjectsReinstancedHandle = {};
	TSharedPtr<IPropertyUtilities> PropUtils = nullptr;
	FDelegateHandle LuaScriptValueChangedHandle = {};
	TSharedPtr<STextBlock> ContentTextWidget = nullptr;
	TSharedPtr<STextBlock> TypeTextWidget = nullptr;
};
