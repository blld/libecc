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

static inline uint32_t positionIndex (const char *chars, uint32_t length, int32_t position, int enableReverse)
{
	uint32_t byte;
	if (position >= 0)
	{
		byte = 0;
		while (position--)
			while (byte < length && (chars[++byte] & 0xc0) == 0x80);
	}
	else if (enableReverse)
	{
		byte = length;
		position = -position;
		while (position--)
			while (byte && (chars[--byte] & 0xc0) == 0x80);
	}
	else
		byte = 0;
	
	return byte;
}

static struct Value toString (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 0);
	
	if (ecc->this.type != Value(stringType))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError((*ops)->text, "is not a string")));
	
	ecc->result = Value.chars(ecc->this.data.string->value);
	
	return Value(undefined);
}

static struct Value valueOf (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 0);
	
	if (ecc->this.type != Value(stringType))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError((*ops)->text, "is not a string")));
	
	ecc->result = Value.chars(ecc->this.data.string->value);
	
	return Value(undefined);
}

static struct Value charAt (const struct Op ** const ops, struct Ecc * const ecc)
{
	int32_t position, index, length;
	const char *chars;
	
	Op.assertParameterCount(ecc, 1);
	
	if (ecc->this.type != Value(stringType))
		ecc->this = Value.toString(ecc->this);
	
	chars = Value.stringChars(ecc->this);
	length = Value.stringLength(ecc->this);
	
	position = Value.toInteger(Op.argument(ecc, 0)).data.integer;
	if (position < 0)
		ecc->result = Value.text(&Text(empty));
	else
	{
		index = positionIndex(chars, length, position, 0);
		length = positionIndex(chars, length, position + 1, 0) - index;
		
		if (length <= 0)
			ecc->result = Value.text(&Text(empty));
		else
		{
			struct Chars *chars = Chars.createSized(length);
			memcpy(chars->chars, Value.stringChars(ecc->this) + index, length);
			ecc->result = Value.chars(chars);
		}
	}
	
	return Value(undefined);
}

static struct Value slice (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value from, to;
	int32_t start, end, length;
	const char *chars;
	
	Op.assertParameterCount(ecc, 2);
	
	if (ecc->this.type != Value(stringType))
		ecc->this = Value.toString(ecc->this);
	
	chars = Value.stringChars(ecc->this);
	length = Value.stringLength(ecc->this);
	
	from = Op.argument(ecc, 0);
	if (from.type == Value(undefinedType))
		start = 0;
	else
		start = positionIndex(chars, length, Value.toInteger(from).data.integer, 1);
	
	to = Op.argument(ecc, 1);
	if (to.type == Value(undefinedType))
		end = length;
	else
		end = positionIndex(chars, length, Value.toInteger(to).data.integer, 1);
	
	length = end - start;
	
	if (length <= 0)
		ecc->result = Value.text(&Text(empty));
	else
	{
		struct Chars *chars = Chars.createSized(length);
		memcpy(chars->chars, Value.stringChars(ecc->this) + start, length);
		ecc->result = Value.chars(chars);
	}
	
	return Value(undefined);
}

static struct Value substring (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value from, to;
	int32_t start, end, length;
	const char *chars;
	
	Op.assertParameterCount(ecc, 2);
	
	if (ecc->this.type != Value(stringType))
		ecc->this = Value.toString(ecc->this);
	
	chars = Value.stringChars(ecc->this);
	length = Value.stringLength(ecc->this);
	
	from = Op.argument(ecc, 0);
	if (from.type == Value(undefinedType))
		start = 0;
	else
		start = positionIndex(chars, length, Value.toInteger(from).data.integer, 0);
	
	to = Op.argument(ecc, 1);
	if (to.type == Value(undefinedType))
		end = length;
	else
		end = positionIndex(chars, length, Value.toInteger(to).data.integer, 0);
	
	if (start > end)
	{
		int32_t temp = start;
		start = end;
		end = temp;
	}
	
	length = end - start;
	
	if (length <= 0)
		ecc->result = Value.text(&Text(empty));
	else
	{
		struct Chars *chars = Chars.createSized(length);
		memcpy(chars->chars, Value.stringChars(ecc->this) + start, length);
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

// MARK: - Static Members

// MARK: - Methods

void setup ()
{
	const enum Object(Flags) flags = Object(writable) | Object(configurable);
	
	String(prototype) = Object.create(Object(prototype));
	Function.addToObject(String(prototype), "toString", toString, 0, flags);
	Function.addToObject(String(prototype), "valueOf", valueOf, 0, flags);
	Function.addToObject(String(prototype), "charAt", charAt, 1, flags);
	Function.addToObject(String(prototype), "slice", slice, 2, flags);
	Function.addToObject(String(prototype), "substring", substring, 2, flags);
	
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
	Object.initialize(&self->object, String(prototype));
	
	self->object.type = &Text(stringType);
	self->value = chars;
	
	return self;
}
