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

static struct Value toString (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Object *object;
	
	Op.assertParameterCount(ecc, 0);
	
	object = Value.toObject(ecc->this, ecc, &(*ops)->text).data.object;
	
	ecc->result = Value.chars(joinWithText(object, (struct Text){ ",", 1 }));
	
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

static struct Value join (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Object *object;
	struct Value separator;
	struct Text text;
	
	Op.assertParameterCount(ecc, 1);
	
	object = Value.toObject(ecc->this, ecc, &(*ops)->text).data.object;
	separator = Op.argument(ecc, 0);
	if (separator.type == Value(undefinedType))
		text = (struct Text){ ",", 1 };
	else
	{
		separator = Value.toString(separator);
		text = Text.make(Value.stringChars(separator), Value.stringLength(separator));
	}
	
	ecc->result = Value.chars(joinWithText(object, text));
	
	return Value(undefined);
}

static struct Value pop (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Object *object;
	
	Op.assertParameterCount(ecc, 0);
	
	object = Value.toObject(ecc->this, ecc, &(*ops)->text).data.object;
	ecc->result = object->elementCount? object->element[--object->elementCount].data.value: Value(undefined);
	
	return Value(undefined);
}

static struct Value push (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Object *object;
	uint32_t length = 0, index, count, base;
	Op.assertVariableParameter(ecc);
	
	object = Value.toObject(ecc->this, ecc, &(*ops)->text).data.object;
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
		object->element[index + base].data.flags = Object(isValue) | Object(enumerable) | Object(writable) | Object(configurable);
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
	
	object = Value.toObject(ecc->this, ecc, &(*ops)->text).data.object;
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
	
	object = Value.toObject(ecc->this, ecc, &(*ops)->text).data.object;
	
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
	
	object = Value.toObject(ecc->this, ecc, &(*ops)->text).data.object;
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
	#warning TODO: check if object?
	Object.resizeElement(ecc->this.data.object, Value.toBinary(Op.argument(ecc, 0)).data.binary);
	return Value(undefined);
}

static struct Value constructorFunction (const struct Op ** const ops, struct Ecc * const ecc)
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
			Ecc.jmpEnv(ecc, Value.error(Error.rangeError((*ops)->text, "invalid array length")));
	}
	
	array = Array.createSized(length);
	
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
	Function.addToObject(Array(prototype), "join", join, 1, flags);
	Function.addToObject(Array(prototype), "pop", pop, 0, flags);
	Function.addToObject(Array(prototype), "push", push, -1, flags);
	Function.addToObject(Array(prototype), "reverse", reverse, 0, flags);
	Function.addToObject(Array(prototype), "shift", shift, 0, flags);
	Function.addToObject(Array(prototype), "slice", slice, 2, flags);
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
