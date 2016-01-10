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

static struct Value toExponential (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value;
	
	Op.assertParameterCount(ecc, 1);
	
	if (ecc->this.type != Value(numberType))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError((*ops)->text, "not an number")));
	
	value = Op.argument(ecc, 0);
	if (value.type != Value(undefinedType))
	{
		int32_t precision = Value.toInteger(value).data.integer;
		if (precision < 0 || precision > 20)
			Ecc.jmpEnv(ecc, Value.error(Error.rangeError((*ops)->text, "precision %d out of range", precision)));
		
		ecc->result = Value.chars(Chars.create("%.*e", precision, ecc->this.data.number->value));
	}
	else
		ecc->result = Value.chars(Chars.create("%e", ecc->this.data.number->value));
	
	return Value(undefined);
}

static struct Value toFixed (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value;
	
	Op.assertParameterCount(ecc, 1);
	
	if (ecc->this.type != Value(numberType))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError((*ops)->text, "not an number")));
	
	value = Op.argument(ecc, 0);
	if (value.type != Value(undefinedType))
	{
		int32_t precision = Value.toInteger(value).data.integer;
		if (precision < 0 || precision > 20)
			Ecc.jmpEnv(ecc, Value.error(Error.rangeError((*ops)->text, "precision %d out of range", precision)));
		
		ecc->result = Value.chars(Chars.create("%.*f", precision, ecc->this.data.number->value));
	}
	else
		ecc->result = Value.chars(Chars.create("%f", ecc->this.data.number->value));
	
	return Value(undefined);
}

static struct Value toPrecision (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value;
	
	Op.assertParameterCount(ecc, 1);
	
	if (ecc->this.type != Value(numberType))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError((*ops)->text, "not an number")));
	
	value = Op.argument(ecc, 0);
	if (value.type != Value(undefinedType))
	{
		int32_t precision = Value.toInteger(value).data.integer;
		if (precision < 0 || precision > 100)
			Ecc.jmpEnv(ecc, Value.error(Error.rangeError((*ops)->text, "precision %d out of range", precision)));
		
		ecc->result = Value.chars(Chars.create("%.*g", precision, ecc->this.data.number->value));
	}
	else
		ecc->result = Value.binaryToString(ecc->this.data.number->value, 10);
	
	return Value(undefined);
}

static struct Value toString (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value;
	int32_t radix = 10;
	
	Op.assertParameterCount(ecc, 1);
	
	if (ecc->this.type != Value(numberType))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError((*ops)->text, "not an number")));
	
	value = Op.argument(ecc, 0);
	if (value.type != Value(undefinedType))
	{
		radix = Value.toInteger(value).data.integer;
		if (radix < 2 || radix > 36)
			Ecc.jmpEnv(ecc, Value.error(Error.rangeError((*ops)->text, "radix must be an integer at least 2 and no greater than 36")));
		
		if (radix != 10 && (ecc->this.data.number->value < LONG_MIN || ecc->this.data.number->value > LONG_MAX))
			Env.printWarning("%g.toString(%d) out of bounds; only long int are supported by radices other than 10", ecc->this.data.number->value, radix);
	}
	
	ecc->result = Value.binaryToString(ecc->this.data.number->value, radix);
	
	return Value(undefined);
}

static struct Value valueOf (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 0);
	if (ecc->this.type != Value(numberType))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError((*ops)->text, "not an number")));
	
	ecc->result = Value.binary(ecc->this.data.number->value);
	return Value(undefined);
}

static struct Value numberConstructor (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value;
	
	Op.assertParameterCount(ecc, 1);
	
	value = Value.toBinary(Op.argument(ecc, 0));
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
	const enum Value(Flags) flags = Value(writable) | Value(configurable);
	
	Number(prototype) = Object.createTyped(&Text(numberType));
	Function.addToObject(Number(prototype), "toString", toString, 1, flags);
	Function.addToObject(Number(prototype), "valueOf", valueOf, 0, flags);
	Function.addToObject(Number(prototype), "toExponential", toExponential, 1, flags);
	Function.addToObject(Number(prototype), "toFixed", toFixed, 1, flags);
	Function.addToObject(Number(prototype), "toPrecision", toPrecision, 1, flags);
	
	Number(constructor) = Function.createWithNative(numberConstructor, 1);
	Object.add(&Number(constructor)->object, Key.makeWithCString("MAX_VALUE"), Value.binary(DBL_MAX), flags);
	Object.add(&Number(constructor)->object, Key.makeWithCString("MIN_VALUE"), Value.binary(DBL_MIN), flags);
	Object.add(&Number(constructor)->object, Key.makeWithCString("NaN"), Value.binary(NAN), flags);
	Object.add(&Number(constructor)->object, Key.makeWithCString("NEGATIVE_INFINITY"), Value.binary(-INFINITY), flags);
	Object.add(&Number(constructor)->object, Key.makeWithCString("POSITIVE_INFINITY"), Value.binary(INFINITY), flags);
	Function.linkPrototype(Number(constructor), Number(prototype), 0);
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
