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
struct Chars;
struct Object;
struct Function;

#include "value.h"


#include "interface.h"

#define Module \
	io_libecc_Pool

Interface(
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(Instance, shared ,(void))
	
	(void, addFunction ,(struct Function *function))
	(void, addObject ,(struct Object *chars))
	(void, addChars ,(struct Chars *chars))
	
	(void, markAll ,(void))
	(void, unmarkValue ,(struct Value value))
	(void, collectMarked ,(void))
	,
	{
		struct Function **functions;
		uint16_t functionsCount;
		uint16_t functionsCapacity;
		
		struct Object **objects;
		uint16_t objectsCount;
		uint16_t objectsCapacity;
		
		struct Chars **chars;
		uint16_t charsCount;
		uint16_t charsCapacity;
	}
)

#endif
