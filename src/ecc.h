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


#include "interface.h"

Interface(Ecc,
	
	(struct Ecc *, create ,(void))
	(void, destroy, (struct Ecc *))
	
	(void, directGlobalAccess ,(struct Ecc *, int allow))
	(void, addNative ,(struct Ecc *, const char *name, const Native native, int argumentCount, enum Object(Flags)))
	(void, addValue ,(struct Ecc *, const char *name, struct Value value, enum Object(Flags)))
	(int, evalInput, (struct Ecc *, struct Input *))
	
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
		
		struct Input **inputs;
		uint16_t inputCount;
	}
)

#endif
