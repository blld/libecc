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
	
	return Value(undefined);
}

static struct Value slice (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value fromValue, toValue;
	int32_t from, to, length;
	
	Op.assertParameterCount(ecc, 2);
	
	if (ecc->this.type != Value(stringType))
		ecc->this = Value.toString(ecc->this);
	
	length = Value.stringLength(ecc->this);
	
	fromValue = Op.argument(ecc, 0);
	if (fromValue.type == Value(undefinedType))
		from = 0;
	else
	{
		from = Value.toInteger(fromValue).data.integer;
		if (from < 0)
		{
			from = length + from;
			if (from < 0)
				from = 0;
		}
		else if (from > length)
			from = length;
	}
	
	toValue = Op.argument(ecc, 1);
	if (toValue.type == Value(undefinedType))
		to = length;
	else
	{
		to = Value.toInteger(toValue).data.integer;
		if (to < 0)
		{
			to = length + to;
			if (to < 0)
				to = 0;
		}
		else if (to > length)
			to = length;
	}
	
	length = to - from;
	
	if (length <= 0)
		ecc->result = Value.text(&Text(empty));
	else
	{
		struct Chars *chars = Chars.createSized(length);
		memcpy(chars->chars, Value.stringChars(ecc->this) + from, length);
		ecc->result = Value.chars(chars);
	}
	
	return Value(undefined);
}

static struct Value constructorFunction (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value;
	
	Op.assertParameterCount(ecc, 1);
	
	value = Value.toString(Op.argument(ecc, 0));
	if (ecc->construct)
		ecc->result = Value.string(String.create(Chars.create(Value.stringChars(value), Value.stringLength(value))));
	else
		ecc->result = value;
	
	return Value(undefined);
}

// MARK: - Static Members

// MARK: - Methods

void setup ()
{
	const enum Object(Flags) flags = Object(writable) | Object(configurable);
	
	String(prototype) = Object.create(Object(prototype));
	
	String(constructor) = Function.createWithNative(NULL, constructorFunction, 1);
	Function.addToObject(&String(constructor)->object, "fromCharCode", fromCharCode, -1, flags);
	Function.addToObject(String(prototype), "slice", slice, 2, flags);
	
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
	Object.initialize(&self->object, String(prototype));
	
	self->object.type = &Text(stringType);
	self->value = chars;
	
	return self;
}
