#pragma once
#include "Framework/Text/SyntaxTokenizer.h"

class UNREALLUATOOLS_API FLuaSyntaxTokenizer : public ISyntaxTokenizer
{
public:
	/** 
	 * Create a new tokenizer
	 */
	static TSharedRef<FLuaSyntaxTokenizer> Create();

	virtual ~FLuaSyntaxTokenizer(){};

	virtual void Process(TArray<FTokenizedLine>& outTokenizedLines, const FString& input) override;

protected:
	FLuaSyntaxTokenizer();

	void TokenizeLineRanges(const FString& input, const TArray<FTextRange>& lineRanges, TArray<FTokenizedLine>& outTokenizedLines);
	
	void ProcessFile(const FString& input, const TArray<FTextRange>& lineRanges, TArray<FTokenizedLine>& outTokenizedLines);

	TArray<FString> Keywords;
	TArray<FString> Annotations;
	TArray<FString> Operators;
};
