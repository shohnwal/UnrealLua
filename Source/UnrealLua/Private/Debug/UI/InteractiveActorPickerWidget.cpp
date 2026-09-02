// Fill out your copyright notice in the Description page of Project Settings.


#include "Debug/UI/InteractiveActorPickerWidget.h"

#include "SlateOptMacros.h"
#include "Components/ButtonSlot.h"
#include "Debug/UnrealLuaDebug.h"
#include "Debug/DebugTools/UnrealLuaDebugActorPickerTool.h"
#include "Widgets/Images/SImage.h"

#define LOCTEXT_NAMESPACE "UnrealLua"

struct FGeometry;


TSharedRef<SWidget> UInteractiveActorPickerWidget::RebuildWidget()
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	MyButton = SNew(SInteractiveActorPicker)
		.OnActorSelected(BIND_UOBJECT_DELEGATE(FSimpleActorNativeDelegate, NotifyActorSelected));
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
		if ( GetChildrenCount() > 0 )
		{
			Cast<UButtonSlot>(GetContentSlot())->BuildSlot(MyButton.ToSharedRef());
		}
	
	return MyButton.ToSharedRef();
}

void UInteractiveActorPickerWidget::NotifyActorSelected(AActor* actor) const
{
	this->OnActorSelected.Broadcast(actor);
}

SInteractiveActorPicker::~SInteractiveActorPicker()
{
	if (UUnrealLuaDebug* debug = UUnrealLuaDebug::Get())
	{
		// make sure we are unregistered when this widget goes away
		//debug->DeactivateTool(UUnrealLuaDebugActorPickerTool::StaticClass());
	}
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SInteractiveActorPicker::Construct( const FArguments& InArgs )
{
	OnActorSelected = InArgs._OnActorSelected;
	OnGetAllowedClasses = InArgs._OnGetAllowedClasses;
	OnShouldFilterActor = InArgs._OnShouldFilterActor;

	SButton::Construct(
		SButton::FArguments()
		.ButtonStyle( FAppStyle::Get(), "HoverHintOnly" )
		.OnClicked( this, &SInteractiveActorPicker::OnClicked )
		.ContentPadding(4.0f)
		.ForegroundColor( FSlateColor::UseForeground() )
		.IsFocusable(false)
		[ 
			SNew( SImage )
			.Image( FAppStyle::GetBrush("Icons.EyeDropper") )
			.ColorAndOpacity( FSlateColor::UseForeground() )
		]
	);
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

FReply SInteractiveActorPicker::OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent )
{
	if(InKeyEvent.GetKey() == EKeys::Escape)
	{
		if (UUnrealLuaDebug* debug = UUnrealLuaDebug::Get())
		{
			//debug->DeactivateTool(UUnrealLuaDebugActorPickerTool::StaticClass());
		}
	}

	return FReply::Unhandled();
}

bool SInteractiveActorPicker::SupportsKeyboardFocus() const
{
	return true;
}

FReply SInteractiveActorPicker::OnClicked()
{
	if (UUnrealLuaDebug* debug = UUnrealLuaDebug::Get())
	{
		// make sure we are unregistered when this widget goes away
		//debug->ToggleTool(UUnrealLuaDebugActorPickerTool::StaticClass(), {});
	}
	
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
