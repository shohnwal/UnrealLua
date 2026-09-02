// Copyright Epic Games, Inc. All Rights Reserved.

#include "UIWidgets/Draggable/SUnrealLuaDraggableBox.h"

#include "Framework/Application/SlateApplication.h"
#include "Overlay/SDraggableBoxOverlay.h"
#include "Input/DragAndDrop.h"
#include "UIWidgets/Draggable/SUnrealLuaDraggableBoxOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"

/** A drag/drop operation used by SDraggableBox. */
class FUnrealLuaDraggableBoxUIDragOperation : public FDragDropOperation
{
public:
	FUnrealLuaDraggableBoxUIDragOperation(const TSharedRef<SUnrealLuaDraggableBox> InDraggableBox,
		const SUnrealLuaDraggableBox::FDragInfo& InDragInfo)
		: DraggableBoxWeak(InDraggableBox)
		, DragInfo(InDragInfo)
	{
	}

	virtual ~FUnrealLuaDraggableBoxUIDragOperation() override = default;

	//~ Begin FDragDropOperation
	virtual void OnDragged(const FDragDropEvent& InDragDropEvent)
	{
		if (TSharedPtr<SUnrealLuaDraggableBox> DraggableBox = DraggableBoxWeak.Pin())
		{
			DraggableBox->OnDragUpdate(InDragDropEvent, DragInfo, /* Dropped */ false);
		}
	}

	virtual void OnDrop(bool bDropWasHandled, const FPointerEvent& InMouseEvent)
	{
		if (TSharedPtr<SUnrealLuaDraggableBox> DraggableBox = DraggableBoxWeak.Pin())
		{
			DraggableBox->OnDragUpdate(InMouseEvent, DragInfo, /* Dropped */ true);
		}
	}
	//~ End FDragDropOperation

protected:
	TWeakPtr<SUnrealLuaDraggableBox> DraggableBoxWeak;
	SUnrealLuaDraggableBox::FDragInfo DragInfo;
};

void SUnrealLuaDraggableBox::Construct(const FArguments& InArgs, const TSharedRef<SUnrealLuaDraggableBoxOverlay>& InDraggableOverlay)
{
	DraggableOverlayWeak = InDraggableOverlay;
	InnerWidget = InArgs._Content.Widget;
	IsDraggableAttr = InArgs._IsDraggable;
	OnUserDraggedToNewPositionDelegate = InArgs._OnUserDraggedToNewPosition;

	ChildSlot
	[
		InArgs._Content.Widget
	];
}

void SUnrealLuaDraggableBox::OnDragUpdate(const FPointerEvent& InMouseEvent, const FDragInfo& InDragInfo, bool bInDropped)
{
	const TSharedPtr<SUnrealLuaDraggableBoxOverlay> DraggableOverlay = DraggableOverlayWeak.Pin();
	if (!DraggableOverlay.IsValid() || !IsDraggableAttr.Get())
	{
		return;
	}

	const FGeometry& MyGeometry = DraggableOverlay->GetTickSpaceGeometry();
	const FVector2f MouseOffset = (InMouseEvent.GetScreenSpacePosition() - InDragInfo.OriginalMousePosition)
		* (MyGeometry.GetLocalSize() / MyGeometry.GetAbsoluteSize());

	FVector2f NewAlignmentOffset = InDragInfo.OriginalAlignmentOffset;
	switch (InDragInfo.OriginalHorizontalAlignment)
	{
	case EHorizontalAlignment::HAlign_Left:
		NewAlignmentOffset.X += MouseOffset.X;
		break;

	case EHorizontalAlignment::HAlign_Right:
		NewAlignmentOffset.X -= MouseOffset.X;
		break;

	default:
		// Do nothing
		break;
	}

	switch (InDragInfo.OriginalVerticalAlignment)
	{
	case EVerticalAlignment::VAlign_Top:
		NewAlignmentOffset.Y += MouseOffset.Y;
		break;

	case EVerticalAlignment::VAlign_Bottom:
		NewAlignmentOffset.Y -= MouseOffset.Y;
		break;

	default:
		// Do nothing
		break;
	}

	DraggableOverlay->SetBoxHorizontalAlignment(InDragInfo.OriginalHorizontalAlignment);
	DraggableOverlay->SetBoxVerticalAlignment(InDragInfo.OriginalVerticalAlignment);
	DraggableOverlay->SetBoxAlignmentOffset(NewAlignmentOffset);

	if (bInDropped)
	{
		OnUserDraggedToNewPositionDelegate.ExecuteIfBound();
	}
}

FReply SUnrealLuaDraggableBox::OnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
	{
		return FReply::Unhandled();
	}
	return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
}

FReply SUnrealLuaDraggableBox::OnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const TSharedPtr<SUnrealLuaDraggableBoxOverlay> DraggableOverlay = DraggableOverlayWeak.Pin();
	if (DraggableOverlay && IsDraggableAttr.Get())
	{
		const FDragInfo DragInfo
		{
			DraggableOverlay->GetBoxHorizontalAlignment(),
			DraggableOverlay->GetBoxVerticalAlignment(),
			DraggableOverlay->GetBoxAlignmentOffset(),
			InMouseEvent.GetScreenSpacePosition()
		};
		const TSharedRef<FUnrealLuaDraggableBoxUIDragOperation> DragDropOperation = MakeShared<FUnrealLuaDraggableBoxUIDragOperation>(
			SharedThis(this), DragInfo
		);
		return FReply::Handled().BeginDragDrop(DragDropOperation);
	}
	return FReply::Unhandled();
}