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

#include "interface.h"


enum Native(Index) {
	Native(noIndex) = -4,
	Native(callIndex) = -3,
	Native(funcIndex) = -2,
	Native(thisIndex) = -1,
};

struct Native(Frame) {
	const struct Op * ops;
	const struct Op ** parent;
	int argumentOffset;
};

typedef __typeof__(struct Value) (* Native(Function)) (const struct Op ** const ops, struct Ecc * const ecc);


Interface(Native,
	
	(void , assertParameterCount ,(struct Ecc * const ecc, int parameterCount))
	(int , argumentCount ,(struct Ecc * const ecc))
	(struct Value, argument ,(struct Ecc * const ecc, int argumentIndex))
	
	(void , assertVariableParameter ,(struct Ecc * const ecc))
	(int , variableArgumentCount ,(struct Ecc * const ecc))
	(struct Value, variableArgument ,(struct Ecc * const ecc, int argumentIndex))
	
	(struct Text, textSeek ,(const struct Op ** ops, struct Ecc * const ecc, enum Native(Index) argumentIndex))
	,
	{
		char dummy;
	}
)

#include "value.h"

#endif
