// Fill out your copyright notice in the Description page of Project Settings.


#include "UIWidgets/SSubobjectListWidget.h"

#include "SlateOptMacros.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SSubobjectListWidget::Construct(const FArguments& InArgs)
{
	this->OnObjectSelected = InArgs._OnObjectSelected;
	ChildSlot
	[
		
		SAssignNew(ScrollBox, SScrollBox)
		.Orientation(Orient_Vertical)
		+ SScrollBox::Slot()
		[
			SAssignNew(SubobjectButtonList, SVerticalBox)
		]
		.AutoSize()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
	];
	this->SetViewedObject(nullptr);
}

void SSubobjectListWidget::SetViewedObject(UObject* obj)
{
	this->ViewedObject = obj;
	this->SubobjectButtonList->ClearChildren();
	if (this->ViewedObject.IsValid())
	{
		TArray<UObject*> subobjects{};
		GetObjectsWithOuter(this->ViewedObject.Get(), subobjects, false);
		for (UObject* subobject : subobjects)
		{
			this->SubobjectButtonList->AddSlot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			.AutoHeight()
			[
				this->ConstructListElement(subobject)	
			];
		}
	}
}

TSharedRef<SButton> SSubobjectListWidget::ConstructListElement(UObject* subobject)
{
	TWeakObjectPtr<UObject> weakPtr{subobject};
	TSharedRef<SButton> newButton = SNew(SButton)
	[
		SNew(STextBlock)
		.Text(FText::AsCultureInvariant(*GetNameSafe(subobject)))
		.ToolTip(FSlateApplication::Get().MakeToolTip(FText::AsCultureInvariant(*GetFullNameSafe(subobject))))
	]
		.OnClicked_Lambda([this, weakPtr]()
		{
			if (weakPtr.IsValid())
			{
				this->NotifySubobjectButtonClicked(weakPtr.Get());
			}
			return FReply::Handled();
		});
	return newButton;
}

void SSubobjectListWidget::NotifySubobjectButtonClicked(UObject* clickedSubobject)
{
	this->OnObjectSelected.ExecuteIfBound(clickedSubobject);
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
