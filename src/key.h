//
//  key.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef monade_key_h
#ifdef Implementation
#undef Implementation
#include __FILE__
#include "implementation.h"
#else
#include "interface.h"
#define monade_key_h

	#include "text.h"

	extern struct Key Key(none);
	extern struct Key Key(prototype);
	extern struct Key Key(constructor);
	extern struct Key Key(length);
	extern struct Key Key(arguments);
	extern struct Key Key(callee);
	extern struct Key Key(name);
	extern struct Key Key(message);
	extern struct Key Key(toString);
	extern struct Key Key(valueOf);
	extern struct Key Key(eval);
	extern struct Key Key(value);
	extern struct Key Key(writable);
	extern struct Key Key(enumerable);
	extern struct Key Key(configurable);
	extern struct Key Key(get);
	extern struct Key Key(set);
	extern struct Key Key(join);
	extern struct Key Key(toISOString);
	extern struct Key Key(input);
	extern struct Key Key(index);
	extern struct Key Key(lastIndex);
	extern struct Key Key(global);
	extern struct Key Key(ignoreCase);
	extern struct Key Key(multiline);
	extern struct Key Key(source);

	enum Key(Flags) {
		Key(copyOnCreate) = (1 << 0),
	};

#endif


Interface(Key,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(struct Key, makeWithCString ,(const char *cString))
	(struct Key, makeWithText ,(const struct Text text, enum Key(Flags) flags))
	
	(int, isEqual, (struct Key, struct Key))
	(const struct Text *, textOf, (struct Key))
	
	(void, dumpTo, (struct Key, FILE *))
	,
	{
		union {
			uint8_t depth[4];
			uint32_t integer;
		} data;
	}
)

#endif
