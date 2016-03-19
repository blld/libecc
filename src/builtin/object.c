//
//  object.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
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

static inline uint32_t getElementOrKey (struct Value property, struct Key *key)
{
	uint32_t element = UINT32_MAX;
	
	assert(Value.isPrimitive(property));
	
	if (property.type == Value(keyType))
		*key = property.data.key;
	else
	{
		if (property.type == Value(integerType) && property.data.integer >= 0)
			element = property.data.integer;
		else if (property.type == Value(binaryType) && property.data.binary >= 0 && property.data.binary < UINT32_MAX && property.data.binary == (uint32_t)property.data.binary)
			element = property.data.binary;
		else if (Value.isString(property))
		{
			struct Text text = Text.make(Value.stringBytes(property), Value.stringLength(property));
			if ((element = Lexer.parseElement(text)) == UINT32_MAX)
				*key = Key.makeWithText(text, 1);
		}
		else
			return getElementOrKey(Value.toString(NULL, property), key);
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

static const struct Value propertyTypeError(struct Native(Context) * const context, struct Value *ref, struct Value this, const char *description, const struct Text text)
{
	if (Value.isObject(this))
	{
		__typeof__(this.data.object->hashmap) hashmap = (__typeof__(hashmap))ref;
		__typeof__(this.data.object->element) element = (__typeof__(element))ref;
		
		if (hashmap >= this.data.object->hashmap && hashmap < this.data.object->hashmap + this.data.object->hashmapCount)
		{
			const struct Text *keyText = Key.textOf(hashmap->data.key);
			return Value.error(Error.typeError(text, "'%.*s' %s", keyText->length, keyText->bytes, description));
		}
		else if (element >= this.data.object->element && element < this.data.object->element + this.data.object->elementCount)
			return Value.error(Error.typeError(text, "'%d' %s", element - this.data.object->element, description));
	}
	return Value.error(Error.typeError(text, "'%.*s' %s", text.length, text.bytes, description));
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
	struct Value value;
	struct Object *object;
	struct Value *ref;
	int own = 1;
	
	Native.assertParameterCount(context, 1);
	
	value = Native.argument(context, 0);
	object = Value.toObject(context, context->this, Native(thisIndex)).data.object;
	ref = property(object, value, &own);
	
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
		return Value.object(create(Object(prototype)));
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
	struct Value value;
	struct Value *ref;
	int own = 1;
	
	Native.assertParameterCount(context, 2);
	
	object = checkObject(context, 0);
	value = Native.argument(context, 1);
	ref = property(object, value, &own);
	
	if (ref)
	{
		struct Object *result = create(Object(prototype));
		
		if (ref->flags & Value(accessor))
		{
			addMember(result, ref->flags & Value(getter)? Key(get): Key(set), Value.function(ref->data.function), 0);
			if (ref->data.function->pair)
				addMember(result, ref->flags & Value(getter)? Key(set): Key(get), Value.function(ref->data.function->pair), 0);
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
			addElement(result, length++, Value.chars(Chars.create("%d", index)), 0);
	
	for (index = 2; index < object->hashmapCount; ++index)
		if (object->hashmap[index].data.value.check == 1)
			addElement(result, length++, Value.key(object->hashmap[index].data.key), 0);
	
	return Value.object(result);
}

static struct Value defineProperty (struct Native(Context) * const context)
{
	struct Object *object, *descriptor;
	struct Value property, value, *getter, *setter, *current;
	struct Text text = Native.textSeek(context, Native(callIndex));
	struct Key key;
	uint32_t element;
	int own = 1;
	
	Native.assertParameterCount(context, 3);
	
	object = checkObject(context, 0);
	property = Value.toString(context, Native.argument(context, 1));
	descriptor = checkObject(context, 2);
	current = Object.property(object, property, &own);
	
	getter = member(descriptor, Key(get), NULL);
	setter = member(descriptor, Key(set), NULL);
	
	if (getter || setter)
	{
		if (getter && getter->type != Value(functionType))
			Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(Native.textSeek(context, 2), "property descriptor's getter field is neither undefined nor a function")));
		
		if (setter && setter->type != Value(functionType))
			Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(Native.textSeek(context, 2), "property descriptor's setter field is neither undefined nor a function")));
		
		if (member(descriptor, Key(value), NULL) || member(descriptor, Key(writable), NULL))
			Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(Native.textSeek(context, 2), "property descriptors must not specify a value or be writable when a getter or setter has been specified")));
		
		if (getter)
		{
			value = *getter;
			if (setter)
				value.data.function->pair = setter->data.function;
			
			value.flags |= Value(getter);
		}
		else
		{
			value = *setter;
			value.flags |= Value(setter);
		}
	}
	else
	{
		value = getMember(descriptor, Key(value), context);
		
		if (!Value.isTrue(getMember(descriptor, Key(writable), context)))
			value.flags |= Value(readonly);
	}
	
	if (!Value.isTrue(getMember(descriptor, Key(enumerable), context)))
		value.flags |= Value(hidden);
	
	if (!Value.isTrue(getMember(descriptor, Key(configurable), context)))
		value.flags |= Value(sealed);
	
	element = getElementOrKey(property, &key);
	
	if (object->type == &Array(type) && element == UINT32_MAX && Key.isEqual(key, Key(length)))
	{
		putProperty(object, property, context, value, &text);
		return Value(true);
	}
	else if (!current)
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
					goto sealedError;
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
				if (!Value.isTrue(Value.same(context, *current, value, &text, &text)))
					goto sealedError;
			}
		}
		
		addProperty(object, property, value, 0);
	}
	
	return Value(true);
	
sealedError:
	Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(Native.textSeek(context, 1), "property is non-configurable")));
}

static struct Value defineProperties (struct Native(Context) * const context)
{
	typeof(context->environment->hashmap) originalHashmap = context->environment->hashmap;
	uint16_t originalHashmapCount = context->environment->hashmapCount;
	
	uint16_t index, hashmapCount = 6;
	struct Object *object, *properties;
	typeof(*context->environment->hashmap) hashmap[hashmapCount];
	
	Native.assertParameterCount(context, 2);
	
	object = checkObject(context, 0);
	properties = Value.toObject(context, Native.argument(context, 1), 1).data.object;
	
	context->environment->hashmap = hashmap;
	context->environment->hashmapCount = hashmapCount;
	
	Native.replaceArgument(context, 0, Value.object(object));
	
	for (index = 0; index < properties->elementCount; ++index)
	{
		if (!properties->element[index].data.value.check)
			continue;
		
		Native.replaceArgument(context, 1, Value.binary(index));
		Native.replaceArgument(context, 2, properties->element[index].data.value);
		defineProperty(context);
	}
	
	for (index = 2; index < properties->hashmapCount; ++index)
	{
		if (!properties->hashmap[index].data.value.check)
			continue;
		
		Native.replaceArgument(context, 1, Value.key(properties->hashmap[index].data.key));
		Native.replaceArgument(context, 2, properties->hashmap[index].data.value);
		defineProperty(context);
	}
	
	context->environment->hashmap = originalHashmap;
	context->environment->hashmapCount = originalHashmapCount;
	
	return Value(undefined);
}

static struct Value objectCreate (struct Native(Context) * const context)
{
	struct Object *object, *result;
	struct Value properties;
	Native.assertParameterCount(context, 2);
	
	object = checkObject(context, 0);
	properties = Native.argument(context, 1);
	
	result = create(object);
	if (properties.type != Value(undefinedType))
	{
		Native.replaceArgument(context, 0, Value.object(result));
		defineProperties(context);
	}
	
	return Value.object(result);
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
			addElement(result, length++, Value.chars(Chars.create("%d", index)), 0);
	
	for (index = 2; index < object->hashmapCount; ++index)
		if (object->hashmap[index].data.value.check == 1 && !(object->hashmap[index].data.value.flags & Value(hidden)))
			addElement(result, length++, Value.key(object->hashmap[index].data.key), 0);
	
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
	Function.addToObject(&Object(constructor)->object, "create", objectCreate, 2, flags);
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

struct Value * member (struct Object *self, struct Key member, int *own)
{
	struct Object *object = self;
	const int searchOwn = !own || !*own;
	uint32_t slot;
	
	assert(object);
	
	do
	{
		if (( slot = getSlot(object, member) ) && object->hashmap[slot].data.value.check == 1)
		{
			if (own)
				*own = object == self;
			
			return &object->hashmap[slot].data.value;
		}
	}
	while (searchOwn && (object = object->prototype));
	
	return NULL;
}

struct Value * element (struct Object *self, uint32_t element, int *own)
{
	struct Object *object = self;
	const int searchOwn = !own || !*own;
	
	assert(object);
	
	do
	{
		if (element < object->elementCount && object->element[element].data.value.check == 1)
		{
			if (own)
				*own = object == self;
			
			return &object->element[element].data.value;
		}
	} while (searchOwn && (object = object->prototype));
	
	return NULL;
}

struct Value * property (struct Object *self, struct Value property, int *own)
{
	struct Key key;
	uint32_t index = getElementOrKey(property, &key);
	
	if (index < UINT32_MAX)
		return element(self, index, own);
	else
		return member(self, key, own);
}

struct Value getValue (struct Object *self, struct Value *ref, struct Native(Context) * const context)
{
	if (!ref)
		return Value(undefined);
	
	if (ref->flags & Value(accessor))
	{
		if (!context)
			Ecc.fatal("cannot use getter outside context");
		
		if (ref->flags & Value(getter))
			return Op.callFunctionVA(context, 0, ref->data.function, Value.object(self), 0);
		else if (ref->data.function->pair)
			return Op.callFunctionVA(context, 0, ref->data.function->pair, Value.object(self), 0);
		else
			return Value(undefined);
	}
	
	return *ref;
}

struct Value getMember (struct Object *self, struct Key key, struct Native(Context) * const context)
{
	return getValue(self, member(self, key, NULL), context);
}

struct Value getElement (struct Object *self, uint32_t index, struct Native(Context) * const context)
{
	return getValue(self, element(self, index, NULL), context);
}

struct Value getProperty (struct Object *self, struct Value property, struct Native(Context) * const context)
{
	struct Key key;
	uint32_t element = getElementOrKey(property, &key);
	
	if (element < UINT32_MAX)
		return getElement(self, element, context);
	else
		return getMember(self, key, context);
}

struct Value *putValue (struct Object *self, struct Value *ref, struct Native(Context) * const context, struct Value value, const struct Text *text)
{
	if (ref->flags & Value(accessor))
	{
		if (!context)
			Ecc.fatal("cannot use setter outside context");
		
		if (ref->flags & Value(setter))
			Op.callFunctionVA(context, 0, ref->data.function, Value.object(self), 1, value);
		else if (ref->data.function->pair)
			Op.callFunctionVA(context, 0, ref->data.function->pair, Value.object(self), 1, value);
		else
			Ecc.jmpEnv(context->ecc, propertyTypeError(context, ref, Value.object(self), "is read-only accessor", *text));
		
		return ref;
	}
	
	if (ref->check == 1)
	{
		if (ref->flags & Value(readonly))
			Ecc.jmpEnv(context->ecc, propertyTypeError(context, ref, Value.object(self), "is read-only property", *text));
		
		value.flags = ref->flags;
	}
	else if (self->flags & Object(sealed))
		Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(*text, "object is not extensible")));
	else
		value.flags = 0;
	
	*ref = value;
	return ref;
}

struct Value *putMember (struct Object *self, struct Key key, struct Native(Context) * const context, struct Value value, const struct Text *text)
{
	int own = 0;
	struct Value *ref = member(self, key, &own);
	if (ref)
	{
		if (own || ref->flags & Value(accessor))
			return putValue(self, ref, context, value, text);
		else if (ref->flags & Value(readonly))
			Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(*text, "'%.*s' is readonly", Key.textOf(key)->length, Key.textOf(key)->bytes)));
	}
	if (self->flags & Object(sealed))
		Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(*text, "object is not extensible")));
	
	return addMember(self, key, value, 0);
}

struct Value *putElement (struct Object *self, uint32_t index, struct Native(Context) * const context, struct Value value, const struct Text *text)
{
	int own = 0;
	struct Value *ref = element(self, index, &own);
	if (own)
		return putValue(self, ref, context, value, text);
	else if (self->flags & Object(sealed))
		Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(*text, "object is not extensible")));
	
	return addElement(self, index, value, 0);
}

struct Value *putProperty (struct Object *self, struct Value property, struct Native(Context) * const context, struct Value value, const struct Text *text)
{
	struct Key key;
	uint32_t element = getElementOrKey(property, &key);
	
	if (element < UINT32_MAX)
		return putElement(self, element, context, value, text);
	else
		return putMember(self, key, context, value, text);
}

struct Value * addMember (struct Object *self, struct Key key, struct Value value, enum Value(Flags) flags)
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
	
	if (value.flags & Value(accessor))
		if (self->hashmap[slot].data.value.check == 1 && self->hashmap[slot].data.value.flags & Value(accessor))
			if ((self->hashmap[slot].data.value.flags & Value(accessor)) != (value.flags & Value(accessor)))
				value.data.function->pair = self->hashmap[slot].data.value.data.function;
	
	self->hashmap[slot].data.value = value;
	self->hashmap[slot].data.value.flags |= flags;
	
	return &self->hashmap[slot].data.value;
}

struct Value * addElement (struct Object *self, uint32_t element, struct Value value, enum Value(Flags) flags)
{
	struct Value *ref;
	
	assert(self);
	
	if (self->elementCapacity <= element)
		resizeElement(self, element + 1);
	else if (self->elementCount <= element)
		self->elementCount = element + 1;
	
	ref = &self->element[element].data.value;
	
	value.flags |= flags;
	*ref = value;
	
	return ref;
}

struct Value * addProperty (struct Object *self, struct Value property, struct Value value, enum Value(Flags) flags)
{
	struct Key key;
	uint32_t element = getElementOrKey(property, &key);
	
	if (element < UINT32_MAX)
		return addElement(self, element, value, flags);
	else
		return addMember(self, key, value, flags);
}

int deleteMember (struct Object *self, struct Key member)
{
	struct Object *object = self;
	uint32_t slot;
	
	assert(object);
	assert(member.data.integer);
	
	do
		slot = getSlot(object, member);
	while (!slot && (object = object->prototype));
	
	if (!object || !(object->hashmap[slot].data.value.check == 1))
		return 1;
	
	if (object->hashmap[slot].data.value.flags & Value(sealed))
		return 0;
	
	memset(&object->hashmap[slot], 0, sizeof(*object->hashmap));
	return 1;
}

int deleteElement (struct Object *self, uint32_t element)
{
	assert(self);
	
	if (element < self->elementCount)
	{
		if (self->element[element].data.value.flags & Value(sealed))
			return 0;
		
		memset(&self->element[element], 0, sizeof(*self->element));
	}
	
	return 1;
}

int deleteProperty (struct Object *self, struct Value property)
{
	struct Key key;
	uint32_t element = getElementOrKey(property, &key);
	
	if (element < UINT32_MAX)
		return deleteElement(self, element);
	else
		return deleteMember(self, key);
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

void populateElementWithCList (struct Object *self, int count, const char * list[])
{
	double binary;
	char *end;
	int index;
	
	if (count > self->elementCount)
		resizeElement(self, count);
	
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
