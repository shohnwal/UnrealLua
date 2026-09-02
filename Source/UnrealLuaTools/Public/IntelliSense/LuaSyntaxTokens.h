#pragma once
#include "CoreTypes.h"

inline const TCHAR* UnrealLuaAnnotations[] =
{
	TEXT("---@UPROPERTY"),
	TEXT("---@UFUNCTION"),
	TEXT("---@Param"),
	TEXT("---@Return"),
	TEXT("---@UNREALLUA"),
	TEXT("---@Type"),
};

inline const TCHAR* LuaOperators[] =
{
	TEXT("~="),
	TEXT("<="),
	TEXT(">="),
	TEXT("=="),
	TEXT("..."),
	TEXT(".."),
	TEXT("."),
	TEXT(":"),
	TEXT("+"),
	TEXT("-"),
	TEXT("*"),
	TEXT("/"),
	TEXT("%"),
	TEXT("#"),
	TEXT("^"),
	TEXT("~"),
	TEXT("<"),
	TEXT(">"),
	TEXT("="),
	TEXT("{"),
	TEXT("}"),
	TEXT("("),
	TEXT(")"),
};

inline const TCHAR* LuaKeywords[] =
{
	TEXT("and"),
	TEXT("or"),
	TEXT("not"),
	TEXT("break"),
	TEXT("do"),
	TEXT("for"),
	TEXT("while"),
	TEXT("repeat"),
	TEXT("until"),
	TEXT("if"),
	TEXT("elseif"),
	TEXT("else"),
	TEXT("then"),
	TEXT("end"),
	TEXT("function"),
	TEXT("in"),
	TEXT("local"),
	TEXT("else"),
	TEXT("return"),
	TEXT("true"),
	TEXT("false"),
};

inline const TCHAR* LuaScopeStartKeywords[] =
{
	TEXT("do"),
	TEXT("while"),
	TEXT("repeat"),
	TEXT("if"),
	//TEXT("then"),			
	TEXT("function"),
};

inline const TCHAR* LuaScopeEndAndStartKeywords[] =
{
	TEXT("elseif"),
	TEXT("else"),
};

inline const TCHAR* LuaScopeEndKeywords[] =
{
	TEXT("until"),
	TEXT("end"),
	TEXT("function"),
};