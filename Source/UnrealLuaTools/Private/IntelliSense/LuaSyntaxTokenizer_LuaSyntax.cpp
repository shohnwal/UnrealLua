
#include "llex.h"
#include "IntelliSense/LuaSyntaxTozenizer.h"
#if 0
typedef union {
	double number;
	int64 integer;
	FString string;
} SemInfo;  /* semantics information */


typedef struct Token {
	int token;
	SemInfo seminfo;
} Token;

struct FLuaSyntaxParser
{
	void luaX_next();


	Token t;  /* current token */
	Token lookahead;  /* look ahead token */
	
	FString* inputFile;
	int32 lastLine = 0;
	int32 lineNumber = 0;
};

void FLuaSyntaxParser_LuaSyntax::luaX_next()
{
	this->lastLine = this->lineNumber;
	if (this->lookahead.token != TK_EOS) {  /* is there a look-ahead token? */
		this->t = this->lookahead;  /* use this one */
		this->lookahead.token = TK_EOS;  /* and discharge it */
	}
	else
		this->t.token = llex(ls, &ls->t.seminfo);  /* read next token */
}



#endif