//
//  object.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#define Implementation
#include "object.h"

#include "../ecc.h"
#include "../pool.h"
#include "../lexer.h"
#include "../op.h"

// MARK: - Private

const uint32_t Object(ElementMax) = 0xffffff;

static const int defaultSize = 8;

struct Object * Object(prototype) = NULL;
struct Function * Object(constructor) = NULL;

const struct Object(Type) Object(type) = {
	.text = &Text(objectType),
};

// MARK: - Static Members

static inline
uint16_t getSlot (const struct Object * const self, const struct Key key)
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

static inline
uint32_t getIndexOrKey (struct Value property, struct Key *key)
{
	uint32_t index = UINT32_MAX;
	
	assert(Value.isPrimitive(property));
	
	if (property.type == Value(keyType))
		*key = property.data.key;
	else
	{
		if (property.type == Value(integerType) && property.data.integer >= 0)
			index = property.data.integer;
		else if (property.type == Value(binaryType) && property.data.binary >= 0 && property.data.binary < UINT32_MAX && property.data.binary == (uint32_t)property.data.binary)
			index = property.data.binary;
		else if (Value.isString(property))
		{
			struct Text text = Value.textOf(&property);
			if ((index = Lexer.scanElement(text)) == UINT32_MAX)
				*key = Key.makeWithText(text, Key(copyOnCreate));
		}
		else
			return getIndexOrKey(Value.toString(NULL, property), key);
	}
	
	return index;
}

static inline
struct Key keyOfIndex (uint32_t index, int create)
{
	char buffer[10 + 1];
	uint16_t length;
	
	length = snprintf(buffer, sizeof(buffer), "%u", (unsigned)index);
	if (create)
		return Key.makeWithText(Text.make(buffer, length), Key(copyOnCreate));
	else
		return Key.search(Text.make(buffer, length));
}

static inline
uint32_t nextPowerOfTwo(uint32_t v)
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

static inline
uint32_t elementCount (struct Object *self)
{
	if (self->elementCount < Object(ElementMax))
		return self->elementCount;
	else
		return Object(ElementMax);
}

static
void readonlyError(struct Context * const context, struct Value *ref, struct Object *this)
{
	struct Text text;
	
	do
	{
		union Object(Hashmap) *hashmap = (union Object(Hashmap) *)ref;
		union Object(Element) *element = (union Object(Element) *)ref;
		
		if (hashmap >= this->hashmap && hashmap < this->hashmap + this->hashmapCount)
		{
			const struct Text *keyText = Key.textOf(hashmap->value.key);
			Context.typeError(context, Chars.create("'%.*s' is read-only", keyText->length, keyText->bytes));
		}
		else if (element >= this->element && element < this->element + this->elementCount)
			Context.typeError(context, Chars.create("'%u' is read-only", element - this->element));
		
	} while (( this = this->prototype ));
	
	text = Context.textSeek(context);
	Context.typeError(context, Chars.create("'%.*s' is read-only", text.length, text.bytes));
}

//

static
struct Object *checkObject (struct Context * const context, int argument)
{
	struct Value value = Context.argument(context, argument);
	if (!Value.isObject(value))
		Context.typeError(context, Chars.create("not an object"));
	
	return value.data.object;
}

static
struct Value valueOf (struct Context * const context)
{
	return Value.toObject(context, Context.this(context));
}

static
struct Value hasOwnProperty (struct Context * const context)
{
	struct Object *self;
	struct Value value;
	struct Key key;
	uint32_t index;
	
	self = Value.toObject(context, Context.this(context)).data.object;
	value = Value.toPrimitive(context, Context.argument(context, 0), Value(hintString));
	index = getIndexOrKey(value, &key);
	
	if (index < UINT32_MAX)
		return Value.truth(element(self, index, Value(asOwn)) != NULL);
	else
		return Value.truth(member(self, key, Value(asOwn)) != NULL);
}

static
struct Value isPrototypeOf (struct Context * const context)
{
	struct Value arg0;
	
	arg0 = Context.argument(context, 0);
	
	if (Value.isObject(arg0))
	{
		struct Object *v = arg0.data.object;
		struct Object *o = Value.toObject(context, Context.this(context)).data.object;
		
		do
			if (v == o)
				return Value(true);
		while (( v = v->prototype ));
	}
	
	return Value(false);
}

static
struct Value propertyIsEnumerable (struct Context * const context)
{
	struct Value value;
	struct Object *object;
	struct Value *ref;
	
	value = Value.toPrimitive(context, Context.argument(context, 0), Value(hintString));
	object = Value.toObject(context, Context.this(context)).data.object;
	ref = property(object, value, Value(asOwn));
	
	if (ref)
		return Value.truth(!(ref->flags & Value(hidden)));
	else
		return Value(false);
}

static
struct Value constructor (struct Context * const context)
{
	struct Value value;
	
	value = Context.argument(context, 0);
	
	if (value.type == Value(nullType) || value.type == Value(undefinedType))
		return Value.object(create(Object(prototype)));
	else if (context->construct && Value.isObject(value))
		return value;
	else
		return Value.toObject(context, value);
}

static
struct Value getPrototypeOf (struct Context * const context)
{
	struct Object *object;
	
	object = Value.toObject(context, Context.argument(context, 0)).data.object;
	
	return object->prototype? Value.objectValue(object->prototype): Value(undefined);
}

static
struct Value getOwnPropertyDescriptor (struct Context * const context)
{
	struct Object *object;
	struct Value value;
	struct Value *ref;
	
	object = Value.toObject(context, Context.argument(context, 0)).data.object;
	value = Value.toPrimitive(context, Context.argument(context, 1), Value(hintString));
	ref = property(object, value, Value(asOwn));
	
	if (ref)
	{
		struct Object *result = create(Object(prototype));
		
		if (ref->flags & Value(accessor))
		{
			if (ref->flags & Value(asData))
			{
				addMember(result, Key(value), Object.getValue(context, object, ref), 0);
				addMember(result, Key(writable), Value.truth(!(ref->flags & Value(readonly))), 0);
			}
			else
			{
				addMember(result, ref->flags & Value(getter)? Key(get): Key(set), Value.function(ref->data.function), 0);
				if (ref->data.function->pair)
					addMember(result, ref->flags & Value(getter)? Key(set): Key(get), Value.function(ref->data.function->pair), 0);
			}
		}
		else
		{
			addMember(result, Key(value), *ref, 0);
			addMember(result, Key(writable), Value.truth(!(ref->flags & Value(readonly))), 0);
		}
		
		addMember(result, Key(enumerable), Value.truth(!(ref->flags & Value(hidden))), 0);
		addMember(result, Key(configurable), Value.truth(!(ref->flags & Value(sealed))), 0);
		
		return Value.object(result);
	}
	
	return Value(undefined);
}

static
struct Value getOwnPropertyNames (struct Context * const context)
{
	struct Object *object, *parent;
	struct Object *result;
	uint32_t index, count, length;
	
	object = checkObject(context, 0);
	result = Array.create();
	length = 0;
	
	for (index = 0, count = elementCount(object); index < count; ++index)
		if (object->element[index].value.check == 1)
			addElement(result, length++, Value.chars(Chars.create("%d", index)), 0);
	
	parent = object;
	while (( parent = parent->prototype ))
	{
		for (index = 2; index < parent->hashmapCount; ++index)
		{
			struct Value value = parent->hashmap[index].value;
			if (value.check == 1 && value.flags & Value(asOwn))
				addElement(result, length++, Value.text(Key.textOf(value.key)), 0);
		}
	}
	
	for (index = 2; index < object->hashmapCount; ++index)
		if (object->hashmap[index].value.check == 1)
			addElement(result, length++, Value.text(Key.textOf(object->hashmap[index].value.key)), 0);
	
	return Value.object(result);
}

static
struct Value defineProperty (struct Context * const context)
{
	struct Object *object, *descriptor;
	struct Value property, value, *getter, *setter, *current, *flag;
	struct Key key;
	uint32_t index;
	
	object = checkObject(context, 0);
	property = Value.toPrimitive(context, Context.argument(context, 1), Value(hintString));
	descriptor = checkObject(context, 2);
	
	getter = member(descriptor, Key(get), 0);
	setter = member(descriptor, Key(set), 0);
	
	current = Object.property(object, property, Value(asOwn));
	
	if (getter || setter)
	{
		if (getter && getter->type == Value(undefinedType))
			getter = NULL;
		
		if (setter && setter->type == Value(undefinedType))
			setter = NULL;
		
		if (getter && getter->type != Value(functionType))
			Context.typeError(context, Chars.create("getter is not a function"));
		
		if (setter && setter->type != Value(functionType))
			Context.typeError(context, Chars.create("setter is not a function"));
		
		if (member(descriptor, Key(value), 0) || member(descriptor, Key(writable), 0))
			Context.typeError(context, Chars.create("value & writable forbidden when a getter or setter are set"));
		
		if (getter)
		{
			value = *getter;
			if (setter)
				value.data.function->pair = setter->data.function;
			
			value.flags |= Value(getter);
		}
		else if (setter)
		{
			value = *setter;
			value.flags |= Value(setter);
		}
		else
		{
			value = Value.function(Function.createWithNative(Op.noop, 0));
			value.flags |= Value(getter);
		}
	}
	else
	{
		value = getMember(context, descriptor, Key(value));
		
		flag = member(descriptor, Key(writable), 0);
		if ((flag && !Value.isTrue(getValue(context, descriptor, flag))) || (!flag && (!current || current->flags & Value(readonly))))
			value.flags |= Value(readonly);
	}
	
	flag = member(descriptor, Key(enumerable), 0);
	if ((flag && !Value.isTrue(getValue(context, descriptor, flag))) || (!flag && (!current || current->flags & Value(hidden))))
		value.flags |= Value(hidden);
	
	flag = member(descriptor, Key(configurable), 0);
	if ((flag && !Value.isTrue(getValue(context, descriptor, flag))) || (!flag && (!current || current->flags & Value(sealed))))
		value.flags |= Value(sealed);
	
	if (!current)
	{
		addProperty(object, property, value, 0);
		return Value(true);
	}
	else
	{
		if (current->flags & Value(sealed))
		{
			if (!(value.flags & Value(sealed)) || (value.flags & Value(hidden)) != (current->flags & Value(hidden)))
				goto sealedError;
			
			if (current->flags & Value(accessor))
			{
				if (!(getter || setter))
				{
					if (current->flags & Value(asData))
					{
						struct Function *currentSetter = current->flags & Value(getter)? current->data.function->pair: current->data.function;
						if (currentSetter)
						{
							Context.callFunction(context, currentSetter, Value.object(object), 1, value);
							return Value(true);
						}
					}
					goto sealedError;
				}
				else
				{
					struct Function *currentGetter = current->flags & Value(getter)? current->data.function: current->data.function->pair;
					struct Function *currentSetter = current->flags & Value(getter)? current->data.function->pair: current->data.function;
					
					if (!getter != !currentGetter || !setter != !currentSetter)
						goto sealedError;
					else if (getter && getter->data.function->pair != currentGetter)
						goto sealedError;
					else if (setter && setter->data.function != currentSetter)
						goto sealedError;
				}
			}
			else
			{
				if (!Value.isTrue(Value.same(context, *current, value)))
					goto sealedError;
			}
		}
		
		addProperty(object, property, value, 0);
	}
	
	return Value(true);
	
sealedError:
	Context.setTextIndexArgument(context, 1);
	index = getIndexOrKey(property, &key);
	if (index == UINT32_MAX)
	{
		const struct Text *text = Key.textOf(key);
		Context.typeError(context, Chars.create("'%.*s' is non-configurable", text->length, text->bytes));
	}
	else
		Context.typeError(context, Chars.create("'%u' is non-configurable", index));
}

static
struct Value defineProperties (struct Context * const context)
{
	union Object(Hashmap) *originalHashmap = context->environment->hashmap;
	uint16_t originalHashmapCount = context->environment->hashmapCount;
	
	uint16_t index, count, hashmapCount = 6;
	struct Object *object, *properties;
	union Object(Hashmap) hashmap[hashmapCount];
	
	memset(hashmap, 0, hashmapCount * sizeof(*hashmap));
	
	object = checkObject(context, 0);
	properties = Value.toObject(context, Context.argument(context, 1)).data.object;
	
	context->environment->hashmap = hashmap;
	context->environment->hashmapCount = hashmapCount;
	
	Context.replaceArgument(context, 0, Value.object(object));
	
	for (index = 0, count = elementCount(properties); index < count; ++index)
	{
		if (!properties->element[index].value.check)
			continue;
		
		Context.replaceArgument(context, 1, Value.binary(index));
		Context.replaceArgument(context, 2, properties->element[index].value);
		defineProperty(context);
	}
	
	for (index = 2; index < properties->hashmapCount; ++index)
	{
		if (!properties->hashmap[index].value.check)
			continue;
		
		Context.replaceArgument(context, 1, Value.key(properties->hashmap[index].value.key));
		Context.replaceArgument(context, 2, properties->hashmap[index].value);
		defineProperty(context);
	}
	
	context->environment->hashmap = originalHashmap;
	context->environment->hashmapCount = originalHashmapCount;
	
	return Value(undefined);
}

static
struct Value objectCreate (struct Context * const context)
{
	struct Object *object, *result;
	struct Value properties;
	
	object = checkObject(context, 0);
	properties = Context.argument(context, 1);
	
	result = create(object);
	if (properties.type != Value(undefinedType))
	{
		Context.replaceArgument(context, 0, Value.object(result));
		defineProperties(context);
	}
	
	return Value.object(result);
}

static
struct Value seal (struct Context * const context)
{
	struct Object *object;
	uint32_t index, count;
	
	object = checkObject(context, 0);
	object->flags |= Object(sealed);
	
	for (index = 0, count = elementCount(object); index < count; ++index)
		if (object->element[index].value.check == 1)
			object->element[index].value.flags |= Value(sealed);
	
	for (index = 2; index < object->hashmapCount; ++index)
		if (object->hashmap[index].value.check == 1)
			object->hashmap[index].value.flags |= Value(sealed);
	
	return Value.object(object);
}

static
struct Value freeze (struct Context * const context)
{
	struct Object *object;
	uint32_t index, count;
	
	object = checkObject(context, 0);
	object->flags |= Object(sealed);
	
	for (index = 0, count = elementCount(object); index < count; ++index)
		if (object->element[index].value.check == 1)
			object->element[index].value.flags |= Value(frozen);
	
	for (index = 2; index < object->hashmapCount; ++index)
		if (object->hashmap[index].value.check == 1)
			object->hashmap[index].value.flags |= Value(frozen);
	
	return Value.object(object);
}

static
struct Value preventExtensions (struct Context * const context)
{
	struct Object *object;
	
	object = checkObject(context, 0);
	object->flags |= Object(sealed);
	
	return Value.object(object);
}

static
struct Value isSealed (struct Context * const context)
{
	struct Object *object;
	uint32_t index, count;
	
	object = checkObject(context, 0);
	if (!(object->flags & Object(sealed)))
		return Value(false);
	
	for (index = 0, count = elementCount(object); index < count; ++index)
		if (object->element[index].value.check == 1 && !(object->element[index].value.flags & Value(sealed)))
			return Value(false);
	
	for (index = 2; index < object->hashmapCount; ++index)
		if (object->hashmap[index].value.check == 1 && !(object->hashmap[index].value.flags & Value(sealed)))
			return Value(false);
	
	return Value(true);
}

static
struct Value isFrozen (struct Context * const context)
{
	struct Object *object;
	uint32_t index, count;
	
	object = checkObject(context, 0);
	if (!(object->flags & Object(sealed)))
		return Value(false);
	
	for (index = 0, count = elementCount(object); index < count; ++index)
		if (object->element[index].value.check == 1 && !(object->element[index].value.flags & Value(frozen)))
			return Value(false);
	
	for (index = 2; index < object->hashmapCount; ++index)
		if (object->hashmap[index].value.check == 1 && !(object->hashmap[index].value.flags & Value(frozen)))
			return Value(false);
	
	return Value(true);
}

static
struct Value isExtensible (struct Context * const context)
{
	struct Object *object;
	
	object = checkObject(context, 0);
	return Value.truth(!(object->flags & Object(sealed)));
}

static
struct Value keys (struct Context * const context)
{
	struct Object *object, *parent;
	struct Object *result;
	uint32_t index, count, length;
	
	object = checkObject(context, 0);
	result = Array.create();
	length = 0;
	
	for (index = 0, count = elementCount(object); index < count; ++index)
		if (object->element[index].value.check == 1 && !(object->element[index].value.flags & Value(hidden)))
			addElement(result, length++, Value.chars(Chars.create("%d", index)), 0);
	
	parent = object;
	while (( parent = parent->prototype ))
	{
		for (index = 2; index < parent->hashmapCount; ++index)
		{
			struct Value value = parent->hashmap[index].value;
			if (value.check == 1 && value.flags & Value(asOwn) & !(value.flags & Value(hidden)))
				addElement(result, length++, Value.text(Key.textOf(value.key)), 0);
		}
	}
	
	for (index = 2; index < object->hashmapCount; ++index)
		if (object->hashmap[index].value.check == 1 && !(object->hashmap[index].value.flags & Value(hidden)))
			addElement(result, length++, Value.text(Key.textOf(object->hashmap[index].value.key)), 0);
	
	return Value.object(result);
}

// MARK: - Methods

void setup ()
{
	const enum Value(Flags) h = Value(hidden);
	
	assert(sizeof(*Object(prototype)->hashmap) == 32);
	
	Function.setupBuiltinObject(
		&Object(constructor), constructor, 1,
		NULL, Value.object(Object(prototype)),
		NULL);
	
	Function.addMethod(Object(constructor), "getPrototypeOf", getPrototypeOf, 1, h);
	Function.addMethod(Object(constructor), "getOwnPropertyDescriptor", getOwnPropertyDescriptor, 2, h);
	Function.addMethod(Object(constructor), "getOwnPropertyNames", getOwnPropertyNames, 1, h);
	Function.addMethod(Object(constructor), "create", objectCreate, 2, h);
	Function.addMethod(Object(constructor), "defineProperty", defineProperty, 3, h);
	Function.addMethod(Object(constructor), "defineProperties", defineProperties, 2, h);
	Function.addMethod(Object(constructor), "seal", seal, 1, h);
	Function.addMethod(Object(constructor), "freeze", freeze, 1, h);
	Function.addMethod(Object(constructor), "preventExtensions", preventExtensions, 1, h);
	Function.addMethod(Object(constructor), "isSealed", isSealed, 1, h);
	Function.addMethod(Object(constructor), "isFrozen", isFrozen, 1, h);
	Function.addMethod(Object(constructor), "isExtensible", isExtensible, 1, h);
	Function.addMethod(Object(constructor), "keys", keys, 1, h);
	
	Function.addToObject(Object(prototype), "toString", toString, 0, h);
	Function.addToObject(Object(prototype), "toLocaleString", toString, 0, h);
	Function.addToObject(Object(prototype), "valueOf", valueOf, 0, h);
	Function.addToObject(Object(prototype), "hasOwnProperty", hasOwnProperty, 1, h);
	Function.addToObject(Object(prototype), "isPrototypeOf", isPrototypeOf, 1, h);
	Function.addToObject(Object(prototype), "propertyIsEnumerable", propertyIsEnumerable, 1, h);
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
	struct Object *self = calloc(sizeof(*self), 1);
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
		// size is == zero = to be initialized later by caller
		self->hashmapCapacity = 2;
	
	return self;
}

struct Object * finalize (struct Object *self)
{
	assert(self);
	
	if (self->type->finalize)
		self->type->finalize(self);
	
	free(self->hashmap), self->hashmap = NULL;
	free(self->element), self->element = NULL;
	
	return self;
}

struct Object * copy (const struct Object *original)
{
	size_t byteSize;
	
	struct Object *self = malloc(sizeof(*self));
	Pool.addObject(self);
	
	*self = *original;
	
	byteSize = sizeof(*self->element) * self->elementCount;
	self->element = malloc(byteSize);
	memcpy(self->element, original->element, byteSize);
	
	byteSize = sizeof(*self->hashmap) * self->hashmapCount;
	self->hashmap = malloc(byteSize);
	memcpy(self->hashmap, original->hashmap, byteSize);
	
	return self;
}

void destroy (struct Object *self)
{
	assert(self);
	
	free(self), self = NULL;
}

struct Value * member (struct Object *self, struct Key member, enum Value(Flags) flags)
{
	int lookupChain = !(flags & Value(asOwn));
	struct Object *object = self;
	struct Value *ref = NULL;
	uint32_t slot;
	
	assert(self);
	
	do
	{
		if (( slot = getSlot(object, member) ))
		{
			ref = &object->hashmap[slot].value;
			if (ref->check == 1)
				return lookupChain || object == self || (ref->flags & flags) ? ref: NULL;
		}
	}
	while ((object = object->prototype));
	
	return NULL;
}

struct Value * element (struct Object *self, uint32_t index, enum Value(Flags) flags)
{
	int lookupChain = !(flags & Value(asOwn));
	struct Object *object = self;
	struct Value *ref = NULL;
	
	assert(self);
	
	if (self->type == &String(type))
	{
		struct Value *ref = Object.addMember(self, Key(none), String.valueAtIndex((struct String *)self, index), 0);
		ref->check = 0;
		return ref;
	}
	else if (index > Object(ElementMax))
	{
		struct Key key = keyOfIndex(index, 0);
		if (key.data.integer)
			return member(self, key, flags);
	}
	else
		do
		{
			if (index < object->elementCount)
			{
				ref = &object->element[index].value;
				if (ref->check == 1)
					return lookupChain || object == self || (ref->flags & flags) ? ref: NULL;
			}
		}
		while ((object = object->prototype));
	
	return NULL;
}

struct Value * property (struct Object *self, struct Value property, enum Value(Flags) flags)
{
	struct Key key;
	uint32_t index = getIndexOrKey(property, &key);
	
	if (index < UINT32_MAX)
		return element(self, index, flags);
	else
		return member(self, key, flags);
}

struct Value getValue (struct Context *context, struct Object *self, struct Value *ref)
{
	if (!ref)
		return Value(undefined);
	
	if (ref->flags & Value(accessor))
	{
		if (!context)
			Ecc.fatal("cannot use getter outside context");
		
		if (ref->flags & Value(getter))
			return Context.callFunction(context, ref->data.function, Value.object(self), 0 | Context(asAccessor));
		else if (ref->data.function->pair)
			return Context.callFunction(context, ref->data.function->pair, Value.object(self), 0 | Context(asAccessor));
		else
			return Value(undefined);
	}
	
	return *ref;
}

struct Value getMember (struct Context *context, struct Object *self, struct Key key)
{
	return getValue(context, self, member(self, key, 0));
}

struct Value getElement (struct Context *context, struct Object *self, uint32_t index)
{
	if (self->type == &String(type))
		return String.valueAtIndex((struct String *)self, index);
	else
		return getValue(context, self, element(self, index, 0));
}

struct Value getProperty (struct Context *context, struct Object *self, struct Value property)
{
	struct Key key;
	uint32_t index = getIndexOrKey(property, &key);
	
	if (index < UINT32_MAX)
		return getElement(context, self, index);
	else
		return getMember(context, self, key);
}

struct Value putValue (struct Context *context, struct Object *self, struct Value *ref, struct Value value)
{
	if (ref->flags & Value(accessor))
	{
		assert(context);
		
		if (ref->flags & Value(setter))
			Context.callFunction(context, ref->data.function, Value.object(self), 1 | Context(asAccessor), value);
		else if (ref->data.function->pair)
			Context.callFunction(context, ref->data.function->pair, Value.object(self), 1 | Context(asAccessor), value);
		else if (context->strictMode || (context->parent && context->parent->strictMode))
			readonlyError(context, ref, self);
		
		return value;
	}
	
	if (ref->check == 1)
	{
		if (ref->flags & Value(readonly))
		{
			if (context->strictMode)
				readonlyError(context, ref, self);
			else
				return value;
		}
		else
			value.flags = ref->flags;
	}
	
	return *ref = value;
}

struct Value putMember (struct Context *context, struct Object *self, struct Key key, struct Value value)
{
	struct Value *ref;
	
	value.flags = 0;
	
	if (( ref = member(self, key, Value(asOwn) | Value(accessor)) ))
		return putValue(context, self, ref, value);
	else if (self->prototype && ( ref = member(self->prototype, key, 0) ))
	{
		if (ref->flags & Value(readonly))
			Context.typeError(context, Chars.create("'%.*s' is readonly", Key.textOf(key)->length, Key.textOf(key)->bytes));
	}
	
	if (self->flags & Object(sealed))
		Context.typeError(context, Chars.create("object is not extensible"));
	
	return *addMember(self, key, value, 0);
}

struct Value putElement (struct Context *context, struct Object *self, uint32_t index, struct Value value)
{
	struct Value *ref;
	
	if (index > Object(ElementMax))
	{
		if (self->elementCapacity <= index)
			resizeElement(self, index < UINT32_MAX? index + 1: index);
		else if (self->elementCount <= index)
			self->elementCount = index + 1;
		
		return putMember(context, self, keyOfIndex(index, 1), value);
	}
	
	value.flags = 0;
	
	if (( ref = element(self, index, Value(asOwn) | Value(accessor)) ))
		return putValue(context, self, ref, value);
	else if (self->prototype && ( ref = element(self, index, 0) ))
	{
		if (ref->flags & Value(readonly))
			Context.typeError(context, Chars.create("'%u' is readonly", index, index));
	}
	
	if (self->flags & Object(sealed))
		Context.typeError(context, Chars.create("object is not extensible"));
	
	return *addElement(self, index, value, 0);
}

struct Value putProperty (struct Context *context, struct Object *self, struct Value primitive, struct Value value)
{
	struct Key key;
	uint32_t index = getIndexOrKey(primitive, &key);
	
	if (index < UINT32_MAX)
		return putElement(context, self, index, value);
	else
		return putMember(context, self, key, value);
}

struct Value * addMember (struct Object *self, struct Key key, struct Value value, enum Value(Flags) flags)
{
	uint32_t slot = 1;
	int depth = 0;
	
	assert(self);
	
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
			break;
		}
		else
			assert(self->hashmap[slot].value.check != 1);
		
		slot = self->hashmap[slot].slot[key.data.depth[depth]];
		assert(slot != 1);
		assert(slot < self->hashmapCount);
	} while (++depth < 4);
	
	if (value.flags & Value(accessor))
		if (self->hashmap[slot].value.check == 1 && self->hashmap[slot].value.flags & Value(accessor))
			if ((self->hashmap[slot].value.flags & Value(accessor)) != (value.flags & Value(accessor)))
				value.data.function->pair = self->hashmap[slot].value.data.function;
	
	value.key = key;
	value.flags |= flags;
	
	self->hashmap[slot].value = value;
	
	return &self->hashmap[slot].value;
}

struct Value * addElement (struct Object *self, uint32_t index, struct Value value, enum Value(Flags) flags)
{
	struct Value *ref;
	
	assert(self);
	
	if (self->elementCapacity <= index)
		resizeElement(self, index < UINT32_MAX? index + 1: index);
	else if (self->elementCount <= index)
		self->elementCount = index + 1;
	
	if (index > Object(ElementMax))
		return addMember(self, keyOfIndex(index, 1), value, flags);
	
	ref = &self->element[index].value;
	
	value.flags |= flags;
	*ref = value;
	
	return ref;
}

struct Value * addProperty (struct Object *self, struct Value primitive, struct Value value, enum Value(Flags) flags)
{
	struct Key key;
	uint32_t index = getIndexOrKey(primitive, &key);
	
	if (index < UINT32_MAX)
		return addElement(self, index, value, flags);
	else
		return addMember(self, key, value, flags);
}

int deleteMember (struct Object *self, struct Key member)
{
	struct Object *object = self;
	uint32_t slot, refSlot;
	
	assert(object);
	assert(member.data.integer);
	
	refSlot =
		self->hashmap[
		self->hashmap[
		self->hashmap[1]
		.slot[member.data.depth[0]]]
		.slot[member.data.depth[1]]]
		.slot[member.data.depth[2]];
	
	slot = self->hashmap[refSlot].slot[member.data.depth[3]];
	
	if (!slot || !(object->hashmap[slot].value.check == 1))
		return 1;
	
	if (object->hashmap[slot].value.flags & Value(sealed))
		return 0;
	
	object->hashmap[slot].value = Value(undefined);
	self->hashmap[refSlot].slot[member.data.depth[3]] = 0;
	return 1;
}

int deleteElement (struct Object *self, uint32_t index)
{
	assert(self);
	
	if (index > Object(ElementMax))
	{
		struct Key key = keyOfIndex(index, 0);
		if (key.data.integer)
			return deleteMember(self, key);
		else
			return 1;
	}
	
	if (index < self->elementCount)
	{
		if (self->element[index].value.flags & Value(sealed))
			return 0;
		
		memset(&self->element[index], 0, sizeof(*self->element));
	}
	
	return 1;
}

int deleteProperty (struct Object *self, struct Value primitive)
{
	struct Key key;
	uint32_t index = getIndexOrKey(primitive, &key);
	
	if (index < UINT32_MAX)
		return deleteElement(self, index);
	else
		return deleteMember(self, key);
}

void packValue (struct Object *self)
{
	union Object(Hashmap) data;
	uint32_t index = 2, valueIndex = 2, copyIndex, slot;
	
	assert(self);
	
	for (; index < self->hashmapCount; ++index)
		if (self->hashmap[index].value.check == 1)
		{
			data = self->hashmap[index];
			for (copyIndex = index; copyIndex > valueIndex; --copyIndex)
			{
				self->hashmap[copyIndex] = self->hashmap[copyIndex - 1];
				if (!(self->hashmap[copyIndex].value.check == 1))
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
	
	while (index < self->hashmapCount && self->hashmap[index].value.check == 1)
		++index;
	
	self->hashmapCapacity = self->hashmapCount = index;
	self->hashmap = realloc(self->hashmap, sizeof(*self->hashmap) * self->hashmapCapacity);
	
	memset(self->hashmap + 1, 0, sizeof(*self->hashmap));
}

void reserveSlots (struct Object *self, uint16_t slots)
{
	int need = (slots * 4) - (self->hashmapCapacity - self->hashmapCount);
	
	assert(slots < self->hashmapCapacity);
	
	if (need > 0)
	{
		uint16_t capacity = self->hashmapCapacity;
		self->hashmapCapacity = self->hashmapCapacity? self->hashmapCapacity * 2: 2;
		self->hashmap = realloc(self->hashmap, sizeof(*self->hashmap) * self->hashmapCapacity);
		memset(self->hashmap + capacity, 0, sizeof(*self->hashmap) * (self->hashmapCapacity - capacity));
	}
}

int resizeElement (struct Object *self, uint32_t size)
{
	uint32_t capacity;
	
	if (size <= self->elementCapacity)
		capacity = self->elementCapacity;
	else if (size < 4)
	{
		/* 64-bytes mini */
		capacity = 4;
	}
	else if (size < 64)
	{
		/* power of two steps between */
		capacity = nextPowerOfTwo(size);
	}
	else if (size > Object(ElementMax))
		capacity = Object(ElementMax) + 1;
	else
	{
		/* 1024-bytes chunk */
		capacity = size - 1;
		capacity |= 63;
		++capacity;
	}
	
	assert(self);
	
	if (capacity != self->elementCapacity)
	{
		if (size > Object(ElementMax))
		{
			Env.printWarning("Faking array length of %u while actual physical length is %u. Using array length > 0x%x is discouraged", size, capacity, Object(ElementMax));
		}
		
		self->element = realloc(self->element, sizeof(*self->element) * capacity);
		if (capacity > self->elementCapacity)
			memset(self->element + self->elementCapacity, 0, sizeof(*self->element) * (capacity - self->elementCapacity));
		
		self->elementCapacity = capacity;
	}
	else if (size < self->elementCount)
	{
		union Object(Element) *element;
		uint32_t until = size, e;
		
		if (self->elementCount > Object(ElementMax))
		{
			union Object(Hashmap) *hashmap;
			uint32_t index, h;
			
			for (h = 2; h < self->hashmapCount; ++h)
			{
				hashmap = &self->hashmap[h];
				if (hashmap->value.check == 1)
				{
					index = Lexer.scanElement(*Key.textOf(hashmap->value.key));
					if (hashmap->value.check == 1 && (hashmap->value.flags & Value(sealed)) && index >= until)
						until = index + 1;
				}
			}
			
			for (h = 2; h < self->hashmapCount; ++h)
			{
				hashmap = &self->hashmap[h];
				if (hashmap->value.check == 1)
					if (Lexer.scanElement(*Key.textOf(hashmap->value.key)) >= until)
						self->hashmap[h].value.check = 0;
			}
			
			if (until > size)
			{
				self->elementCount = until;
				return 1;
			}
			self->elementCount = self->elementCapacity;
		}
		
		for (e = size; e < self->elementCount; ++e)
		{
			element = &self->element[e];
			if (element->value.check == 1 && (element->value.flags & Value(sealed)) && e >= until)
				until = e + 1;
		}
		
		memset(self->element + until, 0, sizeof(*self->element) * (self->elementCount - until));
		
		if (until > size)
		{
			self->elementCount = until;
			return 1;
		}
	}
	self->elementCount = size;
	
	return 0;
}

void populateElementWithCList (struct Object *self, uint32_t count, const char * list[])
{
	double binary;
	char *end;
	int index;
	
	assert(self);
	assert(count <= Object(ElementMax));
	
	if (count > self->elementCount)
		resizeElement(self, count);
	
	for (index = 0; index < count; ++index)
	{
		uint16_t length = (uint16_t)strlen(list[index]);
		binary = strtod(list[index], &end);
		
		if (end == list[index] + length)
			self->element[index].value = Value.binary(binary);
		else
		{
			struct Chars *chars = Chars.createSized(length);
			memcpy(chars->bytes, list[index], length);
			
			self->element[index].value = Value.chars(chars);
		}
	}
}

struct Value toString (struct Context * const context)
{
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

void dumpTo(struct Object *self, FILE *file)
{
	uint32_t index, count;
	int isArray;
	
	assert(self);
	
	isArray = Value.objectIsArray(self);
	
	fprintf(file, isArray? "[ ": "{ ");
	
	for (index = 0, count = elementCount(self); index < count; ++index)
	{
		if (self->element[index].value.check == 1)
		{
			if (!isArray)
				fprintf(file, "%d: ", (int)index);
			
			if (self->element[index].value.type == Value(objectType) && self->element[index].value.data.object == self)
				fprintf(file, "this");
			else
				Value.dumpTo(self->element[index].value, file);
			
			fprintf(file, ", ");
		}
	}
	
	if (!isArray)
	{
		for (index = 0; index < self->hashmapCount; ++index)
		{
			if (self->hashmap[index].value.check == 1)
			{
				fprintf(stderr, "'");
				Key.dumpTo(self->hashmap[index].value.key, file);
				fprintf(file, "': ");
				
				if (self->hashmap[index].value.type == Value(objectType) && self->hashmap[index].value.data.object == self)
					fprintf(file, "this");
				else
					Value.dumpTo(self->hashmap[index].value, file);
				
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
