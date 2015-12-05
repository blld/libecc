//
//  parser.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_parser_h
#define io_libecc_parser_h

#include "namespace_io_libecc.h"

#include "lexer.h"
#include "op.h"
#include "object/function.h"

#include "interface.h"


Interface(Parser,
	
	(struct Parser *, createWithLexer ,(struct Lexer *))
	(void, destroy, (struct Parser *))
	
	(struct Function *, parseWithContext ,(struct Parser * const, struct Object *context))
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
