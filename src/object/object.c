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
static struct Function *objectConstructor = NULL;

static inline uint32_t getSlot (Instance self, struct Identifier identifier)
{
	return
		self->hashmap[
		self->hashmap[
		self->hashmap[
		self->hashmap[1]
		.slot[identifier.data.depth[0]]]
		.slot[identifier.data.depth[1]]]
		.slot[identifier.data.depth[2]]]
		.slot[identifier.data.depth[3]];
}

static inline int32_t getElementOrIdentifier (struct Value property, struct Identifier *identifier)
{
	int32_t element = -1;
	
	if (property.type == Value(identifier))
		*identifier = property.data.identifier;
	else
	{
		struct Text text;
		
		if (property.type == Value(integer) && property.data.integer >= 0)
			element = property.data.integer;
		else
		{
			property = Value.toString(property);
			text = Text.make(Value.stringChars(property), Value.stringLength(property));
			element = Lexer.parseElement(text);
		}
		
		if (element < 0)
			*identifier = Identifier.makeWithText(text, 1);
	}
	
	return element;
}

static struct Value toString (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 0);
	
	if (ecc->this.type == Value(null))
		ecc->result = Value.text(Text.nullType());
	else if (ecc->this.type == Value(undefined))
		ecc->result = Value.text(Text.undefinedType());
	else if (Value.isObject(ecc->this))
//		TODO: check
//		ecc->result = Value.text(ecc->this.data.object->type);
		ecc->result = Value.toString(ecc->this);
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
	
	ecc->result = Value.toObject(ecc->this, ecc, &(*ops)->text);
	
	return Value.undefined();
}

static struct Value hasOwnProperty (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	
	struct Value v = Value.toString(Op.argument(ecc, 0));
	ecc->this = Value.toObject(ecc->this, ecc, &(*ops)->text);
	
	if (v.type == Value(identifier))
		ecc->result = Value.boolean(getSlot(ecc->this.data.object, v.data.identifier));
	else if (v.type == Value(text))
		ecc->result = Value.boolean(getSlot(ecc->this.data.object, Identifier.makeWithText(*v.data.text, 0)));
	else if (v.type == Value(chars))
		ecc->result = Value.boolean(getSlot(ecc->this.data.object, Identifier.makeWithText(Text.make(v.data.string->chars, v.data.string->length), 1)));
	
	return Value.undefined();
}

static struct Value isPrototypeOf (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	
	struct Value arg0 = Op.argument(ecc, 0);
	
	if (Value.isObject(arg0))
	{
		struct Object *v = arg0.data.object;
		struct Object *o = Value.toObject(ecc->this, ecc, &(*ops)->text).data.object;
		
		while (( v = v->prototype ))
			if (v == o)
			{
				ecc->result = Value.true();
				return Value.undefined();
			}
	}
	
	ecc->result = Value.false();
	return Value.undefined();
}

static struct Value propertyIsEnumerable (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	
	struct Value property = Op.argument(ecc, 0);
	struct Object *object = Value.toObject(ecc->this, ecc, &(*ops)->text).data.object;
	enum Object(Flags) flags = 0;
	
	getOwnProperty(object, property, &flags);
	
	if (flags & Object(isValue))
		ecc->result = Value.boolean(flags & Object(enumerable));
	else
		ecc->result = Value.undefined();
	
	return Value.undefined();
}

static struct Value constructorFunction (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	
	struct Value value = Op.argument(ecc, 0);
	
	if (value.type == Value(null) || value.type == Value(undefined))
		ecc->result = Value.object(Object.create(objectPrototype));
	else if (ecc->construct && Value.isObject(value))
		ecc->result = value;
	else
		ecc->result = Value.toObject(value, ecc, &(*ops)->text);
	
	return Value.undefined();
}

static struct Value getPrototypeOf (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	
	struct Value arg0 = Op.argument(ecc, 0);
	if (!Value.isObject(arg0))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError((*ops)->text, "is not an object")));
	
	struct Object *object = arg0.data.object;
	
	ecc->result = object->prototype? Value.object(object->prototype): Value.undefined();
	
	return Value.undefined();
}

static struct Value getOwnPropertyDescriptor (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 2);
	
	struct Value arg0 = Op.argument(ecc, 0);
	if (!Value.isObject(arg0))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError((*ops)->text, "is not an object")));
	
	struct Object *object = arg0.data.object;
	struct Value property = Op.argument(ecc, 1);
	enum Object(Flags) flags = 0;
	struct Value *ref = getOwnProperty(object, property, &flags);
	
	ecc->result = Value.undefined();
	
	if (flags & Object(isValue))
	{
		const enum Object(Flags) resultFlags = Object(writable) | Object(enumerable) | Object(configurable);
		
		struct Object *result = Object.create(Object.prototype());
		
		Object.add(result, Identifier.value(), *ref, resultFlags);
		Object.add(result, Identifier.writable(), Value.boolean(flags & Object(writable)), resultFlags);
		Object.add(result, Identifier.enumerable(), Value.boolean(flags & Object(enumerable)), resultFlags);
		Object.add(result, Identifier.configurable(), Value.boolean(flags & Object(configurable)), resultFlags);
		
		ecc->result = Value.object(result);
	}
	
	return Value.undefined();
}

// MARK: - Static Members

// MARK: - Methods

void setup ()
{
	const enum Object(Flags) flags = Object(writable) | Object(configurable);
	
	objectPrototype = create(NULL);
	Function.addToObject(objectPrototype, "toString", toString, 0, flags);
	Function.addToObject(objectPrototype, "toLocaleString", toString, 0, flags);
	Function.addToObject(objectPrototype, "valueOf", valueOf, 0, flags);
	Function.addToObject(objectPrototype, "hasOwnProperty", hasOwnProperty, 1, flags);
	Function.addToObject(objectPrototype, "isPrototypeOf", isPrototypeOf, 1, flags);
	Function.addToObject(objectPrototype, "propertyIsEnumerable", propertyIsEnumerable, 1, flags);
	
	objectConstructor = Function.createWithNative(NULL /* <!> to fill with function.prototype */, constructorFunction, 1);
	Function.addToObject(&objectConstructor->object, "getPrototypeOf", getPrototypeOf, 1, flags);
	Function.addToObject(&objectConstructor->object, "getOwnPropertyDescriptor", getOwnPropertyDescriptor, 2, flags);
	
	Object.add(objectPrototype, Identifier.constructor(), Value.function(objectConstructor), 0);
	Object.add(&objectConstructor->object, Identifier.prototype(), Value.object(objectPrototype), 0);
}

void teardown (void)
{
	objectPrototype = NULL;
	objectConstructor = NULL;
}

struct Object *prototype (void)
{
	return objectPrototype;
}

struct Function *constructor (void)
{
	return objectConstructor;
}

Instance create (Instance prototype)
{
	return createSized(prototype, defaultSize);
}

Instance createSized (Instance prototype, uint32_t size)
{
	Instance self = malloc(sizeof(*self));
	assert(self);
	Pool.addObject(self);
	return initializeSized(self, prototype, size);
}

Instance initialize (Instance self, Instance prototype)
{
	return initializeSized(self, prototype, defaultSize);
}

Instance initializeSized (Instance self, Instance prototype, uint32_t size)
{
	assert(self);
	
	*self = Object.identity;
	
	self->type = Text.objectType();
	
	self->prototype = prototype;
	self->hashmapCount = 2;
	self->hashmapCapacity = size;
	self->flags = Object(extensible);
	
	// hashmap is always 2 slots minimum
	// slot 0 is self referencing undefined value (all zeroes)
	// slot 1 is entry point, referencing undefined (slot 0) by default (all zeroes)
	
	size_t byteSize = sizeof(*self->hashmap) * self->hashmapCapacity;
	self->hashmap = malloc(byteSize);
	memset(self->hashmap, 0, byteSize);
	
	return self;
}

Instance finalize (Instance self)
{
	assert(self);
	
	free(self->hashmap), self->hashmap = NULL;
	free(self->element), self->element = NULL;
	
	return self;
}

Instance copy (const Instance original)
{
	Instance self = malloc(sizeof(*self));
	assert(self);
	Pool.addObject(self);
	
	*self = *original;
	
	size_t byteSize;
	
	byteSize = sizeof(*self->element) * self->elementCapacity;
	self->element = malloc(byteSize);
	memcpy(self->element, original->element, byteSize);
	
	byteSize = sizeof(*self->hashmap) * self->hashmapCapacity;
	self->hashmap = malloc(byteSize);
	memcpy(self->hashmap, original->hashmap, byteSize);
	
	return self;
}

void destroy (Instance self)
{
	assert(self);
	
	finalize(self);
	
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

struct Value getOwn (Instance self, struct Identifier identifier)
{
	assert(self);
	
	return self->hashmap[getSlot(self, identifier)].data.value;
}

struct Value get (Instance self, struct Identifier identifier)
{
	assert(self);
	
	Instance object = self;
	uint32_t slot = 0;
	do
		slot = getSlot(object, identifier);
	while (!slot && (object = object->prototype));
	
	if (slot)
		return object->hashmap[slot].data.value;
	else
		return Value.undefined();
}

struct Value *getOwnMember (Instance self, struct Identifier identifier)
{
	assert(self);
	
	uint32_t slot = getSlot(self, identifier);
	if (slot)
		return &self->hashmap[slot].data.value;
	
	return NULL;
}

struct Value *getMember (Instance self, struct Identifier identifier)
{
	assert(self);
	
//	fprintf(stderr, "--- > ");
//	Identifier.dumpTo(identifier, stderr);
//	fprintf(stderr, " <\n");
	
	Instance object = self;
	uint32_t slot;
	do
	{
//		dumpTo(object, stderr);
//		fprintf(stderr, "%p --\n", object);
		
		if (( slot = getSlot(object, identifier) ))
			return &object->hashmap[slot].data.value;
	}
	while ((object = object->prototype));
	
	return NULL;
}

struct Value * getOwnProperty (Instance self, struct Value property, enum Object(Flags) *flags)
{
	assert(self);
	assert(flags);
	
	struct Identifier identifier;
	int32_t element = getElementOrIdentifier(property, &identifier);
	uint32_t slot;
	
	if (element >= 0 && element < self->elementCount && (self->element[element].data.flags & Object(isValue)))
	{
		*flags = self->element[element].data.flags;
		return &self->element[element].data.value;
	}
	else if (( slot = getSlot(self, identifier) ))
	{
		*flags = self->hashmap[slot].data.flags;
		return &self->hashmap[slot].data.value;
	}
	
	*flags = 0;
	return NULL;
}

struct Value * getProperty (Instance self, struct Value property, enum Object(Flags) *flags)
{
	assert(self);
	assert(flags);
	
	Instance object = self;
	
	struct Identifier identifier;
	int32_t element = getElementOrIdentifier(property, &identifier);
	uint32_t slot;
	
	if (element >= 0)
		do
			if (element < object->elementCount)
			{
				*flags = object->element[element].data.flags;
				return &object->element[element].data.value;
			}
		while ((object = object->prototype));
	else
		do
			if (( slot = getSlot(object, identifier) ))
			{
				*flags = object->hashmap[slot].data.flags;
				return &object->hashmap[slot].data.value;
			}
		while ((object = object->prototype));
	
	*flags = 0;
	return NULL;
}

void setOwn (Instance self, struct Identifier identifier, struct Value value)
{
	assert(self);
	assert(identifier.data.integer);
	
	uint32_t slot = getSlot(self, identifier);
	
	if (!slot)
	{
		add(self, identifier, value, Object(writable) | Object(configurable));
		return;
	}
	
	if (self->hashmap[slot].data.flags | Object(writable))
		self->hashmap[slot].data.value = value;
}

void set(Instance self, struct Identifier identifier, struct Value value)
{
	assert(self);
	
	Instance object = self;
	uint32_t slot;
	do
		slot = getSlot(object, identifier);
	while (!slot && (object = object->prototype));
	
	if (!slot)
	{
		add(self, identifier, value, Object(writable) | Object(configurable));
		return;
	}
	
	if (self->hashmap[slot].data.flags | Object(writable))
		self->hashmap[slot].data.value = value;
}

void setProperty (Instance self, struct Value property, struct Value value)
{
	assert(self);
	
	Instance object = self;
	
	struct Identifier identifier;
	int32_t element = getElementOrIdentifier(property, &identifier);
	uint32_t slot;
	
	if (element >= 0)
	{
		do
			if (element < object->elementCount)
			{
				if (self->element[element].data.flags | Object(writable))
				{
					object->element[element].data.value = value;
					object->element[element].data.flags |= Object(isValue);
				}
				return;
			}
		while ((object = object->prototype));
		
		addElementAtIndex(self, element, value, 0);
	}
	else
	{
		do
			if (( slot = getSlot(object, identifier) ))
			{
				if (self->element[element].data.flags | Object(writable))
				{
					object->hashmap[slot].data.value = value;
					object->hashmap[slot].data.flags |= Object(isValue);
				}
				return;
			}
		while ((object = object->prototype));
		
		add(self, identifier, value, 0);
	}
}

struct Value * add (Instance self, struct Identifier identifier, struct Value value, enum Object(Flags) flags)
{
	assert(self);
	assert(identifier.data.integer);
	
	uint_fast32_t slot = 1;
	int depth = 0;
	
	do
	{
		if (!self->hashmap[slot].slot[identifier.data.depth[depth]])
		{
			int need = 5 - depth - (self->hashmapCapacity - self->hashmapCount);
			if (need > 0) {
				self->hashmapCapacity *= 2;
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
	assert(self);
	assert(identifier.data.integer);
	
	Instance object = self;
	uint32_t slot;
	do
		slot = getSlot(object, identifier);
	while (!slot && (object = object->prototype));
	
	if (!object || !(object->hashmap[slot].data.flags & Object(isValue)))
		return Value.true();
	
	if (object->hashmap[slot].data.flags & Object(configurable))
	{
		memset(&object->hashmap[slot], 0, sizeof(*object->hashmap));
		return Value.true();
	}
	else
		return Value.false();
}

struct Value deleteProperty (Instance self, struct Value property)
{
	assert(self);
	
	struct Identifier identifier;
	int32_t element = getElementOrIdentifier(property, &identifier);
	uint32_t slot;
	
	if (element >= 0)
	{
		if (element < self->elementCount)
		{
			if (!(self->element[element].data.flags & Object(configurable)))
				return Value.false();
			
			memset(&self->element[element], 0, sizeof(*self->element));
		}
	}
	else if (( slot = getSlot(self, identifier) ))
	{
		if (!(self->hashmap[slot].data.flags & Object(configurable)))
			return Value.false();
		
		memset(&self->hashmap[slot], 0, sizeof(*self->hashmap));
	}
	
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

void addElementAtIndex (Instance self, uint32_t index, struct Value value, enum Object(Flags) flags)
{
	assert(self);
	
	if (self->elementCapacity <= index)
		resizeElement(self, index + 1);
	
	self->element[index].data.value = value;
	self->element[index].data.flags |= Object(isValue) | flags;
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
		
		if (self->element[index].data.flags & Object(isValue))
		{
			Value.dumpTo(self->element[index].data.value, stderr);
			fprintf(stderr, ", ");
		}
	}
	
	for (index = 0; index < self->hashmapCount; ++index)
	{
		if (self->hashmap[index].data.flags & Object(isValue))
		{
			if (!isArray)
			{
				Identifier.dumpTo(self->hashmap[index].data.identifier, stderr);
				fprintf(stderr, ": ");
			}
			Value.dumpTo(self->hashmap[index].data.value, stderr);
			fprintf(stderr, ", ");
		}
	}
	
	fprintf(stderr, isArray? "]": "}");
}
