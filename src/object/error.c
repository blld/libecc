//
//  error.c
//  libecc
//
//  Created by Bouilland Aurélien on 27/06/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
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

static struct Value messageValue (struct Value value)
{
	if (value.type == Value(undefinedType))
		return value;
	else
		return Value.toString(value);
}

static struct Error * createVA (struct Object *errorPrototype, struct Text text, struct Chars *message)
{
	struct Error *self = malloc(sizeof(*self));
	assert(self);
	Pool.addObject(&self->object);
	
	Object.initialize(&self->object, errorPrototype);
	
	self->text = text;
	
	if (message)
		Object.add(&self->object, Key(message), Value.chars(message), Value(writable) | Value(configurable));
	
	return self;
}

static struct Value toString (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Object *object;
	struct Value name, message;
	
	Op.assertParameterCount(ecc, 0);
	
	if (!Value.isObject(ecc->this))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError((*ops)->text, "not an object")));
	
	object = ecc->this.data.object;
	
	name = Object.get(object, Key(name));
	if (name.type == Value(undefinedType))
		name = Value.text(&Text(errorName));
	else
		name = Value.toString(name);
	
	message = Object.get(object, Key(message));
	if (message.type == Value(undefinedType))
		message = Value.text(&Text(empty));
	else
		message = Value.toString(message);
	
	if (Value.stringLength(name) && Value.stringLength(message))
		ecc->result = Value.chars(Chars.create("%.*s: %.*s", Value.stringLength(name), Value.stringChars(name), Value.stringLength(message), Value.stringChars(message)));
	else
		ecc->result = Value.chars(Chars.create("%.*s%.*s", Value.stringLength(name), Value.stringChars(name), Value.stringLength(message), Value.stringChars(message)));
	
	return Value(undefined);
}

static struct Value errorConstructor (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value message;
	
	Op.assertParameterCount(ecc, 1);
	
	message = messageValue(Op.argument(ecc, 0));
	ecc->result = Value.error(error((*ops)->text, "%.*s", Value.stringLength(message), Value.stringChars(message)));
	
	return Value(undefined);
}

static struct Value rangeErrorConstructor (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value message;
	
	Op.assertParameterCount(ecc, 1);
	
	message = messageValue(Op.argument(ecc, 0));
	ecc->result = Value.error(rangeError((*ops)->text, "%.*s", Value.stringLength(message), Value.stringChars(message)));
	
	return Value(undefined);
}

static struct Value referenceErrorConstructor (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value message;
	
	Op.assertParameterCount(ecc, 1);
	
	message = messageValue(Op.argument(ecc, 0));
	ecc->result = Value.error(referenceError((*ops)->text, "%.*s", Value.stringLength(message), Value.stringChars(message)));
	
	return Value(undefined);
}

static struct Value syntaxErrorConstructor (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value message;
	
	Op.assertParameterCount(ecc, 1);
	
	message = messageValue(Op.argument(ecc, 0));
	ecc->result = Value.error(syntaxError((*ops)->text, "%.*s", Value.stringLength(message), Value.stringChars(message)));
	
	return Value(undefined);
}

static struct Value typeErrorConstructor (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value message;
	
	Op.assertParameterCount(ecc, 1);
	
	message = messageValue(Op.argument(ecc, 0));
	ecc->result = Value.error(typeError((*ops)->text, "%.*s", Value.stringLength(message), Value.stringChars(message)));
	
	return Value(undefined);
}

static struct Value uriErrorConstructor (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value message;
	
	Op.assertParameterCount(ecc, 1);
	
	message = messageValue(Op.argument(ecc, 0));
	ecc->result = Value.error(uriError((*ops)->text, "%.*s", Value.stringLength(message), Value.stringChars(message)));
	
	return Value(undefined);
}

static void initName(struct Object *object, const struct Text *text)
{
	Object.add(object, Key(name), Value.text(text), Value(writable) | Value(configurable));
}

static struct Object *createErrorType (const struct Text *text)
{
	struct Object *object = Object.create(Error(prototype));
	initName(object, text);
	return object;
}

// MARK: - Static Members

// MARK: - Methods

void setup (void)
{
	const enum Value(Flags) flags = Value(writable) | Value(configurable);
	
	Error(prototype) = Object.createTyped(&Text(errorType));
	initName(Error(prototype), &Text(errorName));
	Function.addToObject(Error(prototype), "toString", toString, 1, flags);
	
	Error(constructor) = Function.createWithNative(errorConstructor, 1);
	Function.linkPrototype(Error(constructor), Error(prototype), 0);
	
	//
	
	Error(rangePrototype) = createErrorType(&Text(rangeErrorName));
	Error(rangeConstructor) = Function.createWithNative(rangeErrorConstructor, 1);
	Function.linkPrototype(Error(rangeConstructor), Error(rangePrototype), 0);
	
	Error(referencePrototype) = createErrorType(&Text(referenceErrorName));
	Error(referenceConstructor) = Function.createWithNative(referenceErrorConstructor, 1);
	Function.linkPrototype(Error(referenceConstructor), Error(referencePrototype), 0);
	
	Error(syntaxPrototype) = createErrorType(&Text(syntaxErrorName));
	Error(syntaxConstructor) = Function.createWithNative(syntaxErrorConstructor, 1);
	Function.linkPrototype(Error(syntaxConstructor), Error(syntaxPrototype), 0);
	
	Error(typePrototype) = createErrorType(&Text(typeErrorName));
	Error(typeConstructor) = Function.createWithNative(typeErrorConstructor, 1);
	Function.linkPrototype(Error(typeConstructor), Error(typePrototype), 0);
	
	Error(uriPrototype) = createErrorType(&Text(uriErrorName));
	Error(uriConstructor) = Function.createWithNative(uriErrorConstructor, 1);
	Function.linkPrototype(Error(uriConstructor), Error(uriPrototype), 0);
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
	return createVA(Error(prototype), text, chars);
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
	return createVA(Error(rangePrototype), text, chars);
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
	return createVA(Error(referencePrototype), text, chars);
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
	return createVA(Error(syntaxPrototype), text, chars);
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
	return createVA(Error(typePrototype), text, chars);
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
	return createVA(Error(uriPrototype), text, chars);
}

void destroy (struct Error *self)
{
	assert(self);
	if (!self)
		return;
	
	Object.finalize(&self->object);
	
	free(self), self = NULL;
}
