#include "IntelliSense/LuaSyntaxParser/LuaSyntaxParser.h"

#include "IntelliSense/LuaSyntaxParser/LuaSyntaxLexer.h"


void FLuaSyntaxParser::Initialize(const FString& input)
{
	this->CurrentLineNumber = 1;
	this->LastLineNumber = 1;
	this->Input = input;
	this->CurrentToken.token = 0;
	this->LookAheadToken.token = TK_EOS;
	this->CurrentCharacter = input.IsEmpty() ? TK_EOS : input[0];
}

void FLuaSyntaxParser::ProcessFile(const FString& input, const TArray<FTextRange>& lineRanges, TArray<FTokenizedLine>& outTokenizedLines)
{
	this->Initialize(input);
	
	UnrealLuaTools::SyntaxParse::Lexer::luaX_next(*this);
}

void FLuaSyntaxParser::ResetBuffer()
{
	this->Buffer.Reset();
}

void FLuaSyntaxParser::InclineLineNumber()
{
	int32 oldLineNumber = this->CurrentLineNumber;
	verify(this->CurrentisNewLine());
	this->Next();
	if (this->CurrentisNewLine() && this->CurrentLineNumber != oldLineNumber)
	{
		this->Next();
	}
	this->CurrentLineNumber++;
	if (this->CurrentLineNumber > INT32_MAX)
	{
		this->Error("Chunk has too many lines!");
	}
}

void FLuaSyntaxParser::Next()
{
	this->Input.LeftChopInline(1);
	this->CurrentCharacter = this->Input.IsEmpty() ? TK_EOS : this->Input[0];
}

void FLuaSyntaxParser::Error(const FString& errorMsg)
{
	verifyf(false, TEXT("Error during parsing: %s"),*errorMsg);
	checkNoEntry()
}

bool FLuaSyntaxParser::CheckNext1(TCHAR tchar)
{
	if (this->CurrentCharacter == tchar)
	{
		this->Next();
		return true;
	}
	return false;
}

bool FLuaSyntaxParser::CheckNext2(TCHAR tchar1, TCHAR tchar2)
{
	if (this->CurrentCharacter == tchar1 || this->CurrentCharacter == tchar2)
	{
		this->Next();
		return true;
	}
	return false;
}

bool FLuaSyntaxParser::CurrentisNewLine() const
{
	return this->CurrentCharacter == '\n' || this->CurrentCharacter == '\r';
}

void FLuaSyntaxParser::Save_and_Next()
{
	this->Save();
	this->Next();
}

void FLuaSyntaxParser::Save()
{
	this->Buffer.Add(this->CurrentCharacter);
}
