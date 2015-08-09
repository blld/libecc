//
//  value.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "value.h"

// MARK: - Private

static struct Text nullText = { 4, "null" };
static struct Text undefinedText = { 9, "undefined" };
static struct Text trueText = { 4, "true" };
static struct Text falseText = { 5, "false" };
static struct Text booleanText = { 7, "boolean" };
static struct Text numberText = { 6, "number" };
static struct Text stringText = { 6, "string" };
static struct Text objectText = { 6, "object" };
static struct Text functionText = { 8, "function" };
static struct Text zeroText = { 1, "0" };
static struct Text oneText = { 1, "1" };

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

Structure closure (struct Closure *closure)
{
	assert(closure);
	
	return (Structure){
		.data.closure = closure,
		.type = Value(closure),
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
	if (aClosure.type == Value(closure))
	{
		struct Value result = Op.callClosure(aClosure.data.closure, ecc, value, 0);
		if (isPrimitive(result))
			return result;
	}
	
	struct Value bClosure = Object.get(object, bIdentifier);
	if (bClosure.type == Value(closure))
	{
		struct Value result = Op.callClosure(bClosure.data.closure, ecc, value, 0);
		if (isPrimitive(result))
			return result;
	}
	
	Ecc.throw(ecc, Error.typeError(*text, "cannot convert %.*s to primitive", text->length, text->location));
}

int isPrimitive (Structure value)
{
	return value.type < Value(object);
}

int isBoolean (Structure value)
{
	return value.type & 0x01;
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
			return Value.text(&nullText);
		
		case Value(undefined):
			return Value.text(&undefinedText);
		
		case Value(false):
			return Value.text(&falseText);
		
		case Value(true):
			return Value.text(&trueText);
		
		case Value(integer):
			if (value.data.integer == 0)
				return Value.text(&zeroText);
			else if (value.data.integer == 1)
				return Value.text(&oneText);
			else
				return Value.chars(Chars.create("%d", value.data.integer));
		
		case Value(binary):
			if (value.data.binary == 0)
				return Value.text(&zeroText);
			else if (value.data.binary == 1)
				return Value.text(&oneText);
			else
				return Value.chars(Chars.create("%lg", value.data.binary));
		
		case Value(identifier):
			return Value.text(Identifier.textOf(value.data.identifier));
		
		case Value(object):
			return Value.text(value.data.object->type);
		
		case Value(closure):
			return Value.text(&value.data.closure->text);
		
		case Value(date):
		case Value(error):
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
		
		case Value(identifier):
		case Value(text):
		case Value(chars):
		case Value(object):
		case Value(error):
		case Value(string):
		case Value(date):
		case Value(closure):
			#warning TODO
		
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
			Ecc.throw(ecc, Error.typeError(*text, "%.*s is null", text->length, text->location));
		
		case Value(undefined):
			Ecc.throw(ecc, Error.typeError(*text, "%.*s is undefined", text->length, text->location));
		
		case Value(false):
		case Value(true):
		case Value(integer):
		case Value(binary):
		case Value(text):
		case Value(chars):
		#warning TODO create object
			return Value.undefined();
		
		case Value(closure):
		case Value(object):
		case Value(error):
		case Value(string):
		case Value(date):
			return value;
		
		case Value(identifier):
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
			return Value.text(&booleanText);
		
		case Value(undefined):
			return Value.text(&undefinedText);
		
		case Value(integer):
		case Value(binary):
			return Value.text(&numberText);
		
		case Value(identifier):
		case Value(text):
		case Value(chars):
			return Value.text(&stringText);
		
		case Value(null):
		case Value(object):
		case Value(string):
		case Value(error):
		case Value(date):
			return Value.text(&objectText);
		
		case Value(closure):
			return Value.text(&functionText);
		
		case Value(breaker):
		case Value(reference):
			assert(0);
			exit(EXIT_FAILURE);
	}
}

void finalize (Instance self)
{
	assert(self);
	assert(self->type != Value(breaker));
	assert(self->type != Value(reference));
	
	switch (self->type)
	{
		case Value(null):
		case Value(false):
		case Value(undefined):
		case Value(true):
		case Value(integer):
		case Value(binary):
		case Value(identifier):
		case Value(text):
			break;
		
		case Value(chars):
			Chars.destroy(self->data.chars);
			break;
		
		case Value(object):
		case Value(date):
			Object.destroy(self->data.object);
			break;
		
		case Value(error):
			Error.destroy(self->data.error);
			break;
		
		case Value(string):
			String.destroy(self->data.string);
			break;
		
		case Value(closure):
			Closure.destroy(self->data.closure);
			break;
		
		case Value(breaker):
		case Value(reference):
			assert(0);
			exit(EXIT_FAILURE);
	}
	
	self->type = Value(undefined);
}

void dumpTo (Structure value, FILE *file)
{
	switch (value.type)
	{
		case Value(null):
			fprintf(file, "null");
			return;
		
		case Value(undefined):
			fprintf(file, "undefined");
			return;
		
		case Value(false):
			fprintf(file, "false");
			return;
		
		case Value(true):
			fprintf(file, "true");
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
			fprintf(file, "%.*s", text->length, text->location);
			return;
		}
		
		case Value(text):
			fprintf(file, "%.*s", value.data.text->length, value.data.text->location);
			return;
		
		case Value(chars):
			fprintf(file, "%.*s", value.data.chars->length, value.data.chars->chars);
			return;
		
		case Value(string):
			fprintf(file, "%.*s", value.data.string->length, value.data.string->chars);
			return;
		
		case Value(object):
		case Value(date):
			Object.dumpTo(value.data.object, file);
			return;
		
		case Value(error):
		{
			struct Value name = toString(Object.get(&value.data.error->object, Identifier.name()));
			struct Value message = toString(Object.get(&value.data.error->object, Identifier.message()));
			fprintf(file, "%.*s: %.*s", Value.stringLength(name), Value.stringChars(name), Value.stringLength(message), Value.stringChars(message));
			return;
		}
		
		case Value(closure):
			fprintf(file, "%.*s", value.data.closure->text.length, value.data.closure->text.location);
			return;
		
		case Value(reference):
			fprintf(file, "-> ");
			dumpTo(*value.data.reference, file);
			return;
	}
}
