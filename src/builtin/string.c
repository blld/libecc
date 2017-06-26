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
#include "../op.h"

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
	uint16_t index, length;
	const char *chars;
	struct Text text;
	
	Context.assertParameterCount(context, 1);
	Context.assertThisType(context, Value(stringType));
	
	chars = Value.stringBytes(&context->this);
	length = Value.stringLength(&context->this);
	index = Value.toInteger(context, Context.argument(context, 0)).data.integer;
	
	text = textAtIndex(chars, length, index, 0);
	if (!text.length)
		return Value.text(&Text(empty));
	else
	{
		struct Text(Char) c = Text.character(text);
		
		if (c.codepoint < 0x010000)
			return Value.buffer(text.bytes, c.units);
		else
		{
			char buffer[7];
			
			/* simulate 16-bit surrogate */
			
			c.codepoint -= 0x010000;
			if (text.flags & Text(breakFlag))
				c.codepoint = ((c.codepoint >>  0) & 0x3ff) + 0xdc00;
			else
				c.codepoint = ((c.codepoint >> 10) & 0x3ff) + 0xd800;
			
			Chars.writeCodepoint(buffer, c.codepoint);
			return Value.buffer(buffer, 3);
		}
	}
}

static struct Value charCodeAt (struct Context * const context)
{
	int32_t index, length;
	const char *chars;
	struct Text text;
	
	Context.assertParameterCount(context, 1);
	Context.assertThisType(context, Value(stringType));
	
	chars = Value.stringBytes(&context->this);
	length = Value.stringLength(&context->this);
	index = Value.toInteger(context, Context.argument(context, 0)).data.integer;
	
	text = textAtIndex(chars, length, index, 0);
	if (!text.length)
		return Value.binary(NAN);
	else
	{
		struct Text(Char) c = Text.character(text);
		
		if (c.codepoint < 0x010000)
			return Value.binary(c.codepoint);
		else
		{
			/* simulate 16-bit surrogate */
			
			c.codepoint -= 0x010000;
			if (text.flags & Text(breakFlag))
				return Value.binary(((c.codepoint >>  0) & 0x3ff) + 0xdc00);
			else
				return Value.binary(((c.codepoint >> 10) & 0x3ff) + 0xd800);
		}
	}
}

static struct Value concat (struct Context * const context)
{
	struct Chars(Append) chars;
	int32_t index, count;
	
	Context.assertVariableParameter(context);
	
	count = Context.variableArgumentCount(context);
	
	Chars.beginAppend(&chars);
	Chars.appendValue(&chars, context, Context.this(context));
	for (index = 0; index < count; ++index)
		Chars.appendValue(&chars, context, Context.variableArgument(context, index));
	
	return Chars.endAppend(&chars);
}

static struct Value indexOf (struct Context * const context)
{
	struct Text text;
	struct Value search;
	int32_t index, length, searchLength, argumentCount;
	const char *chars, *searchChars;
	
	Context.assertVariableParameter(context);
	
	argumentCount = Context.variableArgumentCount(context);
	
	context->this = Value.toString(context, Context.this(context));
	chars = Value.stringBytes(&context->this);
	length = Value.stringLength(&context->this);
	
	search = argumentCount >= 1? Value.toString(context, Context.variableArgument(context, 0)): Value.text(&Text(undefined));
	searchChars = Value.stringBytes(&search);
	searchLength = Value.stringLength(&search);
	index = argumentCount >= 2? Value.toInteger(context, Context.variableArgument(context, 1)).data.integer: 0;
	
	text = textAtIndex(chars, length, index, 0);
	if (text.flags & Text(breakFlag))
	{
		Text.nextCharacter(&text);
		++index;
	}
	
	while (text.length)
	{
		if (!memcmp(text.bytes, searchChars, searchLength))
			return Value.integer(index);
		
		++index;
		if (Text.nextCharacter(&text).codepoint > 0xffff)
			++index;
	}
	
	return Value.integer(-1);
}

static struct Value lastIndexOf (struct Context * const context)
{
	struct Text text;
	struct Value search;
	int32_t index, length, searchLength, argumentCount;
	const char *chars, *searchChars;
	
	Context.assertVariableParameter(context);
	
	argumentCount = Context.variableArgumentCount(context);
	
	context->this = Value.toString(context, Context.this(context));
	chars = Value.stringBytes(&context->this);
	length = Value.stringLength(&context->this);
	
	search = argumentCount >= 1? Value.toString(context, Context.variableArgument(context, 0)): Value.text(&Text(undefined));
	searchChars = Value.stringBytes(&search);
	searchLength = Value.stringLength(&search);
	
	if (argumentCount < 2 || Context.variableArgument(context, 1).type == Value(undefinedType))
		index = unitIndex(chars, length, length);
	else
		index = Value.toInteger(context, Context.variableArgument(context, 1)).data.integer;
	
	index -= unitIndex(searchChars, searchLength, searchLength) - 1;
	text = textAtIndex(chars, length, index, 0);
	if (text.flags & Text(breakFlag))
	{
		Text.nextCharacter(&text);
		++index;
	}
	text.length = text.bytes - chars;
	
	while (text.length)
	{
		if (!memcmp(text.bytes, searchChars, searchLength))
			return Value.integer(index);
		
		--index;
		if (Text.prevCharacter(&text).codepoint > 0xffff)
			--index;
	}
	
	return Value.integer(-1);
}

static struct Value match (struct Context * const context)
{
	struct RegExp *regexp;
	struct Value value, lastIndex;
	
	Context.assertParameterCount(context, 1);
	
	context->this = Value.toString(context, Context.this(context));
	
	value = Context.argument(context, 0);
	if (value.type == Value(regexpType))
		regexp = value.data.regexp;
	else
		regexp = RegExp.createWith(context, value, Value(undefined));
	
	lastIndex = regexp->global? Value.integer(0): Value.toInteger(context, Object.getMember(context, &regexp->object, Key(lastIndex)));
	
	Object.putMember(context, &regexp->object, Key(lastIndex), Value.integer(0));
	
	if (lastIndex.data.integer >= 0)
	{
		const char *bytes = Value.stringBytes(&context->this);
		uint16_t length = Value.stringLength(&context->this);
		struct Text text = textAtIndex(bytes, length, 0, 0);
		const char *capture[regexp->count * 2];
		const char *index[regexp->count * 2];
		struct Object *array = Array.create();
		struct Chars(Append) chars;
		uint32_t size = 0;
		
		do
		{
			struct RegExp(State) state = { text.bytes, text.bytes + text.length, capture, index };
			
			if (RegExp.matchWithState(regexp, &state))
			{
				Chars.beginAppend(&chars);
				Chars.append(&chars, "%.*s", capture[1] - capture[0], capture[0]);
				Object.addElement(array, size++, Chars.endAppend(&chars), 0);
				
				if (!regexp->global)
				{
					int32_t index, count;
					
					for (index = 1, count = regexp->count; index < count; ++index)
					{
						if (capture[index * 2])
						{
							Chars.beginAppend(&chars);
							Chars.append(&chars, "%.*s", capture[index * 2 + 1] - capture[index * 2], capture[index * 2]);
							Object.addElement(array, size++, Chars.endAppend(&chars), 0);
						}
						else
							Object.addElement(array, size++, Value(undefined), 0);
					}
					break;
				}
				
				if (capture[1] - text.bytes > 0)
					Text.advance(&text, capture[1] - text.bytes);
				else
					Text.nextCharacter(&text);
			}
			else
				break;
		}
		while (text.length);
		
		if (size)
		{
			Object.addMember(array, Key(input), Pool.retainedValue(context->this), 0);
			Object.addMember(array, Key(index), Value.integer(String.unitIndex(bytes, length, (int32_t)(capture[0] - bytes))), 0);
			
			if (regexp->global)
				Object.putMember(context, &regexp->object, Key(lastIndex), Value.integer(String.unitIndex(bytes, length, (int32_t)(text.bytes - bytes))));
			
			return Value.object(array);
		}
	}
	return Value(null);
}

static void replaceText (struct Chars(Append) *chars, struct Text replace, struct Text before, struct Text match, struct Text after, int count, const char *capture[])
{
	struct Text(Char) c;
	
	while (replace.length)
	{
		c = Text.character(replace);
		
		if (c.codepoint == '$')
		{
			int index;
			
			Text.advance(&replace, 1);
			
			switch (Text.character(replace).codepoint)
			{
				case '$':
					Chars.append(chars, "$");
					break;
					
				case '&':
					Chars.append(chars, "%.*s", match.length, match.bytes);
					break;
					
				case '`':
					Chars.append(chars, "%.*s", before.length, before.bytes);
					break;
					
				case '\'':
					Chars.append(chars, "%.*s", after.length, after.bytes);
					break;
					
				case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
					index = replace.bytes[0] - '0';
					if (index < count)
					{
						if (isdigit(replace.bytes[1]) && index * 10 <= count)
						{
							index = index * 10 + replace.bytes[1] - '0';
							Text.advance(&replace, 1);
						}
						
						if (index && index < count)
						{
							if (capture[index * 2])
								Chars.append(chars, "%.*s", capture[index * 2 + 1] - capture[index * 2], capture[index * 2]);
							else
								Chars.append(chars, "");
							
							break;
						}
					}
					/* vvv */
				default:
					Chars.append(chars, "$");
					continue;
			}
		}
		else
			Chars.append(chars, "%.*s", c.units, replace.bytes);
		
		Text.advance(&replace, c.units);
	}
}

static struct Value replace (struct Context * const context)
{
	struct RegExp *regexp = NULL;
	struct Chars(Append) chars;
	struct Value value, replace;
	struct Text text;
	const char *bytes, *searchBytes;
	int32_t length, searchLength;
	
	Context.assertParameterCount(context, 2);
	Context.assertThisCoerciblePrimitive(context);
	
	context->this = Value.toString(context, Context.this(context));
	bytes = Value.stringBytes(&context->this);
	length = Value.stringLength(&context->this);
	text = Text.make(bytes, length);
	
	value = Context.argument(context, 0);
	if (value.type == Value(regexpType))
		regexp = value.data.regexp;
	else
		value = Value.toString(context, value);
	
	replace = Context.argument(context, 1);
	if (replace.type != Value(functionType))
		replace = Value.toString(context, replace);
	
	if (regexp)
	{
		const char *capture[regexp->count * 2];
		const char *index[regexp->count * 2];
		struct Text seek = text;
		
		Chars.beginAppend(&chars);
		do
		{
			struct RegExp(State) state = { seek.bytes, text.bytes + text.length, capture, index };
			
			if (RegExp.matchWithState(regexp, &state))
			{
				Chars.append(&chars, "%.*s", state.capture[0] - text.bytes, text.bytes);
				
				if (replace.type == Value(functionType))
				{
					struct Object *arguments = Array.createSized(regexp->count + 2);
					uint16_t index, count;
					struct Value result;
					
					for (index = 0, count = regexp->count; index < count; ++index)
					{
						if (capture[index * 2])
							arguments->element[index].value = Value.chars(Chars.createWithBytes(capture[index * 2 + 1] - capture[index * 2], capture[index * 2]));
						else
							arguments->element[index].value = Value(undefined);
					}
					arguments->element[regexp->count].value = Value.integer(String.unitIndex(bytes, length, (int32_t)(capture[0] - bytes)));
					arguments->element[regexp->count + 1].value = context->this;
					
					result = Value.toString(context, Op.callFunctionArguments(context, 0, replace.data.function, Value(undefined), arguments));
					Chars.append(&chars, "%.*s", Value.stringLength(&result), Value.stringBytes(&result));
				}
				else
					replaceText(&chars,
								Text.make(Value.stringBytes(&replace), Value.stringLength(&replace)),
								Text.make(bytes, state.capture[0] - bytes),
								Text.make(state.capture[0], state.capture[1] - state.capture[0]),
								Text.make(state.capture[1], (bytes + length) - state.capture[1]),
								regexp->count,
								capture);
				
				Text.advance(&text, state.capture[1] - text.bytes);
				
				seek = text;
				if (text.bytes == state.capture[1])
					Text.nextCharacter(&seek);
			}
			else
				break;
		}
		while (text.length && regexp->global);
		
		Chars.append(&chars, "%.*s", text.length, text.bytes);
		
		return Chars.endAppend(&chars);
	}
	else
	{
		searchBytes = Value.stringBytes(&value);
		searchLength = Value.stringLength(&value);
		
		for (;;)
		{
			if (!text.length)
				return context->this;
			
			if (!memcmp(text.bytes, searchBytes, searchLength))
			{
				text.length = searchLength;
				break;
			}
			Text.nextCharacter(&text);
		}
		
		Chars.beginAppend(&chars);
		Chars.append(&chars, "%.*s", text.bytes - bytes, bytes);
		
		if (replace.type == Value(functionType))
		{
			struct Object *arguments = Array.createSized(1 + 2);
			struct Value result;
			
			arguments->element[0].value = Value.chars(Chars.createWithBytes(text.length, text.bytes));
			arguments->element[1].value = Value.integer(String.unitIndex(bytes, length, (int32_t)(text.bytes - bytes)));
			arguments->element[2].value = context->this;
			
			result = Value.toString(context, Op.callFunctionArguments(context, 0, replace.data.function, Value(undefined), arguments));
			Chars.append(&chars, "%.*s", Value.stringLength(&result), Value.stringBytes(&result));
		}
		else
			replaceText(&chars,
						Text.make(Value.stringBytes(&replace), Value.stringLength(&replace)),
						Text.make(text.bytes, text.bytes - bytes),
						Text.make(text.bytes, text.length),
						Text.make(text.bytes, length - (text.bytes - bytes)),
						0,
						NULL);
		
		Chars.append(&chars, "%.*s", length - (text.bytes - bytes), text.bytes + text.length);
		
		return Chars.endAppend(&chars);
	}
}

static
struct Value search (struct Context * const context)
{
	struct RegExp *regexp;
	struct Value value;
	
	Context.assertParameterCount(context, 1);
	Context.assertThisCoerciblePrimitive(context);
	
	context->this = Value.toString(context, Context.this(context));
	
	value = Context.argument(context, 0);
	if (value.type == Value(regexpType))
		regexp = value.data.regexp;
	else
		regexp = RegExp.createWith(context, value, Value(undefined));
	
	{
		const char *bytes = Value.stringBytes(&context->this);
		uint16_t length = Value.stringLength(&context->this);
		struct Text text = textAtIndex(bytes, length, 0, 0);
		const char *capture[regexp->count * 2];
		const char *index[regexp->count * 2];
		
		struct RegExp(State) state = { text.bytes, text.bytes + text.length, capture, index };
		
		if (RegExp.matchWithState(regexp, &state))
			return Value.integer(unitIndex(bytes, length, (int32_t)(capture[0] - bytes)));
	}
	return Value.integer(-1);
}

static struct Value slice (struct Context * const context)
{
	struct Value from, to;
	struct Text start, end;
	const char *chars;
	ptrdiff_t length;
	uint16_t head = 0, tail = 0;
	uint32_t headcp = 0;
	
	Context.assertParameterCount(context, 2);
	
	if (!Value.isString(context->this))
		context->this = Value.toString(context, Context.this(context));
	
	chars = Value.stringBytes(&context->this);
	length = Value.stringLength(&context->this);
	
	from = Context.argument(context, 0);
	if (from.type == Value(undefinedType))
		start = Text.make(chars, length);
	else
		start = textAtIndex(chars, length, Value.toInteger(context, from).data.integer, 1);
	
	to = Context.argument(context, 1);
	if (to.type == Value(undefinedType))
		end = Text.make(chars + length, 0);
	else
		end = textAtIndex(chars, length, Value.toInteger(context, to).data.integer, 1);
	
	if (start.flags & Text(breakFlag))
		headcp = Text.nextCharacter(&start).codepoint;
	
	length = end.bytes - start.bytes;
	
	if (start.flags & Text(breakFlag))
		head = 3;
	
	if (end.flags & Text(breakFlag))
		tail = 3;
	
	if (head + length + tail <= 0)
		return Value.text(&Text(empty));
	else
	{
		struct Chars *result = Chars.createSized(length + head + tail);
		
		if (start.flags & Text(breakFlag))
		{
			/* simulate 16-bit surrogate */
			Chars.writeCodepoint(result->bytes, (((headcp - 0x010000) >> 0) & 0x3ff) + 0xdc00);
		}
		
		if (length > 0)
			memcpy(result->bytes + head, start.bytes, length);
		
		if (end.flags & Text(breakFlag))
		{
			/* simulate 16-bit surrogate */
			Chars.writeCodepoint(result->bytes + head + length, (((Text.character(end).codepoint - 0x010000) >> 10) & 0x3ff) + 0xd800);
		}
		
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
	text = Value.textOf(&context->this);
	
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
		separator = Value.textOf(&separatorValue);
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
		const char *capture[regexp->count * 2];
		const char *index[regexp->count * 2];
		struct Text seek = text;
		
		for (;;)
		{
			struct RegExp(State) state = { seek.bytes, seek.bytes + seek.length, capture, index };
			uint16_t index, count;
			
			if (size >= limit)
				break;
			
			if (seek.length && RegExp.matchWithState(regexp, &state))
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
		struct Text(Char) c;
		
		while (text.length)
		{
			if (size >= limit)
				break;
			
			c = Text.character(text);
			if (c.codepoint < 0x010000)
				Object.addElement(array, size++, Value.buffer(text.bytes, c.units), 0);
			else
			{
				char buffer[7];
				
				/* simulate 16-bit surrogate */
				
				Chars.writeCodepoint(buffer, (((c.codepoint - 0x010000) >> 10) & 0x3ff) + 0xd800);
				Object.addElement(array, size++, Value.buffer(buffer, 3), 0);
				
				Chars.writeCodepoint(buffer, (((c.codepoint - 0x010000) >> 0) & 0x3ff) + 0xdc00);
				Object.addElement(array, size++, Value.buffer(buffer, 3), 0);
			}
			Text.advance(&text, c.units);
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
			Text.nextCharacter(&seek);
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
	struct Text start, end;
	const char *chars;
	uint16_t length, head = 0, tail = 0;
	uint32_t headcp = 0;
	
	Context.assertParameterCount(context, 2);
	
	if (!Value.isString(context->this))
		context->this = Value.toString(context, Context.this(context));
	
	chars = Value.stringBytes(&context->this);
	length = Value.stringLength(&context->this);
	
	from = Context.argument(context, 0);
	if (from.type == Value(undefinedType))
		start = Text.make(chars, length);
	else
		start = textAtIndex(chars, length, Value.toInteger(context, from).data.integer, 0);
	
	to = Context.argument(context, 1);
	if (to.type == Value(undefinedType))
		end = Text.make(chars + length, 0);
	else
		end = textAtIndex(chars, length, Value.toInteger(context, to).data.integer, 0);
	
	if (start.bytes > end.bytes)
	{
		struct Text temp = start;
		start = end;
		end = temp;
	}
	
	if (start.flags & Text(breakFlag))
		headcp = Text.nextCharacter(&start).codepoint;
	
	length = end.bytes - start.bytes;
	
	if (start.flags & Text(breakFlag))
		head = 3;
	
	if (end.flags & Text(breakFlag))
		tail = 3;
	
	if (head + length + tail <= 0)
		return Value.text(&Text(empty));
	else
	{
		struct Chars *result = Chars.createSized(length + head + tail);
		
		if (start.flags & Text(breakFlag))
		{
			/* simulate 16-bit surrogate */
			Chars.writeCodepoint(result->bytes, (((headcp - 0x010000) >> 0) & 0x3ff) + 0xdc00);
		}
		
		if (length > 0)
			memcpy(result->bytes + head, start.bytes, length);
		
		if (end.flags & Text(breakFlag))
		{
			/* simulate 16-bit surrogate */
			Chars.writeCodepoint(result->bytes + head + length, (((Text.character(end).codepoint - 0x010000) >> 10) & 0x3ff) + 0xd800);
		}
		
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
	
	text = Value.textOf(&context->this);
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
		
		text = Value.textOf(&context->this);
	{
		char buffer[text.length * 3];
		char *end = Text.toUpper(text, buffer);
		chars = Chars.createWithBytes(end - buffer, buffer);
	}
	
	return Value.chars(chars);
}

static struct Value trim (struct Context * const context)
{
	struct Chars *chars;
	struct Text text, last;
	struct Text(Char) c;
	
	Context.assertParameterCount(context, 0);
	
	if (!Value.isString(context->this))
		context->this = Value.toString(context, Context.this(context));
	
	text = Value.textOf(&context->this);
	while (text.length)
	{
		c = Text.character(text);
		if (!Text.isSpace(c))
			break;
		
		Text.advance(&text, c.units);
	}
	
	last = Text.make(text.bytes + text.length, text.length);
	while (last.length)
	{
		c = Text.prevCharacter(&last);
		if (!Text.isSpace(c))
			break;
		
		text.length = last.length;
	}
	
	chars = Chars.createWithBytes(text.length, text.bytes);
	
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
		return Value.string(create(Chars.createWithBytes(Value.stringLength(&value), Value.stringBytes(&value))));
	else
		return value;
}

static struct Value fromCharCode (struct Context * const context)
{
	struct Chars(Append) chars;
	uint16_t index, count;
	
	Context.assertVariableParameter(context);
	
	count = Context.variableArgumentCount(context);
	
	Chars.beginAppend(&chars);
	
	for (index = 0; index < count; ++index)
		Chars.appendCodepoint(&chars, (uint16_t)Value.toInteger(context, Context.variableArgument(context, index)).data.integer);
	
	return Chars.endAppend(&chars);
}

static struct Value getLength (struct Context * const context)
{
	struct Chars *chars;
	
	Context.assertParameterCount(context, 0);
	
	chars = context->this.data.string->value;
	return Value.integer(unitIndex(chars->bytes, chars->length, chars->length));
}

// MARK: - Static Members

// MARK: - Methods

void setup ()
{
	const enum Value(Flags) r = Value(readonly);
	const enum Value(Flags) h = Value(hidden);
	const enum Value(Flags) s = Value(sealed);
	
	Function.setupBuiltinObject(
		&String(constructor), constructor, 1,
		&String(prototype), Value.string(create(Chars.createSized(0))),
		&String(type));
	
	Function.addMethod(String(constructor), "fromCharCode", fromCharCode, -1, h);
	
	Function.addToObject(String(prototype), "toString", toString, 0, h);
	Function.addToObject(String(prototype), "valueOf", valueOf, 0, h);
	Function.addToObject(String(prototype), "charAt", charAt, 1, h);
	Function.addToObject(String(prototype), "charCodeAt", charCodeAt, 1, h);
	Function.addToObject(String(prototype), "concat", concat, -1, h);
	Function.addToObject(String(prototype), "indexOf", indexOf, -1, h);
	Function.addToObject(String(prototype), "lastIndexOf", lastIndexOf, -1, h);
	Function.addToObject(String(prototype), "match", match, 1, h);
	Function.addToObject(String(prototype), "replace", replace, 2, h);
	Function.addToObject(String(prototype), "search", search, 1, h);
	Function.addToObject(String(prototype), "slice", slice, 2, h);
	Function.addToObject(String(prototype), "split", split, 2, h);
	Function.addToObject(String(prototype), "substring", substring, 2, h);
	Function.addToObject(String(prototype), "toLowerCase", toLowerCase, 0, h);
	Function.addToObject(String(prototype), "toUpperCase", toUpperCase, 0, h);
	Function.addToObject(String(prototype), "trim", trim, 0, h);
	
	Object.addMember(String(prototype), Key(length), Function.accessor(getLength, NULL), r|h|s | Value(asOwn) | Value(asData));
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
	
	self->value = chars;
	++chars->referenceCount;
	
	return self;
}

struct Value valueAtIndex (struct String *self, uint32_t position)
{
	struct Text(Char) c;
	struct Text text;
	
	text = textAtIndex(self->value->bytes, self->value->length, position, 0);
	c = Text.character(text);
	
	if (c.units <= 0)
		return Value(undefined);
	else
	{
		if (c.codepoint < 0x010000)
			return Value.buffer(text.bytes, c.units);
		else
		{
			char buffer[7];
			
			/* simulate 16-bit surrogate */
			
			c.codepoint -= 0x010000;
			if (text.flags & Text(breakFlag))
				c.codepoint = ((c.codepoint >>  0) & 0x3ff) + 0xdc00;
			else
				c.codepoint = ((c.codepoint >> 10) & 0x3ff) + 0xd800;
			
			Chars.writeCodepoint(buffer, c.codepoint);
			return Value.buffer(buffer, 3);
		}
	}
}

struct Text textAtIndex (const char *chars, uint16_t length, int32_t position, int enableReverse)
{
	struct Text text = Text.make(chars, length), prev = text;
	struct Text(Char) c;
	
	if (position >= 0)
	{
		while (position-- > 0)
		{
			prev = text;
			c = Text.nextCharacter(&text);
			
			if (c.codepoint > 0xffff && !position--)
			{
				/* simulate 16-bit surrogate */
				text = prev;
				text.flags = Text(breakFlag);
			}
		}
	}
	else if (enableReverse)
	{
		text.bytes += length;
		
		while (position++ < 0)
		{
			c = Text.prevCharacter(&text);
			
			if (c.codepoint > 0xffff && position++ >= 0)
			{
				/* simulate 16-bit surrogate */
				text.flags = Text(breakFlag);
			}
		}
		
		text.length = length - (text.bytes - chars);
	}
	else
		text.length = 0;
	
	return text;
}

uint16_t unitIndex (const char *chars, uint16_t max, int32_t unit)
{
	struct Text text = Text.make(chars, max);
	uint16_t position = 0;
	struct Text(Char) c;
	
	while (unit > 0)
	{
		if (text.length)
		{
			++position;
			c = Text.nextCharacter(&text);
			unit -= c.units;
			
			if (c.codepoint > 0xffff) /* simulate 16-bit surrogate */
				++position;
		}
	}
	
	return position;
}
