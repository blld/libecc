//
//  identifier.h
//  libecc
//
//  Created by Bouilland Aurélien on 20/07/2014.
//  Copyright (c) 2014 Libeccio. All rights reserved.
//

#ifndef monade_identifier_h
#define monade_identifier_h

#include "namespace_io_libecc.h"

#include "text.h"


#define io_libecc_Identifier(X) io_libecc_identifier_ ## X

extern struct Identifier Identifier(none);
extern struct Identifier Identifier(prototype);
extern struct Identifier Identifier(constructor);
extern struct Identifier Identifier(length);
extern struct Identifier Identifier(arguments);
extern struct Identifier Identifier(name);
extern struct Identifier Identifier(message);
extern struct Identifier Identifier(toString);
extern struct Identifier Identifier(valueOf);
extern struct Identifier Identifier(eval);
extern struct Identifier Identifier(value);
extern struct Identifier Identifier(writable);
extern struct Identifier Identifier(enumerable);
extern struct Identifier Identifier(configurable);
extern struct Identifier Identifier(get);
extern struct Identifier Identifier(set);


#include "interface.h"

Interface(Identifier,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(struct Identifier, makeWithCString ,(const char *cString))
	(struct Identifier, makeWithText ,(const struct Text text, int copyOnCreate))
	
	(int, isEqual, (struct Identifier, struct Identifier))
	(struct Text *, textOf, (struct Identifier))
	
	(void, dumpTo, (struct Identifier, FILE *))
	,
	{
		union {
			uint8_t depth[4];
			uint32_t integer;
		} data;
	}
)

#endif
