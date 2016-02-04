//
//  number.c
//  libecc
//
//  Created by Bouilland Aurélien on 17/10/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#include "number.h"

// MARK: - Private

struct Object * Number(prototype) = NULL;
struct Function * Number(constructor) = NULL;

static struct Value toExponential (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value;
	
	Native.assertParameterCount(ecc, 1);
	
	if (Value.isNumber(context->this))
		context->this = Value.toBinary(context->this);
	else
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(Native.textSeek(context, ecc, Native(thisIndex)), "not a number")));
	
	value = Native.argument(ecc, 0);
	if (value.type != Value(undefinedType))
	{
		int32_t precision = Value.toInteger(value).data.integer;
		if (precision < 0 || precision > 20)
			Ecc.jmpEnv(ecc, Value.error(Error.rangeError(Native.textSeek(context, ecc, 0), "precision %d out of range", precision)));
		
		ecc->result = Value.chars(Chars.create("%.*e", precision, context->this.data.binary));
	}
	else
		ecc->result = Value.chars(Chars.create("%e", context->this.data.binary));
	
	return Value(undefined);
}

static struct Value toFixed (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value;
	
	Native.assertParameterCount(ecc, 1);
	
	if (Value.isNumber(context->this))
		context->this = Value.toBinary(context->this);
	else
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(Native.textSeek(context, ecc, Native(thisIndex)), "not a number")));
	
	value = Native.argument(ecc, 0);
	if (value.type != Value(undefinedType))
	{
		int32_t precision = Value.toInteger(value).data.integer;
		if (precision < 0 || precision > 20)
			Ecc.jmpEnv(ecc, Value.error(Error.rangeError(Native.textSeek(context, ecc, 0), "precision %d out of range", precision)));
		
		ecc->result = Value.chars(Chars.create("%.*f", precision, context->this.data.binary));
	}
	else
		ecc->result = Value.chars(Chars.create("%f", context->this.data.binary));
	
	return Value(undefined);
}

static struct Value toPrecision (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value;
	
	Native.assertParameterCount(ecc, 1);
	
	if (Value.isNumber(context->this))
		context->this = Value.toBinary(context->this);
	else
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(Native.textSeek(context, ecc, Native(thisIndex)), "not a number")));
	
	value = Native.argument(ecc, 0);
	if (value.type != Value(undefinedType))
	{
		int32_t precision = Value.toInteger(value).data.integer;
		if (precision < 0 || precision > 100)
			Ecc.jmpEnv(ecc, Value.error(Error.rangeError(Native.textSeek(context, ecc, 0), "precision %d out of range", precision)));
		
		ecc->result = Value.chars(Chars.create("%.*g", precision, context->this.data.binary));
	}
	else
		ecc->result = Value.binaryToString(context->this.data.binary, 10);
	
	return Value(undefined);
}

static struct Value toString (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value;
	int32_t radix = 10;
	
	Native.assertParameterCount(ecc, 1);
	
	if (Value.isNumber(context->this))
		context->this = Value.toBinary(context->this);
	else
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(Native.textSeek(context, ecc, Native(thisIndex)), "not a number")));
	
	value = Native.argument(ecc, 0);
	if (value.type != Value(undefinedType))
	{
		radix = Value.toInteger(value).data.integer;
		if (radix < 2 || radix > 36)
			Ecc.jmpEnv(ecc, Value.error(Error.rangeError(Native.textSeek(context, ecc, 0), "radix must be an integer at least 2 and no greater than 36")));
		
		if (radix != 10 && (context->this.data.binary < LONG_MIN || context->this.data.binary > LONG_MAX))
			Env.printWarning("%g.toString(%d) out of bounds; only long int are supported by radices other than 10", context->this.data.binary, radix);
	}
	
	ecc->result = Value.binaryToString(context->this.data.binary, radix);
	
	return Value(undefined);
}

static struct Value valueOf (struct Native(Context) * const context, struct Ecc * const ecc)
{
	Native.assertParameterCount(ecc, 0);
	
	if (Value.isNumber(context->this))
		context->this = Value.toBinary(context->this);
	else
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(Native.textSeek(context, ecc, Native(thisIndex)), "not a number")));
	
	ecc->result = context->this;
	return Value(undefined);
}

static struct Value numberConstructor (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value;
	
	Native.assertParameterCount(ecc, 1);
	
	value = Native.argument(ecc, 0);
	if (value.type == Value(undefinedType))
		value = Value.binary(0);
	else
		value = Value.toBinary(value);
	
	if (ecc->construct)
		ecc->result = Value.number(Number.create(value.data.binary));
	else
		ecc->result = value;
	
	return Value(undefined);
}

// MARK: - Static Members

// MARK: - Methods

void setup ()
{
	const enum Value(Flags) flags = Value(hidden);
	
	Function.setupBuiltinObject(&Number(constructor), numberConstructor, 1, &Number(prototype), Value.number(create(0)), &Text(numberType));
	
	Function.addToObject(Number(prototype), "toString", toString, 1, flags);
	Function.addToObject(Number(prototype), "valueOf", valueOf, 0, flags);
	Function.addToObject(Number(prototype), "toExponential", toExponential, 1, flags);
	Function.addToObject(Number(prototype), "toFixed", toFixed, 1, flags);
	Function.addToObject(Number(prototype), "toPrecision", toPrecision, 1, flags);
	
	Object.add(&Number(constructor)->object, Key.makeWithCString("MAX_VALUE"), Value.binary(DBL_MAX), flags);
	Object.add(&Number(constructor)->object, Key.makeWithCString("MIN_VALUE"), Value.binary(DBL_MIN), flags);
	Object.add(&Number(constructor)->object, Key.makeWithCString("NaN"), Value.binary(NAN), flags);
	Object.add(&Number(constructor)->object, Key.makeWithCString("NEGATIVE_INFINITY"), Value.binary(-INFINITY), flags);
	Object.add(&Number(constructor)->object, Key.makeWithCString("POSITIVE_INFINITY"), Value.binary(INFINITY), flags);
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
