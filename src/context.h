//
//  context.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_context_h
#ifdef Implementation
#undef Implementation
#include __FILE__
#include "implementation.h"
#else
#include "interface.h"
#define io_libecc_context_h

	#include "value.h"
	
	enum Context(Index) {
		Context(savedIndex) = -1,
		Context(noIndex) = 0,
		Context(callIndex) = 1,
		Context(funcIndex) = 2,
		Context(thisIndex) = 3,
	};

#endif


Interface(Context,
	
	(void, rangeError ,(struct Context * const, struct Chars *) noreturn)
	(void, referenceError ,(struct Context * const, struct Chars *) noreturn)
	(void, syntaxError ,(struct Context * const, struct Chars *) noreturn)
	(void, typeError ,(struct Context * const, struct Chars *) noreturn)
	(void, uriError ,(struct Context * const, struct Chars *) noreturn)
	
	(struct Value, callFunction ,(struct Context * const, struct Function *function, struct Value this, int argumentCount, ... ))
	
	(void , assertParameterCount ,(struct Context * const, int parameterCount))
	(int , argumentCount ,(struct Context * const))
	(struct Value, argument ,(struct Context * const, int argumentIndex))
	(void , replaceArgument ,(struct Context * const, int argumentIndex, struct Value value))
	
	(void , assertVariableParameter ,(struct Context * const))
	(int , variableArgumentCount ,(struct Context * const))
	(struct Value, variableArgument ,(struct Context * const, int argumentIndex))
	
	(struct Value, this ,(struct Context * const))
	(void, assertThisType ,(struct Context * const, enum Value(Type)))
	(void, assertThisMask ,(struct Context * const, enum Value(Mask)))
	
	(void, setText ,(struct Context * const, const struct Text *text))
	(void, setTextIndex ,(struct Context * const, enum Context(Index) index))
	(struct Text, textSeek ,(struct Context * const))
	,
	{
		const struct Op * ops;
		struct Value this;
		struct Object *refObject;
		int breaker;
		
		struct Object * environment;
		struct Context * parent;
		struct Ecc * ecc;
		
		enum Context(Index) textIndex;
		const struct Text *text;
		
		int construct;
		int argumentOffset;
		int depth;
	}
)

#endif
