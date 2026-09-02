// Fill out your copyright notice in the Description page of Project Settings.


#include "UIWidgets/SUFunctionSelectionList.h"

#include "Algo/ForEach.h"
#include "Framework/Application/SlateApplication.h"
#include "UnrealOverrides/LuaClassOverrideRegistry.h"
#include "UnrealOverrides/UnrealLuaOverrideUFunction.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"

TArray<UFunction*> SUFunctionSelectionList::CreateDefaultFunctionSelectionList(UClass* uclass, const TArray<FName>& forbiddenFunctions)
{
	TArray<FName> foundFuncNames{};
	TArray<UFunction*> acceptedFunctions{};
	if (IsValid(uclass))
	{
		while (uclass)
		{
			for (TFieldIterator<UFunction> funcIt(uclass, EFieldIterationFlags::None); funcIt; ++funcIt)
			{
				UFunction* func = *funcIt;
			
				if (UUnrealLuaOverrideUFunction* overrideFunc = Cast<UUnrealLuaOverrideUFunction>(func))
				{
					if (overrideFunc->bReplacedExistingFunc)
					{
						func = overrideFunc->Overridden;
					}
					else
					{
						continue;
					}
				}
			
				FName funcName = func->GetFName();
			
				if (foundFuncNames.Contains(funcName))
				{
					continue;
				}
				foundFuncNames.Emplace(funcName);
				if (forbiddenFunctions.Contains(funcName))
				{
					continue;
				}
				//bool isOverridable = FLuaClassOverrideRegistry::IsOverridableUFunction(func); 
				if (!FLuaClassOverrideRegistry::IsOverridableUFunction(func))
				{
					continue;
				}
				acceptedFunctions.Add(func);
			}		
			uclass = uclass->GetSuperClass();
		}
	}
	return acceptedFunctions;
}

void SUFunctionSelectionList::Construct(const FArguments& InArgs)
{
	TArray<UFunction*> allFunctions = InArgs._Functions;
	Algo::ForEach(allFunctions, [](UFunction*& func)
	{
		if (UUnrealLuaOverrideUFunction* overrideFunc = Cast<UUnrealLuaOverrideUFunction>(func))
		{
			func = overrideFunc->Overridden;
		}
	});
	TArray<UFunction*> preselectedFunctions = InArgs._PreselectedFunctions;
	Algo::ForEach(preselectedFunctions, [](UFunction*& func)
	{
		if (UUnrealLuaOverrideUFunction* overrideFunc = Cast<UUnrealLuaOverrideUFunction>(func))
		{
			func = overrideFunc->Overridden;
		}
	});
	TArray<FName> preselectedFunctionnames = InArgs._PreselectedFunctionNames;
	this->bSingleChoiceMode = InArgs._SingleChoiceMode;
	for (UFunction* func : allFunctions)
	{
		if (preselectedFunctionnames.Contains(func->GetName()))
		{
			preselectedFunctions.AddUnique(func);
		}
	}
	
	bool withSeparators = InArgs._AddSeparators;
	
	this->ChildSlot
	[
		SAssignNew(MainBodyScrollBox, SScrollBox)
		.Orientation(Orient_Vertical)
	]
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill);
	
	UClass* currentClass = nullptr; 
	
	for (UFunction* func : allFunctions)
	{
		if (withSeparators)
		{
			UClass* ownerClass = func->GetOuterUClassUnchecked();
			if (ownerClass != currentClass)
			{
				currentClass = ownerClass;
				if (currentClass)
				{
					this->MainBodyScrollBox->AddSlot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					.Padding(10,10,10,10)
					[
						ConstructClassSeparator(currentClass)
					];				
				}
			}	
		}
		bool preselected = preselectedFunctions.Contains(func);
		this->MainBodyScrollBox->AddSlot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		[
			this->ConstructListElement(func, preselected)	
		];
		if (preselected)
		{
			this->SelectedFunctions.AddUnique(func);
		}
	}
}

TSharedRef<SHorizontalBox> SUFunctionSelectionList::ConstructClassSeparator(UClass* uclass)
{
	TSharedRef<SHorizontalBox> widget = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			SNew(SSeparator)
			.SeparatorImage(FAppStyle::GetBrush("Menu.Separator"))
			.Orientation(Orient_Horizontal)
		]
		.VAlign(VAlign_Center)
		+ SHorizontalBox::Slot()
		[
			SNew(STextBlock)
			.Text(FText::FromString(uclass->GetName()))
			.Justification(ETextJustify::Center)
		]
		.Padding(10, 0, 10, 0)
		.AutoWidth()
		+ SHorizontalBox::Slot()
		[
			SNew(SSeparator)
			.SeparatorImage(FAppStyle::GetBrush("Menu.Separator"))
			.Orientation(Orient_Horizontal)
		]
		.VAlign(VAlign_Center);
	
	return widget;
}

TSharedRef<SHorizontalBox> SUFunctionSelectionList::ConstructListElement(UFunction* function, bool bIsSelected)
{
	if (this->bSingleChoiceMode)
	{
		TSharedRef<SHorizontalBox> elementHBox = SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		[
			SNew(SButton)
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant(function->GetName()))
				.Visibility(EVisibility::SelfHitTestInvisible)
				.ToolTip(FSlateApplication::Get().MakeToolTip(FText::FromString(function->GetName())))
			]
			.OnClicked_Lambda([this, function]()
			{
				this->NotifyFunctionButtonClicked(function);
				return FReply::Handled();
			})
		]	
		.FillWidth(1)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Fill);
		return elementHBox;
	}
	else
	{
		TSharedRef<SCheckBox> cb = SNew(SCheckBox)
							.IsChecked(bIsSelected ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
							.OnCheckStateChanged_Lambda([this, function](ECheckBoxState state)
						{
							this->NotifyCheckedFunctionChanged(function, state);
						});
		this->CheckBoxes.Emplace(function, cb);
		TSharedRef<SHorizontalBox> elementHBox = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			cb
		]
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.AutoWidth()
		+ SHorizontalBox::Slot()
		[
			SNew(STextBlock)
			.Text(FText::FromString(function->GetName()))
			.ToolTip(FSlateApplication::Get().MakeToolTip(FText::FromString(function->GetName())))
			.Visibility(EVisibility::SelfHitTestInvisible)
		]
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Fill)
		.FillWidth(1);
		return elementHBox;
	}
}

TArray<UFunction*> SUFunctionSelectionList::GetSelectedFunctions() const
{
	return this->SelectedFunctions;
}

void SUFunctionSelectionList::ClearAllChecks(UFunction* except)
{
	this->SelectedFunctions.Empty();
	for (auto data : this->CheckBoxes)
	{
		if (data.Func != except)
		{
			data.CheckBox->SetIsChecked(ECheckBoxState::Unchecked);
		}
	}	
}

UFunction* SUFunctionSelectionList::FindFunctionByString(const FString& funcName)
{
	//if (this-.IsValid())
	//{
	//	FName funcName{funcNameStr, FNAME_Find};
	//	if (funcName != NAME_None)
	//	{
	//		UClass* uclass = this->ValueOwner.Get()->GetClass();
	//		return uclass->FindFunctionByName(funcName);
	//	}
	//}
	return nullptr;
}

void SUFunctionSelectionList::SetViewedStruct(UStruct* ustruct)
{
	UClass* uclass = Cast<UClass>(ustruct);
	this->MainBodyScrollBox->ClearChildren();
	this->SelectedFunctions.Empty();
	if (!uclass)
	{
		return;
	}
	TArray<UFunction*> allFunctions = CreateDefaultFunctionSelectionList(uclass);
	bool withSeparators = true;
	
	TArray<UFunction*> preselectedFunctions{};
	UClass* currentClass = uclass;
	for (UFunction* func : allFunctions)
	{
		if (withSeparators)
		{
			UClass* ownerClass = func->GetOuterUClassUnchecked();
			if (ownerClass != currentClass)
			{
				currentClass = ownerClass;
				if (currentClass)
				{
					this->MainBodyScrollBox->AddSlot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					.Padding(10,10,10,10)
					[
						ConstructClassSeparator(currentClass)
					];				
				}
			}	
		}
		bool preselected = preselectedFunctions.Contains(func);
		this->MainBodyScrollBox->AddSlot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		[
			this->ConstructListElement(func, preselected)	
		];
		if (preselected)
		{
			this->SelectedFunctions.AddUnique(func);
		}
	}
	
}

void SUFunctionSelectionList::NotifyFunctionButtonClicked(UFunction* func)
{
	this->OnFunctionCheckboxChanged.ExecuteIfBound(func, true);
}

void SUFunctionSelectionList::NotifyCheckedFunctionChanged(UFunction* function, ECheckBoxState state)
{
	if (state == ECheckBoxState::Checked)
	{
		if (this->bSingleChoiceMode)
		{
			this->ClearAllChecks(function);
		}
		this->SelectedFunctions.AddUnique(function);
	}
	else
	{
		this->SelectedFunctions.Remove(function);
	}
	this->OnFunctionCheckboxChanged.ExecuteIfBound(function, state == ECheckBoxState::Checked);
}
