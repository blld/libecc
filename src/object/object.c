//
//  object.c
//  libecc
//
//  Created by Bouilland Aurélien on 04/07/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#include "object.h"

// MARK: - Private

struct Object * Object(prototype) = NULL;
struct Function * Object(constructor) = NULL;

const struct Object(Type) Object(type) = {
	.text = &Text(objectType),
};

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
	
	assert(Value.isPrimitive(property));
	
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
				struct Text text = Text.make(Value.stringBytes(property), Value.stringLength(property));
				
				if ((element = Lexer.parseElement(text)) < 0)
					*key = Key.makeWithText(text, 1);
			}
			else
			{
				uint16_t length = Value.toLength(NULL, property);
				{
					char bytes[length + 1];
					struct Text text = Text.make(bytes, length);
					
					Value.toBytes(NULL, property, bytes);
					
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

static struct Object *checkObject (struct Native(Context) * const context, int argument)
{
	struct Value value = Native.argument(context, argument);
	if (!Value.isObject(value))
		Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(Native.textSeek(context, argument), "not an object")));
	
	return value.data.object;
}

static struct Value toString (struct Native(Context) * const context)
{
	Native.assertParameterCount(context, 0);
	
	if (context->this.type == Value(nullType))
		return Value.text(&Text(nullType));
	else if (context->this.type == Value(undefinedType))
		return Value.text(&Text(undefinedType));
	else if (Value.isString(context->this))
		return Value.text(&Text(stringType));
	else if (Value.isNumber(context->this))
		return Value.text(&Text(numberType));
	else if (Value.isBoolean(context->this))
		return Value.text(&Text(booleanType));
	else if (Value.isObject(context->this))
		return Value.text(context->this.data.object->type->text);
	else
		assert(0);
	
	return Value(undefined);
}

static struct Value valueOf (struct Native(Context) * const context)
{
	Native.assertParameterCount(context, 0);
	
	return Value.toObject(context, context->this, Native(thisIndex));
}

static struct Value hasOwnProperty (struct Native(Context) * const context)
{
	struct Value v;
	
	Native.assertParameterCount(context, 1);
	
	v = Value.toString(context, Native.argument(context, 0));
	context->this = Value.toObject(context, context->this, Native(thisIndex));
	return Value.truth(getSlot(context->this.data.object, Key.makeWithText((struct Text){ Value.stringBytes(v), Value.stringLength(v) }, 0)));
}

static struct Value isPrototypeOf (struct Native(Context) * const context)
{
	struct Value arg0;
	
	Native.assertParameterCount(context, 1);
	
	arg0 = Native.argument(context, 0);
	
	if (Value.isObject(arg0))
	{
		struct Object *v = arg0.data.object;
		struct Object *o = Value.toObject(context, context->this, Native(thisIndex)).data.object;
		
		while (( v = v->prototype ))
			if (v == o)
				return Value(true);
	}
	
	return Value(false);
}

static struct Value propertyIsEnumerable (struct Native(Context) * const context)
{
	struct Value property;
	struct Object *object;
	struct Value *ref;
	
	Native.assertParameterCount(context, 1);
	
	property = Native.argument(context, 0);
	object = Value.toObject(context, context->this, Native(thisIndex)).data.object;
	ref = getOwnProperty(object, property);
	
	if (ref)
		return Value.truth(!(ref->flags & Value(hidden)));
	else
		return Value(false);
}

static struct Value objectConstructor (struct Native(Context) * const context)
{
	struct Value value;
	
	Native.assertParameterCount(context, 1);
	
	value = Native.argument(context, 0);
	
	if (value.type == Value(nullType) || value.type == Value(undefinedType))
		return Value.object(Object.create(Object(prototype)));
	else if (context->construct && Value.isObject(value))
		return value;
	else
		return Value.toObject(context, context->this, Native(thisIndex));
}

static struct Value getPrototypeOf (struct Native(Context) * const context)
{
	struct Object *object;
	
	Native.assertParameterCount(context, 1);
	
	object = checkObject(context, 0);
	
	return object->prototype? Value.object(object->prototype): Value(undefined);
}

static struct Value getOwnPropertyDescriptor (struct Native(Context) * const context)
{
	struct Object *object;
	struct Value property;
	struct Value *ref;
	
	Native.assertParameterCount(context, 2);
	
	object = checkObject(context, 0);
	property = Native.argument(context, 1);
	ref = getOwnProperty(object, property);
	
	if (ref)
	{
		struct Object *result = Object.create(Object(prototype));
		
		Object.add(result, Key(value), *ref, 0);
		Object.add(result, Key(writable), Value.truth(!(ref->flags & Value(readonly))), 0);
		Object.add(result, Key(enumerable), Value.truth(!(ref->flags & Value(hidden))), 0);
		Object.add(result, Key(configurable), Value.truth(!(ref->flags & Value(sealed))), 0);
		
		return Value.object(result);
	}
	return Value(undefined);
}

static struct Value getOwnPropertyNames (struct Native(Context) * const context)
{
	struct Object *object;
	struct Object *result;
	uint32_t index, length;
	
	Native.assertParameterCount(context, 1);
	
	object = checkObject(context, 0);
	result = Array.create();
	length = 0;
	
	for (index = 0; index < object->elementCount; ++index)
		if (object->element[index].data.value.check == 1)
			Object.addElementAtIndex(result, length++, Value.chars(Chars.create("%d", index)), 0);
	
	for (index = 2; index < object->hashmapCount; ++index)
		if (object->hashmap[index].data.value.check == 1)
			Object.addElementAtIndex(result, length++, Value.key(object->hashmap[index].data.key), 0);
	
	return Value.object(result);
}

static struct Value objectCreate (struct Native(Context) * const context)
{
	Native.assertParameterCount(context, 2);
	#warning TODO
	return Value(undefined);
}

static struct Value defineProperty (struct Native(Context) * const context)
{
	struct Object *object, *descriptor;
	struct Value property, value, *getter, *setter;
	
	Native.assertParameterCount(context, 3);
	
	object = checkObject(context, 0);
	property = Value.toString(context, Native.argument(context, 1));
	descriptor = checkObject(context, 2);
	
	if (!Value.isTrue(Object.get(descriptor, Key(enumerable))))
		value.flags |= Value(hidden);
	
	if (!Value.isTrue(Object.get(descriptor, Key(configurable))))
		value.flags |= Value(sealed);
	
	getter = Object.getOwnMember(descriptor, Key(get));
	setter = Object.getOwnMember(descriptor, Key(set));
	
	if (getter || setter)
	{
		if (Object.getOwnMember(descriptor, Key(value)) || Object.getOwnMember(descriptor, Key(writable)))
			Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(Native.textSeek(context, 2), "property descriptors must not specify a value or be writable when a getter or setter has been specified")));
		
		if (getter)
		{
			value = *getter;
			if (setter)
				value.data.function->pair = setter->data.function;
			
			value.data.function->flags |= Function(isGetter);
		}
		else
		{
			value = *setter;
			value.data.function->flags |= Function(isSetter);
		}
	}
	else
	{
		value = Object.get(descriptor, Key(value));
		
		if (!Value.isTrue(Object.get(descriptor, Key(writable))))
			value.flags |= Value(readonly);
	}
	
	setProperty(object, property, value);
	
	return Value(undefined);
}

static struct Value defineProperties (struct Native(Context) * const context)
{
	Native.assertParameterCount(context, 1);
	#warning TODO
	return Value(undefined);
}

static struct Value seal (struct Native(Context) * const context)
{
	struct Object *object;
	uint32_t index;
	
	Native.assertParameterCount(context, 1);
	
	object = checkObject(context, 0);
	object->flags |= Object(sealed);
	
	for (index = 0; index < object->elementCount; ++index)
		if (object->element[index].data.value.check == 1)
			object->element[index].data.value.flags |= Value(sealed);
	
	for (index = 2; index < object->hashmapCount; ++index)
		if (object->hashmap[index].data.value.check == 1)
			object->hashmap[index].data.value.flags |= Value(sealed);
	
	return Value.object(object);
}

static struct Value freeze (struct Native(Context) * const context)
{
	struct Object *object;
	uint32_t index;
	
	Native.assertParameterCount(context, 1);
	
	object = checkObject(context, 0);
	object->flags |= Object(sealed);
	
	for (index = 0; index < object->elementCount; ++index)
		if (object->element[index].data.value.check == 1)
			object->element[index].data.value.flags |= Value(frozen);
	
	for (index = 2; index < object->hashmapCount; ++index)
		if (object->hashmap[index].data.value.check == 1)
			object->hashmap[index].data.value.flags |= Value(frozen);
	
	return Value.object(object);
}

static struct Value preventExtensions (struct Native(Context) * const context)
{
	struct Object *object;
	
	Native.assertParameterCount(context, 1);
	
	object = checkObject(context, 0);
	object->flags |= Object(sealed);
	
	return Value.object(object);
}

static struct Value isSealed (struct Native(Context) * const context)
{
	struct Object *object;
	uint32_t index;
	
	Native.assertParameterCount(context, 1);
	
	object = checkObject(context, 0);
	if (!(object->flags & Object(sealed)))
		return Value(false);
	
	for (index = 0; index < object->elementCount; ++index)
		if (object->element[index].data.value.check == 1 && !(object->element[index].data.value.flags & Value(sealed)))
			return Value(false);
	
	for (index = 2; index < object->hashmapCount; ++index)
		if (object->hashmap[index].data.value.check == 1 && !(object->hashmap[index].data.value.flags & Value(sealed)))
			return Value(false);
	
	return Value(true);
}

static struct Value isFrozen (struct Native(Context) * const context)
{
	struct Object *object;
	uint32_t index;
	
	Native.assertParameterCount(context, 1);
	
	object = checkObject(context, 0);
	if (!(object->flags & Object(sealed)))
		return Value(false);
		
	for (index = 0; index < object->elementCount; ++index)
		if (object->element[index].data.value.check == 1 && !(object->element[index].data.value.flags & Value(readonly) && object->element[index].data.value.flags & Value(sealed)))
			return Value(false);
	
	for (index = 2; index < object->hashmapCount; ++index)
		if (object->hashmap[index].data.value.check == 1 && !(object->hashmap[index].data.value.flags & Value(readonly) && object->hashmap[index].data.value.flags & Value(sealed)))
			return Value(false);
	
	return Value(true);
}

static struct Value isExtensible (struct Native(Context) * const context)
{
	struct Object *object;
	
	Native.assertParameterCount(context, 1);
	
	object = checkObject(context, 0);
	return Value.truth(!(object->flags & Object(sealed)));
}

static struct Value keys (struct Native(Context) * const context)
{
	struct Object *object;
	struct Object *result;
	uint32_t index, length;
	
	Native.assertParameterCount(context, 1);
	
	object = checkObject(context, 0);
	result = Array.create();
	length = 0;
	
	for (index = 0; index < object->elementCount; ++index)
		if (object->element[index].data.value.check == 1 && !(object->element[index].data.value.flags & Value(hidden)))
			Object.addElementAtIndex(result, length++, Value.chars(Chars.create("%d", index)), 0);
	
	for (index = 2; index < object->hashmapCount; ++index)
		if (object->hashmap[index].data.value.check == 1 && !(object->hashmap[index].data.value.flags & Value(hidden)))
			Object.addElementAtIndex(result, length++, Value.key(object->hashmap[index].data.key), 0);
	
	return Value.object(result);
}

// MARK: - Static Members

// MARK: - Methods

void setup ()
{
	const enum Value(Flags) flags = Value(hidden);
	
	assert(sizeof(*Object(prototype)->hashmap) == 32);
	
	Function.setupBuiltinObject(&Object(constructor), objectConstructor, 1, NULL, Value.object(Object(prototype)), NULL);
	
	Function.addToObject(Object(prototype), "toString", toString, 0, flags);
	Function.addToObject(Object(prototype), "toLocaleString", toString, 0, flags);
	Function.addToObject(Object(prototype), "valueOf", valueOf, 0, flags);
	Function.addToObject(Object(prototype), "hasOwnProperty", hasOwnProperty, 1, flags);
	Function.addToObject(Object(prototype), "isPrototypeOf", isPrototypeOf, 1, flags);
	Function.addToObject(Object(prototype), "propertyIsEnumerable", propertyIsEnumerable, 1, flags);
	
	Function.addToObject(&Object(constructor)->object, "getPrototypeOf", getPrototypeOf, 1, flags);
	Function.addToObject(&Object(constructor)->object, "getOwnPropertyDescriptor", getOwnPropertyDescriptor, 2, flags);
	Function.addToObject(&Object(constructor)->object, "getOwnPropertyNames", getOwnPropertyNames, 1, flags);
	Function.addToObject(&Object(constructor)->object, "create", objectCreate, -1, flags);
	Function.addToObject(&Object(constructor)->object, "defineProperty", defineProperty, 3, flags);
	Function.addToObject(&Object(constructor)->object, "defineProperties", defineProperties, 2, flags);
	Function.addToObject(&Object(constructor)->object, "seal", seal, 1, flags);
	Function.addToObject(&Object(constructor)->object, "freeze", freeze, 1, flags);
	Function.addToObject(&Object(constructor)->object, "preventExtensions", preventExtensions, 1, flags);
	Function.addToObject(&Object(constructor)->object, "isSealed", isSealed, 1, flags);
	Function.addToObject(&Object(constructor)->object, "isFrozen", isFrozen, 1, flags);
	Function.addToObject(&Object(constructor)->object, "isExtensible", isExtensible, 1, flags);
	Function.addToObject(&Object(constructor)->object, "keys", keys, 1, flags);
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

struct Object * createSized (struct Object *prototype, uint16_t size)
{
	struct Object *self = malloc(sizeof(*self));
	assert(self);
	Pool.addObject(self);
	return initializeSized(self, prototype, size);
}

struct Object * createTyped (const struct Object(Type) *type)
{
	struct Object *self = createSized(Object(prototype), defaultSize);
	self->type = type;
	return self;
}

struct Object * initialize (struct Object * restrict self, struct Object * restrict prototype)
{
	return initializeSized(self, prototype, defaultSize);
}

struct Object * initializeSized (struct Object * restrict self, struct Object * restrict prototype, uint16_t size)
{
	size_t byteSize;
	
	assert(self);
	
	*self = Object.identity;
	
	self->type = prototype? prototype->type: &Object(type);
	
	self->prototype = prototype;
	self->hashmapCount = 2;
	
	// hashmap is always 2 slots minimum
	// slot 0 is self referencing undefined value (all zeroes)
	// slot 1 is entry point, referencing undefined (slot 0) by default (all zeroes)
	
	if (size > 0)
	{
		self->hashmapCapacity = size;
		
		byteSize = sizeof(*self->hashmap) * self->hashmapCapacity;
		self->hashmap = malloc(byteSize);
		memset(self->hashmap, 0, byteSize);
	}
	else
		// size is < zero = to be initialized later by caller
		self->hashmapCapacity = 2;
	
	return self;
}

struct Object * finalize (struct Object *self)
{
	assert(self);
	
	free(self->hashmap), self->hashmap = NULL;
	free(self->element), self->element = NULL;
	
	if (self->type->finalize)
		self->type->finalize(self);
	
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
				if (self->element[element].data.value.flags & Value(readonly))
					return NULL;
				
				object->element[element].data.value = value;
			}
		while ((object = object->prototype));
		
		if (self->flags & Object(sealed))
			return NULL;
		
		return addElementAtIndex(self, element, value, value.flags);
	}
	else
	{
		do
			if (( slot = getSlot(object, key) ))
			{
				if (self->hashmap[slot].data.value.flags & Value(readonly))
					return NULL;
				
				object->hashmap[slot].data.value = value;
			}
		while ((object = object->prototype));
		
		if (self->flags & Object(sealed))
			return NULL;
		
		return add(self, key, value, value.flags);
	}
}

struct Value * add (struct Object *self, struct Key key, struct Value value, enum Value(Flags) flags)
{
	uint32_t slot = 1;
	int depth = 0;
	
	assert(self);
	assert(key.data.integer);
	
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
				assert(self->hashmapCount < UINT16_MAX);
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
	typeof(*self->hashmap) data;
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
	
	self->hashmapCapacity = self->hashmapCount = index;
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

void populateElementWithCList (struct Object *self, int count, const char * list[])
{
	double binary;
	char *end;
	int index;
	
	if (count > self->elementCount)
		Object.resizeElement(self, count);
	
	for (index = 0; index < count; ++index)
	{
		uint16_t length = (uint16_t)strlen(list[index]);
		binary = strtod(list[index], &end);
		
		if (end == list[index] + length)
		{
			self->element[index].data.value = Value.binary(binary);
			self->element[index].data.value.flags = 0;
		}
		else
		{
			struct Chars *chars = Chars.createSized(length);
			memcpy(chars->bytes, list[index], length);
			
			self->element[index].data.value = Value.chars(chars);
			self->element[index].data.value.flags = 0;
		}
	}
}

void dumpTo(struct Object *self, FILE *file)
{
	uint32_t index;
	int isArray;
	
	assert(self);
	
	isArray = self->type == &Array(type);
	
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
