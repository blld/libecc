//
//  string.c
//  libecc
//
//  Created by Bouilland Aurélien on 04/07/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#include "string.h"

// MARK: - Private

struct Object * String(prototype) = NULL;
struct Function * String(constructor) = NULL;

static struct Value fromCharCode (const struct Op ** const ops, struct Ecc * const ecc)
{
	int32_t c, index, count, length = 0;
	struct Chars *chars;
	char *b;
	
	Op.assertVariableParameter(ecc);
	
	for (index = 0, count = Op.variableArgumentCount(ecc); index < count; ++index)
	{
		c = Value.toInteger(Op.variableArgument(ecc, index)).data.integer;
		if (c < 0x80) length += 1;
		else if (c < 0x800) length += 2;
		else if (c <= 0xffff) length += 3;
		else length += 1;
	}
	
	chars = Chars.createSized(length);
	b = chars->chars;
	
	for (index = 0, count = Op.variableArgumentCount(ecc); index < count; ++index)
	{
		c = Value.toInteger(Op.variableArgument(ecc, index)).data.integer;
		if (c < 0x80) *b++ = c;
		else if (c < 0x800) *b++ = 192 + c / 64, *b++ = 128 + c % 64;
		else if (c <= 0xffff) *b++ = 224 + c / 4096, *b++ = 128 + c / 64 % 64, *b++ = 128 + c % 64;
		else *b++ = '\0';
	}
	
	while (chars->length && !chars->chars[chars->length - 1])
		--chars->length;
	
	ecc->result = Value.chars(chars);
	
	return Value.undefined();
}

static struct Value constructorFunction (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value;
	
	Op.assertParameterCount(ecc, 1);
	
	value = Value.toString(Op.argument(ecc, 0), 0);
	if (ecc->construct)
		ecc->result = Value.string(String.create(Chars.create(Value.stringChars(value), Value.stringLength(value))));
	else
		ecc->result = value;
	
	return Value.undefined();
}

// MARK: - Static Members

// MARK: - Methods

void setup ()
{
	const enum Object(Flags) flags = Object(writable) | Object(configurable);
	
	String(prototype) = Object.create(Object(prototype));
	
	String(constructor) = Function.createWithNative(NULL, constructorFunction, 1);
	Function.addToObject(&String(constructor)->object, "fromCharCode", fromCharCode, -1, flags);
	
	Object.add(String(prototype), Key(constructor), Value.function(String(constructor)), 0);
	Object.add(&String(constructor)->object, Key(prototype), Value.object(String(prototype)), 0);
}

void teardown (void)
{
	String(prototype) = NULL;
	String(constructor) = NULL;
}

struct String * create (struct Chars *chars)
{
	struct String *self = malloc(sizeof(*self));
	*self = String.identity;
	Pool.addObject(&self->object);
	Object.initialize(&self->object, NULL);
	
	self->object.type = &Text(stringType);
	self->value = chars;
	
	return self;
}
