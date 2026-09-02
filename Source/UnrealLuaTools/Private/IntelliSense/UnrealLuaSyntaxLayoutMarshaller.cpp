// Fill out your copyright notice in the Description page of Project Settings.


#include "IntelliSense/UnrealLuaSyntaxLayoutMarshaller.h"

#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Text/CharRangeList.h"
#include "Framework/Text/SlateTextLayout.h"
#include "Framework/Text/SlateTextRun.h"
#include "Framework/Text/TextLayout.h"
#include "IntelliSense/LuaSyntaxParserState.h"
#include "IntelliSense/LuaSyntaxTokens.h"
#include "IntelliSense/LuaSyntaxTozenizer.h"
#include "IntelliSense/UnrealLuaSyntaxParserScope.h"
#include "IntelliSense/Runs/LuaSyntaxTextRun.h"
#include "IntelliSense/Runs/LuaSyntaxTextRunAssignmentOperator.h"
#include "IntelliSense/Runs/LuaSyntaxTextRunBooleanConstant.h"
#include "IntelliSense/Runs/LuaSyntaxTextRunKeywordFunction.h"
#include "IntelliSense/Runs/LuaSyntaxTextRunKeywordFunctionParenthesis.h"
#include "IntelliSense/Runs/LuaSyntaxTextRunKeywordLocal.h"
#include "IntelliSense/Runs/LuaSyntaxTextRunNumericLiteral.h"
#include "IntelliSense/Runs/LuaSyntaxTextRunStringLiteral.h"
#include "IntelliSense/Runs/LuaSyntaxTextRunVariable.h"
#include "IntelliSense/Runs/LuaSyntaxTextRunVariablePeriodFieldAccessOperator.h"
#include "IntelliSense/Runs/LuaSyntaxTextRunVariableColonFunctionCallFieldAccessOperator.h"
#include "IntelliSense/Runs/LuaSyntaxTextRunVariableType.h"
#include "Session/LuaToolsSession.h"
#include "Utility/LuaLogMacros.h"
#include "Utility/WidgetStyles.h"

class FUnrealLuaSyntaxWhiteSpaceTextRun : public FUnrealLuaSyntaxTextRun
{
public:
	static TSharedRef<FUnrealLuaSyntaxWhiteSpaceTextRun> Create(
		const FRunInfo& InRunInfo,
		const TSharedRef<const FString>& InText,
		const FTextBlockStyle& Style,
		const FTextRange& InRange,
		const TSharedPtr<FUnrealLuaSyntaxTextRun> previousRun,
		int32 NumSpacesPerTab)
	{
		return MakeShareable(new FUnrealLuaSyntaxWhiteSpaceTextRun(InRunInfo, InText, Style, InRange, previousRun, NumSpacesPerTab));
	}
	virtual TSharedPtr<FUnrealLuaSyntaxTextRun> GetNonWhiteSpacedRun() override { return this->PreviousRun.IsValid() ? this->PreviousRun.Pin()->GetNonWhiteSpacedRun() : nullptr;};
public:
	virtual FVector2D Measure(
		int32 StartIndex,
		int32 EndIndex,
		float Scale,
		const FRunTextContext& TextContext
		) const override
	{
		const FVector2D ShadowOffsetToApply((EndIndex == Range.EndIndex) ? FMath::Abs(Style.ShadowOffset.X * Scale) : 0.0f, FMath::Abs(Style.ShadowOffset.Y * Scale));

		if (EndIndex - StartIndex == 0)
		{
			return FVector2D(ShadowOffsetToApply.X * Scale, GetMaxHeight(Scale));
		}

		// count tabs
		int32 TabCount = 0;
		for (int32 Index = StartIndex; Index < EndIndex; Index++)
		{
			if ((*Text)[Index] == TEXT('\t'))
			{
				TabCount++;
			}
		}

		const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		FVector2D Size = FontMeasure->Measure(**Text, StartIndex, EndIndex, Style.Font, true, Scale) + ShadowOffsetToApply;

		Size.X -= TabWidth * TabCount * Scale;
		Size.X += SpaceWidth * TabCount * NumSpacesPerTab * Scale;

		return Size;
	}

	virtual ELuaSyntaxTextRunType GetRunType() const override { return ELuaSyntaxTextRunType::WhiteSpace;}
protected:
	FUnrealLuaSyntaxWhiteSpaceTextRun(
		const FRunInfo& InRunInfo, 
		const TSharedRef<const FString>& InText, 
		const FTextBlockStyle& InStyle, 
		const FTextRange& InRange, 
		const TSharedPtr<FUnrealLuaSyntaxTextRun> previousRun,
		int32 InNumSpacesPerTab) : 
		FUnrealLuaSyntaxTextRun(InRunInfo, InText, InStyle, InRange, previousRun), 
		NumSpacesPerTab(InNumSpacesPerTab)
	{
		// measure tab width
		const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		TabWidth = FontMeasure->Measure(TEXT("\t"), 0, 1, Style.Font, true, 1.0f).X;
		SpaceWidth = FontMeasure->Measure(TEXT(" "), 0, 1, Style.Font, true, 1.0f).X;
	}

private:
	int32 NumSpacesPerTab;

	double TabWidth;

	double SpaceWidth;
};






TSharedRef<FUnrealLuaSyntaxLayoutMarshaller> FUnrealLuaSyntaxLayoutMarshaller::Create(TScriptInterface<ILuaToolsSession> session)
{
	// Create the syntax highlighter

	return MakeShareable(new FUnrealLuaSyntaxLayoutMarshaller(CreateTokenizer(), session));
}

TSharedPtr<ISyntaxTokenizer> FUnrealLuaSyntaxLayoutMarshaller::CreateTokenizer()
{

	return FLuaSyntaxTokenizer::Create();
}

FUnrealLuaSyntaxLayoutMarshaller::FUnrealLuaSyntaxLayoutMarshaller(TSharedPtr<ISyntaxTokenizer> InTokenizer, TScriptInterface<ILuaToolsSession> session)
	: FSyntaxHighlighterTextLayoutMarshaller(MoveTemp(InTokenizer))
	, SyntaxTextStyle(UnrealLuaTools::SlateStyles::GetLuaSyntaxStyle())
	, Session(session)
{
}

void FUnrealLuaSyntaxLayoutMarshaller::SetText(const FString& SourceString, FTextLayout& TargetTextLayout)
{
	//FSyntaxHighlighterTextLayoutMarshaller::SetText(SourceString, TargetTextLayout);
	if(bSyntaxHighlightingEnabled)
	{
		TArray<ISyntaxTokenizer::FTokenizedLine> TokenizedLines;
		Tokenizer->Process(TokenizedLines, SourceString);
		ParseTokens(SourceString, TargetTextLayout, TokenizedLines);
	}
	else
	{
		FPlainTextLayoutMarshaller::SetText(SourceString, TargetTextLayout);
	}
}

namespace UnrealLua::Parser
{
	bool TokenIsIn(const TArrayView<const TCHAR*> arr, const FString& TokenText)
	{
		for(const FString& scopeStart : arr)
		{
			if(FCString::Strncmp(*TokenText, *scopeStart, scopeStart.Len()) == 0)
			{
				return true;
			}
		}
		return false;
	}	
	
}



void FUnrealLuaSyntaxLayoutMarshaller::ParseTokens(const FString& sourceString, FTextLayout& targetTextLayout, TArray<ISyntaxTokenizer::FTokenizedLine> tokenizedLines)
{
	//LUA_LOG("LUASYNTAX : Begin parsing")
	TArray<FTextLayout::FNewLineData> LinesToAdd;
	LinesToAdd.Reserve(tokenizedLines.Num());

	// Parse the tokens, generating the styled runs for each line
	int32 LineNo = 0;
	Parser.Start();
	for(const ISyntaxTokenizer::FTokenizedLine& TokenizedLine : tokenizedLines)
	{
		LinesToAdd.Add(ProcessTokenizedLine(TokenizedLine, LineNo, sourceString, this->Parser));
		LineNo++;
	}

	targetTextLayout.AddLines(LinesToAdd);
	
	//LUA_LOG("LUASYNTAX : End parsing")
	
	this->Parser.EndParse();
	
	this->OnReport.ExecuteIfBound(this->Parser.GetReport());
	
	this->Parser.Reset();
}

FTextLayout::FNewLineData FUnrealLuaSyntaxLayoutMarshaller::ProcessTokenizedLine( const ISyntaxTokenizer::FTokenizedLine& TokenizedLine, const int32& LineNumber, const FString& SourceString, FLuaSyntaxParserState& parserState)
{
	parserState.NewLine(LineNumber);
	
	TSharedRef<FString> ModelString = MakeShareable(new FString());
	TArray< TSharedRef< IRun > > Runs;

	//FStringBuilderBase builder;
	//builder << "Parsing line :";
	//for(const ISyntaxTokenizer::FToken& Token : TokenizedLine.Tokens)
	//{
	//	FStringView strv{GetData(SourceString) + Token.Range.BeginIndex, Token.Range.Len()};
	//	strv.TrimEndInline();
	//	const bool bIsWhitespace = strv.IsEmpty();
	//	if (bIsWhitespace)
	//	{
	//		continue;
	//	}
	//	
	//	if (Token.Type == ISyntaxTokenizer::ETokenType::Syntax)
	//	{
	//		builder << "{" << strv <<"}";
	//	}
	//	else
	//	{
	//		builder << "'" << strv <<"'";
	//	}
	//}
	//LUA_LOG("%s", builder.ToString())
	
	bool firstinLine = true;
	
	//previousRun is always the previous non-whitespace run
	TSharedPtr<FUnrealLuaSyntaxTextRun> previousRun = parserState.GetPreviousLineRun();
	parserState.SetPreviousLineRun(nullptr);
	
	TSharedPtr<FLuaSyntaxTextRunVariableType> previousLineVariableType = parserState.GetSavedVariableType();
	parserState.SetSavedVariableType(nullptr);
	
	for(int32 tokenIndex = 0; tokenIndex < TokenizedLine.Tokens.Num(); ++tokenIndex)
	{
		const ISyntaxTokenizer::FToken& Token = TokenizedLine.Tokens[tokenIndex];
		
		const FString TokenText = SourceString.Mid(Token.Range.BeginIndex, Token.Range.Len());
		
		bool canPeek = TokenizedLine.Tokens.IsValidIndex(tokenIndex + 1);
		
		const ISyntaxTokenizer::FToken* peekToken = canPeek ? &TokenizedLine.Tokens[tokenIndex + 1] : nullptr;  
		const FStringView peekTokenText = peekToken ? FStringView{SourceString}.Mid(peekToken->Range.BeginIndex, peekToken->Range.Len()) : FStringView{};
		
		const FTextRange ModelRange(ModelString->Len(), ModelString->Len() + TokenText.Len());
		ModelString->Append(TokenText);

		//LUA_LOG("ModelString is %s", **ModelString)
		FRunInfo RunInfo{};

		TSharedPtr< FUnrealLuaSyntaxTextRun > Run = nullptr;
		
		int32 offsetInText = Token.Range.BeginIndex - TokenizedLine.Range.BeginIndex; 
		int32 offsetlength = Token.Range.EndIndex - TokenizedLine.Range.BeginIndex; 
		
		parserState.SetLineOffset(FTextRange{offsetInText, offsetlength});
		//parserState.SetPreviousRun(previousRun);
		const bool bIsWhitespace = FString(TokenText).TrimEnd().IsEmpty();
		
		if(bIsWhitespace)
		{
			Run = FUnrealLuaSyntaxWhiteSpaceTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.NormalTextStyle, ModelRange, previousRun, 4);
			Runs.Add(Run.ToSharedRef());	
		}
		else
		{
			if (parserState.IsParseState(ELuaSyntaxParseState::Error))
			{
				Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.NormalTextStyle, ModelRange, previousRun);
			}
			else if(Token.Type == ISyntaxTokenizer::ETokenType::Syntax)
			{
				if(parserState.IsParseState( ELuaSyntaxParseState::None))
				{
					if(TokenText == TEXT("\""))
					{
						Run = FLuaSyntaxTextRunStringLiteralMarker::Create(RunInfo, ModelString, SyntaxTextStyle.StringTextStyle, ModelRange, previousRun);
						parserState.HandleSetAssignedVariableType(Run, "str", LineNumber);
						parserState.PushParseState(ELuaSyntaxParseState::LookingForString);
					}
					else if(TokenText == TEXT("\'"))
					{
						Run = FLuaSyntaxTextRunStringLiteralMarker::Create(RunInfo, ModelString, SyntaxTextStyle.StringTextStyle, ModelRange, previousRun);
						parserState.HandleSetAssignedVariableType(Run, "str", LineNumber);
						parserState.PushParseState(ELuaSyntaxParseState::LookingForCharacter);
					}
					else if(firstinLine && TokenText.StartsWith(TEXT("---@")))
					{
						//The token is the entire line of annotation
						
						TArray<FString> annotationParms;
						TokenText.ParseIntoArrayWS(annotationParms);
						
						parserState.SetParseState(ELuaSyntaxParseState::LookingForSingleLineAnnotation);
						//First part is always non-empty (due to the "---@"
						FStringView firstPart{annotationParms[0]};
						//Chop off the left part, so we get what's after the @
						firstPart.RightChopInline(4);
						
						//can define types with either 
						//---@Type <typename> or 
						//---@<typename> or 
						//---@Import "/Path/To/Asset"
						if (firstPart.Equals("Type", ESearchCase::IgnoreCase))
						{
							if (annotationParms.Num() >= 2)
							{
								//@TODO : handle ---@Type Import "/Path/To/Asset"
								FString typeName = annotationParms[1];
								parserState.SetSavedVariableType(FLuaSyntaxTextRunVariableType::Create(RunInfo, ModelString, SyntaxTextStyle.AnnotationStyle, ModelRange, previousRun, typeName));
								Run = parserState.GetSavedVariableType();
							}
							else
							{
								parserState.SetSavedVariableType(FLuaSyntaxTextRunVariableType::Create(RunInfo, ModelString, SyntaxTextStyle.AnnotationStyle, ModelRange, previousRun, ""));
								Run = parserState.GetSavedVariableType();
							}
						}
						else if (firstPart.Equals("Import", ESearchCase::IgnoreCase))
						{
							if (annotationParms.Num() >= 2)
							{
								FString importPath = annotationParms[1];
								parserState.SetSavedVariableType(FLuaSyntaxTextRunVariableType::Create(RunInfo, ModelString, SyntaxTextStyle.AnnotationStyle, ModelRange, previousRun, importPath));
								Run = parserState.GetSavedVariableType();
							}
							else
							{
								parserState.SetSavedVariableType(FLuaSyntaxTextRunVariableType::Create(RunInfo, ModelString, SyntaxTextStyle.AnnotationStyle, ModelRange, previousRun, ""));
								Run = parserState.GetSavedVariableType();
							}
						}
						else if (firstPart.Equals("Param", ESearchCase::IgnoreCase))
						{
							FString varName;
							FString varType;
							if (annotationParms.Num() >= 3)
							{
								varName = annotationParms[1];
								varType = annotationParms[2];
								if (parserState.AddFuncParam(varName, varType, false))
								{
									Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.AnnotationStyle, ModelRange, previousRun);
								}
								else
								{
									Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.ErrorTextStyle, ModelRange, previousRun);
									parserState.SetParseState(ELuaSyntaxParseState::Error);
								}
							}
							else
							{
								Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.ErrorTextStyle, ModelRange, previousRun);
								parserState.SetParseState(ELuaSyntaxParseState::Error);
								parserState.MakeRecord("Insufficient info for @Return, need a variable name and type.\nExample usage: ---@Param myVar int32");
							}
							
						}
						else if (firstPart.Equals("Return", ESearchCase::IgnoreCase))
						{
							FString varName;
							FString varType;
							if (annotationParms.Num() >= 3)
							{
								varName = annotationParms[1];
								varType = annotationParms[2];
								if (parserState.AddFuncParam(varName, varType, true))
								{
									Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.AnnotationStyle, ModelRange, previousRun);
								}
								else
								{
									Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.ErrorTextStyle, ModelRange, previousRun);
									parserState.SetParseState(ELuaSyntaxParseState::Error);
								}
							}
							else
							{
								Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.ErrorTextStyle, ModelRange, previousRun);
								parserState.SetParseState(ELuaSyntaxParseState::Error);
								parserState.MakeRecord("Insufficient info for @Return, need a variable name and type.\nExample usage: ---@Return ret boolean\n(variable name is unused and only serves as a description");
							}
						}
						else if (firstPart.Equals("UPROPERTY", ESearchCase::IgnoreCase))
						{
							Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.AnnotationStyle, ModelRange, previousRun);
						}
						else if (firstPart.Equals("UFUNCTION", ESearchCase::IgnoreCase))
						{
							Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.AnnotationStyle, ModelRange, previousRun);
						}
						else if(!firstPart.IsEmpty())
						{
							//can define types with ---@<type>
							//i.e. ---@FVector
							FString typeName{firstPart};
							parserState.SetSavedVariableType(FLuaSyntaxTextRunVariableType::Create(RunInfo, ModelString, SyntaxTextStyle.AnnotationStyle, ModelRange, previousRun, typeName));
							Run = parserState.GetSavedVariableType();							
						}
						else
						{
							Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.AnnotationStyle, ModelRange, previousRun);
							parserState.SetParseState(ELuaSyntaxParseState::LookingForSingleLineAnnotation);
						}
					}
					else if (TokenText == TEXT("="))
					{
						Run = FLuaSyntaxTextRunAssignmentOperator::Create(RunInfo, ModelString, SyntaxTextStyle.OperatorTextStyle, ModelRange, previousRun);
						parserState.SetParseState(ELuaSyntaxParseState::None);
					}
					else if(TokenText == TEXT("--[["))
					{
						Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.CommentTextStyle, ModelRange, previousRun);
						parserState.SetParseState(ELuaSyntaxParseState::LookingForMultiLineComment);
					}
					else if(TokenText == TEXT("--"))
					{
						Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.CommentTextStyle, ModelRange, previousRun);
						parserState.SetParseState(ELuaSyntaxParseState::LookingForSingleLineComment);
					}
					else if(TokenText == TEXT("function"))
					{
						if (previousRun && previousRun->IsRunType(ELuaSyntaxTextRunType::Assignment))
						{
							//local name = function()
							//name = function()
						
							ensure(previousRun && previousRun->IsRunType(ELuaSyntaxTextRunType::Assignment));
							TSharedPtr<FLuaSyntaxTextRunVariable> varRun = previousRun->FindCurrentVariableRun();
							if (varRun)
							{
								varRun->SetVariableType("function", LineNumber);
								Run = FLuaSyntaxTextRunKeywordFunction::Create(RunInfo, ModelString, SyntaxTextStyle.KeywordTextStyle, ModelRange, previousRun);
								parserState.SetParseState(ELuaSyntaxParseState::NewFunctionLookingForArgsOpenParanthesis);
							}
							else
							{
								Run = FLuaSyntaxTextRunKeywordFunction::Create(RunInfo, ModelString, SyntaxTextStyle.ErrorTextStyle, ModelRange, previousRun);
								parserState.MakeRecord(MAKE_REPORT("Can not find variable name to assign function to"));
								parserState.SetParseState(ELuaSyntaxParseState::Error);
							}
						}
						else
						{
							//function func()
							//function Script:func()
							Run = FLuaSyntaxTextRunKeywordFunction::Create(RunInfo, ModelString, SyntaxTextStyle.KeywordTextStyle, ModelRange, previousRun);
							
							//next we're looking for a function name
							parserState.SetParseState(ELuaSyntaxParseState::NewFunctionLookingForName);	
						}
					}
					else if (TokenText == TEXT("..."))
					{
						Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.KeywordTextStyle, ModelRange, previousRun);
					}
					else if (TokenText == TEXT(".."))
					{
						Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.KeywordTextStyle, ModelRange, previousRun);
					}
					else if(TokenText == TEXT("."))
					{
						if (previousRun->IsRunType(ELuaSyntaxTextRunType::Variable))
						{
							// myVar.
							//      ^
							//needs runinfo of last variable
							TSharedPtr<FLuaSyntaxTextRunVariable> fieldAccessRun = previousRun->As<FLuaSyntaxTextRunVariable>();
							if (!fieldAccessRun->CanIndex())
							{
								parserState.MakeRecord(MAKE_REPORT("Attempting to index a new local variable %s", *fieldAccessRun->GetVariableName()));
								Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.ErrorTextStyle, ModelRange, previousRun);
								parserState.SetParseState(ELuaSyntaxParseState::Error);
							}
							else
							{
								Run = FLuaSyntaxTextRunVariablePeriodFieldAccessOperator::Create(RunInfo, ModelString, SyntaxTextStyle.KeywordTextStyle, ModelRange, previousRun);
								parserState.SetParseState(ELuaSyntaxParseState::PeriodLookingForFieldName);								
							}
						}
						else if (previousRun->IsRunType(ELuaSyntaxTextRunType::FunctionParamsParenthesis) && previousRun->As<FLuaSyntaxTextRunKeywordFunctionParenthesis>()->IsFunctionCall() && previousRun->As<FLuaSyntaxTextRunKeywordFunctionParenthesis>()->IsCloseParenthesis())
						{
							//can index result of function call: func(args).FieldName
							Run = FLuaSyntaxTextRunVariablePeriodFieldAccessOperator::Create(RunInfo, ModelString, SyntaxTextStyle.KeywordTextStyle, ModelRange, previousRun);
							parserState.SetParseState(ELuaSyntaxParseState::PeriodLookingForFieldName);								
						}
						else
						{
							parserState.MakeRecord(MAKE_REPORT("Attempting to index without a proper context"));
							Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.ErrorTextStyle, ModelRange, previousRun);
							parserState.SetParseState(ELuaSyntaxParseState::Error);
						}
					}
					else if(TokenText == TEXT(":"))
					{
						if (TSharedPtr<FLuaSyntaxTextRunVariable> var = previousRun->FindCurrentVariableRun())
						{						
							Run = FLuaSyntaxTextRunVariableColonFunctionCallFieldAccessOperator::Create(RunInfo, ModelString, SyntaxTextStyle.KeywordTextStyle, ModelRange, previousRun);
							parserState.SetParseState(ELuaSyntaxParseState::ColonLookingForFunctionName);
						}
						else if (previousRun->IsRunType(ELuaSyntaxTextRunType::FunctionParamsParenthesis) && previousRun->As<FLuaSyntaxTextRunKeywordFunctionParenthesis>()->IsFunctionCall() && previousRun->As<FLuaSyntaxTextRunKeywordFunctionParenthesis>()->IsCloseParenthesis())
						{
							//can call result of function call: func(args):FieldName
							Run = FLuaSyntaxTextRunVariablePeriodFieldAccessOperator::Create(RunInfo, ModelString, SyntaxTextStyle.KeywordTextStyle, ModelRange, previousRun);
							parserState.SetParseState(ELuaSyntaxParseState::ColonLookingForFunctionName);								
						}
						else
						{
							parserState.MakeRecord(MAKE_REPORT("Attempting to call function without a proper context"));
							Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.ErrorTextStyle, ModelRange, previousRun);
							parserState.SetParseState(ELuaSyntaxParseState::Error);
						}
					}
					else if(TChar<TCHAR>::IsIdentifier(TokenText[0]))
					{
						if (UnrealLua::Parser::TokenIsIn(LuaScopeStartKeywords, TokenText))
						{
							parserState.EnterScope(TokenText);
							Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.KeywordTextStyle, ModelRange, previousRun);
							parserState.SetParseState(ELuaSyntaxParseState::None);
						}
						else if (UnrealLua::Parser::TokenIsIn(LuaScopeEndKeywords, TokenText))
						{
							if (parserState.LeaveScope(TokenText))
							{
								Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.KeywordTextStyle, ModelRange, previousRun);
								parserState.SetParseState(ELuaSyntaxParseState::None);
							}
							else
							{
								Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.ErrorTextStyle, ModelRange, previousRun);
								parserState.SetParseState(ELuaSyntaxParseState::Error);
							}
						}
						else if (UnrealLua::Parser::TokenIsIn(LuaScopeEndAndStartKeywords, TokenText))
						{
							bool success = parserState.LeaveScope(TokenText);
							if (success)
							{
								Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.KeywordTextStyle, ModelRange, previousRun);
								parserState.EnterScope(TokenText);
								parserState.SetParseState(ELuaSyntaxParseState::None);
							}
							else
							{
								Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.ErrorTextStyle, ModelRange, previousRun);
								parserState.SetParseState(ELuaSyntaxParseState::Error);
							}
						}
						else if(TokenText == TEXT("local"))
						{
							Run = FLuaSyntaxTextRunKeywordLocal::Create(RunInfo, ModelString, SyntaxTextStyle.KeywordTextStyle, ModelRange, previousRun);

							parserState.SetParseState(ELuaSyntaxParseState::LookingForLocalVarName);		
						}
						else if (UnrealLua::Parser::TokenIsIn(LuaKeywords, TokenText))
						{
							if (TokenText == ("true") || TokenText == ("false"))
							{
								Run = FLuaSyntaxTextRunBooleanConstant::Create(RunInfo, ModelString, SyntaxTextStyle.KeywordTextStyle, ModelRange, previousRun);
							}
							else
							{
								Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.KeywordTextStyle, ModelRange, previousRun);
							}
							parserState.SetParseState(ELuaSyntaxParseState::None);
						}
					}
					else if (previousRun && previousRun->IsRunType(ELuaSyntaxTextRunType::Variable) && TokenText == TEXT("("))
					{
						Run = FLuaSyntaxTextRunKeywordFunctionParenthesis::Create(RunInfo, ModelString, SyntaxTextStyle.KeywordTextStyle, ModelRange, previousRun, true, true);
						parserState.SetParseState(ELuaSyntaxParseState::FunctionCallParamList);
					}
					else if(UnrealLua::Parser::TokenIsIn(LuaOperators, TokenText))
					{
						Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.OperatorTextStyle, ModelRange, previousRun);
						parserState.SetParseState(ELuaSyntaxParseState::None);
					}
				}
				else if(parserState.IsParseState(ELuaSyntaxParseState::LookingForString) && TokenText == TEXT("\""))
				{
					Run = FLuaSyntaxTextRunStringLiteralMarker::Create(RunInfo, ModelString, SyntaxTextStyle.StringTextStyle, ModelRange, previousRun);
					parserState.PopParseState();
				}
				else if(parserState.IsParseState( ELuaSyntaxParseState::LookingForCharacter) && TokenText == TEXT("\'"))
				{
					Run = FLuaSyntaxTextRunStringLiteralMarker::Create(RunInfo, ModelString, SyntaxTextStyle.StringTextStyle, ModelRange, previousRun);
					parserState.PopParseState();
				}
				else if(parserState.IsParseState( ELuaSyntaxParseState::LookingForMultiLineComment) && TokenText == TEXT("]]"))
				{
					Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.CommentTextStyle, ModelRange, previousRun);
					parserState.SetParseState(ELuaSyntaxParseState::None);
				}
				else if(parserState.IsParseState( ELuaSyntaxParseState::LookingForLocalVarName) && TokenText == TEXT("function"))
				{
					//local function func()
					Run = FLuaSyntaxTextRunKeywordFunction::Create(RunInfo, ModelString, SyntaxTextStyle.KeywordTextStyle, ModelRange, previousRun);
					//stay at LookingForLocalVarName state
					parserState.SetParseState(ELuaSyntaxParseState::LookingForLocalVarName);
				}
				else if (parserState.IsParseState(ELuaSyntaxParseState::NewFunctionLookingForPeriod) && TokenText == TEXT("."))
				{
					verify(previousRun->IsRunType(ELuaSyntaxTextRunType::Variable))
					Run = FLuaSyntaxTextRunVariablePeriodFieldAccessOperator::Create(RunInfo, ModelString, SyntaxTextStyle.KeywordTextStyle, ModelRange, previousRun);
					parserState.SetParseState(ELuaSyntaxParseState::PeriodLookingForFunctionName);
				}
				else if (parserState.IsParseState(ELuaSyntaxParseState::NewFunctionLookingForColon) && TokenText == TEXT(":"))
				{
					verify(previousRun->IsRunType(ELuaSyntaxTextRunType::Variable))
					Run = FLuaSyntaxTextRunVariableColonFunctionCallFieldAccessOperator::Create(RunInfo, ModelString, SyntaxTextStyle.KeywordTextStyle, ModelRange, previousRun);
					parserState.SetParseState(ELuaSyntaxParseState::ColonLookingForFunctionName);
				}
				else if(parserState.IsParseState( ELuaSyntaxParseState::NewFunctionLookingForArgsOpenParanthesis) && TokenText == TEXT("("))
				{
					Run = FLuaSyntaxTextRunKeywordFunctionParenthesis::Create(RunInfo, ModelString, SyntaxTextStyle.KeywordTextStyle, ModelRange, previousRun, true, false);
					parserState.EnterScope(TokenText);
					
					//If the call syntax is with a colon, try to assign self value
					if (previousRun->IsRunType(ELuaSyntaxTextRunType::Variable) && previousRun->HasPreviousRunType(ELuaSyntaxTextRunType::Colon))
					{
						auto colonRun = previousRun->PreviousRun.Pin();
						if (colonRun->HasPreviousRunType(ELuaSyntaxTextRunType::Variable))
						{
							auto selfRun = colonRun->PreviousAs<FLuaSyntaxTextRunVariable>();
							TSharedPtr<FUnrealLuaSyntaxVariable> selfVar = parserState.AddNewLocalVar("self");
							selfVar->ChangeType(selfRun->GetVariable()->GetType());
						}
					}
					parserState.SetParseState(ELuaSyntaxParseState::NewFunctionParamList);
				}
				else if(parserState.IsParseState( ELuaSyntaxParseState::NewFunctionLookingForName) && TokenText == TEXT("("))
				{
					//anonymous function
					Run = FLuaSyntaxTextRunKeywordFunctionParenthesis::Create(RunInfo, ModelString, SyntaxTextStyle.KeywordTextStyle, ModelRange, previousRun, true, false);
					parserState.EnterScope(TokenText);
					parserState.SetParseState(ELuaSyntaxParseState::NewFunctionParamList);
				}
				else if(parserState.IsParseState( ELuaSyntaxParseState::LookingForFunctionCallArgsOpenParanthesis) && TokenText == TEXT("("))
				{
					Run = FLuaSyntaxTextRunKeywordFunctionParenthesis::Create(RunInfo, ModelString, SyntaxTextStyle.KeywordTextStyle, ModelRange, previousRun, true, true);
					parserState.SetParseState(ELuaSyntaxParseState::FunctionCallParamList);
				}
				else if(parserState.IsParseState( ELuaSyntaxParseState::NewFunctionParamList))
				{
					if (TokenText == TEXT(")"))
					{
						Run = FLuaSyntaxTextRunKeywordFunctionParenthesis::Create(RunInfo, ModelString, SyntaxTextStyle.KeywordTextStyle, ModelRange, previousRun, false, false);
						parserState.ClearStashedFuncParams();
						parserState.SetParseState(ELuaSyntaxParseState::None);
					}
					else
					{
						Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.ErrorTextStyle, ModelRange, previousRun);
						parserState.SetParseState(ELuaSyntaxParseState::Error);
					}
				}
				else if(parserState.IsParseState( ELuaSyntaxParseState::FunctionCallParamList))
				{
					if (TokenText == TEXT(")"))
					{
						Run = FLuaSyntaxTextRunKeywordFunctionParenthesis::Create(RunInfo, ModelString, SyntaxTextStyle.KeywordTextStyle, ModelRange, previousRun, false, true);
						parserState.SetParseState(ELuaSyntaxParseState::None);
					}
					else if(TokenText == TEXT("\""))
					{
						Run = FLuaSyntaxTextRunStringLiteralMarker::Create(RunInfo, ModelString, SyntaxTextStyle.StringTextStyle, ModelRange, previousRun);
						parserState.HandleSetAssignedVariableType(Run, "str", LineNumber);
						parserState.PushParseState(ELuaSyntaxParseState::LookingForString);
					}
					else if(TokenText == TEXT("\'"))
					{
						Run = FLuaSyntaxTextRunStringLiteralMarker::Create(RunInfo, ModelString, SyntaxTextStyle.StringTextStyle, ModelRange, previousRun);
						parserState.HandleSetAssignedVariableType(Run, "str", LineNumber);
						parserState.PushParseState(ELuaSyntaxParseState::LookingForCharacter);
					}
					else if (UnrealLua::Parser::TokenIsIn(LuaOperators, TokenText))
					{
						Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.KeywordTextStyle, ModelRange, previousRun);
						parserState.SetParseState(ELuaSyntaxParseState::Error);						
					}
					else if (TokenText == ("true") || TokenText == ("false"))
					{
						Run = FLuaSyntaxTextRunBooleanConstant::Create(RunInfo, ModelString, SyntaxTextStyle.KeywordTextStyle, ModelRange, previousRun);
					}
					else
					{
						Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.ErrorTextStyle, ModelRange, previousRun);
						parserState.SetParseState(ELuaSyntaxParseState::Error);
					}
				}
			}
			
			// It's possible that we fail to match a syntax token if we're in a state where it isn't parsed
			// In this case, we treat it as a literal token
			if(Token.Type == ISyntaxTokenizer::ETokenType::Literal  || Run == nullptr)
			{
				if(parserState.IsParseState( ELuaSyntaxParseState::LookingForString))
				{
					Run = FLuaSyntaxTextRunStringLiteral::Create(RunInfo, ModelString, SyntaxTextStyle.StringTextStyle, ModelRange, previousRun, TokenText);
				}
				else if(parserState.IsParseState( ELuaSyntaxParseState::LookingForCharacter))
				{
					Run = FLuaSyntaxTextRunStringLiteral::Create(RunInfo, ModelString, SyntaxTextStyle.StringTextStyle, ModelRange, previousRun, TokenText);
				}
				else if (parserState.IsParseState( ELuaSyntaxParseState::NewFunctionLookingForName))
				{
					//Assigning new function
					
					//function func()
					//function Script:func()
					
					//can either be a single funcName or var:funcName... :funcName is already handled, if we put parserState to None after this
					if (previousRun->IsRunType(ELuaSyntaxTextRunType::FunctionKeyword))
					{
						if (TChar<TCHAR>::IsAlpha(TokenText[0]) || TChar<TCHAR>::IsUnderscore(TokenText[0]))
						{
							bool nextIsPeriod = !peekTokenText.IsEmpty() && peekTokenText.Equals(".");
							bool nextIsColon = !peekTokenText.IsEmpty() && peekTokenText.Equals(":");
						
							//we are a new variable for a function
							//function <name>()
							//or
							//function <name>:unknownyet()
							if (nextIsPeriod || nextIsColon)
							{
								//found a period or colon next, so this should be treated as the outer "container"
								//function <Container>:func()
								
								if (nextIsColon)
								{
									//@TODO : flag as self param?
								}
								
								TSharedPtr<FUnrealLuaSyntaxVariable> functionContainerVar = parserState.FindVar(TokenText);
								if (!functionContainerVar.IsValid())
								{
									//no container found, so le's just assume that it's a valid global container somewhere //@TODO : maye invoke warning?
									functionContainerVar = parserState.AddGlobalVar(TokenText, "");	
								}
								Run = FLuaSyntaxTextRunVariable::CreateNewVariableRun(RunInfo, ModelString, SyntaxTextStyle.NormalTextStyle, ModelRange, previousRun, functionContainerVar);
								if (nextIsPeriod)
								{
									parserState.SetParseState(ELuaSyntaxParseState::NewFunctionLookingForPeriod);	
								}
								else if (nextIsColon)
								{
									parserState.SetParseState(ELuaSyntaxParseState::NewFunctionLookingForColon);
								}
							}
							else
							{
								// no period or colon, the variable itself is the function
								// syntax local function <funcname>() is not allowed, so it must be a global
								TSharedPtr<FUnrealLuaSyntaxVariable> newGlobalFunctionVar = parserState.AddGlobalVar(TokenText, "function");
								TSharedPtr<FLuaSyntaxTextRunVariable> newRun = FLuaSyntaxTextRunVariable::CreateNewVariableRun(RunInfo, ModelString, SyntaxTextStyle.NormalTextStyle, ModelRange, previousRun, newGlobalFunctionVar);
								Run = newRun;
								parserState.SetParseState(ELuaSyntaxParseState::NewFunctionLookingForArgsOpenParanthesis);
							}
						}
						else 
						{
							parserState.MakeRecord(MAKE_REPORT("Expected identifier for function name, but found %s", *TokenText));
							Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.ErrorTextStyle, ModelRange, previousRun);
							parserState.SetParseState(ELuaSyntaxParseState::Error);
						}	
					}
					else if (previousRun->IsRunType(ELuaSyntaxTextRunType::Colon))
					{
						//we are a new variable for a function inside a container
						//function Container:<funcname>()
						verify(previousRun->HasPreviousRunType(ELuaSyntaxTextRunType::Variable));
						auto containerRun = previousRun->PreviousAs<FLuaSyntaxTextRunVariable>();
						TSharedPtr<FLuaSyntaxTextRunVariable> newRun = FLuaSyntaxTextRunVariable::CreateAccessNestedVariableRun(RunInfo, ModelString, TokenText, SyntaxTextStyle.NormalTextStyle, ModelRange, previousRun, LineNumber);
						Run = newRun;
						parserState.SetParseState(ELuaSyntaxParseState::NewFunctionLookingForArgsOpenParanthesis);
					}
				}
				else if(parserState.IsParseState( ELuaSyntaxParseState::LookingForLocalVarName))
				{
					//this can either be
					//local varname
					//or
					//local function funcName()
					if (TChar<TCHAR>::IsAlpha(TokenText[0]) || TChar<TCHAR>::IsUnderscore(TokenText[0]))
					{
						TSharedPtr<FUnrealLuaSyntaxVariable> newVar = parserState.AddNewLocalVar(TokenText);
						if (newVar.IsValid())
						{
							if (previousRun->IsRunType(ELuaSyntaxTextRunType::FunctionKeyword))
							{
								//we are a variable for a function
								//local function <name>()
								newVar->ChangeType("function");
								Run = FLuaSyntaxTextRunVariable::CreateNewVariableRun(RunInfo, ModelString, this->SyntaxTextStyle.NormalTextStyle, ModelRange, previousRun, newVar);
								parserState.SetParseState(ELuaSyntaxParseState::NewFunctionLookingForArgsOpenParanthesis);
							}
							else
							{
								//just a normal local variable
 								if (previousLineVariableType)
								{
									newVar->ChangeType(previousLineVariableType->GetVariableTypeName());
									previousLineVariableType = nullptr;
								}
								Run = FLuaSyntaxTextRunVariable::CreateNewVariableRun(RunInfo, ModelString, this->SyntaxTextStyle.NormalTextStyle, ModelRange, previousRun, newVar);
								parserState.SetParseState(ELuaSyntaxParseState::None);		
							}
						}
						else
						{
							Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.ErrorTextStyle, ModelRange, previousRun);
							parserState.SetParseState(ELuaSyntaxParseState::Error);
						}
					}
					else 
					{
						parserState.MakeRecord(MAKE_REPORT("Expected identifier for local variable, but found %s", *TokenText));
						Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.ErrorTextStyle, ModelRange, previousRun);
						parserState.SetParseState(ELuaSyntaxParseState::Error);
					}
				}
				else if(parserState.IsParseState( ELuaSyntaxParseState::LookingForSingleLineComment))
				{
					Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.CommentTextStyle, ModelRange, previousRun);
				}	
				else if(parserState.IsParseState( ELuaSyntaxParseState::LookingForSingleLineAnnotation))
				{
					Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.AnnotationStyle, ModelRange, previousRun);
				}				
				else if(parserState.IsParseState( ELuaSyntaxParseState::LookingForSingleLineAnnotationType))
				{
					parserState.SetSavedVariableType(FLuaSyntaxTextRunVariableType::Create(RunInfo, ModelString, SyntaxTextStyle.AnnotationStyle, ModelRange, previousRun, ""));
					Run = parserState.GetSavedVariableType();
					parserState.SetParseState(ELuaSyntaxParseState::LookingForSingleLineAnnotation);
				}
				else if(parserState.IsParseState( ELuaSyntaxParseState::LookingForMultiLineComment))
				{
					Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.CommentTextStyle, ModelRange, previousRun);
				}
				else if (parserState.IsParseState( ELuaSyntaxParseState::NewFunctionParamList))
				{
					if (TChar<TCHAR>::IsAlpha(TokenText[0]) || TChar<TCHAR>::IsUnderscore(TokenText[0]))
					{
						TSharedPtr<FUnrealLuaSyntaxVariable> newVar = parserState.AddNewLocalVar(TokenText);
						if (newVar)
						{
							FLuaSyntaxFunctionParam* stashedVarInfo = parserState.GetStashedFuncParam(TokenText);
							if (stashedVarInfo)
							{
								newVar->ChangeType(stashedVarInfo->GetVariableType());
							}
							Run = FLuaSyntaxTextRunVariable::CreateNewVariableRun(RunInfo, ModelString, SyntaxTextStyle.NormalTextStyle, ModelRange, previousRun, newVar);
						}
						else
						{
							Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.ErrorTextStyle, ModelRange, previousRun);
							parserState.MakeRecord(MAKE_REPORT("Duplicate function argument %s", *TokenText));
							parserState.SetParseState(ELuaSyntaxParseState::Error);
						}
					}
					else if (TokenText == TEXT(","))
					{
						//a comma in a function args list should have an actual arg name before it
						if (previousRun->IsRunType(ELuaSyntaxTextRunType::Variable))
						{
							Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.NormalTextStyle, ModelRange, previousRun);
						}
						else
						{
							Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.ErrorTextStyle, ModelRange, previousRun);
							parserState.MakeRecord(MAKE_REPORT("Expedted identifier for function argument, but found %s", *TokenText));
							parserState.SetParseState(ELuaSyntaxParseState::Error);
						}
					}
					else if (TokenText == TEXT("..."))
					{
						Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.NormalTextStyle, ModelRange, previousRun);
					}
					else
					{
						parserState.MakeRecord(MAKE_REPORT("Expedted identifier for function argument, but found %s", *TokenText));
						Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.ErrorTextStyle, ModelRange, previousRun);
						parserState.SetParseState(ELuaSyntaxParseState::Error);
					}					
				}
				else if (parserState.IsParseState( ELuaSyntaxParseState::FunctionCallParamList))
				{
					if (TChar<TCHAR>::IsAlpha(TokenText[0]) || TChar<TCHAR>::IsUnderscore(TokenText[0]))
					{
						TSharedPtr<FUnrealLuaSyntaxVariable> funcParam = parserState.FindVar(TokenText);
						if (!funcParam.IsValid())
						{
							//no variable found, so le's just assume that it's a valid global variable somewhere //@TODO : mabe invoke warning?
							funcParam = parserState.AddGlobalVar(TokenText, "");	
						}
						Run = FLuaSyntaxTextRunVariable::CreateFuncCallVariableAccessRun(RunInfo, ModelString, SyntaxTextStyle.NormalTextStyle, ModelRange, previousRun, funcParam);
					}
					else if (TokenText.IsNumeric())
					{
						if (TokenText.Contains("."))
						{
							//float
							Run = FLuaSyntaxTextRunNumericLiteral::Create(RunInfo, ModelString, SyntaxTextStyle.NumberTextStyle, ModelRange, previousRun, FCString::Atof(*TokenText));
							parserState.HandleSetAssignedVariableType(Run, "number", LineNumber);
						}
						else
						{
							//integer
							Run = FLuaSyntaxTextRunNumericLiteral::Create(RunInfo, ModelString, SyntaxTextStyle.NumberTextStyle, ModelRange, previousRun, FCString::Atoi(*TokenText));
							parserState.HandleSetAssignedVariableType(Run, "number", LineNumber);
						}
					}
					else if (TokenText == TEXT(","))
					{
						//a comma in a function args list should have an actual correct argument before it
						if (previousRun->IsRunType(ELuaSyntaxTextRunType::Variable) || previousRun->IsRunType(ELuaSyntaxTextRunType::String) || previousRun->IsRunType(ELuaSyntaxTextRunType::Number) || previousRun->IsRunType(ELuaSyntaxTextRunType::Boolean))
						{
							Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.NormalTextStyle, ModelRange, previousRun);
						}
						else
						{
							Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.ErrorTextStyle, ModelRange, previousRun);
							parserState.MakeRecord(MAKE_REPORT("Expected identifier for function call argument, but found %s", *TokenText));
							parserState.SetParseState(ELuaSyntaxParseState::Error);
						}
					}
					else if (TokenText == TEXT("..."))
					{
						Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.NormalTextStyle, ModelRange, previousRun);
					}
					else
					{
						parserState.MakeRecord(MAKE_REPORT("Expedted identifier for function call argument, but found %s", *TokenText));
						Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.ErrorTextStyle, ModelRange, previousRun);
						parserState.SetParseState(ELuaSyntaxParseState::Error);
					}					
				}
				else if (parserState.IsParseState( ELuaSyntaxParseState::PeriodLookingForFieldName))
				{
					verify(previousRun);
					if (!previousRun->IsRunType(ELuaSyntaxTextRunType::Period))
					{
						parserState.MakeRecord(MAKE_REPORT("Previous run was not period, but found %s", *previousRun->Text.Get()));
						Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.ErrorTextStyle, ModelRange, previousRun);
						parserState.SetParseState(ELuaSyntaxParseState::Error);
					}
					else if (TChar<TCHAR>::IsAlpha(TokenText[0]) || TChar<TCHAR>::IsUnderscore(TokenText[0]))
					{
						if (previousRun->HasPreviousRunType(ELuaSyntaxTextRunType::FunctionParamsParenthesis) && previousRun->PreviousAs<FLuaSyntaxTextRunKeywordFunctionParenthesis>()->IsCloseParenthesis())
						{
							//func(args).Field
							TSharedPtr<FUnrealLuaSyntaxVariable> tempVar = parserState.CreateTemporaryVar(TokenText);
							Run = FLuaSyntaxTextRunVariable::CreateAccessRValueVariableRun(RunInfo, ModelString, SyntaxTextStyle.NormalTextStyle, ModelRange, previousRun, tempVar);
							parserState.SetParseState(ELuaSyntaxParseState::None);
						}
						else
						{
							// [myVar][.]FieldName
							//           ^
							//needs runinfo of last variable so we can scan through last variable type for fields
							TSharedPtr<FLuaSyntaxTextRunVariable> outerVar = previousRun->FindCurrentVariableRun();
							verify(outerVar.IsValid())
							verify(previousRun->PreviousRun == outerVar)
							verify(outerVar->IsRunType(ELuaSyntaxTextRunType::Variable));
							TSharedPtr<FUnrealLuaSyntaxVariable> field = outerVar->Variable;
						
							Run = FLuaSyntaxTextRunVariable::CreateAccessNestedVariableRun(RunInfo, ModelString, TokenText, SyntaxTextStyle.NormalTextStyle, ModelRange, previousRun, LineNumber);
							parserState.SetParseState(ELuaSyntaxParseState::None);	
						}
					}
					else
					{
						parserState.MakeRecord(MAKE_REPORT("Expedted identifier for member access, but found %s", *TokenText));
						Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.ErrorTextStyle, ModelRange, previousRun);
						parserState.SetParseState(ELuaSyntaxParseState::Error);
					}					
				}
				else if (parserState.IsParseState( ELuaSyntaxParseState::ColonLookingForFunctionName) || parserState.IsParseState( ELuaSyntaxParseState::PeriodLookingForFunctionName))
				{
					verify(previousRun);
					if (previousRun->IsRunType(ELuaSyntaxTextRunType::Colon))
					{
						if (TChar<TCHAR>::IsAlpha(TokenText[0]) || TChar<TCHAR>::IsUnderscore(TokenText[0]))
						{
							//This should be the var owning the function : var:funcName() 
							TSharedPtr<FLuaSyntaxTextRunVariable> varRun = previousRun->FindCurrentVariableRun();
							verify(varRun.IsValid())
							verify(previousRun->PreviousRun == varRun);
							verify(varRun->IsRunType(ELuaSyntaxTextRunType::Variable));
							if (varRun->bIsNewVariableRun)
							{
								//part of a new function
								Run = FLuaSyntaxTextRunVariable::CreateAccessNestedVariableRun(RunInfo, ModelString, TokenText, SyntaxTextStyle.NormalTextStyle, ModelRange, previousRun, LineNumber);
								parserState.SetParseState(ELuaSyntaxParseState::NewFunctionLookingForArgsOpenParanthesis);
							}
							else
							{
								Run = FLuaSyntaxTextRunVariable::CreateAccessNestedVariableRun(RunInfo, ModelString, TokenText, SyntaxTextStyle.NormalTextStyle, ModelRange, previousRun, LineNumber);
								parserState.SetParseState(ELuaSyntaxParseState::LookingForFunctionCallArgsOpenParanthesis);
							}
						}					
						else 
						{
							parserState.MakeRecord(MAKE_REPORT("Expedted identifier for function call, but found %s", *TokenText));
							Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.ErrorTextStyle, ModelRange, previousRun);
							parserState.SetParseState(ELuaSyntaxParseState::Error);
						}	
					}
					else if (previousRun->IsRunType(ELuaSyntaxTextRunType::Period))
					{
						if (TChar<TCHAR>::IsAlpha(TokenText[0]) || TChar<TCHAR>::IsUnderscore(TokenText[0]))
						{
							//This should be the var owning the function : var.funcName() 
							TSharedPtr<FLuaSyntaxTextRunVariable> varRun = previousRun->FindCurrentVariableRun();
							verify(varRun.IsValid())
							verify(previousRun->PreviousRun == varRun);
							verify(varRun->IsRunType(ELuaSyntaxTextRunType::Variable));
							if (varRun->bIsNewVariableRun)
							{
								//part of a new function
								Run = FLuaSyntaxTextRunVariable::CreateAccessNestedVariableRun(RunInfo, ModelString, TokenText, SyntaxTextStyle.NormalTextStyle, ModelRange, previousRun, LineNumber);
								parserState.SetParseState(ELuaSyntaxParseState::NewFunctionLookingForArgsOpenParanthesis);
							}
							else
							{
								Run = FLuaSyntaxTextRunVariable::CreateAccessNestedVariableRun(RunInfo, ModelString, TokenText, SyntaxTextStyle.NormalTextStyle, ModelRange, previousRun, LineNumber);
								parserState.SetParseState(ELuaSyntaxParseState::LookingForFunctionCallArgsOpenParanthesis);
							}
						}					
						else 
						{
							parserState.MakeRecord(MAKE_REPORT("Expedted identifier for function call, but found %s", *TokenText));
							Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.ErrorTextStyle, ModelRange, previousRun);
							parserState.SetParseState(ELuaSyntaxParseState::Error);
						}
					}
					else
					{
						parserState.MakeRecord(MAKE_REPORT("Previous run of %s was not colon", *TokenText));
						Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.ErrorTextStyle, ModelRange, previousRun);
						parserState.SetParseState(ELuaSyntaxParseState::Error);
					}
				}
				else
				{
					if (TChar<TCHAR>::IsAlpha(TokenText[0]) || TChar<TCHAR>::IsUnderscore(TokenText[0]))
					{
						TSharedPtr<FUnrealLuaSyntaxVariable> var = parserState.FindVar(TokenText);
						if (!var)
						{
							var = parserState.AddGlobalVar(TokenText, "");
						}
						if (previousLineVariableType)
						{
							parserState.ChangeVarType(var, previousLineVariableType->GetVariableTypeName());
							previousLineVariableType = nullptr;
						}
						Run = FLuaSyntaxTextRunVariable::CreateAccessVariableRun(RunInfo, ModelString, SyntaxTextStyle.NormalTextStyle, ModelRange, previousRun, var);
						parserState.SetParseState(ELuaSyntaxParseState::None);
					}
					else if (TokenText.IsNumeric())
					{
						if (TokenText.Contains("."))
						{
							//float
							Run = FLuaSyntaxTextRunNumericLiteral::Create(RunInfo, ModelString, SyntaxTextStyle.NumberTextStyle, ModelRange, previousRun, FCString::Atof(*TokenText));
							parserState.HandleSetAssignedVariableType(Run, "number", LineNumber);
						}
						else
						{
							//integer
							Run = FLuaSyntaxTextRunNumericLiteral::Create(RunInfo, ModelString, SyntaxTextStyle.NumberTextStyle, ModelRange, previousRun, FCString::Atoi(*TokenText));
							parserState.HandleSetAssignedVariableType(Run, "number", LineNumber);
						}
						parserState.SetParseState(ELuaSyntaxParseState::None);
					}
					else
					{
						Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.NormalTextStyle, ModelRange, previousRun);
						parserState.SetParseState(ELuaSyntaxParseState::None);
					}
				}
			}
			if (!Run.IsValid())
			{
				parserState.MakeRecord(MAKE_REPORT("Token %s at line %d : %d did not produce a run! (char index %d and token index %d), parserState %d",  *TokenText, Token.Type == ISyntaxTokenizer::ETokenType::Literal ? 0 : 1, LineNumber, ModelRange.BeginIndex, tokenIndex, (int32)parserState.GetParseState()));
				Run = FUnrealLuaSyntaxTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.ErrorTextStyle, ModelRange, previousRun);
				parserState.SetParseState(ELuaSyntaxParseState::Error);
			}
			verify(Run.IsValid())
			Runs.Add(Run.ToSharedRef());
		}
		firstinLine = false;
		
		previousRun = Run->GetNonWhiteSpacedRun();
	}

	switch (parserState.GetParseState())
	{
	case ELuaSyntaxParseState::None:
		break;
	case ELuaSyntaxParseState::LookingForString:
	case ELuaSyntaxParseState::LookingForCharacter:
		{
			parserState.MakeRecord(MAKE_REPORT("Unfinished string"));
			parserState.SetParseState(ELuaSyntaxParseState::None);
		}
		break;
	case ELuaSyntaxParseState::LookingForSingleLineComment:
		{
			parserState.SetParseState(ELuaSyntaxParseState::None);
		}
		break;
	case ELuaSyntaxParseState::LookingForSingleLineAnnotation:
		{
			parserState.SetParseState(ELuaSyntaxParseState::None);
		}
		break;
	case ELuaSyntaxParseState::LookingForSingleLineAnnotationType:
		{
			parserState.SetParseState(ELuaSyntaxParseState::None);
		}
		break;
	case ELuaSyntaxParseState::LookingForLocalVarName:
		break;
	case ELuaSyntaxParseState::NewFunctionParamList:
		break;
	case ELuaSyntaxParseState::ColonLookingForFunctionName:
		break;
	case ELuaSyntaxParseState::PeriodLookingForFieldName:
		break;
	case ELuaSyntaxParseState::NewFunctionLookingForName:
		break;
	case ELuaSyntaxParseState::NewFunctionLookingForArgsOpenParanthesis:
		break;
	case ELuaSyntaxParseState::LookingForFunctionCallArgsOpenParanthesis:
		break;
	case ELuaSyntaxParseState::FunctionCallParamList:
		break;
	case ELuaSyntaxParseState::LookingForDoubleColon:
		break;
	default:
		break;
	}
	
	//parserState.SetParseState(ELuaSyntaxParseState::None);
	parserState.SetPreviousLineRun(previousRun);
	
	return FTextLayout::FNewLineData(MoveTemp(ModelString), MoveTemp(Runs));
}