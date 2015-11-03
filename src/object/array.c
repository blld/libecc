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
	return value.type == Value(object) && value.data.object->type == &Text(arrayType);
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

static struct Value isArray (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value;
	Op.assertParameterCount(ecc, 1);
	value = Op.argument(ecc, 0);
	ecc->result = Value.truth(value.type == Value(object) && value.data.object->type == &Text(arrayType));
	return Value.undefined();
}

static struct Value concat (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value;
	uint32_t element = 0, length = 0, index, count;
	struct Object *array = NULL;
	Op.assertVariableParameter(ecc);
	
	value = Value.toObject(ecc->this, ecc, &(*ops)->text);
	count = Op.variableArgumentCount(ecc);
	
	length += valueArrayLength(ecc->this);
	for (index = 0; index < count; ++index)
		length += valueArrayLength(Op.variableArgument(ecc, index));
	
	array = Array.createSized(length);
	
	valueAppendFromElement(ecc->this, array, &element);
	for (index = 0; index < count; ++index)
		valueAppendFromElement(Op.variableArgument(ecc, index), array, &element);
	
	ecc->result = Value.object(array);
	return Value.undefined();
}

static struct Value toString (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 0);
	
	if (ecc->this.data.object->elementCount)
	{
		char bytes[256];
		struct Text buffer = Text.make(bytes, 0);
		struct Chars *chars = malloc(sizeof(*chars));
		uint32_t length = 0, stringLength;
		int notLast = 0;
		struct Value value;
		uint32_t index;
		
		*chars = Chars.identity;
		
		for (index = 0; ; ++index)
		{
			notLast = index < ecc->this.data.object->elementCount - 1;
			
			if (ecc->this.data.object->element[index].data.flags & Object(isValue))
			{
				value = ecc->this.data.object->element[index].data.value;
				
				buffer.length = sizeof(bytes);
				value = Value.toString(value, &buffer);
				
				stringLength = Value.stringLength(value);
				chars = realloc(chars, sizeof(*chars) + length + stringLength + (notLast? 1: 0));
				
				memcpy(chars->chars + length, Value.stringChars(value), stringLength);
				length += stringLength;
			}
			
			if (notLast)
				chars->chars[length++] = ',';
			else
				break;
		}
		
		chars->length = length;
		Pool.addChars(chars);
		ecc->result = Value.chars(chars);
	}
	
	return Value.undefined();
}

static struct Value getLength (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 0);
	
	ecc->result = Value.binary(ecc->this.data.object->elementCount);
	
	return Value.undefined();
}

static struct Value setLength (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	#warning TODO: check if object?
	Object.resizeElement(ecc->this.data.object, Value.toBinary(Op.argument(ecc, 0)).data.binary);
	return Value.undefined();
}

static struct Value constructorFunction (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	#warning TODO
	return Value.undefined();
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
	
	Array(constructor) = Function.createWithNative(Array(prototype), constructorFunction, 1);
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
	assert(self);
	Object.resizeElement(self, size);
	self->type = &Text(arrayType);
	
	return self;
}
