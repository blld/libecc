//
//  pool.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_pool_h
#define io_libecc_pool_h

#include "namespace_io_libecc.h"

#include "value.h"


#include "interface.h"

#define Module \
	io_libecc_Pool

Interface(
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(Instance, shared ,(void))
	
	(void, add ,(struct Value value))
	(void, delete ,(struct Value value))
	(void, collect ,(void))
	,
	{
		struct Value *values;
		uint16_t valueCount;
		uint16_t valueCapacity;
	}
)

#endif
