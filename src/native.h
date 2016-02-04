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
typedef __typeof__(struct Value) (* Native(Function)) (struct Native(Context) * const context, struct Ecc * const ecc);

#include "value.h"

#include "interface.h"


enum Native(Index) {
	Native(noIndex) = -4,
	Native(callIndex) = -3,
	Native(funcIndex) = -2,
	Native(thisIndex) = -1,
};

struct Native(Context) {
	const struct Op * ops;
	struct Native(Context) * parent;
	int argumentOffset;
	struct Value this;
};


Interface(Native,
	
	(void , assertParameterCount ,(struct Ecc * const ecc, int parameterCount))
	(int , argumentCount ,(struct Ecc * const ecc))
	(struct Value, argument ,(struct Ecc * const ecc, int argumentIndex))
	
	(void , assertVariableParameter ,(struct Ecc * const ecc))
	(int , variableArgumentCount ,(struct Ecc * const ecc))
	(struct Value, variableArgument ,(struct Ecc * const ecc, int argumentIndex))
	
	(struct Text, textSeek ,(struct Native(Context) * const, struct Ecc * const ecc, enum Native(Index) argumentIndex))
	,
	{
		char dummy;
	}
)

#endif
