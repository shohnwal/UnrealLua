#pragma once
#include "LuaSyntaxTextRun.h"

class FLuaSyntaxTextRunKeywordLocal : public FUnrealLuaSyntaxTextRun
{
public:
	static TSharedRef<FLuaSyntaxTextRunKeywordLocal> Create( FRunInfo& InRunInfo, const TSharedRef< const FString >& InText, const FTextBlockStyle& Style, const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous);
	
	virtual FString GetRunName() const override { return "local";}
	virtual ELuaSyntaxTextRunType GetRunType() const override;

	FLuaSyntaxTextRunKeywordLocal( FRunInfo& InRunInfo, const TSharedRef< const FString >& InText, const FTextBlockStyle& Style, const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous);
	virtual ~FLuaSyntaxTextRunKeywordLocal() override = default;
	
};
