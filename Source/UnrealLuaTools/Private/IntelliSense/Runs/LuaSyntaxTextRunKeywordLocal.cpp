#include "IntelliSense/Runs/LuaSyntaxTextRunKeywordLocal.h"

TSharedRef<FLuaSyntaxTextRunKeywordLocal> FLuaSyntaxTextRunKeywordLocal::Create(FRunInfo& InRunInfo,
	const TSharedRef<const FString>& InText, const FTextBlockStyle& Style, const FTextRange& InRange,
	const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous)
{
	return MakeShared<FLuaSyntaxTextRunKeywordLocal>(InRunInfo, InText, Style, InRange, previous);
}

FLuaSyntaxTextRunKeywordLocal::FLuaSyntaxTextRunKeywordLocal(FRunInfo& InRunInfo,
	const TSharedRef<const FString>& InText, const FTextBlockStyle& Style, const FTextRange& InRange,
	const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous)
	: FUnrealLuaSyntaxTextRun(InRunInfo, InText, Style, InRange, previous)
{
}

ELuaSyntaxTextRunType FLuaSyntaxTextRunKeywordLocal::GetRunType() const
{
	return ELuaSyntaxTextRunType::KeywordLocal;
}
