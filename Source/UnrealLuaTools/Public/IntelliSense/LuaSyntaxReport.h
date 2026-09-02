#pragma once
#include "CoreMinimal.h"

struct FLuaSyntaxReportEntry
{
	FString Entry = {};
	int32 Line = 0;
	FTextRange Offset = {0,0};
};

DECLARE_DELEGATE_OneParam(FLuaSyntaxReport, const TArray<FLuaSyntaxReportEntry>&)
