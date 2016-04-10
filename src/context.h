//
//  context.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_context_h
#define io_libecc_context_h

#include "namespace_io_libecc.h"

enum Context(Index) {
	Context(savedIndex) = -1,
	Context(noIndex) = 0,
	Context(callIndex) = 1,
	Context(funcIndex) = 2,
	Context(thisIndex) = 3,
};

#include "value.h"

#include "interface.h"


Interface(Context,
	
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
		
		struct Ecc * ecc;
		struct Object * environment;
		struct Context * parent;
		
		int textIndex;
		const struct Text *text;
		
		int construct;
		int argumentOffset;
		int depth;
	}
)

#endif
