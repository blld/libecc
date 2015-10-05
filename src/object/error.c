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
static struct Object *rangeErrorPrototype = NULL;
static struct Object *referenceErrorPrototype = NULL;
static struct Object *syntaxErrorPrototype = NULL;
static struct Object *typeErrorPrototype = NULL;
static struct Object *uriErrorPrototype = NULL;

static struct Value toString (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Object *object;
	struct Value name, message;
	
	Op.assertParameterCount(ecc, 0);
	
	object = Value.toObject(ecc->this, ecc, &(*ops)->text).data.object;
	name = Value.toString(Object.get(object, Identifier.name()));
	message = Value.toString(Object.get(object, Identifier.message()));
	ecc->result = Value.chars(Chars.create("%.*s: %.*s", Value.stringLength(name), Value.stringChars(name), Value.stringLength(message), Value.stringChars(message)));
	return Value.undefined();
}

// MARK: - Static Members

static Instance createVA (struct Object *errorPrototype, struct Text text, struct Chars *message)
{
	Instance self = malloc(sizeof(*self));
	assert(self);
	Pool.addObject(&self->object);
	
	Object.initialize(&self->object, errorPrototype);
	
	self->text = text;
	
	if (message)
		Object.add(&self->object, Identifier.message(), Value.chars(message), Object(writable) | Object(configurable));
	
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
	errorPrototype->type = Text.errorType();
	initName(errorPrototype, Text.errorName());
	
	rangeErrorPrototype = createErrorType(Text.rangeErrorName());
	referenceErrorPrototype = createErrorType(Text.referenceErrorName());
	syntaxErrorPrototype = createErrorType(Text.syntaxErrorName());
	typeErrorPrototype = createErrorType(Text.typeErrorName());
	uriErrorPrototype = createErrorType(Text.uriErrorName());
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

struct Object *prototype (void)
{
	return errorPrototype;
}

struct Object *rangePrototype (void)
{
	return rangeErrorPrototype;
}

struct Object *referencePrototype (void)
{
	return referenceErrorPrototype;
}

struct Object *syntaxPrototype (void)
{
	return syntaxErrorPrototype;
}

struct Object *typePrototype (void)
{
	return typeErrorPrototype;
}

struct Object *uriPrototype (void)
{
	return uriErrorPrototype;
}

Instance error (struct Text text, const char *format, ...)
{
	Instance self;
	va_list ap;
	int16_t length;
	
	va_start(ap, format);
	length = vsnprintf(NULL, 0, format, ap);
	va_end(ap);
	
	va_start(ap, format);
	self = createVA(errorPrototype, text, Chars.createVA(length, format, ap));
	va_end(ap);
	
	return self;
}

Instance rangeError (struct Text text, const char *format, ...)
{
	Instance self;
	va_list ap;
	int16_t length;
	
	va_start(ap, format);
	length = vsnprintf(NULL, 0, format, ap);
	va_end(ap);
	
	va_start(ap, format);
	self = createVA(rangeErrorPrototype, text, Chars.createVA(length, format, ap));
	va_end(ap);
	
	return self;
}

Instance referenceError (struct Text text, const char *format, ...)
{
	Instance self;
	va_list ap;
	int16_t length;
	
	va_start(ap, format);
	length = vsnprintf(NULL, 0, format, ap);
	va_end(ap);
	
	va_start(ap, format);
	self = createVA(referenceErrorPrototype, text, Chars.createVA(length, format, ap));
	va_end(ap);
	
	return self;
}

Instance syntaxError (struct Text text, const char *format, ...)
{
	Instance self;
	va_list ap;
	int16_t length;
	
	va_start(ap, format);
	length = vsnprintf(NULL, 0, format, ap);
	va_end(ap);
	
	va_start(ap, format);
	self = createVA(syntaxErrorPrototype, text, Chars.createVA(length, format, ap));
	va_end(ap);
	return self;
}

Instance typeError (struct Text text, const char *format, ...)
{
	Instance self;
	va_list ap;
	int16_t length;
	
	va_start(ap, format);
	length = vsnprintf(NULL, 0, format, ap);
	va_end(ap);
	
	va_start(ap, format);
	self = createVA(typeErrorPrototype, text, Chars.createVA(length, format, ap));
	va_end(ap);
	return self;
}

Instance uriError (struct Text text, const char *format, ...)
{
	Instance self;
	va_list ap;
	int16_t length;
	
	va_start(ap, format);
	length = vsnprintf(NULL, 0, format, ap);
	va_end(ap);
	
	va_start(ap, format);
	self = createVA(uriErrorPrototype, text, Chars.createVA(length, format, ap));
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
