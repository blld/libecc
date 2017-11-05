//
//  regexp.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_regexp_h
#ifdef Implementation
#undef Implementation
#include __FILE__
#include "../implementation.h"
#else
#include "../interface.h"
#define io_libecc_regexp_h

	#include "global.h"

	extern struct Object * RegExp(prototype);
	extern struct Function * RegExp(constructor);
	extern const struct Object(Type) RegExp(type);

	struct RegExp(State) {
		const char * const start;
		const char * const end;
		const char **capture;
		const char **index;
		int flags;
	};

	struct RegExp(Node);

	enum RegExp(Options) {
		RegExp(allowUnicodeFlags) = 1 << 0,
	};

#endif


Interface(RegExp,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(struct RegExp *, create ,(struct Chars *pattern, struct Error **, enum RegExp(Options)))
	(struct RegExp *, createWith ,(struct Context *context, struct Value pattern, struct Value flags))
	
	(int, matchWithState ,(struct RegExp *, struct RegExp(State) *))
	,
	{
		struct Object object;
		struct Chars *pattern;
		struct Chars *source;
		struct RegExp(Node) *program;
		uint8_t count;
		uint8_t global:1;
		uint8_t ignoreCase:1;
		uint8_t multiline:1;
	}
)

#endif
