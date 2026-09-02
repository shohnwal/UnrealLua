#pragma once
#include "CoreMinimal.h"
#include "llex.h"

class FTokenizedLine;

struct FLuaSyntaxParser
{
	void Initialize(const FString& input);
	void ProcessFile(const FString& input, const TArray<FTextRange>& lineRanges, TArray<FTokenizedLine>& outTokenizedLines);
	void ResetBuffer();
	void InclineLineNumber();
	void Next();

	void Error(const FString& errorMsg);
	bool CheckNext1(TCHAR tchar);
	bool CheckNext2(TCHAR tchar1, TCHAR tchar2);
	bool CurrentisNewLine() const;
	void Save_and_Next();
	void Save();
	int64 LastLineNumber = 0;
	int64 CurrentLineNumber = 0;

	TCHAR CurrentCharacter = 0;
	Token CurrentToken = {};
	Token LookAheadToken = {};
	FStringView Input = nullptr;
	FString FileName = "";
	TArray<TCHAR> Buffer = {};
};