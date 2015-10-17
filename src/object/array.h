//
//  array.h
//  libecc
//
//  Created by Bouilland Aurélien on 20/07/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#ifndef io_libecc_array_h
#define io_libecc_array_h

#include "namespace_io_libecc.h"

#include "value.h"


#define io_libecc_Array(X) io_libecc_array_ ## X

extern struct Object * Array(prototype);
extern struct Function * Array(constructor);


#include "interface.h"

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
