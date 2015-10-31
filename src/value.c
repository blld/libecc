//
//  value.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "value.h"

// MARK: - Private

// MARK: - Static Members

// MARK: - Methods

struct Value undefined (void)
{
	return (struct Value){
		.type = Value(undefined),
	};
}

struct Value null (void)
{
	return (struct Value){
		.type = Value(null),
	};
}

struct Value true (void)
{
	return (struct Value){
		.type = Value(true),
	};
}

struct Value false (void)
{
	return (struct Value){
		.type = Value(false),
	};
}

struct Value truth (int truth)
{
	return (struct Value){
		.type = truth? Value(true): Value(false),
	};
}

struct Value integer (int32_t integer)
{
	return (struct Value){
		.data = { .integer = integer },
		.type = Value(integer),
	};
}

struct Value binary (double binary)
{
	return (struct Value){
		.data = { .binary = binary },
		.type = Value(binary),
	};
}

struct Value key (struct Key key)
{
	return (struct Value){
		.data = { .key = key },
		.type = Value(key),
	};
}

struct Value text (const struct Text *text)
{
	return (struct Value){
		.data = { .text = text },
		.type = Value(text),
	};
}

struct Value chars (struct Chars *chars)
{
	assert(chars);
	
	return (struct Value){
		.data = { .chars = chars },
		.type = Value(chars),
	};
}

struct Value object (struct Object *object)
{
	assert(object);
	
	return (struct Value){
		.data = { .object = object },
		.type = Value(object),
	};
}

struct Value error (struct Error *error)
{
	assert(error);
	
	return (struct Value){
		.data = { .error = error },
		.type = Value(error),
	};
}

struct Value string (struct String *string)
{
	assert(string);
	
	return (struct Value){
		.data = { .string = string },
		.type = Value(string),
	};
}

struct Value number (struct Number *number)
{
	assert(number);
	
	return (struct Value){
		.data = { .number = number },
		.type = Value(number),
	};
}

struct Value boolean (struct Boolean *boolean)
{
	assert(boolean);
	
	return (struct Value){
		.data = { .boolean = boolean },
		.type = Value(boolean),
	};
}

struct Value date (struct Date *date)
{
	assert(date);
	
	return (struct Value){
		.data = { .date = date },
		.type = Value(date),
	};
}

struct Value function (struct Function *function)
{
	assert(function);
	
	return (struct Value){
		.data = { .function = function },
		.type = Value(function),
	};
}

struct Value breaker (int32_t integer)
{
	return (struct Value){
		.data = { .integer = integer },
		.type = Value(breaker),
	};
}

struct Value reference (struct Value *reference)
{
	assert(reference);
	
	return (struct Value){
		.data = { .reference = reference },
		.type = Value(reference),
	};
}

struct Value toPrimitive (struct Value value, struct Ecc *ecc, const struct Text *text, int hint)
{
	struct Object *object;
	struct Key aKey;
	struct Key bKey;
	struct Value aFunction;
	struct Value bFunction;
	
	if (value.type < Value(object))
		return value;
	
	object = value.data.object;
	if (!hint)
		hint = value.type == Value(date)? 1: -1;
	
	aKey = hint > 0? Key(toString): Key(valueOf);
	bKey = hint > 0? Key(valueOf): Key(toString);
	
	aFunction = Object.get(object, aKey);
	if (aFunction.type == Value(function))
	{
		struct Value result = Op.callFunctionVA(aFunction.data.function, ecc, value, 0);
		if (isPrimitive(result))
			return result;
	}
	
	bFunction = Object.get(object, bKey);
	if (bFunction.type == Value(function))
	{
		struct Value result = Op.callFunctionVA(bFunction.data.function, ecc, value, 0);
		if (isPrimitive(result))
			return result;
	}
	
	Ecc.jmpEnv(ecc, Value.error(Error.typeError(*text, "cannot convert %.*s to primitive", text->length, text->location)));
}

int isPrimitive (struct Value value)
{
	return value.type < Value(object);
}

int isBoolean (struct Value value)
{
	return value.type & 0x01;
}

int isDynamic (struct Value value)
{
	return value.type >= Value(chars);
}

int isTrue (struct Value value)
{
	if (value.type <= 0)
		return 0;
	else if (value.type == Value(integer))
		return value.data.integer != 0;
	else if (value.type == Value(binary))
		return value.data.binary != 0;
	else if (isString(value))
		return stringLength(value) > 0;
	else
		return 1;
}

struct Value toString (struct Value value, struct Text *buffer)
{
	switch (value.type)
	{
		case Value(text):
		case Value(chars):
			return value;
		
		case Value(key):
			return Value.text(Key.textOf(value.data.key));
		
		case Value(string):
			return Value.chars(value.data.string->value);
		
		case Value(null):
			return Value.text(&Text(null));
		
		case Value(undefined):
			return Value.text(&Text(undefined));
		
		case Value(false):
			return Value.text(&Text(false));
		
		case Value(true):
			return Value.text(&Text(true));
		
		case Value(boolean):
			return value.data.boolean->truth? Value.text(&Text(true)): Value.text(&Text(false));
		
		case Value(integer):
			return binaryToString(value.data.integer, buffer, 10);
		
		case Value(number):
			value.data.binary = value.data.number->value;
			/*vvv*/
		
		case Value(binary):
			return binaryToString(value.data.binary, buffer, 10);
		
		case Value(object):
			return Value.text(value.data.object->type);
		
		case Value(function):
			return Value.text(&value.data.function->text);
		
		case Value(error):
		{
			struct Object *object = value.data.object;
			struct Value name = Value.toString(Object.get(object, Key(name)), NULL);
			struct Value message = Value.toString(Object.get(object, Key(message)), NULL);
			return Value.chars(Chars.create("%.*s: %.*s", Value.stringLength(name), Value.stringChars(name), Value.stringLength(message), Value.stringChars(message)));
		}
		
		case Value(date):
		case Value(breaker):
		case Value(reference):
			break;
	}
	assert(0);
	abort();
}

struct Value binaryToString (double binary, struct Text *buffer, int base)
{
	if (binary == 0)
		return Value.text(&Text(zero));
	else if (binary == 1)
		return Value.text(&Text(one));
	else if (isnan(binary))
		return Value.text(&Text(nan));
	else if (isinf(binary))
	{
		if (signbit(binary))
			return Value.text(&Text(negativeInfinity));
		else
			return Value.text(&Text(infinity));
	}
	
	if (!base || base == 10)
	{
		const char *format = binary >= -999999999999999 && binary <= 999999999999999? "%.16g": "%.21g";
		
		if (buffer && buffer->length > snprintf(NULL, 0, format, binary))
		{
			buffer->length = snprintf((char *)buffer->location, buffer->length, format, binary);
			return Value.text(buffer);
		}
		else
			return Value.chars(Chars.create(format, binary));
	}
	else
	{
		int sign = signbit(binary);
		unsigned long integer = sign? -binary: binary;
		if (integer == 0)
			return Value.text(&Text(zero));
		else if (integer == 1)
			return Value.text(&Text(one));
		
		if (base == 8 || base == 16)
		{
			const char *format = sign? (base == 8? "-%lo": "-%lx"): (base == 8? "%lo": "%lx");
			
			if (buffer && buffer->length > snprintf(NULL, 0, format, integer))
			{
				buffer->length = snprintf((char *)buffer->location, buffer->length, format, integer);
				return Value.text(buffer);
			}
			else
				return Value.chars(Chars.create(format, integer));
		}
		else
		{
			static char const digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
			char bytes[1 + sizeof(integer) * CHAR_BIT];
			char *p = bytes + sizeof(bytes);
			long length;
			
			while (integer) {
				*(--p) = digits[integer % base];
				integer /= base;
			}
			if (sign)
				*(--p) = '-';
			
			length = bytes + sizeof(bytes) - p;
			
			if (buffer && buffer->length >= length)
			{
				memcpy((char *)buffer->location, p, length);
				buffer->length = length;
				return Value.text(buffer);
			}
			else
			{
				struct Chars *chars = Chars.createSized(length);
				memcpy(chars->chars, p, length);
				return Value.chars(chars);
			}
		}
	}
}

int isString (struct Value value)
{
	return value.type & 0x20;
}

const char * stringChars (struct Value value)
{
	if (value.type == Value(chars))
		return value.data.chars->chars;
	else if (value.type == Value(text))
		return value.data.text->location;
	else if (value.type == Value(string))
		return value.data.string->value->chars;
	else
		return NULL;
}

uint16_t stringLength (struct Value value)
{
	if (value.type == Value(chars))
		return value.data.chars->length;
	else if (value.type == Value(text))
		return value.data.text->length;
	else if (value.type == Value(string))
		return value.data.string->value->length;
	else
		return 0;
}

struct Value toBinary (struct Value value)
{
	switch (value.type)
	{
		case Value(binary):
			return value;
		
		case Value(integer):
			return Value.binary(value.data.integer);
		
		case Value(number):
			return Value.binary(value.data.number->value);
		
		case Value(null):
		case Value(false):
			return Value.binary(0);
		
		case Value(true):
			return Value.binary(1);
		
		case Value(boolean):
			return Value.binary(value.data.boolean->truth? 1: 0);
		
		case Value(undefined):
			return Value.binary(NAN);
		
		case Value(text):
			if (value.data.text == &Text(zero))
				return Value.binary(0);
			else if (value.data.text == &Text(one))
				return Value.binary(1);
			else if (value.data.text == &Text(nan))
				return Value.binary(NAN);
			else if (value.data.text == &Text(infinity))
				return Value.binary(INFINITY);
			else if (value.data.text == &Text(negativeInfinity))
				return Value.binary(-INFINITY);
			
			/*vvv*/
			
		case Value(key):
		case Value(chars):
		case Value(string):
			return Lexer.parseBinary(Text.make(Value.stringChars(value), Value.stringLength(value)));
		
		case Value(object):
		case Value(error):
		case Value(date):
		case Value(function):
			return Value.binary(NAN);
		
		case Value(breaker):
		case Value(reference):
			break;
	}
	assert(0);
	abort();
}

struct Value toInteger (struct Value value)
{
	double binary = toBinary(value).data.binary;
	
	if (isnan(binary) || isinf(binary))
		return Value.integer(0);
	else
		return Value.integer((uint32_t)binary);
}

int isNumber (struct Value value)
{
	return value.type & 0x10;
}

struct Value toObject (struct Value value, struct Ecc *ecc, const struct Text *text)
{
	switch (value.type)
	{
		case Value(null):
			Ecc.jmpEnv(ecc, Value.error(Error.typeError(*text, "%.*s is null", text->length, text->location)));
		
		case Value(undefined):
			Ecc.jmpEnv(ecc, Value.error(Error.typeError(*text, "%.*s is undefined", text->length, text->location)));
		
		case Value(function):
		case Value(object):
		case Value(error):
		case Value(string):
		case Value(number):
		case Value(date):
		case Value(boolean):
			return value;
		
		case Value(key):
		case Value(text):
		case Value(chars):
			return Value.string(String.create(Chars.create("%.*s", stringLength(value), stringChars(value))));
		
		case Value(false):
		case Value(true):
			return Value.boolean(Boolean.create(value.type == Value(true)));
		
		case Value(integer):
			return Value.number(Number.create(value.data.integer));
		
		case Value(binary):
			return Value.number(Number.create(value.data.binary));
		
		case Value(breaker):
		case Value(reference):
			break;
	}
	assert(0);
	abort();
}

int isObject (struct Value value)
{
	return value.type >= Value(object);
}

struct Value toType (struct Value value)
{
	switch (value.type)
	{
		case Value(true):
		case Value(false):
			return Value.text(&Text(boolean));
		
		case Value(undefined):
			return Value.text(&Text(undefined));
		
		case Value(integer):
		case Value(binary):
			return Value.text(&Text(number));
		
		case Value(key):
		case Value(text):
		case Value(chars):
			return Value.text(&Text(string));
		
		case Value(null):
		case Value(object):
		case Value(string):
		case Value(number):
		case Value(boolean):
		case Value(error):
		case Value(date):
			return Value.text(&Text(object));
		
		case Value(function):
			return Value.text(&Text(function));
		
		case Value(breaker):
		case Value(reference):
			break;
	}
	assert(0);
	abort();
}

void dumpTo (struct Value value, FILE *file)
{
	switch (value.type)
	{
		case Value(null):
			fputs("null", file);
			return;
		
		case Value(undefined):
			fputs("undefined", file);
			return;
		
		case Value(false):
			fputs("false", file);
			return;
		
		case Value(true):
			fputs("true", file);
			return;
		
		case Value(boolean):
			fputs(value.data.boolean->truth? "true": "false", file);
			return;
		
		case Value(integer):
			fprintf(file, "%d", value.data.integer);
			return;
		
		case Value(breaker):
			fprintf(file, "[[breaker:%d]]", value.data.integer);
			return;
		
		case Value(number):
			value.data.binary = value.data.number->value;
			/*vvv*/
			
		case Value(binary):
			fprintf(file, "%g", value.data.binary);
			return;
		
		case Value(key):
		{
			struct Text *text = Key.textOf(value.data.key);
			fwrite(text->location, sizeof(char), text->length, file);
			return;
		}
		
		case Value(text):
			fwrite(value.data.text->location, sizeof(char), value.data.text->length, file);
			return;
		
		case Value(chars):
			fwrite(value.data.chars->chars, sizeof(char), value.data.chars->length, file);
			return;
		
		case Value(string):
			fwrite(value.data.string->value->chars, sizeof(char), value.data.string->value->length, file);
			return;
		
		case Value(object):
		case Value(date):
			Object.dumpTo(value.data.object, file);
			return;
		
		case Value(error):
		{
			struct Value name = toString(Object.get(&value.data.error->object, Key(name)), NULL);
			struct Value message = toString(Object.get(&value.data.error->object, Key(message)), NULL);
			fwrite(Value.stringChars(name), sizeof(char), Value.stringLength(name), file);
			fputs(": ", file);
			fwrite(Value.stringChars(message), sizeof(char), Value.stringLength(message), file);
			return;
		}
		
		case Value(function):
			fwrite(value.data.function->text.location, sizeof(char), value.data.function->text.length, file);
			return;
		
		case Value(reference):
			fputs("-> ", file);
			dumpTo(*value.data.reference, file);
			return;
	}
}
