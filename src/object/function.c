//
//  function.c
//  libecc
//
//  Created by Bouilland Aurélien on 15/07/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#include "function.h"

// MARK: - Private

static struct Object *functionPrototype = NULL;
static struct Function *functionConstructor = NULL;

static struct Value prototypeFunction (const struct Op ** const ops, struct Ecc * const ecc)
{
	ecc->result = Value.undefined();
	
	return Value.undefined();
}

static struct Value constructorFunction (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertVariableParameter(ecc);
	
	// TODO
	
	return Value.undefined();
}

// MARK: - Static Members

// MARK: - Methods

void setup ()
{
	functionPrototype = Object.prototype();
	struct Function *functionPrototypeFunction = createWithNative(NULL, prototypeFunction, 0);
	functionPrototype = &functionPrototypeFunction->object;
	
	functionConstructor = Function.createWithNative(functionPrototype, constructorFunction, -1);
	
	Object.add(functionPrototype, Identifier.constructor(), Value.function(functionConstructor), 0);
	Object.add(&functionConstructor->object, Identifier.prototype(), Value.function(functionPrototypeFunction), 0);
	
	Object.constructor()->object.prototype = functionPrototype;
}

void teardown (void)
{
	functionPrototype = NULL;
	functionConstructor = NULL;
}

struct Object *prototype (void)
{
	return functionPrototype;
}

struct Function *constructor (void)
{
	return functionConstructor;
}

Instance create (struct Object *prototype)
{
	return createSized(prototype, 8);
}

Instance createSized (struct Object *prototype, uint32_t size)
{
	Instance self = malloc(sizeof(*self));
	assert(self);
	Pool.addFunction(self);
	
	*self = Module.identity;
	
	Object.initialize(&self->object, functionPrototype);
	Object.initializeSized(&self->context, prototype, size);
	
	self->object.type = Text.functionType();
	
	struct Object *proto = Object.create(Object.prototype());
	Object.add(proto, Identifier.constructor(), Value.function(self), Object(writable) | Object(configurable));
	Object.add(&self->object, Identifier.prototype(), Value.object(proto), Object(writable));
	
	return self;
}

Instance createWithNative (struct Object *prototype, const Native native, int parameterCount)
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
	self->oplist = OpList.create(native, Value.undefined(), Text.make(NULL, 0));
	self->text = *Text.nativeCode();
	
	Object.add(&self->object, Identifier.length(), Value.integer(parameterCount >= 0? parameterCount: 1), 0);
	
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

void addValue(Instance self, const char *name, struct Value value, enum Object(Flags) flags)
{
	assert(self);
	
	Object.add(&self->context, Identifier.makeWithCString(name), value, flags);
}

Instance addNative(Instance self, const char *name, const Native native, int parameterCount, enum Object(Flags) flags)
{
	assert(self);
	
	return addToObject(&self->context, name, native, parameterCount, flags);
}

Instance addToObject(struct Object *object, const char *name, const Native native, int parameterCount, enum Object(Flags) flags)
{
	assert(object);
	
	Instance Function = createWithNative(object, native, parameterCount);
	
	Object.add(object, Identifier.makeWithCString(name), Value.function(Function), flags);
	
	return Function;
}
