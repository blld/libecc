//
//  array.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_array_h
#ifdef Implementation
#undef Implementation
#include __FILE__
#include "../implementation.h"
#else
#include "../interface.h"
#define io_libecc_array_h

	#include "global.h"

	extern struct Object * Array(prototype);
	extern struct Function * Array(constructor);
	extern const struct Object(Type) Array(type);

#endif


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
