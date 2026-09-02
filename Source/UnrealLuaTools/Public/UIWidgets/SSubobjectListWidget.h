// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Utility/UnrealLuaSimpleDelegateSignatures.h"
#include "Widgets/SCompoundWidget.h"

class SScrollBox;
class SButton;
class SVerticalBox;
/**
 * 
 */
class UNREALLUATOOLS_API SSubobjectListWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSubobjectListWidget)
		{
		}
	SLATE_EVENT(FSimpleUObjectNativeDelegate, OnObjectSelected)

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	
	void SetViewedObject(UObject* obj);
	TSharedRef<SButton> ConstructListElement(UObject* subobject);
	void NotifySubobjectButtonClicked(UObject* clickedSubobject);
	TWeakObjectPtr<UObject> ViewedObject = nullptr;
	
	FSimpleUObjectNativeDelegate OnObjectSelected = {};
	TSharedPtr<SScrollBox> ScrollBox;
	TSharedPtr<SVerticalBox> SubobjectButtonList;
};
