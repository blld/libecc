//
//  array.c
//  libecc
//
//  Created by Bouilland Aurélien on 20/07/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#include "array.h"

// MARK: - Private

struct Object * Array(prototype) = NULL;
struct Function * Array(constructor) = NULL;

static int valueIsArray(struct Value value)
{
	return value.type == Value(objectType) && value.data.object->type == &Text(arrayType);
}

static uint32_t valueArrayLength(struct Value value)
{
	if (valueIsArray(value))
		return value.data.object->elementCount;
	
	return 1;
}

static void valueAppendFromElement (struct Value value, struct Object *object, uint32_t *element)
{
	if (valueIsArray(value))
	{
		uint32_t index, count;
		for (index = 0, count = value.data.object->elementCount; index < count; ++index)
			object->element[(*element)++] = value.data.object->element[index];
	}
	else
	{
		object->element[*element].data.value = value;
		object->element[(*element)++].data.flags = Object(isValue) | Object(enumerable) | Object(writable) | Object(configurable);
	}
}

static struct Chars * joinWithText (struct Object *object, struct Text text)
{
	struct Value value;
	struct Chars *chars;
	uint32_t length = 0, offset = 0;
	uint32_t index, count = object->elementCount;
	
	for (index = 0; index < count; ++index)
	{
		if (index)
			length += text.length;
		
		value = object->element[index].data.value;
		if (value.type != Value(undefinedType) && value.type != Value(nullType))
			length += Value.toBufferLength(object->element[index].data.value);
	}
	
	chars = Chars.createSized(length);
	
	for (index = 0; index < count; ++index)
	{
		if (index)
		{
			memcpy(chars->chars + offset, text.location, text.length);
			offset += text.length;
		}
		
		value = object->element[index].data.value;
		if (value.type != Value(undefinedType) && value.type != Value(nullType))
			offset += Value.toBuffer(object->element[index].data.value, chars->chars + offset, length - offset + 1);
	}
	
	return chars;
}

static struct Value isArray (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value;
	Op.assertParameterCount(ecc, 1);
	value = Op.argument(ecc, 0);
	ecc->result = Value.truth(value.type == Value(objectType) && value.data.object->type == &Text(arrayType));
	return Value(undefined);
}

static struct Value concat (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value;
	uint32_t element = 0, length = 0, index, count;
	struct Object *array = NULL;
	Op.assertVariableParameter(ecc);
	
	value = Value.toObject(ecc->this, ecc, &(*ops)->text);
	count = Op.variableArgumentCount(ecc);
	
	length += valueArrayLength(value);
	for (index = 0; index < count; ++index)
		length += valueArrayLength(Op.variableArgument(ecc, index));
	
	array = Array.createSized(length);
	
	valueAppendFromElement(value, array, &element);
	for (index = 0; index < count; ++index)
		valueAppendFromElement(Op.variableArgument(ecc, index), array, &element);
	
	ecc->result = Value.object(array);
	return Value(undefined);
}

static struct Value toString (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Object *value;
	
	Op.assertParameterCount(ecc, 0);
	
	value = Value.toObject(ecc->this, ecc, &(*ops)->text).data.object;
	ecc->result = Value.chars(joinWithText(value, (struct Text){ ",", 1 }));
	
	return Value(undefined);
}

static struct Value getLength (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 0);
	
	ecc->result = Value.binary(ecc->this.data.object->elementCount);
	
	return Value(undefined);
}

static struct Value setLength (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	#warning TODO: check if object?
	Object.resizeElement(ecc->this.data.object, Value.toBinary(Op.argument(ecc, 0)).data.binary);
	return Value(undefined);
}

static struct Value constructorFunction (const struct Op ** const ops, struct Ecc * const ecc)
{
	int index, count;
	struct Object *array;
	Op.assertVariableParameter(ecc);
	
	count = Op.variableArgumentCount(ecc);
	array = Array.createSized(count);
	
	for (index = 0; index < count; ++index)
	{
		array->element[index].data.value = Op.variableArgument(ecc, index);
		array->element[index].data.flags = Object(isValue);
	}
	
	ecc->result = Value.object(array);
	return Value(undefined);
}

// MARK: - Static Members

// MARK: - Methods

void setup (void)
{
	enum Object(Flags) flags = Object(writable) | Object(configurable);
	
	Array(prototype) = Object.create(Object(prototype));
	Function.addToObject(Array(prototype), "toString", toString, 0, flags);
	Function.addToObject(Array(prototype), "concat", concat, -1, flags);
	Object.add(Array(prototype), Key(length), Value.function(Function.createWithNativeAccessor(NULL, getLength, setLength)), Object(writable));
	Array(prototype)->type = &Text(arrayType);
	
	Array(constructor) = Function.createWithNative(Array(prototype), constructorFunction, -1);
	Function.addToObject(&Array(constructor)->object, "isArray", isArray, 1, flags);
	
	Object.add(Array(prototype), Key(constructor), Value.function(Array(constructor)), 0);
	Object.add(&Array(constructor)->object, Key(prototype), Value.object(Array(prototype)), 0);
}

void teardown (void)
{
	Array(prototype) = NULL;
	Array(constructor) = NULL;
}

struct Object *create (void)
{
	return createSized(0);
}

struct Object *createSized (uint32_t size)
{
	struct Object *self = Object.create(Array(prototype));
	
	Object.resizeElement(self, size);
	self->type = &Text(arrayType);
	
	return self;
}

struct Object *createArguments (uint32_t size)
{
	struct Object *self = Object.create(Object(prototype));
	
	Object.resizeElement(self, size);
	Object.add(self, Key(length), Value.function(Function.createWithNativeAccessor(NULL, getLength, setLength)), Object(writable));
	self->type = &Text(argumentsType);
	
	return self;
}

struct Object * populateWithCList (struct Object *self, int count, const char * list[])
{
	enum Object(Flags) flags = Object(writable) | Object(enumerable) | Object(configurable);
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
			self->element[index].data.flags = Object(isValue) | flags;
		}
		else
		{
			struct Chars *chars = Chars.createSized(length);
			memcpy(chars->chars, list[index], length);
			
			self->element[index].data.value = Value.chars(chars);
			self->element[index].data.flags = Object(isValue) | flags;
		}
	}
	
	return self;
}
