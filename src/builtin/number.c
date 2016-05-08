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

static struct Value toExponential (struct Context * const context)
{
	struct Value value;
	double binary;
	
	Context.assertParameterCount(context, 1);
	Context.assertThisMask(context, Value(numberMask));
	
	binary = Value.toBinary(context->this).data.binary;
	value = Context.argument(context, 0);
	if (value.type != Value(undefinedType))
	{
		int32_t precision = Value.toInteger(value).data.integer;
		if (precision < 0 || precision > 20)
			Context.throwError(context, Error.rangeError(Context.textSeek(context), "precision %d out of range", precision));
		
		return Value.chars(Chars.normalizeBinary(Chars.create("%.*e", precision, binary)));
	}
	else
		return Value.chars(Chars.normalizeBinary(Chars.create("%e", binary)));
}

static struct Value toFixed (struct Context * const context)
{
	struct Value value;
	double binary;
	
	Context.assertParameterCount(context, 1);
	Context.assertThisMask(context, Value(numberMask));
	
	binary = Value.toBinary(context->this).data.binary;
	value = Context.argument(context, 0);
	if (value.type != Value(undefinedType))
	{
		int32_t precision = Value.toInteger(value).data.integer;
		if (precision < 0 || precision > 20)
			Context.throwError(context, Error.rangeError(Context.textSeek(context), "precision %d out of range", precision));
		
		return Value.chars(Chars.normalizeBinary(Chars.create("%.*f", precision, binary)));
	}
	else
		return Value.chars(Chars.normalizeBinary(Chars.create("%f", binary)));
}

static struct Value toPrecision (struct Context * const context)
{
	struct Value value;
	double binary;
	
	Context.assertParameterCount(context, 1);
	Context.assertThisMask(context, Value(numberMask));
	
	binary = Value.toBinary(context->this).data.binary;
	value = Context.argument(context, 0);
	if (value.type != Value(undefinedType))
	{
		int32_t precision = Value.toInteger(value).data.integer;
		if (precision < 0 || precision > 100)
			Context.throwError(context, Error.rangeError(Context.textSeek(context), "precision %d out of range", precision));
		
		return Value.chars(Chars.normalizeBinary(Chars.create("%.*g", precision, binary)));
	}
	else
		return Value.binaryToString(binary, 10);
}

static struct Value toString (struct Context * const context)
{
	struct Value value;
	int32_t radix = 10;
	double binary;
	
	Context.assertParameterCount(context, 1);
	Context.assertThisMask(context, Value(numberMask));
	
	binary = Value.toBinary(context->this).data.binary;
	value = Context.argument(context, 0);
	if (value.type != Value(undefinedType))
	{
		radix = Value.toInteger(value).data.integer;
		if (radix < 2 || radix > 36)
			Context.throwError(context, Error.rangeError(Context.textSeek(context), "radix must be an integer at least 2 and no greater than 36"));
		
		if (radix != 10 && (binary < LONG_MIN || binary > LONG_MAX))
			Env.printWarning("%g.toString(%d) out of bounds; only long int are supported by radices other than 10", binary, radix);
	}
	
	return Value.binaryToString(binary, radix);
}

static struct Value valueOf (struct Context * const context)
{
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(numberType));
	
	return Value.binary(context->this.data.number->value);
}

static struct Value numberConstructor (struct Context * const context)
{
	struct Value value;
	
	Context.assertParameterCount(context, 1);
	
	value = Context.argument(context, 0);
	if (value.type == Value(undefinedType))
		value = Value.binary(0);
	else
		value = Value.toBinary(value);
	
	if (context->construct)
		return Value.number(Number.create(value.data.binary));
	else
		return value;
}

// MARK: - Static Members

// MARK: - Methods

void setup ()
{
	const enum Value(Flags) flags = Value(hidden);
	
	Function.setupBuiltinObject(&Number(constructor), numberConstructor, 1, &Number(prototype), Value.number(create(0)), &Number(type));
	
	Function.addToObject(Number(prototype), "toString", toString, 1, flags);
	Function.addToObject(Number(prototype), "valueOf", valueOf, 0, flags);
	Function.addToObject(Number(prototype), "toExponential", toExponential, 1, flags);
	Function.addToObject(Number(prototype), "toFixed", toFixed, 1, flags);
	Function.addToObject(Number(prototype), "toPrecision", toPrecision, 1, flags);
	
	Function.addMember(Number(constructor), "MAX_VALUE", Value.binary(DBL_MAX), flags);
	Function.addMember(Number(constructor), "MIN_VALUE", Value.binary(DBL_MIN), flags);
	Function.addMember(Number(constructor), "NaN", Value.binary(NAN), flags);
	Function.addMember(Number(constructor), "NEGATIVE_INFINITY", Value.binary(-INFINITY), flags);
	Function.addMember(Number(constructor), "POSITIVE_INFINITY", Value.binary(INFINITY), flags);
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
