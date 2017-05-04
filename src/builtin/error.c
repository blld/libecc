//
//  error.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#define Implementation
#include "error.h"

#include "../pool.h"

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
};

static struct Chars *messageValue (struct Context * const context, struct Value value)
{
	if (value.type == Value(undefinedType))
		return NULL;
	else if (value.type == Value(charsType))
		return value.data.chars;
	else
	{
		value = Value.toString(NULL, value);
		return Chars.create("%.*s", Value.stringLength(value), Value.stringBytes(value));
	}
}

static struct Chars * toChars (struct Context * const context, struct Value value)
{
	struct Value name, message;
	struct Object *self;
	struct Chars *chars;
	
	assert(value.type == Value(errorType));
	assert(value.data.error);
	
	self = value.data.object;
	
	name = Object.getMember(self, Key(name), context);
	if (name.type == Value(undefinedType))
		name = Value.text(&Text(errorName));
	else
		name = Value.toString(context, name);
	
	message = Object.getMember(self, Key(message), context);
	if (message.type == Value(undefinedType))
		message = Value.text(&Text(empty));
	else
		message = Value.toString(context, message);
	
	Chars.beginAppend(&chars);
	Chars.appendValue(&chars, context, name);
	
	if (Value.stringLength(name) && Value.stringLength(message))
		Chars.append(&chars, ": ");
	
	Chars.appendValue(&chars, context, message);
	
	return Chars.endAppend(&chars);
}

static struct Error * create (struct Object *errorPrototype, struct Text text, struct Chars *message)
{
	struct Error *self = malloc(sizeof(*self));
	Pool.addObject(&self->object);
	
	*self = Error.identity;
	
	Object.initialize(&self->object, errorPrototype);
	
	self->text = text;
	
	if (message)
	{
		Object.addMember(&self->object, Key(message), Value.chars(message), Value(hidden));
		++message->referenceCount;
	}
	
	return self;
}

static struct Value toString (struct Context * const context)
{
	Context.assertParameterCount(context, 0);
	Context.assertThisMask(context, Value(objectMask));
	
	return Value.chars(toChars(context, context->this));
}

static struct Value errorConstructor (struct Context * const context)
{
	struct Chars *message;
	
	Context.assertParameterCount(context, 1);
	
	message = messageValue(context, Context.argument(context, 0));
	Context.setTextIndex(context, Context(callIndex));
	return Value.error(error(Context.textSeek(context), message));
}

static struct Value rangeErrorConstructor (struct Context * const context)
{
	struct Chars *message;
	
	Context.assertParameterCount(context, 1);
	
	message = messageValue(context, Context.argument(context, 0));
	Context.setTextIndex(context, Context(callIndex));
	return Value.error(rangeError(Context.textSeek(context), message));
}

static struct Value referenceErrorConstructor (struct Context * const context)
{
	struct Chars *message;
	
	Context.assertParameterCount(context, 1);
	
	message = messageValue(context, Context.argument(context, 0));
	Context.setTextIndex(context, Context(callIndex));
	return Value.error(referenceError(Context.textSeek(context), message));
}

static struct Value syntaxErrorConstructor (struct Context * const context)
{
	struct Chars *message;
	
	Context.assertParameterCount(context, 1);
	
	message = messageValue(context, Context.argument(context, 0));
	Context.setTextIndex(context, Context(callIndex));
	return Value.error(syntaxError(Context.textSeek(context), message));
}

static struct Value typeErrorConstructor (struct Context * const context)
{
	struct Chars *message;
	
	Context.assertParameterCount(context, 1);
	
	message = messageValue(context, Context.argument(context, 0));
	Context.setTextIndex(context, Context(callIndex));
	return Value.error(typeError(Context.textSeek(context), message));
}

static struct Value uriErrorConstructor (struct Context * const context)
{
	struct Chars *message;
	
	Context.assertParameterCount(context, 1);
	
	message = messageValue(context, Context.argument(context, 0));
	Context.setTextIndex(context, Context(callIndex));
	return Value.error(uriError(Context.textSeek(context), message));
}

static void setupBuiltinObject (struct Function **constructor, const Native(Function) native, int parameterCount, struct Object **prototype, const struct Text *name)
{
	Function.setupBuiltinObject(constructor, native, 1, prototype, Value.error(error(*name, NULL)), &Error(type));
	Object.addMember(*prototype, Key(name), Value.text(name), Value(hidden));
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
	
	Object.addMember(Error(prototype), Key(message), Value.text(&Text(empty)), flags);
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

struct Error * error (struct Text text, struct Chars *message)
{
	return create(Error(prototype), text, message);
}

struct Error * rangeError (struct Text text, struct Chars *message)
{
	return create(Error(rangePrototype), text, message);
}

struct Error * referenceError (struct Text text, struct Chars *message)
{
	return create(Error(referencePrototype), text, message);
}

struct Error * syntaxError (struct Text text, struct Chars *message)
{
	return create(Error(syntaxPrototype), text, message);
}

struct Error * typeError (struct Text text, struct Chars *message)
{
	return create(Error(typePrototype), text, message);
}

struct Error * uriError (struct Text text, struct Chars *message)
{
	return create(Error(uriPrototype), text, message);
}

void destroy (struct Error *self)
{
	assert(self);
	if (!self)
		return;
	
	Object.finalize(&self->object);
	
	free(self), self = NULL;
}
