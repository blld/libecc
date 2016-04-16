//
//  parser.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_parser_h
#ifdef Implementation
#undef Implementation
#include __FILE__
#include "implementation.h"
#else
#include "interface.h"
#define io_libecc_parser_h

	#include "lexer.h"

#endif


Interface(Parser,
	
	(struct Parser *, createWithLexer ,(struct Lexer *))
	(void, destroy, (struct Parser *))
	
	(struct Function *, parseWithEnvironment ,(struct Parser * const, struct Object *environment))
	,
	{
		struct Lexer *lexer;
		enum Lexer(Token) previewToken;
		struct Error *error;
		
		struct {
			struct Key key;
			char depth;
		} *depths;
		uint16_t depthCount;
		
		struct Function *function;
		uint16_t sourceDepth;
	}
)

#endif
