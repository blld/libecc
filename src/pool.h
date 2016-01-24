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
	(void, addObject ,(struct Object *object))
	(void, addChars ,(struct Chars *chars))
	
	(void, unmarkAll ,(void))
	(void, markValue ,(struct Value value))
	(void, collectUnmarked ,(void))
	,
	{
		struct Function **functionList;
		uint32_t functionCount;
		uint32_t functionCapacity;
		
		struct Object **objectList;
		uint32_t objectCount;
		uint32_t objectCapacity;
		
		struct Chars **charsList;
		uint32_t charsCount;
		uint32_t charsCapacity;
	}
)

#endif
