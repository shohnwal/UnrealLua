// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "Utility/UnrealLuaSimpleDelegateSignatures.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SButton.h"
#include "InteractiveActorPickerWidget.generated.h"


DECLARE_DELEGATE_OneParam( FOnActorSelected, AActor* );
DECLARE_DELEGATE_RetVal_OneParam(bool, FOnShouldFilterActor, const AActor*);

/**
 * 
 */

UCLASS(BlueprintType)
class UNREALLUA_API UInteractiveActorPickerWidget : public UButton
{
	GENERATED_BODY()
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	
	void NotifyActorSelected(AActor* actor) const;
	
	UPROPERTY(BlueprintAssignable, Category = "UnrealLua")
	FActorMulticastDelegate OnActorSelected;
};

class UNREALLUA_API SInteractiveActorPicker : public SButton
{
public:
	SLATE_BEGIN_ARGS( SInteractiveActorPicker )
		{}

	/** Delegate used to filter allowed actors */
	SLATE_EVENT( FSimpleClassArrayNativeDelegate, OnGetAllowedClasses )

	/** Delegate used to filter allowed actors */
	SLATE_EVENT( FOnShouldFilterActor, OnShouldFilterActor )

	/** Delegate called when an actor is selected */
	SLATE_EVENT( FSimpleActorNativeDelegate, OnActorSelected )

	SLATE_END_ARGS()

	virtual ~SInteractiveActorPicker() override;

	void Construct( const FArguments& InArgs );

	/** Begin SWidget interface */
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent ) override;
	virtual bool SupportsKeyboardFocus() const override;
	/** End SWidget interface */

private:
	/** Delegate for when the button is clicked */
	FReply OnClicked();

	/** Delegate used to filter allowed actors */
	FSimpleClassArrayNativeDelegate OnGetAllowedClasses = {};

	/** Delegate used to filter allowed actors */
	FOnShouldFilterActor OnShouldFilterActor = {};

	/** Delegate called when an actor is selected */
	FOnActorSelected OnActorSelected = {};
};