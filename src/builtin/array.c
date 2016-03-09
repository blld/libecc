//
//  array.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "array.h"

// MARK: - Private

struct Object * Array(prototype) = NULL;
struct Function * Array(constructor) = NULL;

const struct Object(Type) Array(type) = {
	.text = &Text(arrayType),
};

static int valueIsArray(struct Value value)
{
	return value.type == Value(objectType) && value.data.object->type == &Array(type);
}

static uint32_t valueArrayLength(struct Value value)
{
	if (valueIsArray(value))
		return value.data.object->elementCount;
	
	return 1;
}

static void valueAppendFromElement (struct Native(Context) * const context, struct Value value, struct Object *object, uint32_t *element)
{
	uint32_t index, count;
	
	if (valueIsArray(value))
		for (index = 0, count = value.data.object->elementCount; index < count; ++index)
			object->element[(*element)++].data.value = Object.getValue(context, &value.data.object->element[index].data.value, value);
	else
		object->element[(*element)++].data.value = value;
}

static struct Value isArray (struct Native(Context) * const context)
{
	struct Value value;
	
	Native.assertParameterCount(context, 1);
	value = Native.argument(context, 0);
	
	return Value.truth(value.type == Value(objectType) && value.data.object->type == &Array(type));
}

static uint16_t toSeparatedLength (struct Native(Context) * const context, struct Value this, struct Text separator)
{
	struct Value value;
	uint16_t offset = 0;
	struct Object *object = this.data.object;
	uint32_t index, count = object->elementCount;
	
	for (index = 0; index < count; ++index)
	{
		if (index)
			offset += separator.length;
		
		value = Object.getValue(context, &object->element[index].data.value, this);
		if (value.type != Value(undefinedType) && value.type != Value(nullType))
			offset += Value.toLength(context, value);
	}
	
	return offset;
}

static uint16_t toSeparatedBytes (struct Native(Context) * const context, struct Value this, struct Text separator, char *bytes)
{
	struct Value value;
	uint16_t offset = 0;
	struct Object *object = this.data.object;
	uint32_t index, count = object->elementCount;
	
	for (index = 0; index < count; ++index)
	{
		if (index)
		{
			memcpy(bytes + offset, separator.bytes, separator.length);
			offset += separator.length;
		}
		
		value = Object.getValue(context, &object->element[index].data.value, this);
		if (value.type != Value(undefinedType) && value.type != Value(nullType))
			offset += Value.toBytes(context, value, bytes + offset);
	}
	
	return offset;
}

static struct Value toString (struct Native(Context) * const context)
{
	struct Value object;
	struct Chars *chars;
	uint32_t length;
	struct Text separator = (struct Text){ ",", 1 };
	
	Native.assertParameterCount(context, 0);
	
	object = Value.toObject(context, context->this, Native(thisIndex));
	length = toSeparatedLength(context, object, separator);
	chars = Chars.createSized(length);
	toSeparatedBytes(context, object, separator, chars->bytes);
	
	return Value.chars(chars);
}

static struct Value concat (struct Native(Context) * const context)
{
	struct Value value;
	uint32_t element = 0, length = 0, index, count;
	struct Object *array = NULL;
	Native.assertVariableParameter(context);
	
	value = Value.toObject(context, context->this, Native(thisIndex));
	count = Native.variableArgumentCount(context);
	
	length += valueArrayLength(value);
	for (index = 0; index < count; ++index)
		length += valueArrayLength(Native.variableArgument(context, index));
	
	array = Array.createSized(length);
	
	valueAppendFromElement(context, value, array, &element);
	for (index = 0; index < count; ++index)
		valueAppendFromElement(context, Native.variableArgument(context, index), array, &element);
	
	return Value.object(array);
}

static struct Value join (struct Native(Context) * const context)
{
	struct Value object;
	struct Value value;
	struct Text separator;
	struct Chars *chars;
	uint32_t length;
	
	Native.assertParameterCount(context, 1);
	
	object = Value.toObject(context, context->this, Native(thisIndex));
	value = Native.argument(context, 0);
	if (value.type == Value(undefinedType))
		separator = (struct Text){ ",", 1 };
	else
	{
		value = Value.toString(context, value);
		separator = Text.make(Value.stringBytes(value), Value.stringLength(value));
	}
	
	length = toSeparatedLength(context, object, separator);
	chars = Chars.createSized(length);
	toSeparatedBytes(context, object, separator, chars->bytes);
	
	return Value.chars(chars);
}

static struct Value pop (struct Native(Context) * const context)
{
	struct Value this, value;
	
	Native.assertParameterCount(context, 0);
	
	this = Value.toObject(context, context->this, Native(thisIndex));
	value = this.data.object->elementCount?
		Object.getValue(context, &this.data.object->element[this.data.object->elementCount - 1].data.value, this):
		Value(undefined);
	
	--this.data.object->elementCount;
	
	return value;
}

static struct Value push (struct Native(Context) * const context)
{
	struct Object *object;
	uint32_t length = 0, index, count, base;
	Native.assertVariableParameter(context);
	
	object = Value.toObject(context, context->this, Native(thisIndex)).data.object;
	count = Native.variableArgumentCount(context);
	
	base = object->elementCount;
	length = object->elementCount + count;
	if (length > object->elementCapacity)
		Object.resizeElement(object, length);
	else
		object->elementCount = length;
	
	for (index = 0; index < count; ++index)
	{
		object->element[index + base].data.value = Native.variableArgument(context, index);
		object->element[index + base].data.value.flags = 0;
	}
	
	return Value.binary(length);
}

static struct Value reverse (struct Native(Context) * const context)
{
	struct Value this, temp;
	struct Object *object;
	uint32_t index, half, last;
	const struct Text text = Native.textSeek(context, Native(callIndex));
	
	Native.assertParameterCount(context, 0);
	
	this = Value.toObject(context, context->this, Native(thisIndex));
	object = this.data.object;
	last = object->elementCount - 1;
	half = object->elementCount / 2;
	
	for (index = 0; index < half; ++index)
	{
		temp = Object.getValue(context, &object->element[index].data.value, this);
		Object.putValue(context, &object->element[index].data.value, this, Object.getValue(context, &object->element[last - index].data.value, this), &text);
		Object.putValue(context, &object->element[last - index].data.value, this, temp, &text);
	}
	
	return this;
}

static struct Value shift (struct Native(Context) * const context)
{
	struct Value this, result;
	struct Object *object;
	uint32_t index, count;
	const struct Text text = Native.textSeek(context, Native(callIndex));
	
	Native.assertParameterCount(context, 0);
	
	this = Value.toObject(context, context->this, Native(thisIndex));
	object = this.data.object;
	
	if (object->elementCount)
	{
		result = Object.getValue(context, &object->element[0].data.value, this);
		
		for (index = 0, count = object->elementCount - 1; index < count; ++index)
			Object.putValue(context, &this.data.object->element[index].data.value, this, Object.getValue(context, &this.data.object->element[index + 1].data.value, this), &text);
		
		--object->elementCount;
	}
	else
		result = Value(undefined);
	
	return result;
}

static struct Value unshift (struct Native(Context) * const context)
{
	struct Value this;
	struct Object *object;
	uint32_t length = 0, index, count, base;
	Native.assertVariableParameter(context);
	const struct Text text = Native.textSeek(context, Native(callIndex));
	
	this = Value.toObject(context, context->this, Native(thisIndex));
	object = this.data.object;
	count = Native.variableArgumentCount(context);
	
	base = object->elementCount;
	length = object->elementCount + count;
	if (length > object->elementCapacity)
		Object.resizeElement(object, length);
	else
		object->elementCount = length;
	
	for (index = count; index < length; ++index)
		Object.putValue(context, &this.data.object->element[index].data.value, this, Object.getValue(context, &this.data.object->element[index - count].data.value, this), &text);
	
	for (index = 0; index < count; ++index)
	{
		object->element[index].data.value = Native.variableArgument(context, index);
		object->element[index].data.value.flags = 0;
	}
	
	return Value.binary(length);
}

static struct Value slice (struct Native(Context) * const context)
{
	struct Object *object, *result;
	struct Value this, start, end;
	uint32_t from, to, length;
	double binary;
	
	Native.assertParameterCount(context, 2);
	
	this = Value.toObject(context, context->this, Native(thisIndex));
	object = this.data.object;
	
	start = Native.argument(context, 0);
	binary = Value.toBinary(start).data.binary;
	if (start.type == Value(undefinedType))
		from = 0;
	else if (binary > 0)
		from = binary < object->elementCount? binary: object->elementCount;
	else
		from = binary + object->elementCount >= 0? object->elementCount + binary: 0;
	
	end = Native.argument(context, 1);
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
		
		for (to = 0; to < length; ++from, ++to)
			result->element[to].data.value = Object.getValue(context, &this.data.object->element[from].data.value, this);
	}
	else
		result = Array.createSized(0);
	
	return Value.object(result);
}

static struct Value getLength (struct Native(Context) * const context)
{
	Native.assertParameterCount(context, 0);
	
	return Value.binary(context->this.data.object->elementCount);
}

static struct Value setLength (struct Native(Context) * const context)
{
	Native.assertParameterCount(context, 1);
	Object.resizeElement(context->this.data.object, Value.toBinary(Native.argument(context, 0)).data.binary);
	
	return Value(undefined);
}

static struct Value arrayConstructor (struct Native(Context) * const context)
{
	uint32_t index, count, length;
	struct Object *array;
	Native.assertVariableParameter(context);
	
	length = count = Native.variableArgumentCount(context);
	if (count == 1 && Value.isNumber(Native.variableArgument(context, 0)))
	{
		double binary = Value.toBinary(Native.variableArgument(context, 0)).data.binary;
		if (binary >= 0 && binary <= UINT32_MAX && binary == (uint32_t)binary)
		{
			length = binary;
			count = 0;
		}
		else
			Ecc.jmpEnv(context->ecc, Value.error(Error.rangeError(Native.textSeek(context, 0), "invalid array length")));
	}
	
	array = Array.createSized(length);
	
	for (index = 0; index < count; ++index)
	{
		array->element[index].data.value = Native.variableArgument(context, index);
		array->element[index].data.value.flags = 0;
	}
	
	return Value.object(array);
}

// MARK: - Static Members

// MARK: - Methods

void setup (void)
{
	enum Value(Flags) flags = Value(hidden);
	
	Function.setupBuiltinObject(&Array(constructor), arrayConstructor, -1, &Array(prototype), Value.object(createSized(0)), &Array(type));
	
	Function.addToObject(Array(prototype), "toString", toString, 0, flags);
	Function.addToObject(Array(prototype), "concat", concat, -1, flags);
	Function.addToObject(Array(prototype), "join", join, 1, flags);
	Function.addToObject(Array(prototype), "pop", pop, 0, flags);
	Function.addToObject(Array(prototype), "push", push, -1, flags);
	Function.addToObject(Array(prototype), "reverse", reverse, 0, flags);
	Function.addToObject(Array(prototype), "shift", shift, 0, flags);
	Function.addToObject(Array(prototype), "slice", slice, 2, flags);
	Function.addToObject(Array(prototype), "unshift", unshift, -1, flags);
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

uint16_t toLength (struct Native(Context) * const context, struct Value value)
{
	assert(value.type == Value(objectType));
	assert(value.data.object);
	
	return toSeparatedLength(context, value, (struct Text){ ",", 1 });
}

uint16_t toBytes (struct Native(Context) * const context, struct Value value, char *bytes)
{
	assert(value.type == Value(objectType));
	assert(value.data.object);
	
	return toSeparatedBytes(context, value, (struct Text){ ",", 1 }, bytes);
}
