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

static inline int32_t positionIndex (const char *chars, int32_t length, int32_t position, int enableReverse)
{
	int32_t byte;
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

static inline int32_t indexPosition (const char *chars, int32_t length, int32_t index)
{
	int32_t byte = 0, position = 0;
	while (index--)
	{
		if (byte < length)
		{
			++position;
			while ((chars[++byte] & 0xc0) == 0x80);
		}
	}
	return position;
}

static struct Value toString (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 0);
	
	if (ecc->this.type != Value(stringType))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(Op.textSeek(ops, ecc, Op(textSeekThis)), "not a string")));
	
	ecc->result = Value.chars(ecc->this.data.string->value);
	
	return Value(undefined);
}

static struct Value valueOf (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 0);
	
	if (ecc->this.type != Value(stringType))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(Op.textSeek(ops, ecc, Op(textSeekThis)), "not a string")));
	
	ecc->result = Value.chars(ecc->this.data.string->value);
	
	return Value(undefined);
}

static struct Value charAt (const struct Op ** const ops, struct Ecc * const ecc)
{
	int32_t position, index, length;
	const char *chars;
	
	Op.assertParameterCount(ecc, 1);
	
	if (!Value.isString(ecc->this))
		ecc->this = Value.toString(ecc->this);
	
	chars = Value.stringChars(ecc->this);
	length = Value.stringLength(ecc->this);
	
	position = Value.toInteger(Op.argument(ecc, 0)).data.integer;
	index = positionIndex(chars, length, position, 0);
	length = positionIndex(chars, length, position + 1, 0) - index;
	
	if (length <= 0)
		ecc->result = Value.text(&Text(empty));
	else
	{
		struct Chars *result = Chars.createSized(length);
		memcpy(result->chars, chars + index, length);
		ecc->result = Value.chars(result);
	}
	
	return Value(undefined);
}

static struct Value charCodeAt (const struct Op ** const ops, struct Ecc * const ecc)
{
	int32_t position, index, length;
	const char *chars;
	
	Op.assertParameterCount(ecc, 1);
	
	if (!Value.isString(ecc->this))
		ecc->this = Value.toString(ecc->this);
	
	chars = Value.stringChars(ecc->this);
	length = Value.stringLength(ecc->this);
	
	position = Value.toInteger(Op.argument(ecc, 0)).data.integer;
	index = positionIndex(chars, length, position, 0);
	length = positionIndex(chars, length, position + 1, 0) - index;
	
	if (length <= 0)
		ecc->result = Value.binary(NAN);
	else
	{
		struct Text text = { chars + index, length };
		ecc->result = Value.binary(Text.nextCodepoint(&text));
	}
	
	return Value(undefined);
}

static struct Value concat (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Chars *result;
	int32_t length = 0, offset = 0, index, count;
	
	Op.assertVariableParameter(ecc);
	
	count = Op.variableArgumentCount(ecc);
	
	length += Value.toBufferLength(ecc->this);
	for (index = 0; index < count; ++index)
		length += Value.toBufferLength(Op.variableArgument(ecc, index));
	
	result = Chars.createSized(length);
	
	offset += Value.toBuffer(ecc->this, result->chars + offset, length - offset + 1);
	for (index = 0; index < count; ++index)
		offset += Value.toBuffer(Op.variableArgument(ecc, index), result->chars + offset, length - offset + 1);
	
	ecc->result = Value.chars(result);
	
	return Value(undefined);
}

static struct Value indexOf (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value search;
	int32_t position, index, offset, length, searchLength, argumentCount;
	const char *chars, *searchChars;
	
	Op.assertVariableParameter(ecc);
	
	argumentCount = Op.variableArgumentCount(ecc);
	
	ecc->this = Value.toString(ecc->this);
	chars = Value.stringChars(ecc->this);
	length = Value.stringLength(ecc->this);
	
	search = argumentCount >= 1? Value.toString(Op.variableArgument(ecc, 0)): Value.text(&Text(undefined));
	searchChars = Value.stringChars(search);
	searchLength = Value.stringLength(search);
	
	length -= searchLength;
	position = argumentCount >= 2? Value.toInteger(Op.variableArgument(ecc, 1)).data.integer: 0;
	index = positionIndex(chars, length, position, 0);
	
	for (; index <= length; ++index)
	{
		if ((chars[index] & 0xc0) == 0x80)
			continue;
		
		if (chars[index] == searchChars[0])
		{
			offset = 0;
			do
			{
				if (offset >= searchLength - 1)
				{
					ecc->result = Value.integer(position);
					return Value(undefined);
				}
				++offset;
			}
			while (chars[index + offset] == searchChars[offset]);
		}
		
		++position;
	}
	
	ecc->result = Value.integer(-1);
	return Value(undefined);
}

static struct Value lastIndexOf (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value search;
	int32_t position, index, offset, length, searchLength, argumentCount;
	const char *chars, *searchChars;
	
	Op.assertVariableParameter(ecc);
	
	argumentCount = Op.variableArgumentCount(ecc);
	
	ecc->this = Value.toString(ecc->this);
	chars = Value.stringChars(ecc->this);
	length = Value.stringLength(ecc->this);
	
	search = argumentCount >= 1? Value.toString(Op.variableArgument(ecc, 0)): Value.text(&Text(undefined));
	searchChars = Value.stringChars(search);
	searchLength = Value.stringLength(search) - 1;
	
	if (argumentCount < 2 || Op.variableArgument(ecc, 1).type == Value(undefinedType))
		position = indexPosition(chars, length, length);
	else
		position = Value.toInteger(Op.variableArgument(ecc, 1)).data.integer;
	
	position -= indexPosition(searchChars, searchLength, searchLength);
	index = positionIndex(chars, length, position, 0);
	
	for (; index >= 0; --index)
	{
		if ((chars[index] & 0xc0) == 0x80)
			continue;
		
		if (chars[index] == searchChars[0])
		{
			offset = 0;
			do
			{
				if (offset >= searchLength - 1)
				{
					ecc->result = Value.integer(position);
					return Value(undefined);
				}
				++offset;
			}
			while (chars[index + offset] == searchChars[offset]);
		}
		
		--position;
	}
	
	ecc->result = Value.integer(-1);
	return Value(undefined);
}

static struct Value slice (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value from, to;
	int32_t start, end, length;
	const char *chars;
	
	Op.assertParameterCount(ecc, 2);
	
	if (!Value.isString(ecc->this))
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
		struct Chars *result = Chars.createSized(length);
		memcpy(result->chars, chars + start, length);
		ecc->result = Value.chars(result);
	}
	
	return Value(undefined);
}

static struct Value substring (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value from, to;
	int32_t start, end, length;
	const char *chars;
	
	Op.assertParameterCount(ecc, 2);
	
	if (!Value.isString(ecc->this))
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
		struct Chars *result = Chars.createSized(length);
		memcpy(result->chars, chars + start, length);
		ecc->result = Value.chars(result);
	}
	
	return Value(undefined);
}

static struct Value stringConstructor (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value;
	
	Op.assertParameterCount(ecc, 1);
	
	value = Value.toString(Op.argument(ecc, 0));
	if (ecc->construct)
		ecc->result = Value.string(String.create(Chars.createWithBuffer(Value.stringLength(value), Value.stringChars(value))));
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
	const enum Value(Flags) flags = Value(hidden);
	
	Function.setupBuiltinObject(&String(constructor), stringConstructor, 1, &String(prototype), Value.string(create(Chars.createSized(0))), &Text(stringType));
	
	Function.addToObject(String(prototype), "toString", toString, 0, flags);
	Function.addToObject(String(prototype), "valueOf", valueOf, 0, flags);
	Function.addToObject(String(prototype), "charAt", charAt, 1, flags);
	Function.addToObject(String(prototype), "charCodeAt", charCodeAt, 1, flags);
	Function.addToObject(String(prototype), "concat", concat, -1, flags);
	Function.addToObject(String(prototype), "indexOf", indexOf, -1, flags);
	Function.addToObject(String(prototype), "lastIndexOf", lastIndexOf, -1, flags);
	Function.addToObject(String(prototype), "slice", slice, 2, flags);
	Function.addToObject(String(prototype), "substring", substring, 2, flags);
	
	Function.addToObject(&String(constructor)->object, "fromCharCode", fromCharCode, -1, flags);
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
	Object.add(&self->object, Key(length), Value.integer(indexPosition(chars->chars, chars->length, chars->length)), Value(readonly));
	
	self->value = chars;
	
	return self;
}
