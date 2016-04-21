//
//  ecc.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_ecc_h
#ifdef Implementation
#undef Implementation
#include __FILE__
#include "implementation.h"
#else
#include "interface.h"
#define io_libecc_ecc_h

	#include "builtin/global.h"
	#include "input.h"

	enum Ecc(EvalFlags) {
		Ecc(globalThis) = 1 << 0,
		Ecc(primitiveResult) = 1 << 1,
	};

#endif


Interface(Ecc,
	
	(struct Ecc *, create ,(void))
	(void, destroy ,(struct Ecc *))
	
	(void, addNative ,(struct Ecc *, const char *name, const Native(Function) native, int argumentCount, enum Value(Flags)))
	(void, addValue ,(struct Ecc *, const char *name, struct Value value, enum Value(Flags)))
	
	(int, evalInput ,(struct Ecc *, struct Input *, enum Ecc(EvalFlags)))
	(void, evalInputWithContext ,(struct Ecc *, struct Input *, struct Context *context))
	
	(jmp_buf *, pushEnv ,(struct Ecc *))
	(void, popEnv ,(struct Ecc *))
	(void, jmpEnv ,(struct Ecc *, struct Value value))
	(void, fatal ,(const char *format, ...))
	
	(struct Input *, findInput ,(struct Ecc *self, struct Text text))
	(void, printTextInput ,(struct Ecc *, struct Text text, int fullLine))
	
	(void, garbageCollect ,(struct Ecc *))
	,
	{
		jmp_buf *envList;
		uint16_t envCount;
		uint16_t envCapacity;
		
		struct Function *global;
		
		struct Value result;
		struct Text text;
		
		struct Input **inputs;
		uint16_t inputCount;
		
		uint16_t maximumCallDepth;
	}
)

#endif
