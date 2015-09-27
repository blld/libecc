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

Structure undefined (void)
{
	return (Structure){
		.type = Value(undefined),
	};
}

Structure null (void)
{
	return (Structure){
		.type = Value(null),
	};
}

Structure true (void)
{
	return (Structure){
		.type = Value(true),
	};
}

Structure false (void)
{
	return (Structure){
		.type = Value(false),
	};
}

Structure boolean (int boolean)
{
	return (Structure){
		.type = boolean? Value(true): Value(false),
	};
}

Structure integer (int32_t integer)
{
	return (Structure){
		.data.integer = integer,
		.type = Value(integer),
	};
}

Structure binary (double binary)
{
	return (Structure){
		.data.binary = binary,
		.type = Value(binary),
	};
}

Structure identifier (struct Identifier identifier)
{
	return (Structure){
		.data.identifier = identifier,
		.type = Value(identifier),
	};
}

Structure text (const struct Text *text)
{
	return (Structure){
		.data.text = text,
		.type = Value(text),
	};
}

Structure chars (struct Chars *chars)
{
	return (Structure){
		.data.chars = chars,
		.type = Value(chars),
	};
}

Structure object (struct Object *object)
{
	assert(object);
	
	return (Structure){
		.data.object = object,
		.type = Value(object),
	};
}

Structure error (struct Error *error)
{
	assert(error);
	
	return (Structure){
		.data.error = error,
		.type = Value(error),
	};
}

Structure string (struct String *string)
{
	assert(string);
	
	return (Structure){
		.data.string = string,
		.type = Value(string),
	};
}

Structure date (struct Date *date)
{
	assert(string);
	
	return (Structure){
		.data.date = date,
		.type = Value(date),
	};
}

Structure function (struct Function *function)
{
	assert(function);
	
	return (Structure){
		.data.function = function,
		.type = Value(function),
	};
}

Structure breaker (int32_t integer)
{
	return (Structure){
		.data.integer = integer,
		.type = Value(breaker),
	};
}

Structure reference (Instance self)
{
	return (Structure){
		.data.reference = self,
		.type = Value(reference),
	};
}

Structure toPrimitive (Structure value, struct Ecc *ecc, const struct Text *text, int hint)
{
	if (value.type < Value(object))
		return value;
	
	struct Object *object = value.data.object;
	if (!hint)
		hint = value.type == Value(date)? 1: -1;
	
	struct Identifier aIdentifier = hint > 0? Identifier.toString(): Identifier.valueOf();
	struct Identifier bIdentifier = hint > 0? Identifier.valueOf(): Identifier.toString();
	
	struct Value aClosure = Object.get(object, aIdentifier);
	if (aClosure.type == Value(function))
	{
		struct Value result = Op.callFunctionVA(aClosure.data.function, ecc, value, 0);
		if (isPrimitive(result))
			return result;
	}
	
	struct Value bClosure = Object.get(object, bIdentifier);
	if (bClosure.type == Value(function))
	{
		struct Value result = Op.callFunctionVA(bClosure.data.function, ecc, value, 0);
		if (isPrimitive(result))
			return result;
	}
	
	Ecc.jmpEnv(ecc, Value.error(Error.typeError(*text, "cannot convert %.*s to primitive", text->length, text->location)));
}

int isPrimitive (Structure value)
{
	return value.type < Value(object);
}

int isBoolean (Structure value)
{
	return value.type & 0x01;
}

int isDynamic (Structure value)
{
	return value.type >= Value(chars);
}

int isTrue (Structure value)
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

Structure toString (Structure value)
{
	switch (value.type)
	{
		case Value(text):
		case Value(chars):
		case Value(string):
			return value;
		
		case Value(null):
			return Value.text(Text.null());
		
		case Value(undefined):
			return Value.text(Text.undefined());
		
		case Value(false):
			return Value.text(Text.false());
		
		case Value(true):
			return Value.text(Text.true());
		
		case Value(integer):
			if (value.data.integer == 0)
				return Value.text(Text.zero());
			else if (value.data.integer == 1)
				return Value.text(Text.one());
			else
				return Value.chars(Chars.create("%d", value.data.integer));
		
		case Value(binary):
			if (value.data.binary == 0)
				return Value.text(Text.zero());
			else if (value.data.binary == 1)
				return Value.text(Text.one());
			else if (isnan(value.data.binary))
				return Value.text(Text.NaN());
			else if (isinf(value.data.binary))
			{
				if (signbit(value.data.binary))
					return Value.text(Text.negativeInfinity());
				else
					return Value.text(Text.Infinity());
			}
			else
				return Value.chars(Chars.create("%lg", value.data.binary));
		
		case Value(identifier):
			return Value.text(Identifier.textOf(value.data.identifier));
		
		case Value(object):
			return Value.text(value.data.object->type);
		
		case Value(function):
			return Value.text(&value.data.function->text);
		
		case Value(error):
		{
			struct Object *object = value.data.object;
			struct Value name = Value.toString(Object.get(object, Identifier.name()));
			struct Value message = Value.toString(Object.get(object, Identifier.message()));
			return Value.chars(Chars.create("%.*s: %.*s", Value.stringLength(name), Value.stringChars(name), Value.stringLength(message), Value.stringChars(message)));
		}
		
		case Value(date):
		case Value(breaker):
		case Value(reference):
			assert(0);
			exit(EXIT_FAILURE);
	}
}

int isString (Structure value)
{
	return value.type & 0x20;
}

const char * stringChars (Structure value)
{
	if (value.type == Value(chars))
		return value.data.chars->chars;
	else if (value.type == Value(text))
		return value.data.text->location;
	else if (value.type == Value(string))
		return value.data.string->chars;
	else
		return NULL;
}

uint16_t stringLength (Structure value)
{
	if (value.type == Value(chars))
		return value.data.chars->length;
	else if (value.type == Value(text))
		return value.data.text->length;
	else if (value.type == Value(string))
		return value.data.string->length;
	else
		return 0;
}

Structure toBinary (Structure value)
{
	switch (value.type)
	{
		case Value(binary):
			return value;
		
		case Value(integer):
			return Value.binary(value.data.integer);
		
		case Value(null):
		case Value(false):
			return Value.binary(0);
		
		case Value(true):
			return Value.binary(1);
		
		case Value(undefined):
			return Value.binary(NAN);
		
		case Value(text):
		{
			if (value.data.text == Text.zero())
				return Value.binary(0);
			else if (value.data.text == Text.one())
				return Value.binary(1);
			else if (value.data.text == Text.NaN())
				return Value.binary(NAN);
			else if (value.data.text == Text.Infinity())
				return Value.binary(INFINITY);
			else if (value.data.text == Text.negativeInfinity())
				return Value.binary(-INFINITY);
			
			/* fallthrought */
		}
		
		case Value(identifier):
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
			assert(0);
			exit(EXIT_FAILURE);
	}
}

Structure toInteger (Structure value)
{
	value = toBinary(value);
	
	if (isnan(value.data.binary) || isinf(value.data.binary))
		return Value.integer(0);
	else
		return Value.integer(value.data.binary);
}

int isNumber (Structure value)
{
	return value.type & 0x10;
}

Structure toObject (Structure value, struct Ecc *ecc, const struct Text *text)
{
	switch (value.type)
	{
		case Value(null):
			Ecc.jmpEnv(ecc, Value.error(Error.typeError(*text, "%.*s is null", text->length, text->location)));
		
		case Value(undefined):
			Ecc.jmpEnv(ecc, Value.error(Error.typeError(*text, "%.*s is undefined", text->length, text->location)));
		
		case Value(identifier):
		case Value(text):
		case Value(chars):
			return Value.string(String.create("%.*s", Value.stringLength(value), Value.stringChars(value)));
		
		case Value(function):
		case Value(object):
		case Value(error):
		case Value(string):
		case Value(date):
			return value;
		
		case Value(integer):
		case Value(binary):
		case Value(false):
		case Value(true):
		case Value(breaker):
		case Value(reference):
			assert(0);
			exit(EXIT_FAILURE);
	}
}

int isObject (Structure value)
{
	return value.type >= Value(object);
}

Structure toType (Structure value)
{
	switch (value.type)
	{
		case Value(true):
		case Value(false):
			return Value.text(Text.boolean());
		
		case Value(undefined):
			return Value.text(Text.undefined());
		
		case Value(integer):
		case Value(binary):
			return Value.text(Text.number());
		
		case Value(identifier):
		case Value(text):
		case Value(chars):
			return Value.text(Text.string());
		
		case Value(null):
		case Value(object):
		case Value(string):
		case Value(error):
		case Value(date):
			return Value.text(Text.object());
		
		case Value(function):
			return Value.text(Text.function());
		
		case Value(breaker):
		case Value(reference):
			assert(0);
			exit(EXIT_FAILURE);
	}
}

void dumpTo (Structure value, FILE *file)
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
		
		case Value(integer):
			fprintf(file, "%d", value.data.integer);
			return;
		
		case Value(breaker):
			fprintf(file, "[[breaker:%d]]", value.data.integer);
			return;
		
		case Value(binary):
			fprintf(file, "%lf", value.data.binary);
			return;
		
		case Value(identifier):
		{
			struct Text *text = Identifier.textOf(value.data.identifier);
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
			fwrite(value.data.string->chars, sizeof(char), value.data.string->length, file);
			return;
		
		case Value(object):
		case Value(date):
			Object.dumpTo(value.data.object, file);
			return;
		
		case Value(error):
		{
			struct Value name = toString(Object.get(&value.data.error->object, Identifier.name()));
			struct Value message = toString(Object.get(&value.data.error->object, Identifier.message()));
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
