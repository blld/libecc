//
//  array.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#define Implementation
#include "array.h"

#include "../ecc.h"
#include "../op.h"
#include "../oplist.h"

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

static void valueAppendFromElement (struct Context * const context, struct Value value, struct Object *object, uint32_t *element)
{
	uint32_t index, count;
	
	if (valueIsArray(value))
		for (index = 0, count = value.data.object->elementCount; index < count; ++index)
			object->element[(*element)++].value = Object.getValue(value.data.object, context, &value.data.object->element[index].value);
	else
		object->element[(*element)++].value = value;
}

static struct Value isArray (struct Context * const context)
{
	struct Value value;
	
	Context.assertParameterCount(context, 1);
	value = Context.argument(context, 0);
	
	return Value.truth(value.type == Value(objectType) && value.data.object->type == &Array(type));
}

static struct Chars * toChars (struct Context * const context, struct Value this, struct Text separator)
{
	struct Object *object = this.data.object;
	struct Value value, length = Object.getMember(object, context, Key(length));
	uint32_t index, count = Value.toBinary(context, length).data.binary;
	struct Chars *chars;
	
	Chars.beginAppend(&chars);
	for (index = 0; index < count; ++index)
	{
		value = Object.getElement(this.data.object, context, index);
		
		if (index)
			Chars.append(&chars, "%.*s", separator.length, separator.bytes);
		
		if (value.type != Value(undefinedType) && value.type != Value(nullType))
			Chars.appendValue(&chars, context, value);
	}
	
	return Chars.endAppend(&chars);
}

static struct Value toString (struct Context * const context)
{
	struct Value function;
	
	Context.assertParameterCount(context, 0);
	
	context->this = Value.toObject(context, Context.this(context));
	function = Object.getMember(context->this.data.object, context, Key(join));
	
	if (function.type == Value(functionType))
		return Context.callFunction(context, function.data.function, context->this, 0);
	else
		return Object.toString(context);
}

static struct Value concat (struct Context * const context)
{
	struct Value value;
	uint32_t element = 0, length = 0, index, count;
	struct Object *array = NULL;
	
	Context.assertVariableParameter(context);
	
	value = Value.toObject(context, Context.this(context));
	count = Context.variableArgumentCount(context);
	
	length += valueArrayLength(value);
	for (index = 0; index < count; ++index)
		length += valueArrayLength(Context.variableArgument(context, index));
	
	array = Array.createSized(length);
	
	valueAppendFromElement(context, value, array, &element);
	for (index = 0; index < count; ++index)
		valueAppendFromElement(context, Context.variableArgument(context, index), array, &element);
	
	return Value.object(array);
}

static struct Value join (struct Context * const context)
{
	struct Value object;
	struct Value value;
	struct Text separator;
	
	Context.assertParameterCount(context, 1);
	
	value = Context.argument(context, 0);
	if (value.type == Value(undefinedType))
		separator = Text.make(",", 1);
	else
	{
		value = Value.toString(context, value);
		separator = Text.make(Value.stringBytes(value), Value.stringLength(value));
	}
	
	object = Value.toObject(context, Context.this(context));
	
	return Value.chars(toChars(context, object, separator));
}

static struct Value pop (struct Context * const context)
{
	struct Value this, value;
	
	Context.assertParameterCount(context, 0);
	
	this = Value.toObject(context, Context.this(context));
	value = this.data.object->elementCount?
		Object.getValue(this.data.object, context, &this.data.object->element[this.data.object->elementCount - 1].value):
		Value(undefined);
	
	--this.data.object->elementCount;
	
	return value;
}

static struct Value push (struct Context * const context)
{
	struct Object *object;
	uint32_t length = 0, index, count, base;
	
	Context.assertVariableParameter(context);
	
	object = Value.toObject(context, Context.this(context)).data.object;
	count = Context.variableArgumentCount(context);
	
	base = object->elementCount;
	length = object->elementCount + count;
	if (length > object->elementCapacity)
		Object.resizeElement(object, length);
	else
		object->elementCount = length;
	
	for (index = 0; index < count; ++index)
		object->element[index + base].value = Context.variableArgument(context, index);
	
	return Value.binary(length);
}

static struct Value reverse (struct Context * const context)
{
	struct Value this, temp;
	struct Object *object;
	uint32_t index, half, last;
	
	Context.assertParameterCount(context, 0);
	
	this = Value.toObject(context, Context.this(context));
	object = this.data.object;
	last = object->elementCount - 1;
	half = object->elementCount / 2;
	
	Context.setTextIndex(context, Context(callIndex));
	
	for (index = 0; index < half; ++index)
	{
		temp = Object.getValue(object, context, &object->element[index].value);
		Object.putValue(object, context, &object->element[index].value, Object.getValue(object, context, &object->element[last - index].value));
		Object.putValue(object, context, &object->element[last - index].value, temp);
	}
	
	return this;
}

static struct Value shift (struct Context * const context)
{
	struct Value this, result;
	struct Object *object;
	uint32_t index, count;
	
	Context.assertParameterCount(context, 0);
	
	this = Value.toObject(context, Context.this(context));
	object = this.data.object;
	
	Context.setTextIndex(context, Context(callIndex));
	
	if (object->elementCount)
	{
		result = Object.getValue(object, context, &object->element[0].value);
		
		for (index = 0, count = object->elementCount - 1; index < count; ++index)
			Object.putValue(object, context, &this.data.object->element[index].value, Object.getValue(object, context, &this.data.object->element[index + 1].value));
		
		--object->elementCount;
	}
	else
		result = Value(undefined);
	
	return result;
}

static struct Value unshift (struct Context * const context)
{
	struct Value this;
	struct Object *object;
	uint32_t length = 0, index, count;
	
	Context.assertVariableParameter(context);
	
	this = Value.toObject(context, Context.this(context));
	object = this.data.object;
	count = Context.variableArgumentCount(context);
	
	length = object->elementCount + count;
	if (length > object->elementCapacity)
		Object.resizeElement(object, length);
	else
		object->elementCount = length;
	
	Context.setTextIndex(context, Context(callIndex));
	
	for (index = count; index < length; ++index)
		Object.putValue(object, context, &object->element[index].value, Object.getValue(object, context, &object->element[index - count].value));
	
	for (index = 0; index < count; ++index)
		object->element[index].value = Context.variableArgument(context, index);
	
	return Value.binary(length);
}

static struct Value slice (struct Context * const context)
{
	struct Object *object, *result;
	struct Value this, start, end;
	uint32_t from, to, length;
	double binary;
	
	Context.assertParameterCount(context, 2);
	
	this = Value.toObject(context, Context.this(context));
	object = this.data.object;
	
	start = Context.argument(context, 0);
	binary = Value.toBinary(context, start).data.binary;
	if (start.type == Value(undefinedType))
		from = 0;
	else if (binary > 0)
		from = binary < object->elementCount? binary: object->elementCount;
	else
		from = binary + object->elementCount >= 0? object->elementCount + binary: 0;
	
	end = Context.argument(context, 1);
	binary = Value.toBinary(context, end).data.binary;
	if (end.type == Value(undefinedType))
		to = object->elementCount;
	else if (binary < 0)
		to = binary + object->elementCount >= 0? object->elementCount + binary: 0;
	else
		to = binary < object->elementCount? binary: object->elementCount;
	
	Context.setTextIndex(context, Context(callIndex));
	
	if (to > from)
	{
		length = to - from;
		result = Array.createSized(length);
		
		for (to = 0; to < length; ++from, ++to)
			result->element[to].value = Object.getValue(object, context, &this.data.object->element[from].value);
	}
	else
		result = Array.createSized(0);
	
	return Value.object(result);
}

struct Compare {
	struct Context context;
	struct Function *function;
	struct Object *arguments;
	struct Op *ops;
};

static
struct Value defaultComparison (struct Context * const context)
{
	struct Value left, right, result;
	
	left = Context.variableArgument(context, 0);
	right = Context.variableArgument(context, 1);
	result = Value.less(context, Value.toString(context, left), Value.toString(context, right));
	
	return Value.integer(Value.isTrue(result)? -1: 0);
}

static inline
int gcd(int m, int n)
{
	while (n)
	{
		int t = m % n;
		m = n;
		n = t;
	}
	return m;
}

static inline
void rotate(struct Object *object, struct Context *context, uint32_t first, uint32_t half, uint32_t last)
{
	struct Value value, leftValue;
	uint32_t n, shift, a, b;
	
	if (first == half || half == last)
		return;
	
	n = gcd(last - first, half - first);
	while (n--)
	{
		shift = half - first;
		a = first + n;
		b = a + shift;
		leftValue = Object.getValue(object, context, &object->element[a].value);
		while (b != first + n)
		{
			value = Object.getValue(object, context, &object->element[b].value);
			Object.putValue(object, context, &object->element[a].value, value);
			a = b;
			if (last - b > shift)
				b += shift;
			else
				b = half - (last - b);
		}
		Object.putValue(object, context, &object->element[a].value, leftValue);
	}
}

static inline
int compare (struct Compare *cmp, struct Value left, struct Value right)
{
	uint16_t hashmapCount;
	
	if (left.check != 1)
		return 0;
	else if (right.check != 1)
		return 1;
	
	if (left.type == Value(undefinedType))
		return 0;
	else if (right.type == Value(undefinedType))
		return 1;
	
	hashmapCount = cmp->context.environment->hashmapCount;
	switch (hashmapCount) {
		default:
			memcpy(cmp->context.environment->hashmap + 5,
				   cmp->function->environment.hashmap,
				   sizeof(*cmp->context.environment->hashmap) * (hashmapCount - 5));
		case 5:
			cmp->context.environment->hashmap[3 + 1].value = right;
		case 4:
			cmp->context.environment->hashmap[3 + 0].value = left;
		case 3:
			break;
		case 2:
		case 1:
		case 0:
			assert(0);
			break;
	}
	
	cmp->context.ops = cmp->ops;
	cmp->arguments->element[0].value = left;
	cmp->arguments->element[1].value = right;
	
	return Value.toInteger(&cmp->context, cmp->context.ops->native(&cmp->context)).data.integer < 0;
}

static inline
uint32_t search(struct Object *object, struct Compare *cmp, uint32_t first, uint32_t last, struct Value right)
{
	struct Value left;
	uint32_t half;
	
	while (first < last)
	{
		half = (first + last) >> 1;
		left = Object.getValue(object, &cmp->context, &object->element[half].value);
		if (compare(cmp, left, right))
			first = half + 1;
		else
			last = half;
	}
	return first;
}

static inline
void merge(struct Object *object, struct Compare *cmp, uint32_t first, uint32_t pivot, uint32_t last, uint32_t len1, uint32_t len2)
{
	uint32_t left, right, half1, half2;
	
	if (len1 == 0 || len2 == 0)
		return;
	
	if (len1 + len2 == 2)
	{
		struct Value left, right;
		left = Object.getValue(object, &cmp->context, &object->element[pivot].value);
		right = Object.getValue(object, &cmp->context, &object->element[first].value);
		if (compare(cmp, left, right))
		{
			Object.putValue(object, &cmp->context, &object->element[pivot].value, right);
			Object.putValue(object, &cmp->context, &object->element[first].value, left);
		}
		return;
	}
	
	if (len1 > len2)
	{
		half1 = len1 >> 1;
		left = first + half1;
		right = search(object, cmp, pivot, last, Object.getValue(object, &cmp->context, &object->element[first + half1].value));
		half2 = right - pivot;
	}
	else
	{
		half2 = len2 >> 1;
		left = search(object, cmp, first, pivot, Object.getValue(object, &cmp->context, &object->element[pivot + half2].value));
		right = pivot + half2;
		half1 = left - first;
	}
	rotate(object, &cmp->context, left, pivot, right);
	
	pivot = left + half2;
	merge(object, cmp, first, left, pivot, half1, half2);
	merge(object, cmp, pivot, right, last, len1 - half1, len2 - half2);
}

static
void sortAndMerge(struct Object *object, struct Compare *cmp, uint32_t first, uint32_t last)
{
	uint32_t half;
	
	if (last - first < 8)
	{
		struct Value left, right;
		uint32_t i, j;
		
		for (i = first + 1; i < last; ++i)
		{
			right = Object.getValue(object, &cmp->context, &object->element[i].value);
			for (j = i; j > first; --j)
			{
				left = Object.getValue(object, &cmp->context, &object->element[j - 1].value);
				if (compare(cmp, left, right))
					break;
				else
					Object.putValue(object, &cmp->context, &object->element[j].value, left);
			}
			Object.putValue(object, &cmp->context, &object->element[j].value, right);
		}
		return;
	}
	
	half = (first + last) >> 1;
	sortAndMerge(object, cmp, first, half);
	sortAndMerge(object, cmp, half, last);
	merge(object, cmp, first, half, last, half - first, last - half);
}

static
void sortInPlace (struct Context * const context, struct Object *object, struct Function *function, int first, int last)
{
	struct Op defaultOps = { defaultComparison, Value(undefined), Text(nativeCode) };
	const struct Op * ops = function? function->oplist->ops: &defaultOps;
	
	struct Compare cmp = {
		{
			.this = Value.object(object),
			.parent = context,
			.ecc = context->ecc,
			.depth = context->depth + 1,
			.ops = ops,
			.textIndex = Context(callIndex),
		},
		function,
		NULL,
		cmp.ops = ops,
	};
	
	if (function && function->flags & Function(needHeap))
	{
		struct Object *environment = Object.copy(&function->environment);
		
		cmp.context.environment = environment;
		cmp.arguments = Arguments.createSized(2);
		++cmp.arguments->referenceCount;
		
		environment->hashmap[2].value = Value.object(cmp.arguments);
		
		sortAndMerge(object, &cmp, first, last);
	}
	else
	{
		struct Object environment = function? function->environment: Object.identity;
		struct Object arguments = Object.identity;
		union Object(Hashmap) hashmap[function? function->environment.hashmapCapacity: 3];
		union Object(Element) element[2];
		
		if (function)
			memcpy(hashmap, function->environment.hashmap, sizeof(hashmap));
		else
			environment.hashmapCount = 3;
		
		cmp.context.environment = &environment;
		cmp.arguments = &arguments;
		
		arguments.element = element;
		arguments.elementCount = 2;
		environment.hashmap = hashmap;
		environment.hashmap[2].value = Value.object(&arguments);
		
		sortAndMerge(object, &cmp, first, last);
	}
}

static struct Value sort (struct Context * const context)
{
	struct Object *this;
	struct Value compare;
	uint32_t count;
	
	Context.assertParameterCount(context, 1);
	
	this = Value.toObject(context, Context.this(context)).data.object;
	count = Value.toInteger(context, Object.getMember(this, context, Key(length))).data.integer;
	compare = Context.argument(context, 0);
	
	if (compare.type == Value(functionType))
		sortInPlace(context, this, compare.data.function, 0, count);
	else if (compare.type == Value(undefinedType))
		sortInPlace(context, this, NULL, 0, count);
	else
		Context.typeError(context, Chars.create("comparison function must be a function or undefined"));
	
	return Value.object(this);
}

static struct Value getLength (struct Context * const context)
{
	Context.assertParameterCount(context, 0);
	
	return Value.binary(context->this.data.object->elementCount);
}

static struct Value setLength (struct Context * const context)
{
	Context.assertParameterCount(context, 1);
	Object.resizeElement(context->this.data.object, Value.toBinary(context, Context.argument(context, 0)).data.binary);
	
	return Value(undefined);
}

static struct Value constructor (struct Context * const context)
{
	struct Value value;
	uint32_t index, count, length;
	struct Object *array;
	Context.assertVariableParameter(context);
	
	length = count = Context.variableArgumentCount(context);
	value = Context.variableArgument(context, 0);
	if (count == 1 && Value.isNumber(value) && Value.isPrimitive(value))
	{
		double binary = Value.toBinary(context, value).data.binary;
		if (binary >= 0 && binary <= UINT32_MAX && binary == (uint32_t)binary)
		{
			length = binary;
			count = 0;
		}
		else
			Context.rangeError(context, Chars.create("invalid array length"));
	}
	
	array = Array.createSized(length);
	
	for (index = 0; index < count; ++index)
		array->element[index].value = Context.variableArgument(context, index);
	
	return Value.object(array);
}

// MARK: - Static Members

// MARK: - Methods

void setup (void)
{
	const enum Value(Flags) h = Value(hidden);
	const enum Value(Flags) s = Value(sealed);
	
	Function.setupBuiltinObject(
		&Array(constructor), constructor, -1,
		&Array(prototype), Value.object(createSized(0)),
		&Array(type));
	
	Function.addMethod(Array(constructor), "isArray", isArray, 1, h);
	
	Function.addToObject(Array(prototype), "toString", toString, 0, h);
	Function.addToObject(Array(prototype), "toLocaleString", toString, 0, h);
	Function.addToObject(Array(prototype), "concat", concat, -1, h);
	Function.addToObject(Array(prototype), "join", join, 1, h);
	Function.addToObject(Array(prototype), "pop", pop, 0, h);
	Function.addToObject(Array(prototype), "push", push, -1, h);
	Function.addToObject(Array(prototype), "reverse", reverse, 0, h);
	Function.addToObject(Array(prototype), "shift", shift, 0, h);
	Function.addToObject(Array(prototype), "slice", slice, 2, h);
	Function.addToObject(Array(prototype), "sort", sort, 1, h);
	Function.addToObject(Array(prototype), "unshift", unshift, -1, h);
	
	Object.addMember(Array(prototype), Key(length), Function.accessor(getLength, setLength), h|s | Value(asOwn) | Value(asData));
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
