//
//  math.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#define Implementation
#include "math.h"

// MARK: - Private

const struct Object(Type) Math(type) = {
	.text = &Text(mathType),
};

// MARK: - Static Members

static struct Value mathAbs (struct Context * const context)
{
	struct Value value;
	
	Context.assertParameterCount(context, 1);
	
	value = Context.argument(context, 0);
	if (value.type != Value(binaryType))
		value = Value.toBinary(value);
	
	return Value.binary(fabs(value.data.binary));
}

static struct Value mathACos (struct Context * const context)
{
	struct Value value;
	
	Context.assertParameterCount(context, 1);
	
	value = Context.argument(context, 0);
	if (value.type != Value(binaryType))
		value = Value.toBinary(value);
	
	return Value.binary(acos(value.data.binary));
}

static struct Value mathASin (struct Context * const context)
{
	struct Value value;
	
	Context.assertParameterCount(context, 1);
	
	value = Context.argument(context, 0);
	if (value.type != Value(binaryType))
		value = Value.toBinary(value);
	
	return Value.binary(asin(value.data.binary));
}

static struct Value mathATan (struct Context * const context)
{
	struct Value value;
	
	Context.assertParameterCount(context, 1);
	
	value = Context.argument(context, 0);
	if (value.type != Value(binaryType))
		value = Value.toBinary(value);
	
	return Value.binary(atan(value.data.binary));
}

static struct Value mathATan2 (struct Context * const context)
{
	struct Value x, y;
	
	Context.assertParameterCount(context, 2);
	
	x = Context.argument(context, 0);
	if (x.type != Value(binaryType))
		x = Value.toBinary(x);
	
	y = Context.argument(context, 1);
	if (y.type != Value(binaryType))
		y = Value.toBinary(y);
	
	return Value.binary(atan2(x.data.binary, y.data.binary));
}

static struct Value mathCeil (struct Context * const context)
{
	struct Value value;
	
	Context.assertParameterCount(context, 1);
	
	value = Context.argument(context, 0);
	if (value.type != Value(binaryType))
		value = Value.toBinary(value);
	
	return Value.binary(ceil(value.data.binary));
}

static struct Value mathCos (struct Context * const context)
{
	struct Value value;
	
	Context.assertParameterCount(context, 1);
	
	value = Context.argument(context, 0);
	if (value.type != Value(binaryType))
		value = Value.toBinary(value);
	
	return Value.binary(cos(value.data.binary));
}

static struct Value mathExp (struct Context * const context)
{
	struct Value value;
	
	Context.assertParameterCount(context, 1);
	
	value = Context.argument(context, 0);
	if (value.type != Value(binaryType))
		value = Value.toBinary(value);
	
	return Value.binary(exp(value.data.binary));
}

static struct Value mathFloor (struct Context * const context)
{
	struct Value value;
	
	Context.assertParameterCount(context, 1);
	
	value = Context.argument(context, 0);
	if (value.type != Value(binaryType))
		value = Value.toBinary(value);
	
	return Value.binary(floor(value.data.binary));
}

static struct Value mathLog (struct Context * const context)
{
	struct Value value;
	
	Context.assertParameterCount(context, 1);
	
	value = Context.argument(context, 0);
	if (value.type != Value(binaryType))
		value = Value.toBinary(value);
	
	return Value.binary(log(value.data.binary));
}

static struct Value mathMax (struct Context * const context)
{
	double result = -INFINITY, value;
	int index, count;
	
	Context.assertVariableParameter(context);
	
	for (index = 0, count = Context.variableArgumentCount(context); index < count; ++index)
	{
		value = Value.toBinary(Context.variableArgument(context, index)).data.binary;
		if (result < value)
			result = value;
	}
	
	return Value.binary(result);
}

static struct Value mathMin (struct Context * const context)
{
	double result = INFINITY, value;
	int index, count;
	
	Context.assertVariableParameter(context);
	
	for (index = 0, count = Context.variableArgumentCount(context); index < count; ++index)
	{
		value = Value.toBinary(Context.variableArgument(context, index)).data.binary;
		if (result > value)
			result = value;
	}
	
	return Value.binary(result);
}

static struct Value mathPow (struct Context * const context)
{
	struct Value x, y;
	
	Context.assertParameterCount(context, 2);
	
	x = Context.argument(context, 0);
	if (x.type != Value(binaryType))
		x = Value.toBinary(x);
	
	y = Context.argument(context, 1);
	if (y.type != Value(binaryType))
		y = Value.toBinary(y);
	
	return Value.binary(pow(x.data.binary, y.data.binary));
}

static struct Value mathRandom (struct Context * const context)
{
	Context.assertParameterCount(context, 0);
	
	return Value.binary((double)rand() / (double)RAND_MAX);
}

static struct Value mathRound (struct Context * const context)
{
	struct Value value;
	
	Context.assertParameterCount(context, 1);
	
	value = Context.argument(context, 0);
	if (value.type != Value(binaryType))
		value = Value.toBinary(value);
	
	if (value.data.binary < 0)
		return Value.binary(1.0 - ceil(0.5 - value.data.binary));
	else
		return Value.binary(floor(0.5 + value.data.binary));
}

static struct Value mathSin (struct Context * const context)
{
	struct Value value;
	
	Context.assertParameterCount(context, 1);
	
	value = Context.argument(context, 0);
	if (value.type != Value(binaryType))
		value = Value.toBinary(value);
	
	return Value.binary(sin(value.data.binary));
}

static struct Value mathSqrt (struct Context * const context)
{
	struct Value value;
	
	Context.assertParameterCount(context, 1);
	
	value = Context.argument(context, 0);
	if (value.type != Value(binaryType))
		value = Value.toBinary(value);
	
	return Value.binary(sqrt(value.data.binary));
}

static struct Value mathTan (struct Context * const context)
{
	struct Value value;
	
	Context.assertParameterCount(context, 1);
	
	value = Context.argument(context, 0);
	if (value.type != Value(binaryType))
		value = Value.toBinary(value);
	
	return Value.binary(tan(value.data.binary));
}

// MARK: - Public

struct Object * Math(object) = NULL;

void setup ()
{
	const enum Value(Flags) flags = Value(hidden);
	
	Math(object) = Object.createTyped(&Math(type));
	
	Object.addMember(Math(object), Key.makeWithCString("E"), Value.binary(2.71828182845904523536), flags);
	Object.addMember(Math(object), Key.makeWithCString("LN10"), Value.binary(2.30258509299404568402), flags);
	Object.addMember(Math(object), Key.makeWithCString("LN2"), Value.binary(0.693147180559945309417), flags);
	Object.addMember(Math(object), Key.makeWithCString("LOG2E"), Value.binary(1.44269504088896340736), flags);
	Object.addMember(Math(object), Key.makeWithCString("LOG10E"), Value.binary(0.434294481903251827651), flags);
	Object.addMember(Math(object), Key.makeWithCString("PI"), Value.binary(3.14159265358979323846), flags);
	Object.addMember(Math(object), Key.makeWithCString("SQRT1_2"), Value.binary(0.707106781186547524401), flags);
	Object.addMember(Math(object), Key.makeWithCString("SQRT2"), Value.binary(1.41421356237309504880), flags);
	
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
