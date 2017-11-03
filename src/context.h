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
		Context(savedIndexAlt) = -2,
		Context(savedIndex) = -1,
		Context(noIndex) = 0,
		Context(callIndex) = 1,
		Context(funcIndex) = 2,
		Context(thisIndex) = 3,
	};
	
	enum Context(Offset) {
		Context(accessorOffset) = -1,
		Context(callOffset) = 1,
		Context(applyOffset) = 2,
	};
	
	enum Context(Special) {
		Context(countMask) = 0x7f,
		Context(asAccessor) = 1 << 8,
	};

#endif


Interface(Context,
	
	(void, rangeError ,(struct Context * const, struct Chars *) noreturn)
	(void, referenceError ,(struct Context * const, struct Chars *) noreturn)
	(void, syntaxError ,(struct Context * const, struct Chars *) noreturn)
	(void, typeError ,(struct Context * const, struct Chars *) noreturn)
	(void, uriError ,(struct Context * const, struct Chars *) noreturn)
	(void, throw ,(struct Context * const, struct Value) noreturn)
	
	(struct Value, callFunction ,(struct Context * const, struct Function *function, struct Value this, int argumentCount, ... ))
	
	(void, assertParameterCount ,(struct Context * const, int parameterCount))
	(int, argumentCount ,(struct Context * const))
	(struct Value, argument ,(struct Context * const, int argumentIndex))
	(void, replaceArgument ,(struct Context * const, int argumentIndex, struct Value value))
	
	(void, assertVariableParameter ,(struct Context * const))
	(int, variableArgumentCount ,(struct Context * const))
	(struct Value, variableArgument ,(struct Context * const, int argumentIndex))
	
	(struct Value, this ,(struct Context * const))
	(void, assertThisType ,(struct Context * const, enum Value(Type)))
	(void, assertThisMask ,(struct Context * const, enum Value(Mask)))
	(void, assertThisCoerciblePrimitive ,(struct Context * const))
	
	(void, setText ,(struct Context * const, const struct Text *text))
	(void, setTexts ,(struct Context * const, const struct Text *text, const struct Text *textAlt))
	(void, setTextIndex ,(struct Context * const, enum Context(Index) index))
	(void, setTextIndexArgument ,(struct Context * const, int argument))
	(struct Text, textSeek ,(struct Context * const))
	
	(void, rewindStatement ,(struct Context * const))
	(void, printBacktrace ,(struct Context * const context))
	
	(struct Object *, environmentRoot ,(struct Context * const context))
	,
	{
		const struct Op * ops;
		struct Object * refObject;
		struct Object * environment;
		struct Context * parent;
		struct Ecc * ecc;
		struct Value this;
		
		const struct Text * text;
		const struct Text * textAlt;
		const struct Text * textCall;
		enum Context(Index) textIndex;
		
		int16_t breaker;
		int16_t depth;
		int8_t construct:1;
		int8_t argumentOffset:3;
		int8_t strictMode:1;
		int8_t inEnvironmentObject:1;
	}
)

#endif
