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
	
	(void, addFunction ,(Instance, const char *name, const Function function, int argumentCount, enum Object(Flags)))
	(void, addValue ,(Instance, const char *name, struct Value value, enum Object(Flags)))
	
	(void, evalInput, (Instance, struct Input *))
	
	(jmp_buf *, pushEnvList ,(Instance))
	(jmp_buf *, popEnvList ,(Instance))
	(void, throw, (Instance, struct Value value) __attribute__((noreturn)))
	
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
		
		int construct;
		struct Closure *global;
		
		struct Input **inputs;
		uint16_t inputCount;
		
		jmp_buf *envList;
		uint16_t envCount;
		uint16_t envCapacity;
	}
)

#endif
