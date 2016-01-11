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

static inline uint32_t getSlot (const struct Object * const self, const struct Key key)
{
	return
		self->hashmap[
		self->hashmap[
		self->hashmap[
		self->hashmap[1]
		.slot[key.data.depth[0]]]
		.slot[key.data.depth[1]]]
		.slot[key.data.depth[2]]]
		.slot[key.data.depth[3]];
}

static inline int32_t getElementOrKey (struct Value property, struct Key *key)
{
	int32_t element = -1;
	
	if (property.type == Value(keyType))
		*key = property.data.key;
	else
	{
		if (property.type == Value(integerType))
			element = property.data.integer;
		else if (property.type == Value(binaryType) && property.data.binary >= 0 && property.data.binary <= INT32_MAX && property.data.binary == (int32_t)property.data.binary)
			element = property.data.binary;
		
		if (element < 0)
		{
			if (Value.isString(property))
			{
				struct Text text = Text.make(Value.stringChars(property), Value.stringLength(property));
				
				if ((element = Lexer.parseElement(text)) < 0)
					*key = Key.makeWithText(text, 1);
			}
			else
			{
				uint16_t length = Value.toBufferLength(property);
				{
					char bytes[length + 1];
					struct Text text = Text.make(bytes, length);
					
					Value.toBuffer(property, bytes, length);
					
					if ((element = Lexer.parseElement(text)) < 0)
						*key = Key.makeWithText(text, 1);
				}
			}
		}
	}
	
	return element;
}

static inline uint32_t nextPowerOfTwo(uint32_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

//

static struct Object *checkObject (const struct Op ** const ops, struct Ecc * const ecc, struct Value value)
{
	if (!Value.isObject(value))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError((*ops)->text, "%.*s not an object", (*ops)->text.length, (*ops)->text.location)));
	
	return value.data.object;
}

static struct Value toString (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 0);
	
	if (ecc->this.type == Value(nullType))
		ecc->result = Value.text(&Text(nullType));
	else if (ecc->this.type == Value(undefinedType))
		ecc->result = Value.text(&Text(undefinedType));
	else if (Value.isString(ecc->this))
		ecc->result = Value.text(&Text(stringType));
	else if (Value.isNumber(ecc->this))
		ecc->result = Value.text(&Text(numberType));
	else if (Value.isBoolean(ecc->this))
		ecc->result = Value.text(&Text(booleanType));
	else if (Value.isObject(ecc->this))
		ecc->result = Value.text(ecc->this.data.object->type);
	else
		assert(0);
	
	return Value(undefined);
}

static struct Value valueOf (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 0);
	
	ecc->result = Value.toObject(ecc->this, ecc, &(*ops)->text);
	
	return Value(undefined);
}

static struct Value hasOwnProperty (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value v;
	
	Op.assertParameterCount(ecc, 1);
	
	v = Value.toString(Op.argument(ecc, 0));
	ecc->this = Value.toObject(ecc->this, ecc, &(*ops)->text);
	ecc->result = Value.truth(getSlot(ecc->this.data.object, Key.makeWithText((struct Text){ Value.stringChars(v), Value.stringLength(v) }, 0)));
	
	return Value(undefined);
}

static struct Value isPrototypeOf (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value arg0;
	
	Op.assertParameterCount(ecc, 1);
	
	arg0 = Op.argument(ecc, 0);
	
	if (Value.isObject(arg0))
	{
		struct Object *v = arg0.data.object;
		struct Object *o = Value.toObject(ecc->this, ecc, &(*ops)->text).data.object;
		
		while (( v = v->prototype ))
			if (v == o)
			{
				ecc->result = Value(true);
				return Value(undefined);
			}
	}
	
	ecc->result = Value(false);
	return Value(undefined);
}

static struct Value propertyIsEnumerable (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value property;
	struct Object *object;
	struct Value *ref;
	
	Op.assertParameterCount(ecc, 1);
	
	property = Op.argument(ecc, 0);
	object = Value.toObject(ecc->this, ecc, &(*ops)->text).data.object;
	ref = getOwnProperty(object, property);
	
	if (ref)
		ecc->result = Value.truth(!(ref->flags & Value(hidden)));
	else
		ecc->result = Value(false);
	
	return Value(undefined);
}

static struct Value objectConstructor (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value;
	
	Op.assertParameterCount(ecc, 1);
	
	value = Op.argument(ecc, 0);
	
	if (value.type == Value(nullType) || value.type == Value(undefinedType))
		ecc->result = Value.object(Object.create(Object(prototype)));
	else if (ecc->construct && Value.isObject(value))
		ecc->result = value;
	else
		ecc->result = Value.toObject(value, ecc, &(*ops)->text);
	
	return Value(undefined);
}

static struct Value getPrototypeOf (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Object *object;
	
	Op.assertParameterCount(ecc, 1);
	
	object = checkObject(ops, ecc, Op.argument(ecc, 0));
	
	ecc->result = object->prototype? Value.object(object->prototype): Value(undefined);
	
	return Value(undefined);
}

static struct Value getOwnPropertyDescriptor (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Object *object;
	struct Value property;
	struct Value *ref;
	
	Op.assertParameterCount(ecc, 2);
	
	object = checkObject(ops, ecc, Op.argument(ecc, 0));
	property = Op.argument(ecc, 1);
	ref = getOwnProperty(object, property);
	
	ecc->result = Value(undefined);
	
	if (ref)
	{
		struct Object *result = Object.create(Object(prototype));
		
		Object.add(result, Key(value), *ref, 0);
		Object.add(result, Key(writable), Value.truth(!(ref->flags & Value(readonly))), 0);
		Object.add(result, Key(enumerable), Value.truth(!(ref->flags & Value(hidden))), 0);
		Object.add(result, Key(configurable), Value.truth(!(ref->flags & Value(sealed))), 0);
		
		ecc->result = Value.object(result);
	}
	
	return Value(undefined);
}

static struct Value getOwnPropertyNames (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Object *object;
	struct Object *result;
	uint32_t index, length;
	
	Op.assertParameterCount(ecc, 1);
	
	object = checkObject(ops, ecc, Op.argument(ecc, 0));
	result = Array.create();
	length = 0;
	
	for (index = 0; index < object->elementCount; ++index)
		if (object->element[index].data.value.check == 1)
			Object.addElementAtIndex(result, length++, Value.chars(Chars.create("%d", index)), 0);
	
	for (index = 2; index < object->hashmapCount; ++index)
		if (object->hashmap[index].data.value.check == 1)
			Object.addElementAtIndex(result, length++, Value.key(object->hashmap[index].data.key), 0);
	
	ecc->result = Value.object(result);
	
	return Value(undefined);
}

static struct Value objectCreate (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	#warning TODO
	return Value(undefined);
}

static struct Value defineProperty (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Object *property;
	struct Value value;
	enum Object(Flags) flags;
	
	Op.assertParameterCount(ecc, 1);
	
	property = checkObject(ops, ecc, Op.argument(ecc, 1));
	value = Object.get(property, Key(value));
	flags = 0;
	
	if (!Value.isTrue(Object.get(property, Key(enumerable))))
		flags |= Value(hidden);
	
	if (!Value.isTrue(Object.get(property, Key(configurable))))
		flags |= Value(sealed);
	
	if (!Value.isTrue(Object.get(property, Key(writable))))
		flags |= Value(readonly);
	
	#warning TODO
	
	return Value(undefined);
}

static struct Value defineProperties (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	#warning TODO
	return Value(undefined);
}

static struct Value seal (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Object *object;
	uint32_t index;
	
	Op.assertParameterCount(ecc, 1);
	
	object = checkObject(ops, ecc, Op.argument(ecc, 0));
	object->flags |= Object(sealed);
	
	for (index = 0; index < object->elementCount; ++index)
		if (object->element[index].data.value.check == 1)
			object->element[index].data.value.flags |= Value(sealed);
	
	for (index = 2; index < object->hashmapCount; ++index)
		if (object->hashmap[index].data.value.check == 1)
			object->hashmap[index].data.value.flags |= Value(sealed);
	
	ecc->result = Value.object(object);
	return Value(undefined);
}

static struct Value freeze (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Object *object;
	uint32_t index;
	
	Op.assertParameterCount(ecc, 1);
	
	object = checkObject(ops, ecc, Op.argument(ecc, 0));
	object->flags |= Object(sealed);
	
	for (index = 0; index < object->elementCount; ++index)
		if (object->element[index].data.value.check == 1)
			object->element[index].data.value.flags |= Value(readonly) | Value(sealed);
	
	for (index = 2; index < object->hashmapCount; ++index)
		if (object->hashmap[index].data.value.check == 1)
			object->hashmap[index].data.value.flags |= Value(readonly) | Value(sealed);
	
	ecc->result = Value.object(object);
	return Value(undefined);
}

static struct Value preventExtensions (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Object *object;
	
	Op.assertParameterCount(ecc, 1);
	
	object = checkObject(ops, ecc, Op.argument(ecc, 0));
	object->flags |= Object(sealed);
	
	ecc->result = Value.object(object);
	return Value(undefined);
}

static struct Value isSealed (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Object *object;
	uint32_t index;
	
	Op.assertParameterCount(ecc, 1);
	
	ecc->result = Value(true);
	
	object = checkObject(ops, ecc, Op.argument(ecc, 0));
	if (!(object->flags & Object(sealed)))
		ecc->result = Value(false);
	
	for (index = 0; index < object->elementCount; ++index)
		if (object->element[index].data.value.check == 1 && !(object->element[index].data.value.flags & Value(sealed)))
			ecc->result = Value(false);
	
	for (index = 2; index < object->hashmapCount; ++index)
		if (object->hashmap[index].data.value.check == 1 && !(object->hashmap[index].data.value.flags & Value(sealed)))
			ecc->result = Value(false);
	
	return Value(undefined);
}

static struct Value isFrozen (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Object *object;
	uint32_t index;
	
	Op.assertParameterCount(ecc, 1);
	
	ecc->result = Value(true);
	
	object = checkObject(ops, ecc, Op.argument(ecc, 0));
	if (!(object->flags & Object(sealed)))
		ecc->result = Value(false);
		
	for (index = 0; index < object->elementCount; ++index)
		if (object->element[index].data.value.check == 1 && !(object->element[index].data.value.flags & Value(readonly) && object->element[index].data.value.flags & Value(sealed)))
			ecc->result = Value(false);
	
	for (index = 2; index < object->hashmapCount; ++index)
		if (object->hashmap[index].data.value.check == 1 && !(object->hashmap[index].data.value.flags & Value(readonly) && object->hashmap[index].data.value.flags & Value(sealed)))
			ecc->result = Value(false);
	
	return Value(undefined);
}

static struct Value isExtensible (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Object *object;
	
	Op.assertParameterCount(ecc, 1);
	
	object = checkObject(ops, ecc, Op.argument(ecc, 0));
	ecc->result = Value.truth(!(object->flags & Object(sealed)));
	
	return Value(undefined);
}

static struct Value keys (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Object *object;
	struct Object *result;
	uint32_t index, length;
	
	Op.assertParameterCount(ecc, 1);
	
	object = checkObject(ops, ecc, Op.argument(ecc, 0));
	result = Array.create();
	length = 0;
	
	for (index = 0; index < object->elementCount; ++index)
		if (object->element[index].data.value.check == 1 && !(object->element[index].data.value.flags & Value(hidden)))
			Object.addElementAtIndex(result, length++, Value.chars(Chars.create("%d", index)), 0);
	
	for (index = 2; index < object->hashmapCount; ++index)
		if (object->hashmap[index].data.value.check == 1 && !(object->hashmap[index].data.value.flags & Value(hidden)))
			Object.addElementAtIndex(result, length++, Value.key(object->hashmap[index].data.key), 0);
	
	ecc->result = Value.object(result);
	
	return Value(undefined);
}

// MARK: - Static Members

// MARK: - Methods

struct Object * Object(prototype) = NULL;
struct Function * Object(constructor) = NULL;

void setup ()
{
	const enum Value(Flags) flags = Value(hidden);
	
	assert(sizeof(*Object(prototype)->hashmap) == 32);
	
	Function.addToObject(Object(prototype), "toString", toString, 0, flags);
	Function.addToObject(Object(prototype), "toLocaleString", toString, 0, flags);
	Function.addToObject(Object(prototype), "valueOf", valueOf, 0, flags);
	Function.addToObject(Object(prototype), "hasOwnProperty", hasOwnProperty, 1, flags);
	Function.addToObject(Object(prototype), "isPrototypeOf", isPrototypeOf, 1, flags);
	Function.addToObject(Object(prototype), "propertyIsEnumerable", propertyIsEnumerable, 1, flags);
	
	Object(constructor) = Function.createWithNative(objectConstructor, 1);
	Function.addToObject(&Object(constructor)->object, "getPrototypeOf", getPrototypeOf, 1, flags);
	Function.addToObject(&Object(constructor)->object, "getOwnPropertyDescriptor", getOwnPropertyDescriptor, 2, flags);
	Function.addToObject(&Object(constructor)->object, "getOwnPropertyNames", getOwnPropertyNames, 1, flags);
	Function.addToObject(&Object(constructor)->object, "create", objectCreate, -1, flags);
	Function.addToObject(&Object(constructor)->object, "defineProperty", defineProperty, 1, flags);
	Function.addToObject(&Object(constructor)->object, "defineProperties", defineProperties, 1, flags);
	Function.addToObject(&Object(constructor)->object, "seal", seal, 1, flags);
	Function.addToObject(&Object(constructor)->object, "freeze", freeze, 1, flags);
	Function.addToObject(&Object(constructor)->object, "preventExtensions", preventExtensions, 1, flags);
	Function.addToObject(&Object(constructor)->object, "isSealed", isSealed, 1, flags);
	Function.addToObject(&Object(constructor)->object, "isFrozen", isFrozen, 1, flags);
	Function.addToObject(&Object(constructor)->object, "isExtensible", isExtensible, 1, flags);
	Function.addToObject(&Object(constructor)->object, "keys", keys, 1, flags);
	Function.linkPrototype(Object(constructor), Object(prototype));
}

void teardown (void)
{
	Object(prototype) = NULL;
	Object(constructor) = NULL;
}

struct Object * create (struct Object *prototype)
{
	return createSized(prototype, defaultSize);
}

struct Object * createSized (struct Object *prototype, uint32_t size)
{
	struct Object *self = malloc(sizeof(*self));
	assert(self);
	Pool.addObject(self);
	return initializeSized(self, prototype, size);
}

struct Object * createTyped (const struct Text *type)
{
	struct Object *self = createSized(Object(prototype), defaultSize);
	self->type = type;
	return self;
}

struct Object * initialize (struct Object * restrict self, struct Object * restrict prototype)
{
	return initializeSized(self, prototype, defaultSize);
}

struct Object * initializeSized (struct Object * restrict self, struct Object * restrict prototype, uint32_t size)
{
	size_t byteSize;
	
	assert(self);
	assert(size >= 2);
	
	*self = Object.identity;
	
	self->type = prototype? prototype->type: &Text(objectType);
	
	self->prototype = prototype;
	self->hashmapCount = 2;
	self->hashmapCapacity = size;
	
	// hashmap is always 2 slots minimum
	// slot 0 is self referencing undefined value (all zeroes)
	// slot 1 is entry point, referencing undefined (slot 0) by default (all zeroes)
	
	byteSize = sizeof(*self->hashmap) * self->hashmapCapacity;
	self->hashmap = malloc(byteSize);
	memset(self->hashmap, 0, byteSize);
	
	return self;
}

struct Object * finalize (struct Object *self)
{
	assert(self);
	
	free(self->hashmap), self->hashmap = NULL;
	free(self->element), self->element = NULL;
	
	return self;
}

struct Object * copy (const struct Object *original)
{
	size_t byteSize;
	
	struct Object *self = malloc(sizeof(*self));
	assert(self);
	Pool.addObject(self);
	
	*self = *original;
	
	byteSize = sizeof(*self->element) * self->elementCapacity;
	self->element = malloc(byteSize);
	memcpy(self->element, original->element, byteSize);
	
	byteSize = sizeof(*self->hashmap) * self->hashmapCapacity;
	self->hashmap = malloc(byteSize);
	memcpy(self->hashmap, original->hashmap, byteSize);
	
	return self;
}

void destroy (struct Object *self)
{
	assert(self);
	
	finalize(self);
	
	free(self), self = NULL;
}

struct Value getOwn (struct Object *self, struct Key key)
{
	assert(self);
	
	return self->hashmap[getSlot(self, key)].data.value;
}

struct Value get (struct Object *self, struct Key key)
{
	struct Object *object = self;
	uint32_t slot = 0;
	
	assert(object);
	do
		slot = getSlot(object, key);
	while (!slot && (object = object->prototype));
	
	if (slot)
		return object->hashmap[slot].data.value;
	else
		return Value(undefined);
}

struct Value * getOwnMember (struct Object *self, struct Key key)
{
	struct Object *object = self;
	uint32_t slot;
	
	assert(object);
	if (( slot = getSlot(object, key) ))
		return &object->hashmap[slot].data.value;
	
	return NULL;
}

struct Value * getMember (struct Object *self, struct Key key)
{
	struct Object *object = self;
	uint32_t slot;
	
	assert(object);
	do
	{
		if (( slot = getSlot(object, key) ))
			return &object->hashmap[slot].data.value;
		
	}
	while ((object = object->prototype));
	
	return NULL;
}

struct Value * getOwnProperty (struct Object *self, struct Value property)
{
	struct Key key;
	int32_t element = getElementOrKey(property, &key);
	uint32_t slot;
	
	assert(self);
	
	if (element >= 0)
	{
		if (element < self->elementCount)
			return &self->element[element].data.value;
	}
	else if (( slot = getSlot(self, key) ))
		return &self->hashmap[slot].data.value;
	
	return NULL;
}

struct Value * getProperty (struct Object *self, struct Value property)
{
	struct Object *object = self;
	
	struct Key key;
	int32_t element = getElementOrKey(property, &key);
	uint32_t slot;
	
	assert(object);
	
	if (element >= 0)
		do
		{
			if (element < object->elementCount)
				return &object->element[element].data.value;
			
		} while ((object = object->prototype));
	else
		do
		{
			if (( slot = getSlot(object, key) ))
				return &object->hashmap[slot].data.value;
			
		} while ((object = object->prototype));
	
	return NULL;
}

struct Value * setProperty (struct Object *self, struct Value property, struct Value value)
{
	struct Object *object = self;
	
	struct Key key;
	int32_t element = getElementOrKey(property, &key);
	uint32_t slot;
	
	assert(object);
	
	if (element >= 0)
	{
		do
			if (element < object->elementCount)
			{
				if (!(self->element[element].data.value.flags & Value(readonly)))
					object->element[element].data.value = value;
				
				return &object->element[element].data.value;
			}
		while ((object = object->prototype));
		
		if (!(self->flags & Object(sealed)))
			return addElementAtIndex(self, element, value, 0);
	}
	else
	{
		do
			if (( slot = getSlot(object, key) ))
			{
				if (!(self->hashmap[slot].data.value.flags & Value(readonly)))
					object->hashmap[slot].data.value = value;
				
				return &object->hashmap[slot].data.value;
			}
		while ((object = object->prototype));
		
		if (!(self->flags & Object(sealed)))
			return add(self, key, value, 0);
	}
	
	return NULL;
}

struct Value * add (struct Object *self, struct Key key, struct Value value, enum Value(Flags) flags)
{
	uint32_t slot = 1;
	int depth = 0;
	
	assert(self);
	assert(key.data.integer);
	
	if (self->flags & Object(sealed))
		return NULL;
	
	do
	{
		if (!self->hashmap[slot].slot[key.data.depth[depth]])
		{
			int need = 4 - depth - (self->hashmapCapacity - self->hashmapCount);
			if (need > 0)
			{
				uint16_t capacity = self->hashmapCapacity;
				self->hashmapCapacity = self->hashmapCapacity? self->hashmapCapacity * 2: 2;
				self->hashmap = realloc(self->hashmap, sizeof(*self->hashmap) * self->hashmapCapacity);
				memset(self->hashmap + capacity, 0, sizeof(*self->hashmap) * (self->hashmapCapacity - capacity));
			}
			
			do
			{
				slot = self->hashmap[slot].slot[key.data.depth[depth]] = self->hashmapCount++;
			} while (++depth < 4);
			
			self->hashmap[slot].data.key = key;
			break;
		}
		else
			assert(self->hashmap[slot].data.value.check != 1);
		
		slot = self->hashmap[slot].slot[key.data.depth[depth]];
		assert(slot != 1);
		assert(slot < self->hashmapCount);
	} while (++depth < 4);
	
	if (value.type == Value(functionType) && value.data.function->flags & Function(isAccessor))
		if (self->hashmap[slot].data.value.check == 1 && self->hashmap[slot].data.value.type == Value(functionType) && self->hashmap[slot].data.value.data.function->flags & Function(isAccessor))
			if ((self->hashmap[slot].data.value.data.function->flags & Function(isAccessor)) != (value.data.function->flags & Function(isAccessor)))
				value.data.function->pair = self->hashmap[slot].data.value.data.function;
	
	self->hashmap[slot].data.value = value;
	self->hashmap[slot].data.value.flags = flags;
	
	return &self->hashmap[slot].data.value;
}

int delete (struct Object *self, struct Key key)
{
	struct Object *object = self;
	uint32_t slot;
	
	assert(object);
	assert(key.data.integer);
	
	do
		slot = getSlot(object, key);
	while (!slot && (object = object->prototype));
	
	if (!object || !(object->hashmap[slot].data.value.check == 1))
		return 1;
	
	if (object->hashmap[slot].data.value.flags & Value(sealed))
		return 0;
	
	memset(&object->hashmap[slot], 0, sizeof(*object->hashmap));
	return 1;
}

int deleteProperty (struct Object *self, struct Value property)
{
	struct Key key;
	int32_t element = getElementOrKey(property, &key);
	uint32_t slot;
	
	assert(self);
	
	if (element >= 0)
	{
		if (element < self->elementCount)
		{
			if (self->element[element].data.value.flags & Value(sealed))
				return 0;
			
			memset(&self->element[element], 0, sizeof(*self->element));
		}
	}
	else if (( slot = getSlot(self, key) ))
	{
		if (self->hashmap[slot].data.value.flags & Value(sealed))
			return 0;
		
		memset(&self->hashmap[slot], 0, sizeof(*self->hashmap));
	}
	
	return 1;
}

void packValue (struct Object *self)
{
	__typeof__(*self->hashmap) data;
	uint32_t index = 2, valueIndex = 2, copyIndex, slot;
	
	assert(self);
	
	for (; index < self->hashmapCount; ++index)
		if (self->hashmap[index].data.value.check == 1)
		{
			data = self->hashmap[index];
			for (copyIndex = index; copyIndex > valueIndex; --copyIndex)
			{
				self->hashmap[copyIndex] = self->hashmap[copyIndex - 1];
				if (!(self->hashmap[copyIndex].data.value.check == 1))
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
	
	self->hashmap = realloc(self->hashmap, sizeof(*self->hashmap) * (self->hashmapCount));
	self->hashmapCapacity = self->hashmapCount;
	
	if (self->elementCount)
		self->elementCapacity = self->elementCount;
}

void stripMap (struct Object *self)
{
	uint32_t index = 2;
	
	assert(self);
	
	while (index < self->hashmapCount && self->hashmap[index].data.value.check == 1)
		++index;
	
//	fprintf(stderr, "%d->%d\n", self->hashmapCount, index);
	
	self->hashmapCount = index;
	self->hashmapCapacity = self->hashmapCount;
	self->hashmap = realloc(self->hashmap, sizeof(*self->hashmap) * self->hashmapCapacity);
}

void resizeElement (struct Object *self, uint32_t size)
{
	uint32_t capacity = size < 8? 8: nextPowerOfTwo(size);
	
	assert(self);
	
	if (capacity != self->elementCapacity)
	{
		self->element = realloc(self->element, sizeof(*self->element) * capacity);
		if (capacity > self->elementCapacity)
			memset(self->element + self->elementCapacity, 0, sizeof(*self->element) * (capacity - self->elementCapacity));
		
		self->elementCapacity = capacity;
	}
	else if (size < self->elementCount)
		memset(self->element + size, 0, sizeof(*self->element) * (self->elementCount - size));
	
	self->elementCount = size;
}

struct Value * addElementAtIndex (struct Object *self, uint32_t index, struct Value value, enum Value(Flags) flags)
{
	assert(self);
	
	if (self->elementCapacity <= index)
		resizeElement(self, index + 1);
	else if (self->elementCount <= index)
		self->elementCount = index + 1;
	
	self->element[index].data.value = value;
	self->element[index].data.value.flags = flags;
	
	return &self->element[index].data.value;
}

void dumpTo(struct Object *self, FILE *file)
{
	uint32_t index;
	int isArray;
	
	assert(self);
	
	isArray = self->prototype == Array(prototype);
	
	fprintf(file, isArray? "[ ": "{ ");
	
	for (index = 0; index < self->elementCount; ++index)
	{
		if (!isArray)
			fprintf(file, "%d: ", (int)index);
		
		if (self->element[index].data.value.check == 1)
		{
			Value.dumpTo(self->element[index].data.value, file);
			fprintf(file, ", ");
		}
	}
	
	if (!isArray)
	{
		for (index = 0; index < self->hashmapCount; ++index)
		{
			if (self->hashmap[index].data.value.check == 1)
			{
				fprintf(stderr, "'");
				Key.dumpTo(self->hashmap[index].data.key, file);
				fprintf(file, "': ");
				Value.dumpTo(self->hashmap[index].data.value, file);
				fprintf(file, ", ");
			}
//			else
//			{
//				fprintf(file, "\n");
//				for (int j = 0; j < 16; ++j)
//					fprintf(file, "%02x ", self->hashmap[index].slot[j]);
//			}
		}
	}
	
	fprintf(file, isArray? "]": "}");
}
