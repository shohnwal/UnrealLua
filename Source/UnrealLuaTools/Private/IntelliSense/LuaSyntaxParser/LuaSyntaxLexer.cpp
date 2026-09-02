// Fill out your copyright notice in the Description page of Project Settings.


#include "IntelliSense/LuaSyntaxParser/LuaSyntaxLexer.h"

#include "IntelliSense/LuaSyntaxParser/LuaSyntaxParser.h"


void UnrealLuaTools::SyntaxParse::Lexer::luaX_next(FLuaSyntaxParser& parser)
{
	parser.LastLineNumber = parser.CurrentLineNumber;
	if (parser.CurrentToken.token != TK_EOS)
	{
		parser.CurrentToken = parser.LookAheadToken;
		parser.LookAheadToken.token = TK_EOS;
	}
	else
	{
		parser.CurrentToken.token = UnrealLuaTools::SyntaxParse::Lexer::Lua_llex(parser, parser.CurrentToken.seminfo);
	}
}

namespace UnrealLuaTools::SyntaxParse::Lexer
{
	void read_string(FLuaSyntaxParser& parser, TCHAR delimiter, SemInfo& seminfo)
	{
#if 0
		parser.Save_and_Next();  /* keep delimiter (for error messages) */
		while (parser.CurrentCharacter != delimiter) {
			switch (parser.CurrentCharacter) {
			case EOZ:
				lexerror(ls, "unfinished string", TK_EOS);
				break;  /* to avoid warnings */
			case '\n':
			case '\r':
				lexerror(ls, "unfinished string", TK_STRING);
				break;  /* to avoid warnings */
			case '\\': {  /* escape sequences */
					int c;  /* final character to be saved */
					parser.Save_and_Next();  /* keep '\\' for error messages */
					switch (parser.CurrentCharacter) {
					case 'a': 
						c = '\a'; 
						goto read_save;
					case 'b': 
						c = '\b'; 
						goto read_save;
					case 'f': 
						c = '\f'; 
						goto read_save;
					case 'n': 
						c = '\n'; 
						goto read_save;
					case 'r': 
						c = '\r'; 
						goto read_save;
					case 't': 
						c = '\t'; 
						goto read_save;
					case 'v': 
						c = '\v'; 
						goto read_save;
					case 'x': 
						c = readhexaesc(ls); goto read_save;
					case 'u': 
						utf8esc(ls); 
						goto no_save;
					case '\n': case '\r':
						parser.InclineLineNumber(); 
						c = '\n'; 
						goto only_save;
					case '\\': case '\"': case '\'':
						c = ls->current; goto read_save;
					case EOZ: goto no_save;  /* will raise an error next loop */
					case 'z': {  /* zap following span of spaces */
							luaZ_buffremove(ls->buff, 1);  /* remove '\\' */
							next(ls);  /* skip the 'z' */
							while (lisspace(ls->current)) {
								if (currIsNewline(ls)) inclinenumber(ls);
								else next(ls);
							}
							goto no_save;
					}
					default: {
							esccheck(ls, lisdigit(ls->current), "invalid escape sequence");
							c = readdecesc(ls);  /* digital escape '\ddd' */
							goto only_save;
					}
					}
					read_save:
					parser.Next();
					/* go through */
					only_save:
					luaZ_buffremove(ls->buff, 1);  /* remove '\\' */
					save(ls, c);
					/* go through */
					no_save: break;
			}
			default:
				save_and_next(ls);
			}
		}
		save_and_next(ls);  /* skip delimiter */
		seminfo->ts = luaX_newstring(ls, luaZ_buffer(ls->buff) + 1,
		luaZ_bufflen(ls->buff) - 2);
	}
#endif
}

int UnrealLuaTools::SyntaxParse::Lexer::Lua_llex(FLuaSyntaxParser& parser, SemInfo& seminfo)
{
#if 0
  parser.ResetBuffer();
  for (;;) {
    switch (parser.CurrentCharacter) {
      case '\n': case '\r': {  /* line breaks */
        parser.InclineLineNumber();
        break;
      }
      case ' ': case '\f': case '\t': case '\v': {  /* spaces */
        parser.Next();
        break;
      }
      case '-': {  /* '-' or '--' (comment) */
      		parser.Next();
        if (parser.CurrentCharacter != '-') return '-';
        /* else is a comment */
      		parser.Next();
        if (parser.CurrentCharacter == '[') {  /* long comment? */
          size_t sep = skip_sep(parser);
          parser.ResetBuffer();  /* 'skip_sep' may dirty the buffer */
          if (sep >= 2) {
            read_long_string(parser, NULL, sep);  /* skip long comment */
            parser.ResetBuffer();  /* previous call may dirty the buff. */
            break;
          }
        }
        /* else short comment */
        while (!parser.CurrentisNewLine() && parser.CurrentCharacter != EOZ)
          parser.Next();  /* skip until end of line (or end of file) */
        break;
      }
      case '[': {  /* long string or simply '[' */
        size_t sep = skip_sep(parser);
        if (sep >= 2) {
          read_long_string(parser, seminfo, sep);
          return TK_STRING;
        }
        else if (sep == 0)  /* '[=...' missing second bracket? */
          lexerror(parser, "invalid long string delimiter", TK_STRING);
        return '[';
      }
      case '=': {
        parser.Next();
        if (parser.CheckNext1('='))
        {
	        return TK_EQ;  /* '==' */
        }
        else
        {
	        return '=';
        }
      }
      case '<': 
      {
      	parser.Next();
        if (parser.CheckNext1('='))
        {
	        return TK_LE;  /* '<=' */
        }
        else if (parser.CheckNext1('<'))
        {
	        return TK_SHL;  /* '<<' */
        }
        else
        {
	        return '<';
        }
      }
      case '>': {
      	parser.Next();
        if (parser.CheckNext1('='))
        {
	        return TK_GE;  /* '>=' */
        }
        else if  (parser.CheckNext1('>'))
        {
	        return TK_SHR;  /* '>>' */
        }
        else
        {
	        return '>';
        }
      }
      case '/': {
      	parser.Next();
        if (parser.CheckNext1('/'))
        {
	        return TK_IDIV;  /* '//' */
        }
        else
        {
	        return '/';
        }
      }
      case '~': case '!': { //@UNREALLUA MOD: ! and != are now supported
      	parser.Next();
        if (parser.CheckNext1('='))
        {
	        return TK_NE;  /* '~=' */
        }
        else
        {
	        return '~';
        }
      }
      case ':': {
      	parser.Next();
        if (parser.CheckNext1(':'))
        {
	        return TK_DBCOLON;  /* '::' */
        }
        else
        {
	        return ':';
        }
      }
      case '"': case '\'': {  /* short literal strings */
        read_string(parser, parser.CurrentCharacter, seminfo);
        return TK_STRING;
      }
      case '.': {  /* '.', '..', '...', or number */
        save_and_next(parser);
        if (parser.CheckNext1('.')) {
          if (parser.CheckNext1('.'))
            return TK_DOTS;   /* '...' */
          else return TK_CONCAT;   /* '..' */
        }
        else if (!FChar::IsDigit(parser.CurrentCharacter))
        {
	        return '.';
        }
        else
        {
	        return read_numeral(parser, seminfo);
        }
      }
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9': {
        return read_numeral(parser, seminfo);
      }
      case EOZ: {
        return TK_EOS;
      }
      default: {
        if (FChar::IsIdentifier(parser.CurrentCharacter)) {  /* identifier or reserved word? */
          TString *ts;
          do {
            save_and_next(parser);
          } while (FChar::IsIdentifier(parser.CurrentCharacter) || FChar::IsAlnum(parser.CurrentCharacter));
          /* find or create string */
          ts = luaS_newparsertr(parser->L, luaZ_buffer(parser->buff),
                                   luaZ_bufflen(parser->buff));
          if (isreserved(ts))   /* reserved word? */
            return ts->extra - 1 + FIRST_RESERVED;
          else {
            seminfo.ts = anchorstr(parser, ts);
            return TK_NAME;
          }
        }
        else {  /* single-char tokens ('+', '*', '%', '{', '}', ...) */
          int c = parser.CurrentCharacter;
          parser.Next();
          return c;
        }
      }
    }
#endif
	return 0;
  }
}


#if 0
struct FLuaSyntaxParser_LuaSyntax;

void UnrealLuaTools::SyntaxParse::Lexer::luaX_init(sol::state_view& lua)
{
}

namespace UnrealLuaTools::SyntaxParse::Lexer
{
	int llex (FLuaSyntaxParser_LuaSyntax *ls, SemInfo *seminfo)
	{
		luaZ_resetbuffer(ls->buff);
		for (;;) {
			switch (ls->current) {
			case '\n': case '\r': {  /* line breaks */
					inclinenumber(ls);
					break;
			}
			case ' ': case '\f': case '\t': case '\v': {  /* spaces */
					next(ls);
					break;
			}
			case '-': {  /* '-' or '--' (comment) */
					next(ls);
					if (ls->current != '-') return '-';
					/* else is a comment */
					next(ls);
					if (ls->current == '[') {  /* long comment? */
						size_t sep = skip_sep(ls);
						luaZ_resetbuffer(ls->buff);  /* 'skip_sep' may dirty the buffer */
						if (sep >= 2) {
							read_long_string(ls, NULL, sep);  /* skip long comment */
							luaZ_resetbuffer(ls->buff);  /* previous call may dirty the buff. */
							break;
						}
					}
					/* else short comment */
					while (!currIsNewline(ls) && ls->current != EOZ)
						next(ls);  /* skip until end of line (or end of file) */
					break;
			}
			case '[': {  /* long string or simply '[' */
					size_t sep = skip_sep(ls);
					if (sep >= 2) {
						read_long_string(ls, seminfo, sep);
						return TK_STRING;
					}
					else if (sep == 0)  /* '[=...' missing second bracket? */
						lexerror(ls, "invalid long string delimiter", TK_STRING);
					return '[';
			}
			case '=': {
					next(ls);
					if (check_next1(ls, '=')) return TK_EQ;  /* '==' */
					else return '=';
			}
			case '<': {
					next(ls);
					if (check_next1(ls, '=')) return TK_LE;  /* '<=' */
					else if (check_next1(ls, '<')) return TK_SHL;  /* '<<' */
					else return '<';
			}
			case '>': {
					next(ls);
					if (check_next1(ls, '=')) return TK_GE;  /* '>=' */
					else if (check_next1(ls, '>')) return TK_SHR;  /* '>>' */
					else return '>';
			}
			case '/': {
					next(ls);
					if (check_next1(ls, '/')) return TK_IDIV;  /* '//' */
					else return '/';
			}
			case '~': case '!': { //@UNREALLUA MOD: ! and != are now supported
					next(ls);
					if (check_next1(ls, '=')) return TK_NE;  /* '~=' */
					else return '~';
			}
			case ':': {
					next(ls);
					if (check_next1(ls, ':')) return TK_DBCOLON;  /* '::' */
					else return ':';
			}
			case '"': case '\'': {  /* short literal strings */
					read_string(ls, ls->current, seminfo);
					return TK_STRING;
			}
			case '.': {  /* '.', '..', '...', or number */
					save_and_next(ls);
					if (check_next1(ls, '.')) {
						if (check_next1(ls, '.'))
							return TK_DOTS;   /* '...' */
						else return TK_CONCAT;   /* '..' */
					}
					else if (!lisdigit(ls->current)) return '.';
					else return read_numeral(ls, seminfo);
			}
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9': {
					return read_numeral(ls, seminfo);
			}
			case EOZ: {
					return TK_EOS;
			}
			default: {
					if (lislalpha(ls->current)) {  /* identifier or reserved word? */
						TString *ts;
						do {
							save_and_next(ls);
						} while (lislalnum(ls->current));
						/* find or create string */
						ts = luaS_newlstr(ls->L, luaZ_buffer(ls->buff),
												 luaZ_bufflen(ls->buff));
						if (isreserved(ts))   /* reserved word? */
							return ts->extra - 1 + FIRST_RESERVED;
						else {
							seminfo->ts = anchorstr(ls, ts);
							return TK_NAME;
						}
					}
					else {  /* single-char tokens ('+', '*', '%', '{', '}', ...) */
						int c = ls->current;
						next(ls);
						return c;
					}
			}
			}
		}
	}
}
#endif

