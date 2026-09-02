// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UnrealLuaDragBoxPosition.h"
#include "Widgets/SCompoundWidget.h"
#include "Layout/Margin.h"
#include "Types/SlateEnums.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class SUnrealLuaDraggableBox;
class SBox;

/**
 * Class that can be used to place a draggable box into a viewport or some other large widget as an overlay.
 * Just place the widget that you want to be draggable as the contents.
 * 
 * This overlay will remain confined to the bounds of the viewport dimensions.
 * It anchors the content to the corners of the parent widget: e.g. when a user drags it from the bottom-left to the bottom-right corner and
 * drags the right side of the viewport, the widget moves relative to the right side.
 */
class UNREALLUATOOLS_API SUnrealLuaDraggableBoxOverlay : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SUnrealLuaDraggableBoxOverlay)
		: _HAlign(HAlign_Left)
		, _VAlign(VAlign_Bottom)
		, _InitialAlignmentOffset(FVector2f::ZeroVector)
		, _IsDraggable(true)
	{}
	/** Use this to specify whether to align to the left or right. Center and Fill prevent repositioning horizontally. */
	SLATE_ARGUMENT(EHorizontalAlignment, HAlign)
	/** Use this to specify whether to align to the top or bottom. Center and Fill prevent repositioning vertically. */
	SLATE_ARGUMENT(EVerticalAlignment, VAlign)
	
	/** The initial offset this widget should have relative to HAlign and VAlign. */
	SLATE_ARGUMENT(FVector2f, InitialAlignmentOffset)
	/** Whether this widget should be allowed to be dragged. */
	SLATE_ATTRIBUTE(bool, IsDraggable)

	/**
	 * Invoked when the user has finished dragging the widget to a new position.
	 * This is useful if you want to save the box position persistently: call GetDragBoxPosition and save it in a config file.
	 */
	SLATE_EVENT(FSimpleDelegate, OnUserDraggedToNewPosition)
	
	/** The content you want to be able to drag around in the parent widget. */
	SLATE_DEFAULT_SLOT(FArguments, Content)
SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** @return The current position of the box on the viewport relative to HAlign and VAlign. */
	FVector2f GetBoxAlignmentOffset() const;

	/*
	 * Sets the content's position relative to the current anchor points.
	 * 
	 * @param InOffset Offset relative to the current HAlign and VAlign, e.g. X = 100 means an offset of 100 from the left or right widget side.
	 * @param bInRecomputeAnchorPoints Whether you want HAlign and VAlign recomputed,
	 *	e.g. if HAlign is currently left, parent is 200 wide, and InOffset.X == 150, this would recompute HAlign to be right.
	 */
	void SetBoxAlignmentOffset(const FVector2f& InOffset, bool bInRecomputeAnchorPoints = true);

	EHorizontalAlignment GetBoxHorizontalAlignment() const;
	void SetBoxHorizontalAlignment(EHorizontalAlignment InAlignment);

	EVerticalAlignment GetBoxVerticalAlignment() const;
	void SetBoxVerticalAlignment(EVerticalAlignment InAlignment);

	/** Gets information to restore this content's position. This is useful for saving in config files. */
	FUnrealLuaDragBoxPosition GetDragBoxPosition() const;
	/** Restores the content position. This is useful for loading a widget position saved in a config file. */
	void RestoreFromDragBoxPosition(const FUnrealLuaDragBoxPosition& InWidgetPosition);
	
	TSharedPtr<SBox> GetDraggableContainer() const;
	TSharedPtr<SUnrealLuaDraggableBox> GetDraggableBox() const;

protected:

	/** Contains the DraggableBox and repositions it relative to our parent. */
	TSharedPtr<SBox> Container;
	/** Contains the content and handles the drag-drop behaviour. */
	TSharedPtr<SWidget> DraggableBox;

	/** Affects the horizontal alignment of the content widget relative to the parent widget. */
	EHorizontalAlignment HorizontalAlignment = HAlign_Fill;
	/** Affects the vertical alignment of the content widget relative to the parent widget. */
	EVerticalAlignment VerticalAlignment = VAlign_Fill;
	/** The padding of the content widget. */
	FMargin Padding;

	FMargin GetPadding() const;

	/**
	 * Recomputes HorizontalAlignment and VerticalAlignment if InOffset is too far away from the current corner.
	 * @param InOffset Offset relative to current HorizontalAlignment and VerticalAlignment
	 * @return New offset relative to recomputed HorizontalAlignment and VerticalAlignment
	 */
	FVector2f RecomputeAnchorPoints(const FVector2f& InOffset);
};
