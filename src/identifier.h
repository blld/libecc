//
//  identifier.h
//  monade
//
//  Created by Bouilland Aurélien on 20/07/2014.
//  Copyright (c) 2014 Teppen Game. All rights reserved.
//

#ifndef monade_identifier_h
#define monade_identifier_h

#include "namespace_io_libecc.h"

#include "text.h"


#include "interface.h"

#define Module \
	io_libecc_Identifier

Interface(
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(Structure, none ,(void))
	(Structure, prototype ,(void))
	(Structure, constructor ,(void))
	(Structure, length ,(void))
	(Structure, arguments ,(void))
	(Structure, name ,(void))
	(Structure, message ,(void))
	(Structure, toString ,(void))
	(Structure, valueOf ,(void))
	(Structure, eval ,(void))
	(Structure, makeWithCString ,(const char *cString))
	(Structure, makeWithText ,(const struct Text text, int copyOnCreate))
	
	(int, isEqual, (Structure, Structure))
	(struct Text *, textOf, (Structure))
	
	(void, dumpTo, (Structure, FILE *))
	,
	{
		union {
			uint8_t depth[4];
			uint32_t integer;
		} data;
	}
)

#endif
