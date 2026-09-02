// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "forward.hpp"
#include "CoreMinimal.h"
#include "llex.h"

struct FLuaSyntaxParser;

namespace UnrealLuaTools::SyntaxParse::Lexer
{
	void luaX_next(FLuaSyntaxParser& parser);
	int Lua_llex(FLuaSyntaxParser& Parser, SemInfo& Seminfo);
}


#if 0
/**
 * 
 */
namespace UnrealLuaTools::SyntaxParse::Lexer
{
	void luaX_init (sol::state_view& lua);
	void luaX_setinput (sol::state_view& lua, LexState *ls, ZIO *z,
								  TString *source, int firstchar);
	FString luaX_newstring (LexState *ls, const char *str, size_t l);
	void luaX_next (LexState *ls);
	int luaX_lookahead (LexState *ls);
	l_noret luaX_syntaxerror (LexState *ls, const char *s);
	const char *luaX_token2str (LexState *ls, int token);
}
#endif