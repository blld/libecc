//
//  string.h
//  libecc
//
//  Created by Bouilland Aurélien on 04/07/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#ifndef io_libecc_string_h
#define io_libecc_string_h

#include "namespace_io_libecc.h"

#include "object.h"


#define io_libecc_String(X) io_libecc_string_ ## X

extern struct Object * String(prototype);
extern struct Function * String(constructor);


#include "interface.h"

Interface(String,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(struct String *, create ,(const char *format, ...))
	(void, destroy ,(struct String *))
	,
	{
		struct Object object;
		uint32_t length;
		char *chars;
	}
)

#endif
