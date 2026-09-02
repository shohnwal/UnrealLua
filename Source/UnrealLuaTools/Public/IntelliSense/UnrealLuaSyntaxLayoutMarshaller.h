// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaSyntaxParserState.h"
#include "Framework/Text/SlateTextRun.h"
#include "Framework/Text/SyntaxHighlighterTextLayoutMarshaller.h"
#include "UObject/ScriptInterface.h"
#include "UObject/WeakInterfacePtr.h"


/**
 * 
 */

class ILuaToolsSession;
struct FScopedLuaContext;
struct FLuaSyntaxTextStyle;
struct FUnrealLuaSyntaxVariable;




class UNREALLUATOOLS_API FUnrealLuaSyntaxLayoutMarshaller : public FSyntaxHighlighterTextLayoutMarshaller
{
public:

	static TSharedRef<FUnrealLuaSyntaxLayoutMarshaller> Create(TScriptInterface<ILuaToolsSession> session);
	static TSharedPtr<ISyntaxTokenizer> CreateTokenizer();

	FLuaSyntaxReport OnReport = {};
protected:
	
	explicit FUnrealLuaSyntaxLayoutMarshaller(TSharedPtr<ISyntaxTokenizer> InTokenizer, TScriptInterface<ILuaToolsSession> session);
	
	virtual void SetText(const FString& SourceString, FTextLayout& TargetTextLayout) override;

	//virtual void SetText(const FString& sourceString, FTextLayout& targetTextLayout) override;
	//virtual void GetText(FString& targetString, const FTextLayout& sourceTextLayout) override;
	virtual void ParseTokens(const FString& sourceString, FTextLayout& targetTextLayout, TArray<ISyntaxTokenizer::FTokenizedLine> tokenizedLines) override;
	
	FTextLayout::FNewLineData ProcessTokenizedLine(const ISyntaxTokenizer::FTokenizedLine& TokenizedLine, const int32& LineNumber, const FString& SourceString, FLuaSyntaxParserState& parserState);
	
	/** Styles used to display the text */
	const FLuaSyntaxTextStyle& SyntaxTextStyle;
	
	FLuaSyntaxParserState Parser = {};
	
	TWeakInterfacePtr<ILuaToolsSession> Session = nullptr;
};
