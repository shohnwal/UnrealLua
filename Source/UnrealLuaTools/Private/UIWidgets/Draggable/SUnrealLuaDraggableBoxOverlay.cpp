// Copyright Epic Games, Inc. All Rights Reserved.
#include "UIWidgets/Draggable/SUnrealLuaDraggableBoxOverlay.h"

#include "Framework/Application/SlateApplication.h"
#include "Overlay/DragBoxPosition.h"
#include "UIWidgets/Draggable/SUnrealLuaDraggableBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SWindow.h"
namespace UnrealLua::UIWidgets
{
	static constexpr float DraggableBorderArea = 3.f;
}

void SUnrealLuaDraggableBoxOverlay::Construct(const FArguments& InArgs)
{
	this->HorizontalAlignment = InArgs._HAlign;
	this->VerticalAlignment = InArgs._VAlign;

	this->ChildSlot
	[
		SAssignNew(this->Container, SBox)
		.HAlign(HorizontalAlignment)
		.VAlign(VerticalAlignment)
		[
			SAssignNew(DraggableBox, SUnrealLuaDraggableBox, SharedThis(this))
			.IsDraggable(InArgs._IsDraggable)
			.OnUserDraggedToNewPosition(InArgs._OnUserDraggedToNewPosition)
			[
				InArgs._Content.Widget
			]
		]
	];

	SetBoxAlignmentOffset(InArgs._InitialAlignmentOffset, false);
}

FVector2f SUnrealLuaDraggableBoxOverlay::GetBoxAlignmentOffset() const
{
	FVector2f AlignmentOffset = FVector2f::ZeroVector;

	if (Container.IsValid())
	{
		switch (HorizontalAlignment)
		{
		case EHorizontalAlignment::HAlign_Left:
			AlignmentOffset.X = Padding.Left;
			break;
		case EHorizontalAlignment::HAlign_Right:
			AlignmentOffset.X = Padding.Right;
			break;
		default: break;
		}

		switch (VerticalAlignment)
		{
		case EVerticalAlignment::VAlign_Top:
			AlignmentOffset.Y = Padding.Top;
			break;
		case EVerticalAlignment::VAlign_Bottom:
			AlignmentOffset.Y = Padding.Bottom;
			break;
		default: break;
		}
	}

	return AlignmentOffset;
}

void SUnrealLuaDraggableBoxOverlay::SetBoxAlignmentOffset(const FVector2f& InOffset, bool bInRecomputeAnchorPoints)
{
	if (!Container.IsValid() || !DraggableBox.IsValid())
	{
		return;
	}

	FVector2f ConstrainedOffset = {
		FMath::Max(InOffset.X, UnrealLua::UIWidgets::DraggableBorderArea),
		FMath::Max(InOffset.Y, UnrealLua::UIWidgets::DraggableBorderArea)
	};
	if (bInRecomputeAnchorPoints)
	{
		ConstrainedOffset = RecomputeAnchorPoints(ConstrainedOffset);
	}

	switch (HorizontalAlignment)
	{
	case EHorizontalAlignment::HAlign_Left:
		Padding.Left = ConstrainedOffset.X;
		Padding.Right = 0.f;
		break;

	case EHorizontalAlignment::HAlign_Right:
		Padding.Left = 0.f;
		Padding.Right = ConstrainedOffset.X;
		break;

	default: break;
	}

	switch (VerticalAlignment)
	{
	case EVerticalAlignment::VAlign_Top:
		Padding.Top = ConstrainedOffset.Y;
		Padding.Bottom = 0.f;
		break;

	case EVerticalAlignment::VAlign_Bottom:
		Padding.Top = 0.f;
		Padding.Bottom = ConstrainedOffset.Y;
		break;

	default: break;
	}

	if (Container.IsValid())
	{
		Container->SetPadding(Padding);
	}
}

EHorizontalAlignment SUnrealLuaDraggableBoxOverlay::GetBoxHorizontalAlignment() const
{
	return HorizontalAlignment;
}

void SUnrealLuaDraggableBoxOverlay::SetBoxHorizontalAlignment(EHorizontalAlignment InAlignment)
{
	HorizontalAlignment = InAlignment;

	if (Container.IsValid())
	{
		Container->SetHAlign(InAlignment);
	}
}

EVerticalAlignment SUnrealLuaDraggableBoxOverlay::GetBoxVerticalAlignment() const
{
	return VerticalAlignment;
}

void SUnrealLuaDraggableBoxOverlay::SetBoxVerticalAlignment(EVerticalAlignment InAlignment)
{
	VerticalAlignment = InAlignment;

	if (Container.IsValid())
	{
		Container->SetVAlign(InAlignment);
	}
}

FMargin SUnrealLuaDraggableBoxOverlay::GetPadding() const
{
	return Padding;
}

FVector2f SUnrealLuaDraggableBoxOverlay::RecomputeAnchorPoints(const FVector2f& InOffset)
{
	const FGeometry& MyGeometry = GetTickSpaceGeometry();

	const FVector2f AvailableSpace = (MyGeometry.GetAbsoluteSize() - DraggableBox->GetTickSpaceGeometry().GetAbsoluteSize())
		* (MyGeometry.GetLocalSize() / MyGeometry.GetAbsoluteSize());
	const FVector2f MidPoint = AvailableSpace * 0.5f;
	if (!ensure(!MidPoint.ContainsNaN())) // Don't call this during construction because the geometry is not initialized, yet. 
	{
		return InOffset;
	}

	FVector2f ConstrainedOffset = InOffset;
	ConstrainedOffset.X = FMath::Min(ConstrainedOffset.X, AvailableSpace.X);
	ConstrainedOffset.Y = FMath::Min(ConstrainedOffset.Y, AvailableSpace.Y);
	if (ConstrainedOffset.X > MidPoint.X)
	{
		ConstrainedOffset.X = FMath::Max(AvailableSpace.X - ConstrainedOffset.X, UnrealLua::UIWidgets::DraggableBorderArea); // Circle value around

		switch (HorizontalAlignment)
		{
		case EHorizontalAlignment::HAlign_Left:
			SetBoxHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
			break;

		case EHorizontalAlignment::HAlign_Right:
			SetBoxHorizontalAlignment(EHorizontalAlignment::HAlign_Left);
			break;

		default:
			// Do nothing
			break;
		}
	}

	if (ConstrainedOffset.Y > MidPoint.Y)
	{
		ConstrainedOffset.Y = FMath::Max(AvailableSpace.Y - ConstrainedOffset.Y, UnrealLua::UIWidgets::DraggableBorderArea); // Circle value around

		switch (VerticalAlignment)
		{
		case EVerticalAlignment::VAlign_Top:
			SetBoxVerticalAlignment(EVerticalAlignment::VAlign_Bottom);
			break;

		case EVerticalAlignment::VAlign_Bottom:
			SetBoxVerticalAlignment(EVerticalAlignment::VAlign_Top);
			break;

		default:
			// Do nothing
			break;
		}
	}

	return ConstrainedOffset;
}

FUnrealLuaDragBoxPosition SUnrealLuaDraggableBoxOverlay::GetDragBoxPosition() const
{
	return FUnrealLuaDragBoxPosition{ GetBoxAlignmentOffset(), HorizontalAlignment, VerticalAlignment };
}

void SUnrealLuaDraggableBoxOverlay::RestoreFromDragBoxPosition(const FUnrealLuaDragBoxPosition& InWidgetPosition)
{
	SetBoxHorizontalAlignment(InWidgetPosition.HAlign);
	SetBoxVerticalAlignment(InWidgetPosition.VAlign);

	// Do not recompute the anchors. Suppose the viewport is now much smaller than it was when InWidgetPosition was saved.
	// If the user now increases the size of the viewport, the widget should stay anchored to the same corner as when it was saved.
	constexpr bool bRecomputeAnchors = false;
	SetBoxAlignmentOffset(InWidgetPosition.RelativeOffset, bRecomputeAnchors);
}

TSharedPtr<SBox> SUnrealLuaDraggableBoxOverlay::GetDraggableContainer() const
{
	return this->Container;
}

TSharedPtr<SUnrealLuaDraggableBox> SUnrealLuaDraggableBoxOverlay::GetDraggableBox() const
{
	return StaticCastSharedPtr<SUnrealLuaDraggableBox>(this->DraggableBox);
}
