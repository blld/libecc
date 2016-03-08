//
//  error.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "error.h"

// MARK: - Private

struct Object * Error(prototype) = NULL;
struct Object * Error(rangePrototype) = NULL;
struct Object * Error(referencePrototype) = NULL;
struct Object * Error(syntaxPrototype) = NULL;
struct Object * Error(typePrototype) = NULL;
struct Object * Error(uriPrototype) = NULL;

struct Function * Error(constructor) = NULL;
struct Function * Error(rangeConstructor) = NULL;
struct Function * Error(referenceConstructor) = NULL;
struct Function * Error(syntaxConstructor) = NULL;
struct Function * Error(typeConstructor) = NULL;
struct Function * Error(uriConstructor) = NULL;

const struct Object(Type) Error(type) = {
	.text = &Text(errorType),
	.toLength = toLength,
	.toBytes = toBytes,
};

static struct Value messageValue (struct Value value)
{
	if (value.type == Value(undefinedType))
		return value;
	else
		return Value.toString(NULL, value);
}

static struct Error * create (struct Object *errorPrototype, struct Text text, struct Chars *message)
{
	struct Error *self = malloc(sizeof(*self));
	assert(self);
	Pool.addObject(&self->object);
	
	*self = Error.identity;
	
	Object.initialize(&self->object, errorPrototype);
	
	self->text = text;
	
	if (message)
		Object.add(&self->object, Key(message), Value.chars(message), Value(hidden));
	
	return self;
}

static struct Value toString (struct Native(Context) * const context)
{
	struct Chars *chars;
	uint16_t length;
	
	Native.assertParameterCount(context, 0);
	
	if (!Value.isObject(context->this))
		Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(Native.textSeek(context, Native(thisIndex)), "not an object")));
	
	length = toLength(context, context->this);
	chars = Chars.createSized(length);
	toBytes(context, context->this, chars->bytes);
	
	return Value.chars(chars);
}

static struct Value errorConstructor (struct Native(Context) * const context)
{
	struct Value message;
	
	Native.assertParameterCount(context, 1);
	
	message = messageValue(Native.argument(context, 0));
	return Value.error(error(Native.textSeek(context, Native(callIndex)), "%.*s", Value.stringLength(message), Value.stringBytes(message)));
}

static struct Value rangeErrorConstructor (struct Native(Context) * const context)
{
	struct Value message;
	
	Native.assertParameterCount(context, 1);
	
	message = messageValue(Native.argument(context, 0));
	return Value.error(rangeError(Native.textSeek(context, Native(callIndex)), "%.*s", Value.stringLength(message), Value.stringBytes(message)));
}

static struct Value referenceErrorConstructor (struct Native(Context) * const context)
{
	struct Value message;
	
	Native.assertParameterCount(context, 1);
	
	message = messageValue(Native.argument(context, 0));
	return Value.error(referenceError(Native.textSeek(context, Native(callIndex)), "%.*s", Value.stringLength(message), Value.stringBytes(message)));
}

static struct Value syntaxErrorConstructor (struct Native(Context) * const context)
{
	struct Value message;
	
	Native.assertParameterCount(context, 1);
	
	message = messageValue(Native.argument(context, 0));
	return Value.error(syntaxError(Native.textSeek(context, Native(callIndex)), "%.*s", Value.stringLength(message), Value.stringBytes(message)));
}

static struct Value typeErrorConstructor (struct Native(Context) * const context)
{
	struct Value message;
	
	Native.assertParameterCount(context, 1);
	
	message = messageValue(Native.argument(context, 0));
	return Value.error(typeError(Native.textSeek(context, Native(callIndex)), "%.*s", Value.stringLength(message), Value.stringBytes(message)));
}

static struct Value uriErrorConstructor (struct Native(Context) * const context)
{
	struct Value message;
	
	Native.assertParameterCount(context, 1);
	
	message = messageValue(Native.argument(context, 0));
	return Value.error(uriError(Native.textSeek(context, Native(callIndex)), "%.*s", Value.stringLength(message), Value.stringBytes(message)));
}

static void setupBuiltinObject (struct Function **constructor, const Native(Function) native, int parameterCount, struct Object **prototype, const struct Text *name)
{
	Function.setupBuiltinObject(constructor, native, 1, prototype, Value.error(error(*name, NULL)), &Error(type));
	Object.add(*prototype, Key(name), Value.text(name), Value(hidden));
}

// MARK: - Static Members

// MARK: - Methods

void setup (void)
{
	const enum Value(Flags) flags = Value(hidden);
	
	setupBuiltinObject(&Error(constructor), errorConstructor, 1, &Error(prototype), &Text(errorName));
	setupBuiltinObject(&Error(rangeConstructor), rangeErrorConstructor, 1, &Error(rangePrototype), &Text(rangeErrorName));
	setupBuiltinObject(&Error(referenceConstructor), referenceErrorConstructor, 1, &Error(referencePrototype), &Text(referenceErrorName));
	setupBuiltinObject(&Error(syntaxConstructor), syntaxErrorConstructor, 1, &Error(syntaxPrototype), &Text(syntaxErrorName));
	setupBuiltinObject(&Error(typeConstructor), typeErrorConstructor, 1, &Error(typePrototype), &Text(typeErrorName));
	setupBuiltinObject(&Error(uriConstructor), uriErrorConstructor, 1, &Error(uriPrototype), &Text(uriErrorName));
	
	Function.addToObject(Error(prototype), "toString", toString, 0, flags);
}

void teardown (void)
{
	Error(prototype) = NULL;
	Error(constructor) = NULL;
	
	Error(rangePrototype) = NULL;
	Error(rangeConstructor) = NULL;
	
	Error(referencePrototype) = NULL;
	Error(referenceConstructor) = NULL;
	
	Error(syntaxPrototype) = NULL;
	Error(syntaxConstructor) = NULL;
	
	Error(typePrototype) = NULL;
	Error(typeConstructor) = NULL;
	
	Error(uriPrototype) = NULL;
	Error(uriConstructor) = NULL;
}

struct Error * error (struct Text text, const char *format, ...)
{
	struct Chars *chars = NULL;
	if (format)
	{
		va_list ap;
		int16_t length;
		
		va_start(ap, format);
		length = vsnprintf(NULL, 0, format, ap);
		va_end(ap);
		
		va_start(ap, format);
		chars = Chars.createVA(length, format, ap);
		va_end(ap);
	}
	return create(Error(prototype), text, chars);
}

struct Error * rangeError (struct Text text, const char *format, ...)
{
	struct Chars *chars = NULL;
	if (format)
	{
		va_list ap;
		int16_t length;
		
		va_start(ap, format);
		length = vsnprintf(NULL, 0, format, ap);
		va_end(ap);
		
		va_start(ap, format);
		chars = Chars.createVA(length, format, ap);
		va_end(ap);
	}
	return create(Error(rangePrototype), text, chars);
}

struct Error * referenceError (struct Text text, const char *format, ...)
{
	struct Chars *chars = NULL;
	if (format)
	{
		va_list ap;
		int16_t length;
		
		va_start(ap, format);
		length = vsnprintf(NULL, 0, format, ap);
		va_end(ap);
		
		va_start(ap, format);
		chars = Chars.createVA(length, format, ap);
		va_end(ap);
	}
	return create(Error(referencePrototype), text, chars);
}

struct Error * syntaxError (struct Text text, const char *format, ...)
{
	struct Chars *chars = NULL;
	if (format)
	{
		va_list ap;
		int16_t length;
		
		va_start(ap, format);
		length = vsnprintf(NULL, 0, format, ap);
		va_end(ap);
		
		va_start(ap, format);
		chars = Chars.createVA(length, format, ap);
		va_end(ap);
	}
	return create(Error(syntaxPrototype), text, chars);
}

struct Error * typeError (struct Text text, const char *format, ...)
{
	struct Chars *chars = NULL;
	if (format)
	{
		va_list ap;
		int16_t length;
		
		va_start(ap, format);
		length = vsnprintf(NULL, 0, format, ap);
		va_end(ap);
		
		va_start(ap, format);
		chars = Chars.createVA(length, format, ap);
		va_end(ap);
	}
	return create(Error(typePrototype), text, chars);
}

struct Error * uriError (struct Text text, const char *format, ...)
{
	struct Chars *chars = NULL;
	if (format)
	{
		va_list ap;
		int16_t length;
		
		va_start(ap, format);
		length = vsnprintf(NULL, 0, format, ap);
		va_end(ap);
		
		va_start(ap, format);
		chars = Chars.createVA(length, format, ap);
		va_end(ap);
	}
	return create(Error(uriPrototype), text, chars);
}

void destroy (struct Error *self)
{
	assert(self);
	if (!self)
		return;
	
	Object.finalize(&self->object);
	
	free(self), self = NULL;
}

uint16_t toLength (struct Native(Context) * const context, struct Value value)
{
	struct Value name, message;
	struct Object *self;
	
	assert(value.type == Value(errorType));
	assert(value.data.error);
	
	self = value.data.object;
	
	name = Object.get(self, Key(name));
	if (name.type == Value(undefinedType))
		name = Value.text(&Text(errorName));
	else
		name = Value.toString(context, name);
	
	message = Object.get(self, Key(message));
	if (message.type == Value(undefinedType))
		message = Value.text(&Text(empty));
	else
		message = Value.toString(context, message);
	
	if (Value.stringLength(name) && Value.stringLength(message))
		return sizeof(": ")-1 + Value.stringLength(name) + Value.stringLength(message);
	else
		return Value.stringLength(name) + Value.stringLength(message);
}

uint16_t toBytes (struct Native(Context) * const context, struct Value value, char *bytes)
{
	struct Value name, message;
	uint16_t offset = 0;
	struct Object *self;
	
	assert(value.type == Value(errorType));
	assert(value.data.error);
	
	self = value.data.object;
	
	name = Object.get(self, Key(name));
	if (name.type == Value(undefinedType))
		name = Value.text(&Text(errorName));
	else
		name = Value.toString(context, name);
	
	message = Object.get(self, Key(message));
	if (message.type == Value(undefinedType))
		message = Value.text(&Text(empty));
	else
		message = Value.toString(context, message);
	
	memcpy(bytes, Value.stringBytes(name), Value.stringLength(name));
	offset += Value.stringLength(name);
	
	if (Value.stringLength(name) && Value.stringLength(message))
	{
		const char separator[] = ": ";
		memcpy(bytes + offset, separator, sizeof(separator)-1);
		offset += sizeof(separator)-1;
	}
	
	memcpy(bytes + offset, Value.stringBytes(message), Value.stringLength(message));
	offset += Value.stringLength(message);
	
	return offset;
}
