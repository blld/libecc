//
//  array.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_array_h
#define io_libecc_array_h

#include "namespace_io_libecc.h"

#include "object.h"

#include "interface.h"

extern struct Object * Array(prototype);
extern struct Function * Array(constructor);
extern const struct Object(Type) Array(type);


Interface(Array,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(struct Object *, create ,(void))
	(struct Object *, createSized ,(uint32_t size))
	,
	{
		struct Object object;
	}
)

#endif
