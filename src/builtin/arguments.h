//
//  arguments.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_arguments_h
#ifdef Implementation
#undef Implementation
#include __FILE__
#include "implementation.h"
#else
#include "interface.h"
#define io_libecc_arguments_h

	#include "global.h"

	extern struct Object * Arguments(prototype);
	extern const struct Object(Type) Arguments(type);

#endif


Interface(Arguments,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(struct Object *, createSized ,(uint32_t size))
	(struct Object *, createWithCList ,(int count, const char * list[]))
	
	(struct Object *, initializeSized ,(struct Object *, uint32_t size))
	,
	{
		struct Object object;
	}
)

#endif
