//
//  ecc.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_ecc_h
#define io_libecc_ecc_h

#include "namespace_io_libecc.h"

#include "value.h"
#include "parser.h"
#include "date.h"
#include "array.h"
#include "error.h"


#include "interface.h"

#define Module \
	io_libecc_Ecc

Interface(
	(Instance, create ,(void))
	(void, destroy, (Instance))
	
	(void, eval, (Instance, struct Input *))
	(void, throw, (Instance, struct Error *error) __attribute__((noreturn)))
	
	(void, printTextInput, (Instance, struct Text text))
	
//	(void *, allocate ,(Instance, size_t size))
//	(void *, reallocate ,(Instance, void *pointer, size_t size))
//	(void, deallocate ,(Instance, void *pointer))
	,
	{
		struct Object *context;
		struct Value this;
		struct Value refObject;
		struct Value result;
		
		struct Closure *global;
		
		struct Input **inputs;
		uint16_t inputCount;
		
		jmp_buf *envList;
		uint16_t envIndex;
		uint16_t envCapacity;
	}
)

#endif
