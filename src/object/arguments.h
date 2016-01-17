//
//  arguments.h
//  libecc
//
//  Created by Bouilland Aurélien on 17/01/2016.
//  Copyright (c) 2016 Libeccio. All rights reserved.
//

#ifndef io_libecc_arguments_h
#define io_libecc_arguments_h

#include "namespace_io_libecc.h"

#include "value.h"

#include "interface.h"


extern struct Object * Arguments(prototype);


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
