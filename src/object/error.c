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
	
	object = Value.toObject(ecc->this, ecc, &(*ops)->text).data.object;
	name = Value.toString(Object.get(object, Key(name)));
	message = Value.toString(Object.get(object, Key(message)));
	ecc->result = Value.chars(Chars.create("%.*s: %.*s", Value.stringLength(name), Value.stringChars(name), Value.stringLength(message), Value.stringChars(message)));
	return Value(undefined);
}

static struct Value errorConstructor (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value message;
	
	Op.assertParameterCount(ecc, 1);
	
	message = Value.toString(Op.argument(ecc, 0));
	ecc->result = Value.error(error((*ops)->text, "%.s*", Value.stringLength(message), Value.stringChars(message)));
	
	return Value(undefined);
}

static struct Value rangeErrorConstructor (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value message;
	
	Op.assertParameterCount(ecc, 1);
	
	message = Value.toString(Op.argument(ecc, 0));
	ecc->result = Value.error(rangeError((*ops)->text, "%.s*", Value.stringLength(message), Value.stringChars(message)));
	
	return Value(undefined);
}

static struct Value referenceErrorConstructor (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value message;
	
	Op.assertParameterCount(ecc, 1);
	
	message = Value.toString(Op.argument(ecc, 0));
	ecc->result = Value.error(referenceError((*ops)->text, "%.s*", Value.stringLength(message), Value.stringChars(message)));
	
	return Value(undefined);
}

static struct Value syntaxErrorConstructor (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value message;
	
	Op.assertParameterCount(ecc, 1);
	
	message = Value.toString(Op.argument(ecc, 0));
	ecc->result = Value.error(syntaxError((*ops)->text, "%.s*", Value.stringLength(message), Value.stringChars(message)));
	
	return Value(undefined);
}

static struct Value typeErrorConstructor (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value message;
	
	Op.assertParameterCount(ecc, 1);
	
	message = Value.toString(Op.argument(ecc, 0));
	ecc->result = Value.error(typeError((*ops)->text, "%.s*", Value.stringLength(message), Value.stringChars(message)));
	
	return Value(undefined);
}

static struct Value uriErrorConstructor (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value message;
	
	Op.assertParameterCount(ecc, 1);
	
	message = Value.toString(Op.argument(ecc, 0));
	ecc->result = Value.error(uriError((*ops)->text, "%.s*", Value.stringLength(message), Value.stringChars(message)));
	
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
	
	Error(constructor) = Function.createWithNative(Error(prototype), errorConstructor, 1);
	
	//
	
	Error(rangePrototype) = createErrorType(&Text(rangeErrorName));
	Error(rangeConstructor) = Function.createWithNative(Error(rangePrototype), rangeErrorConstructor, 1);
	
	Error(referencePrototype) = createErrorType(&Text(referenceErrorName));
	Error(referenceConstructor) = Function.createWithNative(Error(referencePrototype), referenceErrorConstructor, 1);
	
	Error(syntaxPrototype) = createErrorType(&Text(syntaxErrorName));
	Error(syntaxConstructor) = Function.createWithNative(Error(syntaxPrototype), syntaxErrorConstructor, 1);
	
	Error(typePrototype) = createErrorType(&Text(typeErrorName));
	Error(typeConstructor) = Function.createWithNative(Error(typePrototype), typeErrorConstructor, 1);
	
	Error(uriPrototype) = createErrorType(&Text(uriErrorName));
	Error(uriConstructor) = Function.createWithNative(Error(uriPrototype), uriErrorConstructor, 1);
}

void teardown (void)
{
//	Object.destroy(errorPrototype), errorPrototype = NULL;
//	
//	Object.destroy(rangeErrorPrototype), rangeErrorPrototype = NULL;
//	Object.destroy(referenceErrorPrototype), referenceErrorPrototype = NULL;
//	Object.destroy(syntaxErrorPrototype), syntaxErrorPrototype = NULL;
//	Object.destroy(typeErrorPrototype), typeErrorPrototype = NULL;
//	Object.destroy(uriErrorPrototype), uriErrorPrototype = NULL;
}

struct Error * error (struct Text text, const char *format, ...)
{
	struct Error *self;
	va_list ap;
	int16_t length;
	
	va_start(ap, format);
	length = vsnprintf(NULL, 0, format, ap);
	va_end(ap);
	
	va_start(ap, format);
	self = createVA(Error(prototype), text, Chars.createVA(length, format, ap));
	va_end(ap);
	
	return self;
}

struct Error * rangeError (struct Text text, const char *format, ...)
{
	struct Error *self;
	va_list ap;
	int16_t length;
	
	va_start(ap, format);
	length = vsnprintf(NULL, 0, format, ap);
	va_end(ap);
	
	va_start(ap, format);
	self = createVA(Error(rangePrototype), text, Chars.createVA(length, format, ap));
	va_end(ap);
	
	return self;
}

struct Error * referenceError (struct Text text, const char *format, ...)
{
	struct Error *self;
	va_list ap;
	int16_t length;
	
	va_start(ap, format);
	length = vsnprintf(NULL, 0, format, ap);
	va_end(ap);
	
	va_start(ap, format);
	self = createVA(Error(referencePrototype), text, Chars.createVA(length, format, ap));
	va_end(ap);
	
	return self;
}

struct Error * syntaxError (struct Text text, const char *format, ...)
{
	struct Error *self;
	va_list ap;
	int16_t length;
	
	va_start(ap, format);
	length = vsnprintf(NULL, 0, format, ap);
	va_end(ap);
	
	va_start(ap, format);
	self = createVA(Error(syntaxPrototype), text, Chars.createVA(length, format, ap));
	va_end(ap);
	return self;
}

struct Error * typeError (struct Text text, const char *format, ...)
{
	struct Error *self;
	va_list ap;
	int16_t length;
	
	va_start(ap, format);
	length = vsnprintf(NULL, 0, format, ap);
	va_end(ap);
	
	va_start(ap, format);
	self = createVA(Error(typePrototype), text, Chars.createVA(length, format, ap));
	va_end(ap);
	return self;
}

struct Error * uriError (struct Text text, const char *format, ...)
{
	struct Error *self;
	va_list ap;
	int16_t length;
	
	va_start(ap, format);
	length = vsnprintf(NULL, 0, format, ap);
	va_end(ap);
	
	va_start(ap, format);
	self = createVA(Error(uriPrototype), text, Chars.createVA(length, format, ap));
	va_end(ap);
	return self;
}

void destroy (struct Error *self)
{
	assert(self);
	if (!self)
		return;
	
	Object.finalize(&self->object);
	
	free(self), self = NULL;
}
