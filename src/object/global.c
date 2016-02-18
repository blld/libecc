//
//  global.c
//  libecc
//
//  Created by Bouilland Aurélien on 10/02/2016.
//  Copyright (c) 2016 Libeccio. All rights reserved.
//

#include "global.h"

// MARK: - Private

const struct Object(Type) Global(type) = {
	.text = &Text(globalType),
};

static struct Value eval (struct Native(Context) * const context)
{
	struct Value value;
	struct Input *input;
	struct Native(Context) subContext = {
		.parent = context,
		.this = Value.object(&context->ecc->global->environment),
		.environment = &context->ecc->global->environment,
		.ecc = context->ecc,
	};
	
	Native.assertParameterCount(context, 1);
	
	value = Value.toString(Native.argument(context, 0));
	input = Input.createFromBytes(Value.stringChars(value), Value.stringLength(value), "(eval)");
	
	Ecc.evalInputWithContext(context->ecc, input, &subContext);
	
	return context->ecc->result;
}

static struct Value parseInt (struct Native(Context) * const context)
{
	struct Value value;
	struct Text text;
	int32_t base;
	
	Native.assertParameterCount(context, 2);
	
	value = Value.toString(Native.argument(context, 0));
	base = Value.toInteger(Native.argument(context, 1)).data.integer;
	
	text = Text.make(Value.stringChars(value), Value.stringLength(value));
	
	if (!base)
	{
		// prevent octal auto-detection
		
		if (text.length > 2 && text.location[0] == '-')
		{
			if (text.location[1] == '0' && tolower(text.location[2]) != 'x')
				base = 10;
		}
		else if (text.length > 1 && text.location[0] == '0' && tolower(text.location[1]) != 'x')
			base = 10;
	}
	
	return Lexer.parseInteger(text, base);
}

static struct Value parseFloat (struct Native(Context) * const context)
{
	struct Value value;
	struct Text text;
	
	Native.assertParameterCount(context, 1);
	
	value = Value.toString(Native.argument(context, 0));
	
	text = Text.make(Value.stringChars(value), Value.stringLength(value));
	return Lexer.parseBinary(text);
}

static struct Value isFinite (struct Native(Context) * const context)
{
	struct Value value;
	
	Native.assertParameterCount(context, 1);
	
	value = Value.toBinary(Native.argument(context, 0));
	return Value.truth(!isnan(value.data.binary) && !isinf(value.data.binary));
}

static struct Value isNaN (struct Native(Context) * const context)
{
	struct Value value;
	
	Native.assertParameterCount(context, 1);
	
	value = Value.toBinary(Native.argument(context, 0));
	return Value.truth(isnan(value.data.binary));
}

static struct Value decodeURIExcept (struct Native(Context) * const context, const char *exclude)
{
	struct Value value;
	const char *bytes;
	uint16_t index = 0, offset = 0, count;
	struct Chars *chars;
	int c;
	
	Native.assertParameterCount(context, 1);
	
	value = Value.toString(Native.argument(context, 0));
	bytes = Value.stringChars(value);
	count = Value.stringLength(value);
	chars = Chars.createSized(count);
	
	while (index < count)
	{
		c = bytes[index++];
		if (c != '%')
			chars->chars[offset++] = c;
		else if (index + 2 > count || !isxdigit(bytes[index]) || !isxdigit(bytes[index + 1]))
			goto error;
		else
		{
			c = Lexer.uint8Hex(bytes[index], bytes[index + 1]);
			index += 2;
			
			if (c >= 0x80)
			{
				int continuation = (c & 0xf8) == 0xf0? 3: (c & 0xf0) == 0xe0? 2: (c & 0xe0) == 0xc0? 1: 0;
				if (!continuation || index + continuation * 3 > count)
					goto error;
				
				chars->chars[offset++] = c;
				while (continuation--)
				{
					if (bytes[index++] != '%' || !isxdigit(bytes[index]) || !isxdigit(bytes[index + 1]))
						goto error;
					
					chars->chars[offset++] = Lexer.uint8Hex(bytes[index], bytes[index + 1]);
					index += 2;
				}
			}
			else if (exclude && strchr(exclude, c))
				chars->chars[offset++] = '%', chars->chars[offset++] = bytes[index - 2], chars->chars[offset++] = bytes[index - 1];
			else
				chars->chars[offset++] = c;
		}
	}
	
	chars->length = offset;
	return Value.chars(chars);
	
	error:
	Ecc.jmpEnv(context->ecc, Value.error(Error.uriError(Native.textSeek(context, 0), "malformed URI")));
}

static struct Value decodeURI (struct Native(Context) * const context)
{
	return decodeURIExcept(context, ";/?:@&=+$,#");
}

static struct Value decodeURIComponent (struct Native(Context) * const context)
{
	return decodeURIExcept(context, NULL);
}

// MARK: - Static Members

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
	enum Value(Flags) flags = Value(hidden);
	
	struct Function * self = Function.create(Object(prototype));
	self->environment.type = &Global(type);
	
	Function.addValue(self, "NaN", Value.binary(NAN), flags | Value(frozen));
	Function.addValue(self, "Infinity", Value.binary(INFINITY), flags | Value(frozen));
	Function.addValue(self, "undefined", Value(undefined), flags | Value(frozen));
	Function.addNative(self, "eval", eval, 1, flags);
	Function.addNative(self, "parseInt", parseInt, 2, flags);
	Function.addNative(self, "parseFloat", parseFloat, 1, flags);
	Function.addNative(self, "isNaN", isNaN, 1, flags);
	Function.addNative(self, "isFinite", isFinite, 1, flags);
	Function.addNative(self, "decodeURI", decodeURI, 1, flags);
	Function.addNative(self, "decodeURIComponent", decodeURIComponent, 1, flags);
	#warning encodeURI
	#warning encodeURIComponent
	Function.addValue(self, "Object", Value.function(Object(constructor)), flags);
	Function.addValue(self, "Function", Value.function(Function(constructor)), flags);
	Function.addValue(self, "Array", Value.function(Array(constructor)), flags);
	Function.addValue(self, "String", Value.function(String(constructor)), flags);
	Function.addValue(self, "Boolean", Value.function(Boolean(constructor)), flags);
	Function.addValue(self, "Number", Value.function(Number(constructor)), flags);
	#warning Date
	Function.addValue(self, "RegExp", Value.function(RegExp(constructor)), flags);
	Function.addValue(self, "Error", Value.function(Error(constructor)), flags);
	Function.addValue(self, "RangeError", Value.function(Error(rangeConstructor)), flags);
	Function.addValue(self, "ReferenceError", Value.function(Error(referenceConstructor)), flags);
	Function.addValue(self, "SyntaxError", Value.function(Error(syntaxConstructor)), flags);
	Function.addValue(self, "TypeError", Value.function(Error(typeConstructor)), flags);
	Function.addValue(self, "URIError", Value.function(Error(uriConstructor)), flags);
	Function.addValue(self, "Math", Value.object(Math(object)), flags);
	#warning JSON
	
	return self;
}
