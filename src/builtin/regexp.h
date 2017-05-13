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
#include "implementation.h"
#else
#include "interface.h"
#define io_libecc_regexp_h

	#include "global.h"

	extern struct Object * RegExp(prototype);
	extern struct Function * RegExp(constructor);
	extern const struct Object(Type) RegExp(type);

#endif


Interface(RegExp,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(struct RegExp *, create ,(struct Chars *pattern, struct Error **))
	,
	{
		struct Object object;
		struct Chars *pattern;
		struct Chars *source;
		void *program;
		int count;
		int global;
		int ignoreCase;
		int multiline;
	}
)

#endif
