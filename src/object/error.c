//
//  error.c
//  libecc
//
//  Created by Bouilland Aurélien on 27/06/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#include "error.h"

// MARK: - Private

static struct Object *errorPrototype = NULL;
static struct Object *errorConstructor = NULL;

static struct Object *rangeErrorPrototype = NULL;
static struct Object *referenceErrorPrototype = NULL;
static struct Object *syntaxErrorPrototype = NULL;
static struct Object *typeErrorPrototype = NULL;
static struct Object *uriErrorPrototype = NULL;

static struct Value toString (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 0);
	
	ecc->this = Value.toObject(ecc->this, ecc, &(*ops)->text);
	
	struct Object *object = ecc->this.data.object;
	struct Value name = Value.toString(Object.get(object, Identifier.name()));
	struct Value message = Value.toString(Object.get(object, Identifier.message()));
	ecc->result = Value.chars(Chars.create("%.*s: %.*s", Value.stringLength(name), Value.stringChars(name), Value.stringLength(message), Value.stringChars(message)));
	return Value.undefined();
}

// MARK: - Static Members

Instance createVA (struct Object *errorPrototype, struct Text text, const char *format, va_list ap)
{
	Instance self = malloc(sizeof(*self));
	assert(self);
	
	Object.initialize(&self->object, errorPrototype);
	
	if (!format)
		return self;
	
	Object.add(&self->object, Identifier.message(), Value.chars(Chars.createVA(format, ap)), Object(writable) | Object(configurable));
	
	self->text = text;
	
	return self;
}

static void initName(struct Object *object, const struct Text *text)
{
	Object.add(object, Identifier.name(), Value.text(text), Object(writable) | Object(configurable));
}

static struct Object *createErrorType (const struct Text *text)
{
	struct Object *object = Object.create(errorPrototype);
	initName(object, text);
	return object;
}

// MARK: - Methods

void setup (void)
{
	errorPrototype = Object.create(Object.prototype());
	Object.setType(errorPrototype, Text.errorType());
	initName(errorPrototype, Text.errorName());
	
	rangeErrorPrototype = createErrorType(Text.rangeErrorName());
	referenceErrorPrototype = createErrorType(Text.referenceErrorName());
	syntaxErrorPrototype = createErrorType(Text.syntaxErrorName());
	typeErrorPrototype = createErrorType(Text.typeErrorName());
	uriErrorPrototype = createErrorType(Text.uriErrorName());
}

struct Object *prototype (void)
{
	return errorPrototype;
}

struct Object *constructor (void)
{
	return errorConstructor;
}

Instance error (struct Text text, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	Instance self = createVA(errorPrototype, text, format, ap);
	va_end(ap);
	return self;
}

Instance rangeError (struct Text text, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	Instance self = createVA(rangeErrorPrototype, text, format, ap);
	va_end(ap);
	return self;
}

Instance referenceError (struct Text text, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	Instance self = createVA(referenceErrorPrototype, text, format, ap);
	va_end(ap);
	return self;
}

Instance syntaxError (struct Text text, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	Instance self = createVA(syntaxErrorPrototype, text, format, ap);
	va_end(ap);
	return self;
}

Instance typeError (struct Text text, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	Instance self = createVA(typeErrorPrototype, text, format, ap);
	va_end(ap);
	return self;
}

Instance uriError (struct Text text, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	Instance self = createVA(uriErrorPrototype, text, format, ap);
	va_end(ap);
	return self;
}

void destroy (Instance self)
{
	assert(self);
	if (!self)
		return;
	
	Object.finalize(&self->object);
	
	free(self), self = NULL;
}
