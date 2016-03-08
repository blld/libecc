//
//  arguments.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_arguments_h
#define io_libecc_arguments_h

#include "namespace_io_libecc.h"

#include "value.h"

#include "interface.h"


extern struct Object * Arguments(prototype);
extern const struct Object(Type) Arguments(type);


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
