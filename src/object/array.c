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
		object->element[(*element)++].data.value.flags = 0;
	}
}

static struct Value isArray (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value;
	Op.assertParameterCount(ecc, 1);
	value = Op.argument(ecc, 0);
	ecc->result = Value.truth(value.type == Value(objectType) && value.data.object->type == &Text(arrayType));
	return Value(undefined);
}

static struct Value toString (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Object *object;
	struct Chars *chars;
	uint32_t length;
	struct Text separator = (struct Text){ ",", 1 };
	
	Op.assertParameterCount(ecc, 0);
	
	object = Value.toObject(ops, ecc, ecc->this, Op(textSeekThis)).data.object;
	length = toBufferLength(object, separator);
	chars = Chars.createSized(length);
	toBuffer(object, separator, chars->chars, chars->length);
	ecc->result = Value.chars(chars);
	
	return Value(undefined);
}

static struct Value concat (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value;
	uint32_t element = 0, length = 0, index, count;
	struct Object *array = NULL;
	Op.assertVariableParameter(ecc);
	
	value = Value.toObject(ops, ecc, ecc->this, Op(textSeekThis));
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

static struct Value join (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Object *object;
	struct Value value;
	struct Text separator;
	struct Chars *chars;
	uint32_t length;
	
	Op.assertParameterCount(ecc, 1);
	
	object = Value.toObject(ops, ecc, ecc->this, Op(textSeekThis)).data.object;
	value = Op.argument(ecc, 0);
	if (value.type == Value(undefinedType))
		separator = (struct Text){ ",", 1 };
	else
	{
		value = Value.toString(value);
		separator = Text.make(Value.stringChars(value), Value.stringLength(value));
	}
	
	length = toBufferLength(object, separator);
	chars = Chars.createSized(length);
	toBuffer(object, separator, chars->chars, chars->length);
	ecc->result = Value.chars(chars);
	
	return Value(undefined);
}

static struct Value pop (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Object *object;
	
	Op.assertParameterCount(ecc, 0);
	
	object = Value.toObject(ops, ecc, ecc->this, Op(textSeekThis)).data.object;
	ecc->result = object->elementCount? object->element[--object->elementCount].data.value: Value(undefined);
	
	return Value(undefined);
}

static struct Value push (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Object *object;
	uint32_t length = 0, index, count, base;
	Op.assertVariableParameter(ecc);
	
	object = Value.toObject(ops, ecc, ecc->this, Op(textSeekThis)).data.object;
	count = Op.variableArgumentCount(ecc);
	
	base = object->elementCount;
	length = object->elementCount + count;
	if (length > object->elementCapacity)
		Object.resizeElement(object, length);
	else
		object->elementCount = length;
	
	for (index = 0; index < count; ++index)
	{
		object->element[index + base].data.value = Op.variableArgument(ecc, index);
		object->element[index + base].data.value.flags = 0;
	}
	
	ecc->result = Value.binary(length);
	
	return Value(undefined);
}

static struct Value reverse (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Object *object;
	__typeof__(*object->element) temp;
	uint32_t last, index, half;
	
	Op.assertParameterCount(ecc, 0);
	
	object = Value.toObject(ops, ecc, ecc->this, Op(textSeekThis)).data.object;
	last = object->elementCount - 1;
	half = object->elementCount / 2;
	
	for (index = 0; index < half; ++index)
	{
		temp = object->element[index];
		object->element[index] = object->element[last - index];
		object->element[last - index] = temp;
	}
	
	ecc->result = Value.object(object);
	
	return Value(undefined);
}

static struct Value shift (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Object *object;
	
	Op.assertParameterCount(ecc, 0);
	
	object = Value.toObject(ops, ecc, ecc->this, Op(textSeekThis)).data.object;
	
	if (object->elementCount)
	{
		ecc->result = object->element[0].data.value;
		memmove(object->element, object->element + 1, sizeof(*object->element) * --object->elementCount);
	}
	else
		ecc->result = Value(undefined);
	
	return Value(undefined);
}

static struct Value slice (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Object *object, *result;
	struct Value start, end;
	uint32_t from, to, length;
	double binary;
	
	Op.assertParameterCount(ecc, 2);
	
	object = Value.toObject(ops, ecc, ecc->this, Op(textSeekThis)).data.object;
	start = Op.argument(ecc, 0);
	end = Op.argument(ecc, 1);
	
	start = Op.argument(ecc, 0);
	binary = Value.toBinary(start).data.binary;
	if (start.type == Value(undefinedType))
		from = 0;
	else if (binary > 0)
		from = binary < object->elementCount? binary: object->elementCount;
	else
		from = binary + object->elementCount >= 0? object->elementCount + binary: 0;
	
	end = Op.argument(ecc, 1);
	binary = Value.toBinary(end).data.binary;
	if (end.type == Value(undefinedType))
		to = object->elementCount;
	else if (binary < 0)
		to = binary + object->elementCount >= 0? object->elementCount + binary: 0;
	else
		to = binary < object->elementCount? binary: object->elementCount;
	
	if (to > from)
	{
		length = to - from;
		result = Array.createSized(length);
		memcpy(result->element, object->element + from, sizeof(*result->element) * length);
	}
	else
		result = Array.createSized(0);
	
	ecc->result = Value.object(result);
	
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
	Object.resizeElement(ecc->this.data.object, Value.toBinary(Op.argument(ecc, 0)).data.binary);
	return Value(undefined);
}

static struct Value arrayConstructor (const struct Op ** const ops, struct Ecc * const ecc)
{
	uint32_t index, count, length;
	struct Object *array;
	Op.assertVariableParameter(ecc);
	
	length = count = Op.variableArgumentCount(ecc);
	if (count == 1 && Value.isNumber(Op.variableArgument(ecc, 0)))
	{
		double binary = Value.toBinary(Op.variableArgument(ecc, 0)).data.binary;
		if (binary >= 0 && binary <= UINT32_MAX && binary == (uint32_t)binary)
		{
			length = binary;
			count = 0;
		}
		else
			Ecc.jmpEnv(ecc, Value.error(Error.rangeError(Op.textSeek(ops, ecc, 0), "invalid array length")));
	}
	
	array = Array.createSized(length);
	
	for (index = 0; index < count; ++index)
	{
		array->element[index].data.value = Op.variableArgument(ecc, index);
		array->element[index].data.value.flags = 0;
	}
	
	ecc->result = Value.object(array);
	return Value(undefined);
}

// MARK: - Static Members

// MARK: - Methods

void setup (void)
{
	enum Value(Flags) flags = Value(hidden);
	
	Function.setupBuiltinObject(&Array(constructor), arrayConstructor, -1, &Array(prototype), Value.object(createSized(0)), &Text(arrayType));
	
	Function.addToObject(Array(prototype), "toString", toString, 0, flags);
	Function.addToObject(Array(prototype), "concat", concat, -1, flags);
	Function.addToObject(Array(prototype), "join", join, 1, flags);
	Function.addToObject(Array(prototype), "pop", pop, 0, flags);
	Function.addToObject(Array(prototype), "push", push, -1, flags);
	Function.addToObject(Array(prototype), "reverse", reverse, 0, flags);
	Function.addToObject(Array(prototype), "shift", shift, 0, flags);
	Function.addToObject(Array(prototype), "slice", slice, 2, flags);
	Object.add(Array(prototype), Key(length), Value.function(Function.createWithNativeAccessor(getLength, setLength)), Value(hidden) | Value(sealed));
	
	Function.addToObject(&Array(constructor)->object, "isArray", isArray, 1, flags);
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
	
	return self;
}

uint16_t toBufferLength (struct Object *object, struct Text separator)
{
	struct Value value;
	uint16_t offset = 0;
	uint16_t index, count = object->elementCount;
	
	for (index = 0; index < count; ++index)
	{
		if (index)
			offset += separator.length;
		
		value = object->element[index].data.value;
		if (value.type != Value(undefinedType) && value.type != Value(nullType))
			offset += Value.toBufferLength(object->element[index].data.value);
	}
	
	return offset;
}

uint16_t toBuffer (struct Object *object, struct Text separator, char *buffer, uint16_t length)
{
	struct Value value;
	uint16_t offset = 0;
	uint16_t index, count = object->elementCount;
	
	for (index = 0; index < count; ++index)
	{
		if (index)
		{
			memcpy(buffer + offset, separator.location, separator.length);
			offset += separator.length;
		}
		
		value = object->element[index].data.value;
		if (value.type != Value(undefinedType) && value.type != Value(nullType))
			offset += Value.toBuffer(object->element[index].data.value, buffer + offset, length - offset + 1);
	}
	
	return offset;
}
