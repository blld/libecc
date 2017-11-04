//
//  arguments.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#define Implementation
#include "arguments.h"

// MARK: - Private

struct Object * Arguments(prototype);

const struct Object(Type) Arguments(type) = {
	.text = &Text(argumentsType),
};

// MARK: - Static Members

static
struct Value getLength (struct Context * const context)
{
	return Value.binary(context->this.data.object->elementCount);
}

static
struct Value setLength (struct Context * const context)
{
	double length;
	
	length = Value.toBinary(context, Context.argument(context, 0)).data.binary;
	if (!isfinite(length) || length < 0 || length > UINT32_MAX || length != (uint32_t)length)
		Context.rangeError(context, Chars.create("invalid array length"));
	
	if (Object.resizeElement(context->this.data.object, length) && context->strictMode)
	{
		Context.typeError(context, Chars.create("'%u' is non-configurable", context->this.data.object->elementCount));
	}
	
	return Value(undefined);
}

static
struct Value getCallee (struct Context * const context)
{
	Context.rewindStatement(context->parent);
	Context.typeError(context, Chars.create("'callee' cannot be accessed in this context"));
	
	return Value(undefined);
}

static
struct Value setCallee (struct Context * const context)
{
	Context.rewindStatement(context->parent);
	Context.typeError(context, Chars.create("'callee' cannot be accessed in this context"));
	
	return Value(undefined);
}

// MARK: - Methods

void setup (void)
{
	const enum Value(Flags) h = Value(hidden);
	const enum Value(Flags) s = Value(sealed);
	
	Arguments(prototype) = Object.createTyped(&Arguments(type));
	
	Object.addMember(Arguments(prototype), Key(length), Function.accessor(getLength, setLength), h|s | Value(asOwn) | Value(asData));
	Object.addMember(Arguments(prototype), Key(callee), Function.accessor(getCallee, setCallee), h|s | Value(asOwn));
}

void teardown (void)
{
	Arguments(prototype) = NULL;
}

struct Object *createSized (uint32_t size)
{
	struct Object *self = Object.create(Arguments(prototype));
	
	Object.resizeElement(self, size);
	
	return self;
}

struct Object *createWithCList (int count, const char * list[])
{
	struct Object *self = createSized(count);
	
	Object.populateElementWithCList(self, count, list);
	
	return self;
}
