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
#include "closure.h"
#include "op.h"


#include "interface.h"

#define Module \
	io_libecc_Parser

Interface(
	(Instance, createWithLexer ,(struct Lexer *))
	(void, destroy, (Instance))
	(struct Closure *, parseWithContext ,(Instance const, struct Object *context))
	,
	{
		struct Lexer *lexer;
		enum Lexer(Token) previewToken;
		struct Error *error;
		
		struct {
			struct Identifier identifier;
			char depth;
		} *depths;
		uint16_t depthCount;
		
		struct Closure *closure;
	}
)

#endif
