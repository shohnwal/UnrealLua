#pragma once
#include "CoreMinimal.h"
#include "Types/SlateEnums.h"
#include "UObject/UnrealType.h"
#include "UnrealLuaDragBoxPosition.generated.h"

USTRUCT()
struct FUnrealLuaDragBoxPosition
{
	GENERATED_BODY()

	UPROPERTY()
	FVector2f RelativeOffset = FVector2f::ZeroVector;

	UPROPERTY()
	TEnumAsByte<EHorizontalAlignment> HAlign = HAlign_Left;

	UPROPERTY()
	TEnumAsByte<EVerticalAlignment> VAlign = VAlign_Bottom;

	FUnrealLuaDragBoxPosition() = default;
	explicit FUnrealLuaDragBoxPosition(
		const FVector2f& RelativeOffset, const TEnumAsByte<EHorizontalAlignment>& HAlign, const TEnumAsByte<EVerticalAlignment>& VAlign
		)
		: RelativeOffset(RelativeOffset)
		, HAlign(HAlign)
		, VAlign(VAlign)
	{}
};

