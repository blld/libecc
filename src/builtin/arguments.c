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

static struct Value getLength (struct Context * const context)
{
	Context.assertParameterCount(context, 0);
	return Value.binary(context->this.data.object->elementCount);
}

static struct Value setLength (struct Context * const context)
{
	Context.assertParameterCount(context, 1);
	Object.resizeElement(context->this.data.object, Value.toBinary(context, Context.argument(context, 0)).data.binary);
	return Value(undefined);
}

static struct Value poison (struct Context * const context)
{
	Context.rewindStatement(context->parent);
	Context.typeError(context, Chars.create("'callee' cannot be accessed in this context"));
	return Value(undefined);
}

// MARK: - Static Members

// MARK: - Methods

void setup (void)
{
	Arguments(prototype) = Object.createTyped(&Arguments(type));
	
	Object.addMember(Arguments(prototype), Key(length), Function.accessor(getLength, setLength), Value(hidden) | Value(sealed));
	Object.addMember(Arguments(prototype), Key(callee), Function.accessor(poison, poison), Value(hidden) | Value(sealed));
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
