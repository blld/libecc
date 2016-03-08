//
//  arguments.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "arguments.h"

// MARK: - Private

struct Object * Arguments(prototype);

const struct Object(Type) Arguments(type) = {
	.text = &Text(argumentsType),
};

static struct Value getLength (struct Native(Context) * const context)
{
	Native.assertParameterCount(context, 0);
	return Value.binary(context->this.data.object->elementCount);
}

static struct Value setLength (struct Native(Context) * const context)
{
	Native.assertParameterCount(context, 1);
	Object.resizeElement(context->this.data.object, Value.toBinary(Native.argument(context, 0)).data.binary);
	return Value(undefined);
}

// MARK: - Static Members

// MARK: - Methods

void setup (void)
{
	Arguments(prototype) = Object.createTyped(&Arguments(type));
	Object.add(Arguments(prototype), Key(length), Value.function(Function.createWithNativeAccessor(getLength, setLength)), Value(hidden) | Value(sealed));
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

struct Object *initializeSized (struct Object *self, uint32_t size)
{
	Object.initializeSized(self, Arguments(prototype), 0);
	
	if (size)
		Object.resizeElement(self, size);
	
	return self;
}
