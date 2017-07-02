//
//  global.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#define Implementation
#include "global.h"

#include "../ecc.h"
#include "../lexer.h"

// MARK: - Private

const struct Object(Type) Global(type) = {
	.text = &Text(globalType),
};

// MARK: - Static Members

static
struct Value eval (struct Context * const context)
{
	struct Value value;
	struct Input *input;
	struct Context subContext = {
		.parent = context,
		.this = Value.object(&context->ecc->global->environment),
		.ecc = context->ecc,
		.depth = context->depth + 1,
		.environment = Context.environmentRoot(context->parent),
	};
	
	Context.assertParameterCount(context, 1);
	
	value = Context.argument(context, 0);
	if (!Value.isString(value) || !Value.isPrimitive(value))
		return value;
	
	input = Input.createFromBytes(Value.stringBytes(&value), Value.stringLength(&value), "(eval)");
	
	Context.setTextIndex(context, Context(noIndex));
	Ecc.evalInputWithContext(context->ecc, input, &subContext);
	
	return context->ecc->result;
}

static
struct Value parseInt (struct Context * const context)
{
	struct Value value;
	struct Text text;
	int32_t base;
	
	Context.assertParameterCount(context, 2);
	
	value = Value.toString(context, Context.argument(context, 0));
	base = Value.toInteger(context, Context.argument(context, 1)).data.integer;
	text = Value.textOf(&value);
	
	if (!base)
	{
		// prevent octal auto-detection
		
		if (text.length > 2 && text.bytes[0] == '-')
		{
			if (text.bytes[1] == '0' && tolower(text.bytes[2]) != 'x')
				base = 10;
		}
		else if (text.length > 1 && text.bytes[0] == '0' && tolower(text.bytes[1]) != 'x')
			base = 10;
	}
	
	return Lexer.scanInteger(text, base, Lexer(scanLazy) | (context->ecc->sloppyMode? Lexer(scanSloppy): 0));
}

static
struct Value parseFloat (struct Context * const context)
{
	struct Value value;
	struct Text text;
	
	Context.assertParameterCount(context, 1);
	
	value = Value.toString(context, Context.argument(context, 0));
	text = Value.textOf(&value);
	return Lexer.scanBinary(text, Lexer(scanLazy) | (context->ecc->sloppyMode? Lexer(scanSloppy): 0));
}

static
struct Value isFinite (struct Context * const context)
{
	struct Value value;
	
	Context.assertParameterCount(context, 1);
	
	value = Value.toBinary(context, Context.argument(context, 0));
	return Value.truth(!isnan(value.data.binary) && !isinf(value.data.binary));
}

static
struct Value isNaN (struct Context * const context)
{
	struct Value value;
	
	Context.assertParameterCount(context, 1);
	
	value = Value.toBinary(context, Context.argument(context, 0));
	return Value.truth(isnan(value.data.binary));
}

static
struct Value decodeExcept (struct Context * const context, const char *exclude)
{
	char buffer[5], *b;
	struct Value value;
	const char *bytes;
	uint16_t index = 0, count;
	struct Chars(Append) chars;
	uint8_t byte;
	
	Context.assertParameterCount(context, 1);
	
	value = Value.toString(context, Context.argument(context, 0));
	bytes = Value.stringBytes(&value);
	count = Value.stringLength(&value);
	
	Chars.beginAppend(&chars);
	
	while (index < count)
	{
		byte = bytes[index++];
		
		if (byte != '%')
			Chars.append(&chars, "%c", byte);
		else if (index + 2 > count || !isxdigit(bytes[index]) || !isxdigit(bytes[index + 1]))
			goto error;
		else
		{
			byte = Lexer.uint8Hex(bytes[index], bytes[index + 1]);
			index += 2;
			
			if (byte >= 0x80)
			{
				struct Text(Char) c;
				int continuation = (byte & 0xf8) == 0xf0? 3: (byte & 0xf0) == 0xe0? 2: (byte & 0xe0) == 0xc0? 1: 0;
				
				if (!continuation || index + continuation * 3 > count)
					goto error;
				
				b = buffer;
				(*b++) = byte;
				while (continuation--)
				{
					if (bytes[index++] != '%' || !isxdigit(bytes[index]) || !isxdigit(bytes[index + 1]))
						goto error;
					
					byte = Lexer.uint8Hex(bytes[index], bytes[index + 1]);
					index += 2;
					
					if ((byte & 0xc0) != 0x80)
						goto error;
					
					(*b++) = byte;
				}
				*b = '\0';
				
				c = Text.character(Text.make(buffer, b - buffer));
				Chars.appendCodepoint(&chars, c.codepoint);
			}
			else if (byte && exclude && strchr(exclude, byte))
				Chars.append(&chars, "%%%c%c", bytes[index - 2], bytes[index - 1]);
			else
				Chars.append(&chars, "%c", byte);
		}
	}
	
	return Chars.endAppend(&chars);
	
	error:
	Context.uriError(context, Chars.create("malformed URI"));
}

static
struct Value decodeURI (struct Context * const context)
{
	return decodeExcept(context, ";/?:@&=+$,#");
}

static
struct Value decodeURIComponent (struct Context * const context)
{
	return decodeExcept(context, NULL);
}

static
struct Value encodeExpect (struct Context * const context, const char *exclude)
{
	const char hex[] = "0123456789ABCDEF";
	struct Value value;
	const char *bytes;
	uint16_t offset = 0, unit, length;
	struct Chars *chars;
	struct Text text;
	struct Text(Char) c;
	int needPair = 0;
	
	Context.assertParameterCount(context, 1);
	
	value = Value.toString(context, Context.argument(context, 0));
	bytes = Value.stringBytes(&value);
	length = Value.stringLength(&value);
	text = Text.make(bytes, length);
	
	chars = Chars.createSized(length * 3);
	
	while (text.length)
	{
		c = Text.character(text);
		
		if (c.codepoint < 0x80 && c.codepoint && strchr(exclude, c.codepoint))
			chars->bytes[offset++] = c.codepoint;
		else
		{
			if (c.codepoint >= 0xDC00 && c.codepoint <= 0xDFFF)
			{
				if (!needPair)
					goto error;
			}
			else if (needPair)
				goto error;
			else if (c.codepoint >= 0xD800 && c.codepoint <= 0xDBFF)
				needPair = 1;
			
			for (unit = 0; unit < c.units; ++unit)
			{
				chars->bytes[offset++] = '%';
				chars->bytes[offset++] = hex[(uint8_t)text.bytes[unit] >> 4];
				chars->bytes[offset++] = hex[(uint8_t)text.bytes[unit] & 0xf];
			}
		}
		
		Text.advance(&text, c.units);
	}
	
	if (needPair)
		goto error;
	
	chars->length = offset;
	return Value.chars(chars);
	
	error:
	Context.uriError(context, Chars.create("malformed URI"));
}

static
struct Value encodeURI (struct Context * const context)
{
	return encodeExpect(context, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.!~*'()" ";/?:@&=+$,#");
}

static
struct Value encodeURIComponent (struct Context * const context)
{
	return encodeExpect(context, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.!~*'()");
}

static
struct Value escape (struct Context * const context)
{
	const char *exclude = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 @*_+-./";
	struct Value value;
	struct Chars(Append) chars;
	struct Text text;
	struct Text(Char) c;
	
	Context.assertParameterCount(context, 1);
	
	value = Value.toString(context, Context.argument(context, 0));
	text = Value.textOf(&value);
	
	Chars.beginAppend(&chars);
	
	while (text.length)
	{
		c = Text.nextCharacter(&text);
		
		if (c.codepoint < 0x80 && c.codepoint && strchr(exclude, c.codepoint))
			Chars.append(&chars, "%c", c.codepoint);
		else
		{
			if (c.codepoint <= 0xff)
				Chars.append(&chars, "%%%02X", c.codepoint);
			else if (c.codepoint < 0x010000)
				Chars.append(&chars, "%%u%04X", c.codepoint);
			else
			{
				c.codepoint -= 0x010000;
				Chars.append(&chars, "%%u%04X", ((c.codepoint >> 10) & 0x3ff) + 0xd800);
				Chars.append(&chars, "%%u%04X", ((c.codepoint >>  0) & 0x3ff) + 0xdc00);
			}
		}
	}
	
	return Chars.endAppend(&chars);
}

static
struct Value unescape (struct Context * const context)
{
	struct Value value;
	struct Chars(Append) chars;
	struct Text text;
	struct Text(Char) c;
	
	Context.assertParameterCount(context, 1);
	
	value = Value.toString(context, Context.argument(context, 0));
	text = Value.textOf(&value);
	
	Chars.beginAppend(&chars);
	
	while (text.length)
	{
		c = Text.nextCharacter(&text);
		
		if (c.codepoint == '%')
		{
			switch (Text.character(text).codepoint)
			{
				case '%':
					Chars.append(&chars, "%%");
					break;
					
				case 'u':
				{
					uint32_t cp = Lexer.uint16Hex(text.bytes[1], text.bytes[2], text.bytes[3], text.bytes[4]);
					
					Chars.appendCodepoint(&chars, cp);
					Text.advance(&text, 5);
					break;
				}
					
				default:
				{
					uint32_t cp = Lexer.uint8Hex(text.bytes[0], text.bytes[1]);
					
					Chars.appendCodepoint(&chars, cp);
					Text.advance(&text, 2);
					break;
				}
			}
		}
		else
			Chars.append(&chars, "%c", c.codepoint);
	}
	
	return Chars.endAppend(&chars);
}

// MARK: - Methods

void setup (void)
{
	Function(prototype) = Object(prototype) = Object.create(NULL);
	
	Function.setup();
	Object.setup();
	String.setup();
	Error.setup();
	Array.setup();
	Date.setup();
	Math.setup();
	Number.setup();
	Boolean.setup();
	RegExp.setup();
	Arguments.setup();
}

void teardown (void)
{
	Function.teardown();
	Object.teardown();
	String.teardown();
	Error.teardown();
	Array.teardown();
	Date.teardown();
	Math.teardown();
	Number.teardown();
	Boolean.teardown();
	RegExp.teardown();
	Arguments.teardown();
}

struct Function * create (void)
{
	const enum Value(Flags) r = Value(readonly);
	const enum Value(Flags) h = Value(hidden);
	const enum Value(Flags) s = Value(sealed);
	
	struct Function * self = Function.create(Object(prototype));
	self->environment.type = &Global(type);
	
	Function.addValue(self, "NaN", Value.binary(NAN), r|h|s);
	Function.addValue(self, "Infinity", Value.binary(INFINITY), r|h|s);
	Function.addValue(self, "undefined", Value(undefined), r|h|s);
	
	Function.addFunction(self, "eval", eval, 1, h);
	Function.addFunction(self, "escape", escape, 1, h);
	Function.addFunction(self, "unescape", unescape, 1, h);
	Function.addFunction(self, "parseInt", parseInt, 2, h);
	Function.addFunction(self, "parseFloat", parseFloat, 1, h);
	Function.addFunction(self, "isNaN", isNaN, 1, h);
	Function.addFunction(self, "isFinite", isFinite, 1, h);
	Function.addFunction(self, "decodeURI", decodeURI, 1, h);
	Function.addFunction(self, "decodeURIComponent", decodeURIComponent, 1, h);
	Function.addFunction(self, "encodeURI", encodeURI, 1, h);
	Function.addFunction(self, "encodeURIComponent", encodeURIComponent, 1, h);
	Function.addValue(self, "Object", Value.function(Object(constructor)), h);
	Function.addValue(self, "Function", Value.function(Function(constructor)), h);
	Function.addValue(self, "Array", Value.function(Array(constructor)), h);
	Function.addValue(self, "String", Value.function(String(constructor)), h);
	Function.addValue(self, "Boolean", Value.function(Boolean(constructor)), h);
	Function.addValue(self, "Number", Value.function(Number(constructor)), h);
	Function.addValue(self, "Date", Value.function(Date(constructor)), h);
	Function.addValue(self, "RegExp", Value.function(RegExp(constructor)), h);
	Function.addValue(self, "Error", Value.function(Error(constructor)), h);
	Function.addValue(self, "RangeError", Value.function(Error(rangeConstructor)), h);
	Function.addValue(self, "ReferenceError", Value.function(Error(referenceConstructor)), h);
	Function.addValue(self, "SyntaxError", Value.function(Error(syntaxConstructor)), h);
	Function.addValue(self, "TypeError", Value.function(Error(typeConstructor)), h);
	Function.addValue(self, "URIError", Value.function(Error(uriConstructor)), h);
	Function.addValue(self, "EvalError", Value.function(Error(evalConstructor)), h);
	Function.addValue(self, "Math", Value.object(Math(object)), h);
	#warning JSON
	
	return self;
}
