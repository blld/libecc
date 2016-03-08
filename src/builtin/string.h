//
//  string.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_string_h
#define io_libecc_string_h

#include "namespace_io_libecc.h"

#include "object.h"

#include "interface.h"


extern struct Object * String(prototype);
extern struct Function * String(constructor);
extern const struct Object(Type) String(type);


Interface(String,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(struct String *, create ,(struct Chars *))
	,
	{
		struct Object object;
		struct Chars *value;
	}
)

#endif
