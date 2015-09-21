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
	Pool.addClosure(self);
	
	*self = Module.identity;
	
	Object.initialize(&self->object, Object.prototype()/* TODO: check */);
	Object.initializeSized(&self->context, prototype, size);
	
	struct Object *proto = Object.create(Object.prototype());
	Object.add(proto, Identifier.constructor(), Value.closure(self), Object(writable) | Object(configurable));
	Object.add(&self->object, Identifier.prototype(), Value.object(proto), Object(writable));
	
	return self;
}

Instance createWithFunction (struct Object *prototype, const Function function, int parameterCount)
{
	Instance self = NULL;
	
	if (parameterCount < 0)
	{
		self = createSized(prototype, 3);
		self->needHeap = 1;
		self->needArguments = 1;
		self->object.hashmap[2].data.flags = Object(writable) | Object(isValue);
		self->object.hashmap[2].data.identifier = Identifier.arguments();
	}
	else
	{
		self = createSized(prototype, 3 + parameterCount);
		self->parameterCount = parameterCount;
		
		for (uint_fast16_t index = 0; index < parameterCount; ++index)
			self->object.hashmap[3 + index].data.flags = Object(writable) | Object(isValue);
	}
	self->context.hashmapCount = self->context.hashmapCapacity;
	self->oplist = OpList.create(function, Value.undefined(), Text.make(NULL, 0));
	self->text = *Text.nativeCode();
	
	Object.add(&self->object, Identifier.length(), Value.integer(parameterCount), 0);
	
	return self;
}

Instance copy (Instance original)
{
	assert(original);
	
	Instance self = malloc(sizeof(*self));
	assert(self);
	Pool.addObject(&self->object);
	
	*self = *original;
	
	size_t byteSize;
	
	byteSize = sizeof(*self->object.hashmap) * self->object.hashmapCapacity;
	self->object.hashmap = malloc(byteSize);
	memcpy(self->object.hashmap, original->object.hashmap, byteSize);
	
	return self;
}

void destroy (Instance self)
{
	assert(self);
	
	Object.finalize(&self->object);
	Object.finalize(&self->context);
	
	if (self->oplist)
		OpList.destroy(self->oplist), self->oplist = NULL;
	
	free(self), self = NULL;
}

//void retain (Instance self, int count)
//{
//	assert(self);
//	
//	fprintf(stderr, "RETAIN %p\n", self);
//	
//	Object.retain(&self->context, count);
//	Object.retain(&self->object, count);
//}
//
//void release (Instance self, int count)
//{
//	assert(self);
//	
//	fprintf(stderr, "RELEASE %p\n", self);
//	fprintf(stderr, "object:\n");
//	Object.dumpTo(&self->object, stderr);
//	fprintf(stderr, "\ncontext:\n");
//	Object.dumpTo(&self->context, stderr);
//	fprintf(stderr, "\n-\n");
//	
//	Object.release(&self->context, count, 1);
//	Object.release(&self->object, count, 0);
//}

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
	
	Instance closure = createWithFunction(object, function, parameterCount);
	
	Object.add(object, Identifier.makeWithCString(name), Value.closure(closure), flags);
}
