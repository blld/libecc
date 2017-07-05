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

	#define io_libecc_key_Keys \
		_( prototype )\
		_( constructor )\
		_( length )\
		_( arguments )\
		_( callee )\
		_( name )\
		_( message )\
		_( toString )\
		_( valueOf )\
		_( eval )\
		_( value )\
		_( writable )\
		_( enumerable )\
		_( configurable )\
		_( get )\
		_( set )\
		_( join )\
		_( toISOString )\
		_( input )\
		_( index )\
		_( lastIndex )\
		_( global )\
		_( ignoreCase )\
		_( multiline )\
		_( source )\
		\

	#define _(X) extern struct Key Key(X);
	io_libecc_key_Keys
	#undef _

	enum Key(Flags) {
		Key(copyOnCreate) = (1 << 0),
	};

#endif


Interface(Key,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(struct Key, makeWithCString ,(const char *cString))
	(struct Key, makeWithText ,(const struct Text text, enum Key(Flags) flags))
	(struct Key, search ,(const struct Text text))
	
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
