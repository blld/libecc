//
//  number.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#define Implementation
#include "number.h"

#include "../ecc.h"
#include "../pool.h"

// MARK: - Private

struct Object * Number(prototype) = NULL;
struct Function * Number(constructor) = NULL;

const struct Object(Type) Number(type) = {
	.text = &Text(numberType),
};

// MARK: - Static Members

static
struct Value toExponential (struct Context * const context)
{
	struct Chars(Append) chars;
	struct Value value;
	double binary, precision = 0;
	
	Context.assertParameterCount(context, 1);
	Context.assertThisMask(context, Value(numberMask));
	
	binary = Value.toBinary(context, context->this).data.binary;
	value = Context.argument(context, 0);
	if (value.type != Value(undefinedType))
	{
		precision = Value.toBinary(context, value).data.binary;
		if (precision <= -1 || precision >= 21)
			Context.rangeError(context, Chars.create("precision '%.0f' out of range", precision));
		
		if (isnan(precision))
			precision = 0;
	}
	
	if (isnan(binary))
		return Value.text(&Text(nan));
	else if (binary == INFINITY)
		return Value.text(&Text(infinity));
	else if (binary == -INFINITY)
		return Value.text(&Text(negativeInfinity));
	
	Chars.beginAppend(&chars);
	
	if (value.type != Value(undefinedType))
		Chars.append(&chars, "%.*e", (int32_t)precision, binary);
	else
		Chars.append(&chars, "%e", binary);
	
	Chars.normalizeBinary(&chars);
	return Chars.endAppend(&chars);
}

static
struct Value toFixed (struct Context * const context)
{
	struct Chars(Append) chars;
	struct Value value;
	double binary, precision = 0;
	
	Context.assertParameterCount(context, 1);
	Context.assertThisMask(context, Value(numberMask));
	
	binary = Value.toBinary(context, context->this).data.binary;
	value = Context.argument(context, 0);
	if (value.type != Value(undefinedType))
	{
		precision = Value.toBinary(context, value).data.binary;
		if (precision <= -1 || precision >= 21)
			Context.rangeError(context, Chars.create("precision '%.0f' out of range", precision));
		
		if (isnan(precision))
			precision = 0;
	}
	
	if (isnan(binary))
		return Value.text(&Text(nan));
	else if (binary == INFINITY)
		return Value.text(&Text(infinity));
	else if (binary == -INFINITY)
		return Value.text(&Text(negativeInfinity));
	
	Chars.beginAppend(&chars);
	
	if (binary <= -1e+21 || binary >= 1e+21)
		Chars.appendBinary(&chars, binary, 10);
	else
		Chars.append(&chars, "%.*f", (int32_t)precision, binary);
	
	return Chars.endAppend(&chars);
}

static
struct Value toPrecision (struct Context * const context)
{
	struct Chars(Append) chars;
	struct Value value;
	double binary, precision = 0;
	
	Context.assertParameterCount(context, 1);
	Context.assertThisMask(context, Value(numberMask));
	
	binary = Value.toBinary(context, context->this).data.binary;
	value = Context.argument(context, 0);
	if (value.type != Value(undefinedType))
	{
		precision = Value.toBinary(context, value).data.binary;
		if (precision <= -1 || precision >= 101)
			Context.rangeError(context, Chars.create("precision '%.0f' out of range", precision));
		
		if (isnan(precision))
			precision = 0;
	}
	
	if (isnan(binary))
		return Value.text(&Text(nan));
	else if (binary == INFINITY)
		return Value.text(&Text(infinity));
	else if (binary == -INFINITY)
		return Value.text(&Text(negativeInfinity));
	
	Chars.beginAppend(&chars);
	
	if (value.type != Value(undefinedType))
	{
		Chars.append(&chars, "%.*g", (int32_t)precision, binary);
		Chars.normalizeBinary(&chars);
	}
	else
		Chars.appendBinary(&chars, binary, 10);
	
	return Chars.endAppend(&chars);
}

static
struct Value toString (struct Context * const context)
{
	struct Value value;
	int32_t radix = 10;
	double binary;
	
	Context.assertParameterCount(context, 1);
	Context.assertThisMask(context, Value(numberMask));
	
	binary = Value.toBinary(context, context->this).data.binary;
	value = Context.argument(context, 0);
	if (value.type != Value(undefinedType))
	{
		radix = Value.toInteger(context, value).data.integer;
		if (radix < 2 || radix > 36)
			Context.rangeError(context, Chars.create("radix must be an integer at least 2 and no greater than 36"));
		
		if (radix != 10 && (binary < LONG_MIN || binary > LONG_MAX))
			Env.printWarning("%g.toString(%d) out of bounds; only long int are supported by radices other than 10", binary, radix);
	}
	
	return Value.binaryToString(binary, radix);
}

static
struct Value valueOf (struct Context * const context)
{
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(numberType));
	
	return Value.binary(context->this.data.number->value);
}

static
struct Value constructor (struct Context * const context)
{
	struct Value value;
	
	Context.assertParameterCount(context, 1);
	
	value = Context.argument(context, 0);
	if (value.type == Value(undefinedType))
		value = Value.binary(value.check == 1? NAN: 0);
	else
		value = Value.toBinary(context, value);
	
	if (context->construct)
		return Value.number(Number.create(value.data.binary));
	else
		return value;
}

// MARK: - Methods

void setup ()
{
	const enum Value(Flags) r = Value(readonly);
	const enum Value(Flags) h = Value(hidden);
	const enum Value(Flags) s = Value(sealed);
	
	Function.setupBuiltinObject(
		&Number(constructor), constructor, 1,
		&Number(prototype), Value.number(create(0)),
		&Number(type));
	
	Function.addMember(Number(constructor), "MAX_VALUE", Value.binary(DBL_MAX), r|h|s);
	Function.addMember(Number(constructor), "MIN_VALUE", Value.binary(DBL_MIN * DBL_EPSILON), r|h|s);
	Function.addMember(Number(constructor), "NaN", Value.binary(NAN), r|h|s);
	Function.addMember(Number(constructor), "NEGATIVE_INFINITY", Value.binary(-INFINITY), r|h|s);
	Function.addMember(Number(constructor), "POSITIVE_INFINITY", Value.binary(INFINITY), r|h|s);
	
	Function.addToObject(Number(prototype), "toString", toString, 1, h);
	Function.addToObject(Number(prototype), "toLocaleString", toString, 1, h);
	Function.addToObject(Number(prototype), "valueOf", valueOf, 0, h);
	Function.addToObject(Number(prototype), "toFixed", toFixed, 1, h);
	Function.addToObject(Number(prototype), "toExponential", toExponential, 1, h);
	Function.addToObject(Number(prototype), "toPrecision", toPrecision, 1, h);
}

void teardown (void)
{
	Number(prototype) = NULL;
	Number(constructor) = NULL;
}

struct Number * create (double binary)
{
	struct Number *self = malloc(sizeof(*self));
	*self = Number.identity;
	Pool.addObject(&self->object);
	Object.initialize(&self->object, Number(prototype));
	
	self->value = binary;
	
	return self;
}
