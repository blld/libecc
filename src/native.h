//
//  native.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_native_h
#define io_libecc_native_h

#include "namespace_io_libecc.h"

struct Op;
struct Ecc;
struct Value;
struct Native(Context);

typedef typeof(struct Value) (* Native(Function)) (struct Native(Context) * const context);

enum Native(Index) {
	Native(noIndex) = -4,
	Native(callIndex) = -3,
	Native(funcIndex) = -2,
	Native(thisIndex) = -1,
};

#include "value.h"

#include "interface.h"


struct Native(Context) {
	const struct Op * ops;
	struct Value this;
	struct Value refObject;
	int breaker;
	int construct;
	int argumentOffset;
	int depth;
	
	struct Ecc * ecc;
	struct Object * environment;
	struct Native(Context) * parent;
};


Interface(Native,
	
	(void , assertParameterCount ,(struct Native(Context) * const, int parameterCount))
	(int , argumentCount ,(struct Native(Context) * const))
	(struct Value, argument ,(struct Native(Context) * const, int argumentIndex))
	
	(void , assertVariableParameter ,(struct Native(Context) * const))
	(int , variableArgumentCount ,(struct Native(Context) * const))
	(struct Value, variableArgument ,(struct Native(Context) * const, int argumentIndex))
	
	(struct Text, textSeek ,(struct Native(Context) * const, enum Native(Index) argumentIndex))
	,
	{
		char dummy;
	}
)

#endif
