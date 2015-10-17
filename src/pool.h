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

Interface(Pool,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(void, addFunction ,(struct Function *function))
	(void, addObject ,(struct Object *chars))
	(void, addChars ,(struct Chars *chars))
	
	(void, markAll ,(void))
	(void, unmarkValue ,(struct Value value))
	(void, collectMarked ,(void))
	,
	{
		struct Function **functions;
		uint32_t functionsCount;
		uint32_t functionsCapacity;
		
		struct Object **objects;
		uint32_t objectsCount;
		uint32_t objectsCapacity;
		
		struct Chars **chars;
		uint32_t charsCount;
		uint32_t charsCapacity;
	}
)

#endif
