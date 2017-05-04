//
//  pool.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_pool_h
#ifdef Implementation
#undef Implementation
#include __FILE__
#include "implementation.h"
#else
#include "interface.h"
#define io_libecc_pool_h

	#include "builtin/function.h"

#endif


Interface(Pool,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(void, addFunction ,(struct Function *function))
	(void, addObject ,(struct Object *object))
	(void, addChars ,(struct Chars *chars))
	(void, reindexChars ,(struct Chars *chars, struct Chars *was))
	
	(void, unmarkAll ,(void))
	(void, markValue ,(struct Value value))
	
	(void, collectUnmarked ,(void))
	(void, collectUnreferencedFromIndices ,(uint32_t indices[3]))
	
	(void, getCounts ,(uint32_t counts[3]))
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
