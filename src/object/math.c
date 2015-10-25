//
//  math.c
//  libecc
//
//  Created by Bouilland Aurélien on 17/10/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#include "math.h"

// MARK: - Private

// MARK: - Static Members

static struct Value mathAbs (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	ecc->result = Value.binary(abs(Value.toBinary(Op.argument(ecc, 0)).data.binary));
	return Value.undefined();
}

static struct Value mathACos (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	ecc->result = Value.binary(acos(Value.toBinary(Op.argument(ecc, 0)).data.binary));
	return Value.undefined();
}

static struct Value mathASin (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	ecc->result = Value.binary(asin(Value.toBinary(Op.argument(ecc, 0)).data.binary));
	return Value.undefined();
}

static struct Value mathATan (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	ecc->result = Value.binary(atan(Value.toBinary(Op.argument(ecc, 0)).data.binary));
	return Value.undefined();
}

static struct Value mathATan2 (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 2);
	ecc->result = Value.binary(atan2(Value.toBinary(Op.argument(ecc, 0)).data.binary, Value.toBinary(Op.argument(ecc, 1)).data.binary));
	return Value.undefined();
}

static struct Value mathCeil (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	ecc->result = Value.binary(ceil(Value.toBinary(Op.argument(ecc, 0)).data.binary));
	return Value.undefined();
}

static struct Value mathCos (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	ecc->result = Value.binary(cos(Value.toBinary(Op.argument(ecc, 0)).data.binary));
	return Value.undefined();
}

static struct Value mathExp (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	ecc->result = Value.binary(exp(Value.toBinary(Op.argument(ecc, 0)).data.binary));
	return Value.undefined();
}

static struct Value mathFloor (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	ecc->result = Value.binary(floor(Value.toBinary(Op.argument(ecc, 0)).data.binary));
	return Value.undefined();
}

static struct Value mathLog (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	ecc->result = Value.binary(log(Value.toBinary(Op.argument(ecc, 0)).data.binary));
	return Value.undefined();
}

static struct Value mathMax (const struct Op ** const ops, struct Ecc * const ecc)
{
	double result = -INFINITY, value;
	int index, count;
	
	Op.assertVariableParameter(ecc);
	
	for (index = 0, count = Op.variableArgumentCount(ecc); index < count; ++index)
	{
		value = Value.toBinary(Op.variableArgument(ecc, index)).data.binary;
		if (result < value)
			result = value;
	}
	
	ecc->result = Value.binary(result);
	
	return Value.undefined();
}

static struct Value mathMin (const struct Op ** const ops, struct Ecc * const ecc)
{
	double result = INFINITY, value;
	int index, count;
	
	Op.assertVariableParameter(ecc);
	
	for (index = 0, count = Op.variableArgumentCount(ecc); index < count; ++index)
	{
		value = Value.toBinary(Op.variableArgument(ecc, index)).data.binary;
		if (result > value)
			result = value;
	}
	
	ecc->result = Value.binary(result);
	
	return Value.undefined();
}

static struct Value mathPow (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 2);
	ecc->result = Value.binary(pow(Value.toBinary(Op.argument(ecc, 0)).data.binary, Value.toBinary(Op.argument(ecc, 1)).data.binary));
	return Value.undefined();
}

static struct Value mathRandom (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 0);
	ecc->result = Value.binary((double)rand() / (double)RAND_MAX);
	return Value.undefined();
}

static struct Value mathRound (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	ecc->result = Value.binary(round(Value.toBinary(Op.argument(ecc, 0)).data.binary));
	return Value.undefined();
}

static struct Value mathSin (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	ecc->result = Value.binary(sin(Value.toBinary(Op.argument(ecc, 0)).data.binary));
	return Value.undefined();
}

static struct Value mathSqrt (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	ecc->result = Value.binary(sqrt(Value.toBinary(Op.argument(ecc, 0)).data.binary));
	return Value.undefined();
}

static struct Value mathTan (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	ecc->result = Value.binary(tan(Value.toBinary(Op.argument(ecc, 0)).data.binary));
	return Value.undefined();
}

// MARK: - Public

struct Object * Math(object) = NULL;

void setup ()
{
	srand((unsigned)time(NULL));
	
	const enum Object(Flags) flags = Object(writable) | Object(configurable);
	
	Math(object) = Object.create(Object(prototype));
	
	Object.add(Math(object), Key.makeWithCString("E"), Value.binary(M_E), flags);
	Object.add(Math(object), Key.makeWithCString("LN2"), Value.binary(M_LN2), flags);
	Object.add(Math(object), Key.makeWithCString("LN10"), Value.binary(M_LN10), flags);
	Object.add(Math(object), Key.makeWithCString("LOG2E"), Value.binary(M_LOG2E), flags);
	Object.add(Math(object), Key.makeWithCString("LOG10E"), Value.binary(M_LOG10E), flags);
	Object.add(Math(object), Key.makeWithCString("PI"), Value.binary(M_PI), flags);
	Object.add(Math(object), Key.makeWithCString("SQRT1_2"), Value.binary(M_SQRT1_2), flags);
	Object.add(Math(object), Key.makeWithCString("SQRT2"), Value.binary(M_SQRT2), flags);
	
	Function.addToObject(Math(object), "abs", mathAbs, 1, flags);
	Function.addToObject(Math(object), "acos", mathACos, 1, flags);
	Function.addToObject(Math(object), "asin", mathASin, 1, flags);
	Function.addToObject(Math(object), "atan", mathATan, 1, flags);
	Function.addToObject(Math(object), "atan2", mathATan2, 2, flags);
	Function.addToObject(Math(object), "ceil", mathCeil, 1, flags);
	Function.addToObject(Math(object), "cos", mathCos, 1, flags);
	Function.addToObject(Math(object), "exp", mathExp, 1, flags);
	Function.addToObject(Math(object), "floor", mathFloor, 1, flags);
	Function.addToObject(Math(object), "log", mathLog, 1, flags);
	Function.addToObject(Math(object), "max", mathMax, -2, flags);
	Function.addToObject(Math(object), "min", mathMin, -2, flags);
	Function.addToObject(Math(object), "pow", mathPow, 2, flags);
	Function.addToObject(Math(object), "random", mathRandom, 0, flags);
	Function.addToObject(Math(object), "round", mathRound, 1, flags);
	Function.addToObject(Math(object), "sin", mathSin, 1, flags);
	Function.addToObject(Math(object), "sqrt", mathSqrt, 1, flags);
	Function.addToObject(Math(object), "tan", mathTan, 1, flags);
}

void teardown (void)
{
	Math(object) = NULL;
}
