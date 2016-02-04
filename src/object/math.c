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

static struct Value mathAbs (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value;
	Native.assertParameterCount(context, 1);
	
	value = Native.argument(context, 0);
	if (value.type != Value(binaryType))
		value = Value.toBinary(value);
	
	ecc->result = Value.binary(abs(value.data.binary));
	return Value(undefined);
}

static struct Value mathACos (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value;
	Native.assertParameterCount(context, 1);
	
	value = Native.argument(context, 0);
	if (value.type != Value(binaryType))
		value = Value.toBinary(value);
	
	ecc->result = Value.binary(acos(value.data.binary));
	return Value(undefined);
}

static struct Value mathASin (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value;
	Native.assertParameterCount(context, 1);
	
	value = Native.argument(context, 0);
	if (value.type != Value(binaryType))
		value = Value.toBinary(value);
	
	ecc->result = Value.binary(asin(value.data.binary));
	return Value(undefined);
}

static struct Value mathATan (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value;
	Native.assertParameterCount(context, 1);
	
	value = Native.argument(context, 0);
	if (value.type != Value(binaryType))
		value = Value.toBinary(value);
	
	ecc->result = Value.binary(atan(value.data.binary));
	return Value(undefined);
}

static struct Value mathATan2 (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value x, y;
	Native.assertParameterCount(context, 2);
	
	x = Native.argument(context, 0);
	if (x.type != Value(binaryType))
		x = Value.toBinary(x);
	
	y = Native.argument(context, 1);
	if (y.type != Value(binaryType))
		y = Value.toBinary(y);
	
	ecc->result = Value.binary(atan2(x.data.binary, y.data.binary));
	return Value(undefined);
}

static struct Value mathCeil (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value;
	Native.assertParameterCount(context, 1);
	
	value = Native.argument(context, 0);
	if (value.type != Value(binaryType))
		value = Value.toBinary(value);
	
	ecc->result = Value.binary(ceil(value.data.binary));
	return Value(undefined);
}

static struct Value mathCos (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value;
	Native.assertParameterCount(context, 1);
	
	value = Native.argument(context, 0);
	if (value.type != Value(binaryType))
		value = Value.toBinary(value);
	
	ecc->result = Value.binary(cos(value.data.binary));
	return Value(undefined);
}

static struct Value mathExp (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value;
	Native.assertParameterCount(context, 1);
	
	value = Native.argument(context, 0);
	if (value.type != Value(binaryType))
		value = Value.toBinary(value);
	
	ecc->result = Value.binary(exp(value.data.binary));
	return Value(undefined);
}

static struct Value mathFloor (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value;
	Native.assertParameterCount(context, 1);
	
	value = Native.argument(context, 0);
	if (value.type != Value(binaryType))
		value = Value.toBinary(value);
	
	ecc->result = Value.binary(floor(value.data.binary));
	return Value(undefined);
}

static struct Value mathLog (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value;
	Native.assertParameterCount(context, 1);
	
	value = Native.argument(context, 0);
	if (value.type != Value(binaryType))
		value = Value.toBinary(value);
	
	ecc->result = Value.binary(log(value.data.binary));
	return Value(undefined);
}

static struct Value mathMax (struct Native(Context) * const context, struct Ecc * const ecc)
{
	double result = -INFINITY, value;
	int index, count;
	
	Native.assertVariableParameter(context);
	
	for (index = 0, count = Native.variableArgumentCount(context); index < count; ++index)
	{
		value = Value.toBinary(Native.variableArgument(context, index)).data.binary;
		if (result < value)
			result = value;
	}
	
	ecc->result = Value.binary(result);
	
	return Value(undefined);
}

static struct Value mathMin (struct Native(Context) * const context, struct Ecc * const ecc)
{
	double result = INFINITY, value;
	int index, count;
	
	Native.assertVariableParameter(context);
	
	for (index = 0, count = Native.variableArgumentCount(context); index < count; ++index)
	{
		value = Value.toBinary(Native.variableArgument(context, index)).data.binary;
		if (result > value)
			result = value;
	}
	
	ecc->result = Value.binary(result);
	
	return Value(undefined);
}

static struct Value mathPow (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value x, y;
	Native.assertParameterCount(context, 2);
	
	x = Native.argument(context, 0);
	if (x.type != Value(binaryType))
		x = Value.toBinary(x);
	
	y = Native.argument(context, 1);
	if (y.type != Value(binaryType))
		y = Value.toBinary(y);
	
	ecc->result = Value.binary(pow(x.data.binary, y.data.binary));
	return Value(undefined);
}

static struct Value mathRandom (struct Native(Context) * const context, struct Ecc * const ecc)
{
	Native.assertParameterCount(context, 0);
	ecc->result = Value.binary((double)rand() / (double)RAND_MAX);
	return Value(undefined);
}

static struct Value mathRound (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value;
	Native.assertParameterCount(context, 1);
	
	value = Native.argument(context, 0);
	if (value.type != Value(binaryType))
		value = Value.toBinary(value);
	
	ecc->result = Value.binary(floor(value.data.binary + 0.5));
	
	return Value(undefined);
}

static struct Value mathSin (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value;
	Native.assertParameterCount(context, 1);
	
	value = Native.argument(context, 0);
	if (value.type != Value(binaryType))
		value = Value.toBinary(value);
	
	ecc->result = Value.binary(sin(value.data.binary));
	return Value(undefined);
}

static struct Value mathSqrt (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value;
	Native.assertParameterCount(context, 1);
	
	value = Native.argument(context, 0);
	if (value.type != Value(binaryType))
		value = Value.toBinary(value);
	
	ecc->result = Value.binary(sqrt(value.data.binary));
	return Value(undefined);
}

static struct Value mathTan (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value;
	Native.assertParameterCount(context, 1);
	
	value = Native.argument(context, 0);
	if (value.type != Value(binaryType))
		value = Value.toBinary(value);
	
	ecc->result = Value.binary(tan(value.data.binary));
	return Value(undefined);
}

// MARK: - Public

struct Object * Math(object) = NULL;

void setup ()
{
	const enum Value(Flags) flags = Value(hidden);
	
	Math(object) = Object.createTyped(&Text(mathType));
	
	Object.add(Math(object), Key.makeWithCString("E"), Value.binary(2.71828182845904523536), flags);
	Object.add(Math(object), Key.makeWithCString("LN2"), Value.binary(0.693147180559945309417), flags);
	Object.add(Math(object), Key.makeWithCString("LN10"), Value.binary(2.30258509299404568402), flags);
	Object.add(Math(object), Key.makeWithCString("LOG2E"), Value.binary(1.44269504088896340736), flags);
	Object.add(Math(object), Key.makeWithCString("LOG10E"), Value.binary(0.434294481903251827651), flags);
	Object.add(Math(object), Key.makeWithCString("PI"), Value.binary(3.14159265358979323846), flags);
	Object.add(Math(object), Key.makeWithCString("SQRT1_2"), Value.binary(0.707106781186547524401), flags);
	Object.add(Math(object), Key.makeWithCString("SQRT2"), Value.binary(1.41421356237309504880), flags);
	
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
	
	srand((unsigned)time(NULL));
}

void teardown (void)
{
	Math(object) = NULL;
}
