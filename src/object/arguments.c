//
//  arguments.c
//  libecc
//
//  Created by Bouilland Aurélien on 17/01/2016.
//  Copyright (c) 2016 Libeccio. All rights reserved.
//

#include "arguments.h"

// MARK: - Private

struct Object * Arguments(prototype);

static struct Value getLength (struct Native(Context) * const context, struct Ecc * const ecc)
{
	Native.assertParameterCount(ecc, 0);
	ecc->result = Value.binary(ecc->this.data.object->elementCount);
	return Value(undefined);
}

static struct Value setLength (struct Native(Context) * const context, struct Ecc * const ecc)
{
	Native.assertParameterCount(ecc, 1);
	Object.resizeElement(ecc->this.data.object, Value.toBinary(Native.argument(ecc, 0)).data.binary);
	return Value(undefined);
}

// MARK: - Static Members

// MARK: - Methods

void setup (void)
{
	Arguments(prototype) = Object.createTyped(&Text(argumentsType));
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
