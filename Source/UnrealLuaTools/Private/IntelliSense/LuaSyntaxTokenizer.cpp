
#include "IntelliSense/LuaSyntaxTokens.h"
#include "IntelliSense/LuaSyntaxTozenizer.h"




TSharedRef<FLuaSyntaxTokenizer> FLuaSyntaxTokenizer::Create()
{
	return MakeShareable(new FLuaSyntaxTokenizer());
}

void FLuaSyntaxTokenizer::Process(TArray<FTokenizedLine>& outTokenizedLines, const FString& input)
{
	TArray<FTextRange> LineRanges;
	FTextRange::CalculateLineRangesFromString(input, LineRanges);
	TokenizeLineRanges(input, LineRanges, outTokenizedLines);
}

FLuaSyntaxTokenizer::FLuaSyntaxTokenizer()
{
	// operators
	for(const auto& Operator : LuaOperators)
	{
		Operators.Emplace(Operator);
	}	

	// keywords
	for(const auto& Keyword : LuaKeywords)
	{
		Keywords.Emplace(Keyword);
	}

	// Pre-processor Keywords
	for(const auto& annotation : UnrealLuaAnnotations)
	{
		Annotations.Emplace(annotation);
	}
}

void FLuaSyntaxTokenizer::TokenizeLineRanges(const FString& input, const TArray<FTextRange>& lineRanges, TArray<FTokenizedLine>& outTokenizedLines)
{
	// Tokenize line ranges
	for(const FTextRange& LineRange : lineRanges)
	{
		FTokenizedLine TokenizedLine;
		TokenizedLine.Range = LineRange;
		
		if(TokenizedLine.Range.IsEmpty())
		{
			TokenizedLine.Tokens.Emplace(FToken(ETokenType::Literal, TokenizedLine.Range));
		}
		else
		{
			int32 CurrentOffset = LineRange.BeginIndex;
			
			while(CurrentOffset < LineRange.EndIndex)
			{
				const TCHAR* CurrentString = &input[CurrentOffset];
				const TCHAR CurrentChar = input[CurrentOffset];

				bool bHasMatchedSyntax = false;
				
				//try to match annotations
				if (CurrentChar == TEXT('-'))
				{
					for (auto annotation : this->Annotations)
					{
						if(FCString::Strncmp(CurrentString, *annotation, annotation.Len()) == 0)
						{
							const int32 SyntaxTokenEnd = LineRange.EndIndex;
							TokenizedLine.Tokens.Emplace(FToken(ETokenType::Syntax, FTextRange(CurrentOffset, SyntaxTokenEnd)));
						
							check(SyntaxTokenEnd <= LineRange.EndIndex);
							
							bHasMatchedSyntax = true;
						}
					}
					
					//Check for Lua comment
					if (!bHasMatchedSyntax)
					{
						int32 PeekOffset = CurrentOffset + 1;
						if(PeekOffset < LineRange.EndIndex && input[PeekOffset] == TEXT('-'))
						{
							const int32 SyntaxTokenEnd = LineRange.EndIndex;
							TokenizedLine.Tokens.Emplace(FToken(ETokenType::Syntax, FTextRange(CurrentOffset, SyntaxTokenEnd)));
							check(SyntaxTokenEnd <= LineRange.EndIndex);
							bHasMatchedSyntax = true;
						}
					}
				}
				
				if (bHasMatchedSyntax)
				{
					//Entire line got consumed by annotation or comment
					break;
				}
				
				//Check for strings
				if (CurrentChar == TCHAR{'\"'} || CurrentChar == TCHAR{'\''})
				{
					TokenizedLine.Tokens.Emplace(FToken(ETokenType::Syntax, FTextRange(CurrentOffset, CurrentOffset + 1)));
					CurrentOffset++;
					continue;
				}
				
				//Check for numbers, this needs to be done before operators due to dot in floating point numbers
				if (TChar<TCHAR>::IsDigit(CurrentChar))
				{
					// We have a number
					// They start with a number and can contain numbers or a single dot and contain
					
					int32 numberPeekOffset = CurrentOffset + 1;
					int32 numLength = 0;
					bool bHasDot = false;
					bool bIsValidNumber = true;
					while(numberPeekOffset < LineRange.EndIndex)
					{
						const TCHAR PeekChar = input[numberPeekOffset];
						++numLength;
						if (PeekChar == '.')
						{
							if (bHasDot)
							{
								//"tesdt.lua:21: malformed number near %s, this line"
								bIsValidNumber = false;
							}
							bHasDot = true;
						}
						else if (!FChar::IsDigit(PeekChar))
						{
							bIsValidNumber = false;
						}
						numberPeekOffset++;
					}
					if (bIsValidNumber)
					{
						bHasMatchedSyntax = true;
						TokenizedLine.Tokens.Emplace(FToken(ETokenType::Literal, FTextRange(CurrentOffset, numberPeekOffset)));
					
						CurrentOffset = numberPeekOffset;
					}
				}	
				
				if(bHasMatchedSyntax)
				{
					continue;
				}
				
				// Greedy matching for operators
				for(const FString& Operator : Operators)
				{
					if(FCString::Strncmp(CurrentString, *Operator, Operator.Len()) == 0)
					{
						const int32 SyntaxTokenEnd = CurrentOffset + Operator.Len();
						TokenizedLine.Tokens.Emplace(FToken(ETokenType::Syntax, FTextRange(CurrentOffset, SyntaxTokenEnd)));
					
						check(SyntaxTokenEnd <= LineRange.EndIndex);
					
						bHasMatchedSyntax = true;
						CurrentOffset = SyntaxTokenEnd;
						break;
					}
				}
			
				if(bHasMatchedSyntax)
				{
					continue;
				}
				
				int32 PeekOffset = CurrentOffset + 1;
				if (TChar<TCHAR>::IsAlpha(CurrentChar))
				{
					// Match Identifiers,
					// They start with a letter and contain
					// letters or numbers
					while(PeekOffset < LineRange.EndIndex)
					{
						const TCHAR PeekChar = input[PeekOffset];

						if (!TChar<TCHAR>::IsIdentifier(PeekChar))
						{
							break;
						}
						
						PeekOffset++;
					}
				}

				const int32 CurrentStringLength = PeekOffset - CurrentOffset;
				
				// Check if it is an reserved keyword
				for(const FString& Keyword : Keywords)
				{
					if (FCString::Strncmp(CurrentString, *Keyword, FMath::Max(CurrentStringLength, Keyword.Len())) == 0)
					{
						const int32 SyntaxTokenEnd = CurrentOffset + CurrentStringLength;
						TokenizedLine.Tokens.Emplace(FToken(ETokenType::Syntax, FTextRange(CurrentOffset, SyntaxTokenEnd)));
					
						check(SyntaxTokenEnd <= LineRange.EndIndex);
					
						bHasMatchedSyntax = true;
						CurrentOffset = SyntaxTokenEnd;
						break;
					}
				}

				if (bHasMatchedSyntax)
				{
					continue;
				}
	
				// If none matched, consume the character(s) as text
				const int32 TextTokenEnd = CurrentOffset + CurrentStringLength;
				TokenizedLine.Tokens.Emplace(FToken(ETokenType::Literal, FTextRange(CurrentOffset, TextTokenEnd)));
				CurrentOffset = TextTokenEnd;
			}
		}

		outTokenizedLines.Add(TokenizedLine);
	}
}

void FLuaSyntaxTokenizer::ProcessFile(const FString& input, const TArray<FTextRange>& lineRanges, TArray<FTokenizedLine>& outTokenizedLines)
{
	
}

