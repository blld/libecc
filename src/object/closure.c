//
//  closure.c
//  libecc
//
//  Created by Bouilland Aurélien on 15/07/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#include "closure.h"

// MARK: - Private

// MARK: - Static Members

// MARK: - Methods

Instance create (struct Object *prototype)
{
	return createSized(prototype, 8);
}

Instance createSized (struct Object *prototype, uint32_t size)
{
	Instance self = malloc(sizeof(*self));
	assert(self);
	Pool.add(Value.closure(self));
	*self = Module.identity;
	
	Object.initialize(&self->object, NULL);
	Object.initializeSized(&self->context, prototype, size);
	
	return self;
}

Instance copy (Instance original)
{
	assert(original);
	
	Instance self = malloc(sizeof(*self));
	assert(self);
	Pool.add(Value.closure(self));
	*self = *original;
	
	return self;
}

void destroy (Instance self)
{
	assert(self);
	
//	fprintf(stderr, "destroy %p\n", self);
	
	Object.finalize(&self->object);
	Object.finalize(&self->context);
	
	if (self->oplist)
		OpList.destroy(self->oplist), self->oplist = NULL;
	
//	Pool.delete(Value.closure(self));
	free(self), self = NULL;
}

void addFunction(Instance self, const char *name, const Function function, int parameterCount, enum Object(Flags) flags)
{
	assert(self);
	
	addToObject(&self->context, name, function, parameterCount, flags);
}

void addValue(Instance self, const char *name, struct Value value, enum Object(Flags) flags)
{
	assert(self);
	
	Object.add(&self->context, Identifier.makeWithCString(name), value, flags);
}

void addToObject(struct Object *object, const char *name, const Function function, int parameterCount, enum Object(Flags) flags)
{
	assert(object);
	
	Instance closure = NULL;
	
	if (parameterCount < 0)
	{
		closure = createSized(object, 3);
		closure->needHeap = 1;
		closure->needArguments = 1;
		closure->object.hashmap[2].data.flags = Object(writable) | Object(isValue);
		closure->object.hashmap[2].data.identifier = Identifier.arguments();
	}
	else
	{
		closure = createSized(object, 3 + parameterCount);
		closure->parameterCount = parameterCount;
		
		for (uint_fast16_t index = 0; index < parameterCount; ++index)
			closure->object.hashmap[3 + index].data.flags = Object(writable) | Object(isValue);
	}
	closure->object.hashmapCount = closure->object.hashmapCapacity;
	closure->oplist = OpList.create(function, Value.undefined(), Text.make(NULL, 0));
	closure->text = *Text.nativeCode();
	
	Object.add(object, Identifier.makeWithCString(name), Value.closure(closure), flags);
}
