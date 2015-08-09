//
//  object.c
//  libecc
//
//  Created by Bouilland Aurélien on 04/07/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#include "object.h"

// MARK: - Private

static const int defaultSize = 8;
static struct Object *objectPrototype = NULL;
static struct Closure *objectConstructor = NULL;

static struct Value toString (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 0);
	
	if (ecc->this.type == Value(null))
		ecc->result = Value.text(Text.nullType());
	else if (ecc->this.type == Value(undefined))
		ecc->result = Value.text(Text.undefinedType());
	else if (ecc->this.type == Value(object))
		ecc->result = Value.text(ecc->this.data.object->type);
	else
	{
		// null & undefined is already checked
		ecc->this = Value.toObject(ecc->this, ecc, &(*ops)->text);
		toString(ops, ecc);
	}
	
	return Value.undefined();
}

static struct Value valueOf (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 0);
	
	// todo: check null & undefined
	ecc->this = Value.toObject(ecc->this, ecc, &(*ops)->text);
	
	ecc->result = ecc->this;
	
	return Value.undefined();
}

static struct Value hasOwnProperty (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	
	struct Value v = Value.toString(*Op.argument(ecc, 0));
	ecc->this = Value.toObject(ecc->this, ecc, &(*ops)->text);
	
	if (v.type == Value(identifier))
		ecc->result = Value.boolean(refOwn(ecc->this.data.object, v.data.identifier, 0) != NULL);
	else if (v.type == Value(text))
		ecc->result = Value.boolean(refOwn(ecc->this.data.object, Identifier.makeWithText(*v.data.text, 0), 0) != NULL);
	else if (v.type == Value(chars))
		ecc->result = Value.boolean(refOwn(ecc->this.data.object, Identifier.makeWithText(Text.make(v.data.string->chars, v.data.string->length), 1), 0) != NULL);
	
	return Value.undefined();
}

static struct Value isPrototypeOf (const struct Op ** const ops, struct Ecc * const ecc)
{
//	if (self->data.object->name)
//		return Value.string(String.createWithChars("[object Object]", 15));
	printf("ok valueOf");
	return Value.undefined();
}

static struct Value propertyIsEnumerable (const struct Op ** const ops, struct Ecc * const ecc)
{
//	if (self->data.object->name)
//		return Value.string(String.createWithChars("[object Object]", 15));
	printf("ok valueOf");
	return Value.undefined();
}

// MARK: - Static Members

// MARK: - Methods

void setup (void)
{
	objectPrototype = create(NULL);
	
	enum Object(Flags) flags = Object(writable) | Object(configurable);
	
	Closure.addToObject(objectPrototype, "toString", toString, 0, flags);
	Closure.addToObject(objectPrototype, "toLocaleString", toString, 0, flags);
	Closure.addToObject(objectPrototype, "valueOf", valueOf, 0, flags);
	Closure.addToObject(objectPrototype, "hasOwnProperty", hasOwnProperty, 1, flags);
	Closure.addToObject(objectPrototype, "isPrototypeOf", isPrototypeOf, 0, flags);
	Closure.addToObject(objectPrototype, "propertyIsEnumerable", propertyIsEnumerable, 0, flags);
	
//	objectConstructor = Closure.create();
}

struct Object *prototype (void)
{
	return objectPrototype;
}

struct Closure *constructor (void)
{
	return objectConstructor;
}

Instance create(Instance prototype)
{
	return createSized(prototype, defaultSize);
}

Instance createSized(Instance prototype, uint32_t size)
{
	Instance self = malloc(sizeof(*self));
	assert(self);
	Pool.add(Value.object(self));
	return initializeSized(self, prototype, size);
}

Instance initialize(Instance self, Instance prototype)
{
	return initializeSized(self, prototype, defaultSize);
}

Instance initializeSized(Instance self, Instance prototype, uint32_t size)
{
	assert(self);
	
	*self = Object.identity;
	
	self->type = Text.objectType();
	
	self->prototype = prototype;
	self->hashmapCount = 2;
	self->hashmapCapacity = size;
	
	// hashmap is always 2 slots minimum
	// slot 0 is self referencing undefined value (all zeroes)
	// slot 1 is entry point, referencing undefined (slot 0) by default (all zeroes)
	
	size_t byteSize = sizeof(*self->hashmap) * self->hashmapCapacity;
	self->hashmap = malloc(byteSize);
	memset(self->hashmap, 0, byteSize);
	
	return self;
}

Instance finalize(Instance self)
{
	assert(self);
	
//	while (self->elementCount--)
//		Value.finalize(&self->element[self->elementCount]);
//	
//	while (self->hashmapCount--)
//		if (self->hashmap[self->hashmapCount].data.flags & Object(isValue))
//		{
//			fprintf(stderr, "finalize %p ", &self->hashmap[self->hashmapCount].data.value);
//			Identifier.dumpTo(self->hashmap[self->hashmapCount].data.identifier, stderr);
//			fprintf(stderr, "\n");
//			Value.finalize(&self->hashmap[self->hashmapCount].data.value);
//		}
	
	free(self->hashmap), self->hashmap = NULL;
	free(self->element), self->element = NULL;
	
	return self;
}

Instance copy (const Instance original)
{
	Instance self = malloc(sizeof(*self));
	assert(self);
	Pool.add(Value.object(self));
	
	*self = *original;
	
	size_t byteSize = sizeof(*self->hashmap) * self->hashmapCapacity;
	self->hashmap = malloc(byteSize);
	memcpy(self->hashmap, original->hashmap, byteSize);
	
	return self;
}

void destroy(Instance self)
{
	assert(self);
	
//	fprintf(stderr, "destroy %p\n", self);
	
	finalize(self);
	
//	Pool.delete(Value.object(self));
	free(self), self = NULL;
}

void setType (Instance self, const struct Text *type)
{
	assert(self);
	
	self->type = type;
//	self->name = Bytes.make(type->location + sizeof("[object "), type->length - sizeof("[object ]"));
}

void mark (Instance self)
{
	assert(self);
	
	self->flags |= Object(mark);
	
	for (uint_fast32_t index = 2; index < self->hashmapCount; ++index)
		if (self->hashmap[index].data.flags & Object(isValue))
		{
			if (self->hashmap[index].data.value.type == Value(object))
				mark(self->hashmap[index].data.value.data.object);
		}
}

struct Value getOwn(Instance self, struct Identifier identifier)
{
	assert(self);
	
	return
		self->hashmap[
		self->hashmap[
		self->hashmap[
		self->hashmap[
		self->hashmap[1]
		.slot[identifier.data.depth[0]]]
		.slot[identifier.data.depth[1]]]
		.slot[identifier.data.depth[2]]]
		.slot[identifier.data.depth[3]]]
		.data.value;
}

struct Value get(Instance self, struct Identifier identifier)
{
	assert(self);
	
	Instance object = self;
	struct Value value;
	do
	{
		value =
			object->hashmap[
			object->hashmap[
			object->hashmap[
			object->hashmap[
			object->hashmap[1]
			.slot[identifier.data.depth[0]]]
			.slot[identifier.data.depth[1]]]
			.slot[identifier.data.depth[2]]]
			.slot[identifier.data.depth[3]]]
			.data.value;
	} while (value.type == Value(undefined) && (object = object->prototype));
	
	return value;
}

struct Value *refOwn(Instance self, struct Identifier identifier, int create)
{
	assert(self);
	
	uint32_t slot =
	
	slot =
		self->hashmap[
		self->hashmap[
		self->hashmap[
		self->hashmap[1]
		.slot[identifier.data.depth[0]]]
		.slot[identifier.data.depth[1]]]
		.slot[identifier.data.depth[2]]]
		.slot[identifier.data.depth[3]];
	
	if (slot)
		return &self->hashmap[slot].data.value;
	else if (create)
		return add(self, identifier, Value.undefined(), Object(writable) | Object(enumerable) | Object(configurable));
	else
		return NULL;
}

struct Value *ref(Instance self, struct Identifier identifier, int create)
{
	assert(self);
	
	Instance object = self;
	uint32_t slot;
	
//	fprintf(stderr, "--- *\n");
	
	do
	{
//		dumpTo(object, stderr);
//		fprintf(stderr, "--\n");
		
		
		slot =
			object->hashmap[
			object->hashmap[
			object->hashmap[
			object->hashmap[1]
			.slot[identifier.data.depth[0]]]
			.slot[identifier.data.depth[1]]]
			.slot[identifier.data.depth[2]]]
			.slot[identifier.data.depth[3]];
	} while (!slot && (object = object->prototype));
	
	if (slot)
		return &object->hashmap[slot].data.value;
	else if (create)
		return add(self, identifier, Value.undefined(), Object(writable) | Object(enumerable) | Object(configurable));
	else
		return NULL;
}

void setOwn (Instance self, struct Identifier identifier, struct Value value)
{
	assert(self);
	assert(identifier.data.integer);
	
	uint32_t slot =
		self->hashmap[
		self->hashmap[
		self->hashmap[
		self->hashmap[1]
		.slot[identifier.data.depth[0]]]
		.slot[identifier.data.depth[1]]]
		.slot[identifier.data.depth[2]]]
		.slot[identifier.data.depth[3]];
	
	if (!slot)
	{
		add(self, identifier, value, Object(writable));
		return;
	}
	
	self->hashmap[slot].data.value = value;
}

void set(Instance self, struct Identifier identifier, struct Value value)
{
	assert(self);
	
	Instance object = self;
	uint32_t slot;
	
	do
	{
		slot =
			object->hashmap[
			object->hashmap[
			object->hashmap[
			object->hashmap[1]
			.slot[identifier.data.depth[0]]]
			.slot[identifier.data.depth[1]]]
			.slot[identifier.data.depth[2]]]
			.slot[identifier.data.depth[3]];
	} while (!slot && (object = object->prototype));
	
	fprintf(stderr, "set ");
	Identifier.dumpTo(identifier, stderr);
	fprintf(stderr, " = ");
	Value.dumpTo(value, stderr);
	fprintf(stderr, "\n");
	
	if (!slot)
	{
		add(self, identifier, value, Object(writable));
		return;
	}
	
	self->hashmap[slot].data.value = value;
}

struct Value * add (Instance self, struct Identifier identifier, struct Value value, enum Object(Flags) flags)
{
	assert(self);
	assert(identifier.data.integer);
	
//	fprintf(stderr, "set ");
//	Identifier.dumpTo(identifier, stderr);
//	fprintf(stderr, " = ");
//	Value.dumpTo(&value, stderr);
//	fprintf(stderr, "\n");
	
	uint_fast32_t slot = 1;
	int depth = 0;
	
	do
	{
		if (!self->hashmap[slot].slot[identifier.data.depth[depth]])
		{
			int need = 5 - depth - (self->hashmapCapacity - self->hashmapCount);
			if (need > 0) {
//				fprintf(stderr, "was %d need %d\n", self->hashmapCapacity, need);
				self->hashmapCapacity *= 2;
//				fprintf(stderr, "now %d\n", self->hashmapCapacity);
				self->hashmap = realloc(self->hashmap, sizeof(*self->hashmap) * self->hashmapCapacity);
				memset(self->hashmap + self->hashmapCount, 0, sizeof(*self->hashmap) * (self->hashmapCapacity - self->hashmapCount));
			}
			
			do
			{
				slot = self->hashmap[slot].slot[identifier.data.depth[depth]] = self->hashmapCount++;
			} while (++depth < 4);
			
			self->hashmap[slot].data.identifier = identifier;
			break;
		}
		
		slot = self->hashmap[slot].slot[identifier.data.depth[depth]];
	} while (++depth < 4);
	
	self->hashmap[slot].data.value = value;
	self->hashmap[slot].data.flags = Object(isValue) | flags;
	return &self->hashmap[slot].data.value;
}

struct Value delete (Instance self, struct Identifier identifier)
{
#warning checkme
	if (!self)
		return Value.true();
	
	Instance object = self;
	uint32_t slot;
	
	do
	{
		slot =
			object->hashmap[
			object->hashmap[
			object->hashmap[1]
			.slot[identifier.data.depth[0]]]
			.slot[identifier.data.depth[1]]]
			.slot[identifier.data.depth[2]];
	} while (!slot && (object = object->prototype));
	
	if (!object || !object->hashmap[slot].slot[identifier.data.depth[3]])
		return Value.false();
	
	object->hashmap[slot].slot[identifier.data.depth[3]] = 0;
	return Value.true();
}

void packValue (Instance self)
{
	assert(self);
	
	__typeof__(*self->hashmap) data;
	uint_fast32_t index = 2, valueIndex = 2, copyIndex, slot;
	
	for (; index <= self->hashmapCount; ++index)
		if (self->hashmap[index].data.flags & Object(isValue))
		{
			data = self->hashmap[index];
			for (copyIndex = index; copyIndex > valueIndex; --copyIndex)
			{
				self->hashmap[copyIndex] = self->hashmap[copyIndex - 1];
				if (!(self->hashmap[copyIndex].data.flags & Object(isValue)))
					for (slot = 0; slot < 16; ++slot)
					{
						if (self->hashmap[copyIndex].slot[slot] == index)
							self->hashmap[copyIndex].slot[slot] = valueIndex;
						else if (self->hashmap[copyIndex].slot[slot] >= valueIndex && self->hashmap[copyIndex].slot[slot] < index)
							self->hashmap[copyIndex].slot[slot]++;
					}
			}
			for (slot = 0; slot < 16; ++slot)
				if (self->hashmap[1].slot[slot] >= valueIndex && self->hashmap[1].slot[slot] < index)
					self->hashmap[1].slot[slot]++;
			
			self->hashmap[valueIndex++] = data;
		}
	
	self->hashmap = realloc(self->hashmap, sizeof(*self->hashmap) * (self->hashmapCount + 1));
	self->hashmapCapacity = self->hashmapCount + 1;
}

void resizeElement (Instance self, uint32_t size)
{
	assert(self);
	
	self->element = realloc(self->element, sizeof(*self->element) * size);
	if (size > self->elementCount)
		memset(self->element + self->elementCount, 0, sizeof(*self->element) * (size - self->elementCount));
	
	self->elementCount = self->elementCapacity = size;
}

void addElementAtIndex (Instance self, uint32_t index, struct Value value)
{
	assert(self);
	
	if (self->elementCapacity <= index)
		resizeElement(self, index + 1);
	
	self->element[index].value = value;
	self->element[index].flags |= Object(isValue);
}

void dumpTo(Instance self, FILE *file)
{
	assert(self);
	
	uint_fast32_t index;
	
	int isArray = self->prototype == Array.prototype();
	
	fprintf(stderr, isArray? "[ ": "{ ");
	
	for (index = 0; index < self->elementCount; ++index)
	{
		if (!isArray)
			fprintf(stderr, "%d: ", index);
		
		if (self->element[index].flags & Object(isValue))
		{
			Value.dumpTo(self->element[index].value, stderr);
			fprintf(stderr, ", ");
		}
	}
	
	for (index = 0; index < self->hashmapCount; ++index)
	{
		if (self->hashmap[index].data.flags & Object(isValue))
		{
//			fprintf(stderr, "(%d) ", index);
			if (!isArray)
			{
				Identifier.dumpTo(self->hashmap[index].data.identifier, stderr);
				fprintf(stderr, ": ");
			}
			Value.dumpTo(self->hashmap[index].data.value, stderr);
			fprintf(stderr, ", ");
		}
//		else
//		{
//			fprintf(stderr, "(%d) ", index);
//			for (int i = 0; i < 16; ++ i)
//				fprintf(stderr, "%02d ", self->hashmap[index].slot[i]);
//			
//			putc('\n', stderr);
//		}
	}
	
	fprintf(stderr, isArray? "]\n": "}\n");
}
