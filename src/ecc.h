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
#include "object/math.h"
#include "object/number.h"
#include "object/regexp.h"

#include "interface.h"

enum Ecc(EvalFlags) {
	Ecc(globalThis) = 1 << 0,
	Ecc(primitiveResult) = 1 << 1,
};


Interface(Ecc,
	
	(struct Ecc *, create ,(void))
	(void, destroy, (struct Ecc *))
	
	(void, addNative ,(struct Ecc *, const char *name, const Native(Function) native, int argumentCount, enum Value(Flags)))
	(void, addValue ,(struct Ecc *, const char *name, struct Value value, enum Value(Flags)))
	(int, evalInput, (struct Ecc *, struct Input *, enum Ecc(EvalFlags)))
	
	(jmp_buf *, pushEnv ,(struct Ecc *))
	(void, popEnv ,(struct Ecc *))
	(void, jmpEnv, (struct Ecc *, struct Value value) dead)
	
	(void, printTextInput, (struct Ecc *, struct Text text))
	
	(void, garbageCollect ,(struct Ecc *))
	,
	{
		struct Object *context;
		struct Value this;
		
		struct {
			struct Object *context;
			struct Value this;
			
			jmp_buf buf;
		} *envList;
		uint16_t envCount;
		uint16_t envCapacity;
		
		struct Value refObject;
		struct Value result;
		
		int construct;
		struct Function *global;
		struct Text text;
		
		struct Input **inputs;
		uint16_t inputCount;
	}
)

#endif
