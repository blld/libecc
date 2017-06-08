//
//  string.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#define Implementation
#include "string.h"

#include "../pool.h"

// MARK: - Private

struct Object * String(prototype) = NULL;
struct Function * String(constructor) = NULL;

static
void mark (struct Object *object)
{
	struct String *self = (struct String *)object;
	
	Pool.markValue(Value.chars(self->value));
}

static
void finalize (struct Object *object)
{
	struct String *self = (struct String *)object;
	
	--self->value->referenceCount;
}

const struct Object(Type) String(type) = {
	.text = &Text(stringType),	
	.mark = mark,
	.finalize = finalize,
};


static inline
struct Text positionText (const char *chars, uint16_t length, int32_t position, int enableReverse)
{
	struct Text text = Text.make(chars, length), prev = text;
	uint32_t codepoint;
	
	if (position >= 0)
	{
		while (position-- > 0)
		{
			prev = text;
			codepoint = Text.nextCodepoint(&text);
			
			if (codepoint > 0xffff && !position--)
			{
				/* simulate 16-bit surrogate */
				text = prev;
				text.flags = Text(breakFlag);
			}
		}
	}
	else if (enableReverse)
	{
		text.bytes += text.length;
		
		while (position++ < 0)
		{
			prev = text;
			codepoint = Text.prevCodepoint(&text);
			
			if (codepoint > 0xffff && !position--)
			{
				/* simulate 16-bit surrogate */
				text = prev;
				text.flags = Text(breakFlag);
			}
		}
	}
	else
		text.length = 0;
	
	return text;
}

static inline
uint16_t unitPosition (const char *chars, uint16_t max, uint16_t unit)
{
	struct Text text = Text.make(chars, max);
	uint16_t position = 0;
	uint32_t codepoint;
	
	while (unit--)
	{
		if (text.length)
		{
			++position;
			codepoint = Text.nextCodepoint(&text);
			
			if (codepoint > 0xffff) /* simulate 16-bit surrogate */
				++position;
		}
	}
	
	return position;
}

static struct Value toString (struct Context * const context)
{
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(stringType));
	
	return Value.chars(context->this.data.string->value);
}

static struct Value valueOf (struct Context * const context)
{
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(stringType));
	
	return Value.chars(context->this.data.string->value);
}

static struct Value charAt (struct Context * const context)
{
	uint16_t position, length;
	const char *chars;
	struct Text text;
	
	Context.assertParameterCount(context, 1);
	Context.assertThisType(context, Value(stringType));
	
	chars = Value.stringBytes(context->this);
	length = Value.stringLength(context->this);
	
	position = Value.toInteger(context, Context.argument(context, 0)).data.integer;
	
	text = positionText(chars, length, position, 0);
	if (!text.length)
		return Value.text(&Text(empty));
	else
	{
		struct Chars *result;
		uint8_t units;
		uint32_t cp = Text.codepoint(text, &units);
		
		if (cp < 0x010000)
		{
			result = Chars.createSized(units);
			memcpy(result->bytes, text.bytes, units);
		}
		else
		{
			/* simulate 16-bit surrogate */
			
			cp -= 0x010000;
			if (text.bytes == positionText(chars, length, position + 1, 0).bytes)
				cp = ((cp >> 10) & 0x3ff) + 0xd800;
			else
				cp = ((cp >>  0) & 0x3ff) + 0xdc00;
			
			result = Chars.createSized(3);
			*(result->bytes + 0) = 224 + cp / 4096;
			*(result->bytes + 1) = 128 + cp / 64 % 64;
			*(result->bytes + 2) = 128 + cp % 64;
		}
		
		return Value.chars(result);
	}
}

static struct Value charCodeAt (struct Context * const context)
{
	int32_t position, length;
	const char *chars;
	struct Text text;
	
	Context.assertParameterCount(context, 1);
	Context.assertThisType(context, Value(stringType));
	
	chars = Value.stringBytes(context->this);
	length = Value.stringLength(context->this);
	
	position = Value.toInteger(context, Context.argument(context, 0)).data.integer;
	
	text = positionText(chars, length, position, 0);
	if (!text.length)
		return Value.binary(NAN);
	else
	{
		uint32_t cp = Text.codepoint(text, NULL);
		
		if (cp < 0x010000)
			return Value.binary(cp);
		else
		{
			/* simulate 16-bit surrogate */
			
			cp -= 0x010000;
			if (text.bytes == positionText(chars, length, position + 1, 0).bytes)
				return Value.binary(((cp >> 10) & 0x3ff) + 0xd800);
			else
				return Value.binary(((cp >>  0) & 0x3ff) + 0xdc00);
		}
	}
}

static struct Value concat (struct Context * const context)
{
	struct Chars *chars;
	int32_t index, count;
	
	Context.assertVariableParameter(context);
	
	count = Context.variableArgumentCount(context);
	
	Chars.beginAppend(&chars);
	Chars.appendValue(&chars, context, Context.this(context));
	for (index = 0; index < count; ++index)
		chars = Chars.appendValue(&chars, context, Context.variableArgument(context, index));
	
	return Value.chars(Chars.endAppend(&chars));
}

static struct Value indexOf (struct Context * const context)
{
	struct Text text;
	struct Value search;
	int32_t position, index, length, searchLength, argumentCount;
	uint32_t cp;
	const char *chars, *searchChars;
	
	Context.assertVariableParameter(context);
	
	argumentCount = Context.variableArgumentCount(context);
	
	context->this = Value.toString(context, Context.this(context));
	chars = Value.stringBytes(context->this);
	length = Value.stringLength(context->this);
	
	search = argumentCount >= 1? Value.toString(context, Context.variableArgument(context, 0)): Value.text(&Text(undefined));
	searchChars = Value.stringBytes(search);
	searchLength = Value.stringLength(search);
	
	position = argumentCount >= 2? Value.toInteger(context, Context.variableArgument(context, 1)).data.integer: 0;
	text = positionText(chars, length, position, 0);
	if (text.flags & Text(breakFlag))
	{
		Text.nextCodepoint(&text);
		++position;
	}
	
	while (text.length)
	{
		if (text.bytes[0] == searchChars[0])
		{
			index = 0;
			do
			{
				if (index >= searchLength - 1)
					return Value.integer(position);
				
				++index;
			}
			while (text.bytes[index] == searchChars[index]);
		}
		
		++position;
		cp = Text.nextCodepoint(&text);
		if (cp > 0xffff)
			++position;
	}
	
	return Value.integer(-1);
}

static struct Value lastIndexOf (struct Context * const context)
{
	struct Text text;
	struct Value search;
	int32_t position, index, length, searchLength, argumentCount;
	uint32_t cp;
	const char *chars, *searchChars;
	
	Context.assertVariableParameter(context);
	
	argumentCount = Context.variableArgumentCount(context);
	
	context->this = Value.toString(context, Context.this(context));
	chars = Value.stringBytes(context->this);
	length = Value.stringLength(context->this);
	
	search = argumentCount >= 1? Value.toString(context, Context.variableArgument(context, 0)): Value.text(&Text(undefined));
	searchChars = Value.stringBytes(search);
	searchLength = Value.stringLength(search);
	
	if (argumentCount < 2 || Context.variableArgument(context, 1).type == Value(undefinedType))
		position = unitPosition(chars, length, length);
	else
		position = Value.toInteger(context, Context.variableArgument(context, 1)).data.integer;
	
	position -= unitPosition(searchChars, searchLength, searchLength) - 1;
	text = positionText(chars, length, position, 0);
	if (text.flags & Text(breakFlag))
	{
		Text.nextCodepoint(&text);
		++position;
	}
	text.length = text.bytes - chars;
	
	do
	{
		if (text.bytes[0] == searchChars[0])
		{
			index = 0;
			do
			{
				if (index >= searchLength - 1)
					return Value.integer(position);
				
				++index;
			}
			while (text.bytes[index] == searchChars[index]);
		}
		
		--position;
		cp = Text.prevCodepoint(&text);
		if (cp > 0xffff)
			--position;
	}
	while (text.length);
	
	return Value.integer(-1);
}

static struct Value slice (struct Context * const context)
{
	struct Value from, to;
	ptrdiff_t start, end, length;
	const char *chars;
	
	Context.assertParameterCount(context, 2);
	
	if (!Value.isString(context->this))
		context->this = Value.toString(context, Context.this(context));
	
	chars = Value.stringBytes(context->this);
	length = Value.stringLength(context->this);
	
	from = Context.argument(context, 0);
	if (from.type == Value(undefinedType))
		start = 0;
	else
		start = positionText(chars, length, Value.toInteger(context, from).data.integer, 1).bytes - chars;
	
	to = Context.argument(context, 1);
	if (to.type == Value(undefinedType))
		end = length;
	else
		end = positionText(chars, length, Value.toInteger(context, to).data.integer, 1).bytes - chars;
	
	length = end - start;
	
	if (length <= 0)
		return Value.text(&Text(empty));
	else
	{
		struct Chars *result = Chars.createSized(length);
		memcpy(result->bytes, chars + start, length);
		return Value.chars(result);
	}
}

static struct Value split (struct Context * const context)
{
	struct Value separatorValue, limitValue;
	struct RegExp *regexp = NULL;
	struct Object *array;
	struct Chars *element;
	struct Text text, separator = { 0 };
	uint32_t size = 0, limit = UINT32_MAX;
	
	Context.assertParameterCount(context, 2);
	
	context->this = Value.toString(context, Context.this(context));
	text = Text.make(Value.stringBytes(context->this), Value.stringLength(context->this));
	
	separatorValue = Context.argument(context, 0);
	if (separatorValue.type == Value(undefinedType) || !text.length)
	{
		struct Object *array = Array.createSized(1);
		array->element[0].value = context->this;
		return Value.object(array);
	}
	else if (separatorValue.type == Value(regexpType))
		regexp = separatorValue.data.regexp;
	else
	{
		separatorValue = Value.toString(context, separatorValue);
		separator = Text.make(Value.stringBytes(separatorValue), Value.stringLength(separatorValue));
	}
	
	limitValue = Context.argument(context, 1);
	if (limitValue.type != Value(undefinedType))
	{
		limit = Value.toInteger(context, limitValue).data.integer;
		if (!limit)
			return Value.object(Array.createSized(0));
	}
	
	Context.setTextIndex(context, Context(callIndex));
	
	array = Array.create();
	
	if (regexp)
	{
		const char *capture[2 + regexp->count * 2];
		const char *index[2 + regexp->count * 2];
		struct Text seek = text;
		
		for (;;)
		{
			struct RegExp(State) state = { seek.bytes, seek.bytes + seek.length, capture, index };
			uint16_t index, count;
			
			if (size >= limit)
				break;
			
			if (RegExp.matchWithState(regexp, &state))
			{
				if (state.capture[1] <= text.bytes)
				{
					Text.advance(&seek, 1);
					continue;
				}
				
				element = Chars.createWithBytes(state.capture[0] - text.bytes, text.bytes);
				Object.addElement(array, size++, Value.chars(element), 0);
				
				for (index = 1, count = regexp->count; index < count; ++index)
				{
					if (size >= limit)
						break;
					
					if (capture[index * 2])
					{
						element = Chars.createWithBytes(capture[index * 2 + 1] - capture[index * 2], capture[index * 2]);
						Object.addElement(array, size++, Value.chars(element), 0);
					}
					else
						Object.addElement(array, size++, Value(undefined), 0);
				}
				
				Text.advance(&text, state.capture[1] - text.bytes);
				seek = text;
			}
			else
			{
				element = Chars.createWithBytes(text.length, text.bytes);
				Object.addElement(array, size++, Value.chars(element), 0);
				break;
			}
		}
		return Value.object(array);
	}
	else if (!separator.length)
	{
		uint8_t units;
		
		while (text.length)
		{
			if (size >= limit)
				break;
			
			Text.codepoint(text, &units);
			element = Chars.createSized(units);
			memcpy(element->bytes, text.bytes, units);
			Object.addElement(array, size++, Value.chars(element), 0);
			Text.advance(&text, units);
		}
		
		return Value.object(array);
	}
	else
	{
		struct Text seek = text;
		ptrdiff_t length;
		
		while (seek.length >= separator.length)
		{
			if (size >= limit)
				break;
			
			if (!memcmp(seek.bytes, separator.bytes, separator.length))
			{
				length = seek.bytes - text.bytes;
				element = Chars.createSized(length);
				memcpy(element->bytes, text.bytes, length);
				Object.addElement(array, size++, Value.chars(element), 0);
				
				Text.advance(&text, length + separator.length);
				seek = text;
				continue;
			}
			Text.nextCodepoint(&seek);
		}
		
		if (text.length && size < limit)
		{
			element = Chars.createSized(text.length);
			memcpy(element->bytes, text.bytes, text.length);
			Object.addElement(array, size++, Value.chars(element), 0);
		}
	}
	
	return Value.object(array);
}

static struct Value substring (struct Context * const context)
{
	struct Value from, to;
	ptrdiff_t start, end, temp, length;
	const char *chars;
	
	Context.assertParameterCount(context, 2);
	
	if (!Value.isString(context->this))
		context->this = Value.toString(context, Context.this(context));
	
	chars = Value.stringBytes(context->this);
	length = Value.stringLength(context->this);
	
	from = Context.argument(context, 0);
	if (from.type == Value(undefinedType))
		start = 0;
	else
		start = positionText(chars, length, Value.toInteger(context, from).data.integer, 0).bytes - chars;
	
	to = Context.argument(context, 1);
	if (to.type == Value(undefinedType))
		end = length;
	else
		end = positionText(chars, length, Value.toInteger(context, to).data.integer, 0). bytes - chars;
	
	if (start > end)
	{
		temp = start;
		start = end;
		end = temp;
	}
	
	length = end - start;
	
	if (length <= 0)
		return Value.text(&Text(empty));
	else
	{
		struct Chars *result = Chars.createSized(length);
		memcpy(result->bytes, chars + start, length);
		return Value.chars(result);
	}
}

static struct Value toLowerCase (struct Context * const context)
{
	struct Chars *chars;
	struct Text text;
	
	Context.assertParameterCount(context, 0);
	
	if (!Value.isString(context->this))
		context->this = Value.toString(context, Context.this(context));
	
	text = Text.make(Value.stringBytes(context->this), Value.stringLength(context->this));
	{
		char buffer[text.length * 2];
		char *end = Text.toLower(text, buffer);
		chars = Chars.createWithBytes(end - buffer, buffer);
	}
	
	return Value.chars(chars);
}

static struct Value toUpperCase (struct Context * const context)
{
	struct Chars *chars;
	struct Text text;
	
	Context.assertParameterCount(context, 0);
	
	if (!Value.isString(context->this))
		context->this = Value.toString(context, Context.this(context));
		
		text = Text.make(Value.stringBytes(context->this), Value.stringLength(context->this));
	{
		char buffer[text.length * 3];
		char *end = Text.toUpper(text, buffer);
		chars = Chars.createWithBytes(end - buffer, buffer);
	}
	
	return Value.chars(chars);
}

static struct Value constructor (struct Context * const context)
{
	struct Value value;
	
	Context.assertParameterCount(context, 1);
	
	value = Context.argument(context, 0);
	if (value.type == Value(undefinedType))
		value = Value.text(value.check == 1? &Text(undefined): &Text(empty));
	else
		value = Value.toString(context, value);
	
	if (context->construct)
		return Value.string(create(Chars.createWithBytes(Value.stringLength(value), Value.stringBytes(value))));
	else
		return value;
}

static struct Value fromCharCode (struct Context * const context)
{
	struct Chars *chars;
	uint16_t index, count;
	
	Context.assertVariableParameter(context);
	
	count = Context.variableArgumentCount(context);
	
	Chars.beginAppendSized(&chars, count);
	
	for (index = 0; index < count; ++index)
		Chars.appendCodepoint(&chars, Value.toInteger(context, Context.variableArgument(context, index)).data.integer);
	
	return Value.chars(Chars.endAppend(&chars));
}

// MARK: - Static Members

// MARK: - Methods

void setup ()
{
	const enum Value(Flags) flags = Value(hidden);
	
	Function.setupBuiltinObject(
		&String(constructor), constructor, 1,
		&String(prototype), Value.string(create(Chars.createSized(0))),
		&String(type));
	
	Function.addMethod(String(constructor), "fromCharCode", fromCharCode, -1, flags);
	
	Function.addToObject(String(prototype), "toString", toString, 0, flags);
	Function.addToObject(String(prototype), "valueOf", valueOf, 0, flags);
	Function.addToObject(String(prototype), "charAt", charAt, 1, flags);
	Function.addToObject(String(prototype), "charCodeAt", charCodeAt, 1, flags);
	Function.addToObject(String(prototype), "concat", concat, -1, flags);
	Function.addToObject(String(prototype), "indexOf", indexOf, -1, flags);
	Function.addToObject(String(prototype), "lastIndexOf", lastIndexOf, -1, flags);
	Function.addToObject(String(prototype), "slice", slice, 2, flags);
	Function.addToObject(String(prototype), "split", split, 2, flags);
	Function.addToObject(String(prototype), "substring", substring, 2, flags);
	Function.addToObject(String(prototype), "toLowerCase", toLowerCase, 0, flags);
	Function.addToObject(String(prototype), "toUpperCase", toUpperCase, 0, flags);
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
	Object.addMember(&self->object, Key(length), Value.integer(unitPosition(chars->bytes, chars->length, chars->length)), Value(readonly));
	
	self->value = chars;
	++chars->referenceCount;
	
	return self;
}

struct Value valueAtPosition (struct String *self, uint32_t position)
{
	struct Text text;
	uint8_t units;
	
	text = positionText(self->value->bytes, self->value->length, position, 0);
	Text.codepoint(text, &units);
	
	if (units <= 0)
		return Value(undefined);
	else
	{
		struct Chars *result = Chars.createSized(units);
		memcpy(result->bytes, text.bytes, units);
		return Value.chars(result);
	}
}
