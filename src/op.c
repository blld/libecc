//
//  op.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "op.h"

// MARK: - Private

#define nextOp() (++*ops)->native(ops, ecc)
#define opValue() (*ops)->value
#define opText() &(*ops)->text

//

static struct Value addition (struct Ecc * const ecc, struct Value a, const struct Text *aText, struct Value b, const struct Text *bText)
{
	if (a.type == Value(binary) && b.type == Value(binary))
		return Value.binary(a.data.binary + b.data.binary);
	else if (a.type == Value(integer) && b.type == Value(integer))
		return Value.binary(a.data.integer + b.data.integer);
	else if (Value.isNumber(a) && Value.isNumber(b))
		return Value.binary(Value.toBinary(a).data.binary + Value.toBinary(b).data.binary);
	else
	{
		a = Value.toPrimitive(a, ecc, aText, 0);
		b = Value.toPrimitive(b, ecc, bText, 0);
		
		if (Value.isString(a) || Value.isString(b))
		{
			a = Value.toString(a);
			b = Value.toString(b);
			return Value.chars(Chars.create("%.*s%.*s", Value.stringLength(a), Value.stringChars(a), Value.stringLength(b), Value.stringChars(b)));
		}
		else
			return Value.binary(Value.toBinary(a).data.binary + Value.toBinary(b).data.binary);
	}
}

static struct Value subtraction (struct Ecc * const ecc, struct Value a, const struct Text *aText, struct Value b, const struct Text *bText)
{
	if (a.type == Value(binary) && b.type == Value(binary))
		return Value.binary(a.data.binary - b.data.binary);
	else if (a.type == Value(integer) && b.type == Value(integer))
		return Value.binary(a.data.integer - b.data.integer);
	
	return Value.binary(Value.toBinary(a).data.binary - Value.toBinary(b).data.binary);
}

static struct Value equality (struct Ecc * const ecc, struct Value a, const struct Text *aText, struct Value b, const struct Text *bText)
{
	if (a.type == Value(binary) && b.type == Value(binary))
		return Value.boolean(a.data.binary == b.data.binary);
	else if (a.type == Value(integer) && b.type == Value(integer))
		return Value.boolean(a.data.integer == b.data.integer);
	else if (Value.isNumber(a) && Value.isNumber(b))
		return Value.boolean(Value.toBinary(a).data.binary == Value.toBinary(b).data.binary);
	else if (Value.isString(a) && Value.isString(b))
	{
		uint32_t aLength = Value.stringLength(a);
		uint32_t bLength = Value.stringLength(b);
		if (aLength != bLength)
			return Value.false();
		
		return Value.boolean(!memcmp(Value.stringChars(a), Value.stringChars(b), aLength));
	}
	else if (Value.isBoolean(a) && Value.isBoolean(b))
		return Value.boolean(a.type == b.type);
	else if (Value.isObject(a) && Value.isObject(b))
		return Value.boolean(a.data.object == b.data.object);
	else if (a.type == b.type)
		return Value.true();
	else if (a.type == Value(null) && b.type == Value(undefined))
		return Value.true();
	else if (a.type == Value(undefined) && b.type == Value(null))
		return Value.true();
	else if (Value.isNumber(a) && Value.isString(b))
		return equality(ecc, a, aText, Value.toBinary(b), bText);
	else if (Value.isString(a) && Value.isNumber(b))
		return equality(ecc, Value.toBinary(a), aText, b, bText);
	else if (Value.isBoolean(a))
		return equality(ecc, Value.toBinary(a), aText, b, bText);
	else if (Value.isBoolean(b))
		return equality(ecc, a, aText, Value.toBinary(b), bText);
	else if (((Value.isString(a) || Value.isNumber(a)) && Value.isObject(b))
		|| (Value.isObject(a) && (Value.isString(b) || Value.isNumber(b)))
		)
	{
		a = Value.toPrimitive(a, ecc, aText, 0);
		b = Value.toPrimitive(b, ecc, bText, 0);
		return equality(ecc, a, aText, b, bText);
	}
	
	return Value.false();
}

static struct Value identicality (struct Ecc * const ecc, struct Value a, const struct Text *aText, struct Value b, const struct Text *bText)
{
	if (a.type == Value(binary) && b.type == Value(binary))
		return Value.boolean(a.data.binary == b.data.binary);
	else if (a.type == Value(integer) && b.type == Value(integer))
		return Value.boolean(a.data.integer == b.data.integer);
	else if (Value.isNumber(a) && Value.isNumber(b))
		return Value.boolean(Value.toBinary(a).data.binary == Value.toBinary(b).data.binary);
	else if (Value.isString(a) && Value.isString(b))
	{
		uint32_t aLength = Value.stringLength(a);
		uint32_t bLength = Value.stringLength(b);
		if (aLength != bLength)
			return Value.false();
		
		return Value.boolean(!memcmp(Value.stringChars(a), Value.stringChars(b), aLength));
	}
	else if (Value.isBoolean(a) && Value.isBoolean(b))
		return Value.boolean(a.type == b.type);
	else if (Value.isObject(a) && Value.isObject(b))
		return Value.boolean(a.data.object == b.data.object);
	else if (a.type == b.type)
		return Value.true();
	
	return Value.false();
}

static struct Value compare (struct Ecc * const ecc, struct Value a, const struct Text *aText, struct Value b, const struct Text *bText)
{
	a = Value.toPrimitive(a, ecc, aText, -1);
	b = Value.toPrimitive(b, ecc, bText, -1);
	
	if (Value.isString(a) && Value.isString(b))
	{
		uint32_t aLength = Value.stringLength(a);
		uint32_t bLength = Value.stringLength(b);
		if (aLength == bLength)
			return Value.boolean(memcmp(Value.stringChars(a), Value.stringChars(b), aLength) < 0);
		else
			return Value.boolean(aLength < bLength);
	}
	else
	{
		a = Value.toBinary(a);
		b = Value.toBinary(b);
		
		if (isnan(a.data.binary) || isnan(b.data.binary))
			return Value.undefined();
		
		return Value.boolean(a.data.binary < b.data.binary);
	}
}

static struct Value valueLess (struct Ecc * const ecc, struct Value a, const struct Text *aText, struct Value b, const struct Text *bText)
{
	a = compare(ecc, a, aText, b, bText);
	if (a.type == Value(undefined))
		return Value.false();
	else
		return a;
}

static struct Value valueMore (struct Ecc * const ecc, struct Value a, const struct Text *aText, struct Value b, const struct Text *bText)
{
	a = compare(ecc, b, bText, a, aText);
	if (a.type == Value(undefined))
		return Value.false();
	else
		return a;
}

static struct Value valueLessOrEqual (struct Ecc * const ecc, struct Value a, const struct Text *aText, struct Value b, const struct Text *bText)
{
	a = compare(ecc, b, bText, a, aText);
	if (a.type == Value(undefined) || a.type == Value(true))
		return Value.false();
	else
		return Value.true();
}

static struct Value valueMoreOrEqual (struct Ecc * const ecc, struct Value a, const struct Text *aText, struct Value b, const struct Text *bText)
{
	a = compare(ecc, a, aText, b, bText);
	if (a.type == Value(undefined) || a.type == Value(true))
		return Value.false();
	else
		return Value.true();
}

static int integerLess(int32_t a, int32_t b)
{
	return a < b;
}

static int integerLessOrEqual(int32_t a, int32_t b)
{
	return a <= b;
}

static int integerMore(int32_t a, int32_t b)
{
	return a > b;
}

static int integerMoreOrEqual(int32_t a, int32_t b)
{
	return a >= b;
}

static int integerWontOverflowPositive(int32_t a, int32_t positive)
{
	return a <= INT32_MAX - positive;
}

static int integerWontOverflowNegative(int32_t a, int32_t negative)
{
	return a >= INT32_MIN - negative;
}

static struct Value getEntry(struct Entry entry, struct Ecc *ecc, struct Value this)
{
	if (!entry.value)
		return Value.undefined();
	
	if (entry.value->type == Value(function) && entry.value->data.function->flags & Function(isAccessor))
	{
		if (entry.value->data.function->flags & Function(isGetter))
			return callFunctionVA(entry.value->data.function, ecc, this, 0);
		else if (entry.value->data.function->pair)
			return callFunctionVA(entry.value->data.function->pair, ecc, this, 0);
	}
	
	return *entry.value;
}

static struct Value setEntry(struct Entry entry, struct Ecc *ecc, struct Value this, struct Value value)
{
	if (entry.value->type == Value(function) && entry.value->data.function->flags & Function(isAccessor))
	{
		if (entry.value->data.function->flags & Function(isSetter))
			return callFunctionVA(entry.value->data.function, ecc, this, 1, value);
		else if (entry.value->data.function->pair)
			return callFunctionVA(entry.value->data.function->pair, ecc, this, 1, value);
	}
	
	*entry.value = value;
	*entry.flags |= Object(isValue);
	
	return value;
}

// MARK: - Static Members

// MARK: - Methods

struct Op make (const Native native, struct Value value, struct Text text)
{
	return (struct Op){ native, value, text };
}

const char * toChars (const Native native)
{
	#define _(X) { #X, X, },
	struct {
		const char *name;
		const Native native;
	} static const functionList[] = {
		io_libecc_op_List
	};
	#undef _
	
	int index;
	for (index = 0; index < sizeof(functionList); ++index)
		if (functionList[index].native == native)
			return functionList[index].name;
	
	assert(0);
	return "unknow";
}

void assertParameterCount (struct Ecc * const ecc, int parameterCount)
{
	assert(ecc->context->hashmapCount == parameterCount + 3);
}

int argumentCount (struct Ecc * const ecc)
{
	return ecc->context->hashmapCount - 3;
}

struct Value argument (struct Ecc * const ecc, int argumentIndex)
{
	return ecc->context->hashmap[argumentIndex + 3].data.value;
}

void assertVariableParameter (struct Ecc * const ecc)
{
	assert(ecc->context->hashmap[2].data.value.type == Value(object));
}

int variableArgumentCount (struct Ecc * const ecc)
{
	return ecc->context->hashmap[2].data.value.data.object->elementCount;
}

struct Value variableArgument (struct Ecc * const ecc, int argumentIndex)
{
	return ecc->context->hashmap[2].data.value.data.object->element[argumentIndex].data.value;
}


// MARK: call

static struct Value getArgumentsLength (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 0);
	ecc->result = Value.binary(ecc->this.data.object->elementCount);
	return Value.undefined();
}

static struct Value setArgumentsLength (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	Object.resizeElement(ecc->this.data.object, Value.toBinary(Op.argument(ecc, 0)).data.binary);
	return Value.undefined();
}

static inline void populateContextWithArgumentsVA (struct Object *context, int parameterCount, int argumentCount, va_list ap)
{
	uint32_t index = 0;
	
	struct Object *arguments = Object.create(Object.prototype());
	Object.resizeElement(arguments, argumentCount);
	Object.add(arguments, Key(length), Value.function(Function.createWithNativeAccessor(NULL, getArgumentsLength, setArgumentsLength)), Object(writable));
	arguments->type = &Text(argumentsType);
	context->hashmap[2].data.value = Value.object(arguments);
	
	if (argumentCount <= parameterCount)
		for (; index < argumentCount; ++index)
		{
			context->hashmap[index + 3].data.value = arguments->element[index].data.value = va_arg(ap, struct Value);
			arguments->element[index].data.flags = Object(isValue);
		}
	else
	{
		for (; index < parameterCount; ++index)
		{
			context->hashmap[index + 3].data.value = arguments->element[index].data.value = va_arg(ap, struct Value);
			arguments->element[index].data.flags = Object(isValue);
		}
		
		for (; index < argumentCount; ++index)
		{
			arguments->element[index].data.value = va_arg(ap, struct Value);
			arguments->element[index].data.flags = Object(isValue);
		}
	}
}

static inline void populateContextWithArguments (const struct Op ** const ops, struct Ecc * const ecc, struct Object *context, int parameterCount, int argumentCount)
{
	uint32_t index = 0;
	
	struct Object *arguments = Object.create(Object.prototype());
	Object.resizeElement(arguments, argumentCount);
	Object.add(arguments, Key(length), Value.function(Function.createWithNativeAccessor(NULL, getArgumentsLength, setArgumentsLength)), Object(writable));
	arguments->type = &Text(argumentsType);
	context->hashmap[2].data.value = Value.object(arguments);
	
	if (argumentCount <= parameterCount)
		for (; index < argumentCount; ++index)
		{
			context->hashmap[index + 3].data.value = arguments->element[index].data.value = nextOp();
			arguments->element[index].data.flags = Object(isValue);
		}
	else
	{
		for (; index < parameterCount; ++index)
		{
			context->hashmap[index + 3].data.value = arguments->element[index].data.value = nextOp();
			arguments->element[index].data.flags = Object(isValue);
		}
		
		for (; index < argumentCount; ++index)
		{
			arguments->element[index].data.value = nextOp();
			arguments->element[index].data.flags = Object(isValue);
		}
	}
}

static inline void populateContextVA (struct Object *context, int parameterCount, int argumentCount, va_list ap)
{
	uint32_t index = 0;
	if (argumentCount <= parameterCount)
		for (; index < argumentCount; ++index)
			context->hashmap[index + 3].data.value = va_arg(ap, struct Value);
	else
	{
		for (; index < parameterCount; ++index)
			context->hashmap[index + 3].data.value = va_arg(ap, struct Value);
		
		for (; index < argumentCount; ++index)
			va_arg(ap, struct Value);
	}
}

static inline void populateContext (const struct Op ** const ops, struct Ecc * const ecc, struct Object *context, int parameterCount, int argumentCount)
{
	uint32_t index = 0;
	if (argumentCount <= parameterCount)
		for (; index < argumentCount; ++index)
			context->hashmap[index + 3].data.value = nextOp();
	else
	{
		for (; index < parameterCount; ++index)
			context->hashmap[index + 3].data.value = nextOp();
		
		for (; index < argumentCount; ++index)
			nextOp();
	}
}

static inline struct Value callOps (const struct Op * const ops, struct Ecc * const ecc, struct Object *context, struct Value this, int construct)
{
	struct Value callerThis = ecc->this;
	struct Object *callerContext = ecc->context;
	int callerConstruct = ecc->construct;
	const struct Op *callOps = ops;
	struct Value value;
	
	ecc->this = this;
	ecc->context = context;
	ecc->construct = construct;
	ecc->result = Value.undefined();
	
	ops->native(&callOps, ecc);
	value = ecc->result;
	
	ecc->this = callerThis;
	ecc->context = callerContext;
	ecc->construct = callerConstruct;
	
	return value;
}

struct Value callFunctionArguments (struct Function *function, struct Ecc * const ecc, struct Value this, struct Object *arguments)
{
	struct Object *context = Object.copy(&function->context);
	int parameterCount = function->parameterCount;
	int argumentCount = arguments->elementCount;
	uint32_t index = 0;
	
	context->hashmap[2].data.value = Value.object(arguments);
	
	if (argumentCount <= parameterCount)
		for (; index < argumentCount; ++index)
			context->hashmap[index + 3].data.value = arguments->element[index].data.value;
	else
		for (; index < parameterCount; ++index)
			context->hashmap[index + 3].data.value = arguments->element[index].data.value;
	
	return callOps(function->oplist->ops, ecc, context, this, 0);
}

struct Value callFunctionVA (struct Function *function, struct Ecc * const ecc, struct Value this, int argumentCount, ... )
{
	if (function->flags & Function(needHeap))
	{
		struct Object *context = Object.copy(&function->context);
		va_list ap;
		
		va_start(ap, argumentCount);
		if (function->flags & Function(needArguments))
			populateContextWithArgumentsVA(context, function->parameterCount, argumentCount, ap);
		else
			populateContextVA(context, function->parameterCount, argumentCount, ap);
		
		va_end(ap);
		
		return callOps(function->oplist->ops, ecc, context, this, 0);
	}
	else
	{
		struct Object stackContext;
		__typeof__(*function->context.hashmap) hashmap[function->context.hashmapCapacity];
		va_list ap;
		
		memcpy(hashmap, function->context.hashmap, sizeof(hashmap));
		
		stackContext = function->context;
		stackContext.hashmap = hashmap;
		
		va_start(ap, argumentCount);
		populateContextVA(&stackContext, function->parameterCount, argumentCount, ap);
		va_end(ap);
		
		return callOps(function->oplist->ops, ecc, &stackContext, this, 0);
	}
}

static inline struct Value callFunction (const struct Op ** const ops, struct Ecc * const ecc, struct Function * const function, int32_t argumentCount, int construct)
{
	struct Value this = ecc->refObject;
	
	if (function->flags & Function(needHeap))
	{
		struct Object *context = Object.copy(&function->context);
		
		if (function->flags & Function(needArguments))
			populateContextWithArguments(ops, ecc, context, function->parameterCount, argumentCount);
		else
			populateContext(ops, ecc, context, function->parameterCount, argumentCount);
		
		return callOps(function->oplist->ops, ecc, context, this, construct);
	}
	else
	{
		struct Object stackContext;
		__typeof__(*function->context.hashmap) hashmap[function->context.hashmapCapacity];
		
		memcpy(hashmap, function->context.hashmap, sizeof(hashmap));
		
		stackContext = function->context;
		stackContext.hashmap = hashmap;
		
		populateContext(ops, ecc, &stackContext, function->parameterCount, argumentCount);
		
		return callOps(function->oplist->ops, ecc, &stackContext, this, construct);
	}
}

struct Value construct (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Text text = (*ops)->text;
	int32_t argumentCount = opValue().data.integer;
	struct Value value = nextOp();
	if (value.type != Value(function))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(text, "%.*s is not a constructor", text.length, text.location)));
	
	// TODO
	ecc->refObject = Value.object(Object.create(Object.get(&value.data.function->object, Key(prototype)).data.object));
	
	return callFunction(ops, ecc, value.data.function, argumentCount, 1);
}

struct Value call (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Text text = (*ops)->text;
	int32_t argumentCount = opValue().data.integer;
	struct Value value = nextOp();
	if (value.type != Value(function))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(text, "%.*s is not a function", text.length, text.location)));
	
	return callFunction(ops, ecc, value.data.function, argumentCount, 0);
}

struct Value eval (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value;
	struct Input *input;
	int32_t argumentCount = opValue().data.integer;
	
	if (!argumentCount)
		return nextOp();
	
	value = nextOp();
	while (--argumentCount)
		nextOp();
	
	input = Input.createFromBytes(Value.stringChars(value), Value.stringLength(value), "(eval)");
	Ecc.evalInput(ecc, input);
	
	return ecc->result;
}


// Expression

struct Value noop (const struct Op ** const ops, struct Ecc * const ecc)
{
	return Value.undefined();
}

struct Value value (const struct Op ** const ops, struct Ecc * const ecc)
{
	return opValue();
}

struct Value valueConstRef (const struct Op ** const ops, struct Ecc * const ecc)
{
	return Value.reference((struct Value *)&opValue());
}

struct Value text (const struct Op ** const ops, struct Ecc * const ecc)
{
	return Value.text(opText());
}

struct Value function (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Function *function = Function.copy(opValue().data.function);
	function->context.prototype = ecc->context;
	return Value.function(function);
}

struct Value object (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Object *object = Object.create(Object.prototype());
	struct Value value;
	int isGetter, isSetter;
	uint32_t count;
	
	for (count = opValue().data.integer; count--;)
	{
		value = nextOp();
		isGetter = 0;
		isSetter = 0;
		
		if (value.type == Value(breaker))
		{
			if (value.data.integer == 0)
				isGetter = 1;
			else
				isSetter = 1;
			
			value = nextOp();
		}
		
		if (value.type == Value(key))
			Object.add(object, value.data.key, nextOp(), Object(writable) | Object(enumerable) | Object(configurable));
		else if (value.type == Value(integer))
			Object.addElementAtIndex(object, value.data.integer, nextOp(), Object(writable) | Object(enumerable) | Object(configurable));
	}
	return Value.object(object);
}

struct Value array (const struct Op ** const ops, struct Ecc * const ecc)
{
	uint32_t length = opValue().data.integer;
	struct Object *object = Array.createSized(length);
	struct Value value;
	uint32_t index;
	
	for (index = 0; index < length; ++index)
	{
		value = nextOp();
		if (value.type != Value(breaker))
		{
			object->element[index].data.value = value;
			object->element[index].data.flags = Object(isValue) | Object(writable) | Object(enumerable) | Object(configurable);
		}
	}
	
	return Value.object(object);
}

struct Value this (const struct Op ** const ops, struct Ecc * const ecc)
{
	return ecc->this;
}

struct Value getLocal (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Key key;
	struct Entry entry;
	
	ecc->refObject = Value.undefined();
	key = opValue().data.key;
	entry = Object.getMember(ecc->context, key);
	if (!entry.value)
		Ecc.jmpEnv(ecc, Value.error(Error.referenceError((*ops)->text, "%.*s is not defined", (*ops)->text.length, (*ops)->text.location)));
	
	return *entry.value;
}

struct Value getLocalRef (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Key key = opValue().data.key;
	struct Entry entry = Object.getMember(ecc->context, key);
	if (!entry.value)
		Ecc.jmpEnv(ecc, Value.error(Error.referenceError((*ops)->text, "%.*s is not defined", (*ops)->text.length, (*ops)->text.location)));
	
	return Value.reference(entry.value);
}

struct Value setLocal (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value;
	struct Key key = opValue().data.key;
	struct Entry entry = Object.getMember(ecc->context, key);
	if (!entry.value)
		Ecc.jmpEnv(ecc, Value.error(Error.referenceError((*ops)->text, "%.*s is not defined", (*ops)->text.length, (*ops)->text.location)));
	
	value = nextOp();
	return *entry.value = value;
}

struct Value getLocalSlot (const struct Op ** const ops, struct Ecc * const ecc)
{
	int32_t slot;
	
	ecc->refObject = Value.undefined();
	slot = opValue().data.integer;
	return ecc->context->hashmap[slot].data.value;
}

struct Value getLocalSlotRef (const struct Op ** const ops, struct Ecc * const ecc)
{
	int32_t slot = opValue().data.integer;
	return Value.reference(&ecc->context->hashmap[slot].data.value);
}

struct Value setLocalSlot (const struct Op ** const ops, struct Ecc * const ecc)
{
	int32_t slot = opValue().data.integer;
	struct Value value = nextOp();
	return ecc->context->hashmap[slot].data.value = value;
}

struct Value getMember (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Key key = opValue().data.key;
	struct Value value = nextOp();
	struct Value object = Value.toObject(value, ecc, opText());
	struct Entry entry;
	ecc->refObject = object;
	entry = Object.getMember(object.data.object, key);
	return getEntry(entry, ecc, object);
}

struct Value getMemberRef (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Key key = opValue().data.key;
//	const struct Text *text = opText();
	struct Value value = nextOp();
	struct Value object = Value.toObject(value, ecc, opText());
	struct Entry entry = Object.getMember(object.data.object, key);
	if (!entry.value)
		entry = Object.add(object.data.object, key, value, Object(writable) | Object(enumerable) | Object(configurable));
//		Ecc.jmpEnv(ecc, Value.error(Error.referenceError(*text, "%.*s is not defined", text->length, text->location)));
	
	return Value.reference(entry.value);
}

struct Value setMember (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Key key = opValue().data.key;
	struct Value value = nextOp();
	struct Value object = Value.toObject(value, ecc, opText());
	struct Entry entry = Object.getMember(object.data.object, key);
	value = nextOp();
	if (entry.value)
		return setEntry(entry, ecc, object, value);
	
	Object.add(object.data.object, key, value, Object(writable) | Object(enumerable) | Object(configurable));
	return value;
}

struct Value deleteMember (const struct Op ** const ops, struct Ecc * const ecc)
{
	const struct Text *text = opText();
	struct Key key = opValue().data.key;
	struct Value value = nextOp();
	struct Value object = Value.toObject(value, ecc, opText());
	struct Value result = Object.delete(object.data.object, key);
	if (!Value.isTrue(result))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(*text, "property '%.*s' is non-configurable and can't be deleted", Key.textOf(key)->length, Key.textOf(key)->location)));
	
	return result;
}

struct Value getProperty (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value = nextOp();
	struct Value object = Value.toObject(value, ecc, opText());
	struct Entry entry;
	ecc->refObject = object;
	entry = Object.getProperty(object.data.object, nextOp());
	return getEntry(entry, ecc, object);
}

struct Value getPropertyRef (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value = nextOp();
	struct Value object = Value.toObject(value, ecc, opText());
	struct Entry entry;
	ecc->refObject = object;
	entry = Object.getProperty(object.data.object, nextOp());
	if (!entry.value)
		Ecc.jmpEnv(ecc, Value.error(Error.referenceError((*ops)->text, "%.*s is not defined", (*ops)->text.length, (*ops)->text.location)));
	
	return Value.reference(entry.value);
}

struct Value setProperty (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value = nextOp();
	struct Value object = Value.toObject(value, ecc, opText());
	struct Value property = nextOp();
	struct Entry entry;
	value = nextOp();
	
	entry = Object.getProperty(object.data.object, property);
	if (entry.value)
		return setEntry(entry, ecc, object, value);
	
	Object.setProperty(object.data.object, property, value);
	
	
	return value;
}

struct Value deleteProperty (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value = nextOp();
	struct Value object = Value.toObject(value, ecc, opText());
	struct Value property = nextOp();
	const struct Text *text = opText();
	struct Value result = Object.deleteProperty(object.data.object, property);
	if (!Value.isTrue(result))
	{
		struct Value string = Value.toString(property);
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(*text, "property '%.*s' is non-configurable and can't be deleted", Value.stringLength(string), Value.stringChars(string))));
	}
	
	return result;
}

struct Value result (const struct Op ** const ops, struct Ecc * const ecc)
{
	ecc->result = nextOp();
	return Value.breaker(0);
}

struct Value exchange (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value = opValue();
	nextOp();
	return value;
}

struct Value typeOf (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value = nextOp();
	return Value.toType(value);
}

struct Value equal (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	const struct Text *text = opText();
	struct Value b = nextOp();
	return equality(ecc, a, text, b, opText());
}

struct Value notEqual (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	const struct Text *text = opText();
	struct Value b = nextOp();
	return Value.boolean(!Value.isTrue(equality(ecc, a, text, b, opText())));
}

struct Value identical (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	const struct Text *text = opText();
	struct Value b = nextOp();
	return identicality(ecc, a, text, b, opText());
}

struct Value notIdentical (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	const struct Text *text = opText();
	struct Value b = nextOp();
	return Value.boolean(!Value.isTrue(identicality(ecc, a, text, b, opText())));
}

struct Value less (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	const struct Text *text = opText();
	struct Value b = nextOp();
	return valueLess(ecc, a, text, b, opText());
}

struct Value lessOrEqual (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	const struct Text *text = opText();
	struct Value b = nextOp();
	return valueLessOrEqual(ecc, a, text, b, opText());
}

struct Value more (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	const struct Text *text = opText();
	struct Value b = nextOp();
	return valueMore(ecc, a, text, b, opText());
}

struct Value moreOrEqual (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	const struct Text *text = opText();
	struct Value b = nextOp();
	return valueMoreOrEqual(ecc, a, text, b, opText());
}

struct Value instanceOf (const struct Op ** const ops, struct Ecc * const ecc)
{
	const struct Text *text;
	struct Value a = nextOp();
	struct Value b;
	struct Object *object;
	
	if (!Value.isObject(a))
		return Value.false();
	
	text = opText();
	b = nextOp();
	if (!Value.isObject(b))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(*text, "%.*s is not an object", text->length, text->location)));
	
	object = b.data.object;
	do
		if (a.data.object->prototype == object)
			return Value.true();
	while (( object = object->prototype ));
	
	return Value.false();
}

struct Value in (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value property = nextOp();
	struct Value object = nextOp();
	struct Entry entry;
	
	if (!Value.isObject(object))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError((*ops)->text, "invalid 'in' operand %.*s", (*ops)->text.length, (*ops)->text.location)));
	
	entry = Object.getProperty(object.data.object, property);
	return Value.boolean(*entry.flags & Object(isValue));
}

struct Value multiply (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.binary(Value.toBinary(a).data.binary * Value.toBinary(b).data.binary);
}

struct Value divide (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.binary(Value.toBinary(a).data.binary / Value.toBinary(b).data.binary);
}

struct Value modulo (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.binary(fmod(Value.toBinary(a).data.binary, Value.toBinary(b).data.binary));
}

struct Value add (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	const struct Text *text = opText();
	struct Value b = nextOp();
	return addition(ecc, a, text, b, opText());
}

struct Value minus (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.binary(Value.toBinary(a).data.binary - Value.toBinary(b).data.binary);
}

struct Value leftShift (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer(Value.toInteger(a).data.integer << (uint32_t)Value.toInteger(b).data.integer);
}

struct Value rightShift (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer(Value.toInteger(a).data.integer >> (uint32_t)Value.toInteger(b).data.integer);
}

struct Value unsignedRightShift (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer((uint32_t)Value.toInteger(a).data.integer >> (uint32_t)Value.toInteger(b).data.integer);
}

struct Value bitwiseAnd (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer(Value.toInteger(a).data.integer & Value.toInteger(b).data.integer);
}

struct Value bitwiseXor (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer(Value.toInteger(a).data.integer ^ Value.toInteger(b).data.integer);
}

struct Value bitwiseOr (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer(Value.toInteger(a).data.integer | Value.toInteger(b).data.integer);
}

struct Value logicalAnd (const struct Op ** const ops, struct Ecc * const ecc)
{
	const int32_t opCount = opValue().data.integer;
	if (!Value.isTrue(nextOp()))
	{
		*ops += opCount;
		return Value.false();
	}
	else
		return Value.boolean(Value.isTrue(nextOp()));
}

struct Value logicalOr (const struct Op ** const ops, struct Ecc * const ecc)
{
	const int32_t opCount = opValue().data.integer;
	if (Value.isTrue(nextOp()))
	{
		*ops += opCount;
		return Value.true();
	}
	else
		return Value.boolean(Value.isTrue(nextOp()));
}

struct Value positive (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	return Value.toBinary(a);
}

struct Value negative (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	return Value.binary(-Value.toBinary(a).data.binary);
}

struct Value invert (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	return Value.integer(~Value.toInteger(a).data.integer);
}

struct Value not (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	return Value.boolean(!Value.isTrue(a));
}

// MARK: assignement

struct Value incrementRef (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value *ref = nextOp().data.reference;
	if (ref->type == Value(integer))
		++ref->data.integer;
	else if (ref->type == Value(binary))
		++ref->data.binary;
	else
	{
		*ref = Value.toBinary(*ref);
		++ref->data.binary;
	}
	return *ref;
}

struct Value decrementRef (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value *ref = nextOp().data.reference;
	if (ref->type == Value(integer))
		--ref->data.integer;
	else if (ref->type == Value(binary))
		--ref->data.binary;
	else
	{
		*ref = Value.toBinary(*ref);
		--ref->data.binary;
	}
	return *ref;
}

struct Value postIncrementRef (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value *ref = nextOp().data.reference;
	struct Value value = *ref;
	if (ref->type == Value(integer))
		++ref->data.integer;
	else if (ref->type == Value(binary))
		++ref->data.binary;
	else
	{
		*ref = Value.toBinary(*ref);
		++ref->data.binary;
	}
	return value;
}

struct Value postDecrementRef (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value *ref = nextOp().data.reference;
	struct Value value = *ref;
	if (ref->type == Value(integer))
		--ref->data.integer;
	else if (ref->type == Value(binary))
		--ref->data.binary;
	else
	{
		*ref = Value.toBinary(*ref);
		--ref->data.binary;
	}
	return value;
}

struct Value multiplyAssignRef (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	*ref = Value.toBinary(*ref);
	ref->data.binary *= Value.toBinary(b).data.binary;
	return *ref;
}

struct Value divideAssignRef (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	*ref = Value.toBinary(*ref);
	ref->data.binary /= Value.toBinary(b).data.binary;
	return *ref;
}

struct Value moduloAssignRef (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	*ref = Value.toBinary(*ref);
	ref->data.binary = fmod(ref->data.binary, Value.toBinary(b).data.binary);
	return *ref;
}

struct Value addAssignRef (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value *a = nextOp().data.reference;
	const struct Text *text = opText();
	struct Value b = nextOp();
	*a = addition(ecc, *a, text, b, opText());
	return *a;
}

struct Value minusAssignRef (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	*ref = Value.toBinary(*ref);
	ref->data.binary -= Value.toBinary(b).data.binary;
	return *ref;
}

struct Value leftShiftAssignRef (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	*ref = Value.toInteger(*ref);
	ref->data.integer <<= (uint32_t)Value.toInteger(b).data.integer;
	return *ref;
}

struct Value rightShiftAssignRef (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	*ref = Value.toInteger(*ref);
	ref->data.integer >>= (uint32_t)Value.toInteger(b).data.integer;
	return *ref;
}

struct Value unsignedRightShiftAssignRef (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	uint32_t uint;
	
	*ref = Value.toInteger(*ref);
	uint = ref->data.integer;
	uint >>= (uint32_t)Value.toInteger(b).data.integer;
	ref->data.integer = uint;
	return *ref;
}

struct Value bitAndAssignRef (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	*ref = Value.toInteger(*ref);
	ref->data.integer &= Value.toInteger(b).data.integer;
	return *ref;
}

struct Value bitXorAssignRef (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	*ref = Value.toInteger(*ref);
	ref->data.integer ^= Value.toInteger(b).data.integer;
	return *ref;
}

struct Value bitOrAssignRef (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	*ref = Value.toInteger(*ref);
	ref->data.integer |= Value.toInteger(b).data.integer;
	return *ref;
}


// MARK: Statement

struct Value try(const struct Op ** const ops, struct Ecc * const ecc)
{
	const struct Op *end = *ops + opValue().data.integer;
	struct Object *context = Object.create(ecc->context);
	struct Key key = Key(none);
	
	const struct Op * volatile rethrowOps = NULL;
	volatile int rethrow = 0;
	volatile struct Value value = Value.undefined();
	struct Value finallyValue;
	
	ecc->context = context;
	
	if (!setjmp(*Ecc.pushEnv(ecc))) // try
		value = nextOp();
	else
	{
		value = ecc->result;
		rethrowOps = *ops;
		
		if (!rethrow) // catch
		{
			rethrow = 1;
			*ops = end + 1; // bypass catch jump
			key = nextOp().data.key;
			
			if (!Key.isEqual(key, Key(none)))
			{
				Object.add(context, key, ecc->result, 0);
				ecc->result = Value.undefined();
				value = nextOp(); // execute until noop
				rethrow = 0;
			}
		}
	}
	
	Ecc.popEnv(ecc);
	
	ecc->context = context->prototype;
	
	*ops = end; // op[end] = Op.jump, to after catch
	finallyValue = nextOp(); // jump to after catch, and execute until noop
	
	if (finallyValue.type != Value(undefined)) /* return breaker */
		return finallyValue;
	else if (rethrow)
	{
		*ops = rethrowOps;
		Ecc.jmpEnv(ecc, value);
	}
	else if (value.type != Value(undefined)) /* return breaker */
		return value;
	else
		return nextOp();
}

dead struct Value throw (const struct Op ** const ops, struct Ecc * const ecc)
{
	const struct Op *throwOps = *ops;
	struct Value value = nextOp();
	*ops = throwOps + 1;
	Ecc.jmpEnv(ecc, value);
}

struct Value debug (const struct Op ** const ops, struct Ecc * const ecc)
{
	__asm__("int3");
	return nextOp();
}

struct Value next (const struct Op ** const ops, struct Ecc * const ecc)
{
	return nextOp();
}

struct Value nextIf (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value = opValue();
	if (!Value.isTrue(nextOp()))
		return value;
	
	return nextOp();
}

struct Value expression (const struct Op ** const ops, struct Ecc * const ecc)
{
	ecc->result = nextOp();
	return nextOp();
}

struct Value discard (const struct Op ** const ops, struct Ecc * const ecc)
{
	nextOp();
	return nextOp();
}

struct Value jump (const struct Op ** const ops, struct Ecc * const ecc)
{
	int32_t offset = opValue().data.integer;
	*ops += offset;
	return nextOp();
}

struct Value jumpIf (const struct Op ** const ops, struct Ecc * const ecc)
{
	int32_t offset = opValue().data.integer;
	struct Value value = nextOp();
	if (Value.isTrue(value))
		*ops += offset;
	
	return nextOp();
}

struct Value jumpIfNot (const struct Op ** const ops, struct Ecc * const ecc)
{
	int32_t offset = opValue().data.integer;
	struct Value value = nextOp();
	if (!Value.isTrue(value))
		*ops += offset;
	
	return nextOp();
}

struct Value switchOp (const struct Op ** const ops, struct Ecc * const ecc)
{
	int32_t offset = opValue().data.integer;
	const struct Op *nextOps = *ops + offset;
	struct Value a = nextOp(), b;
	const struct Text *text = opText();
	
	while (*ops < nextOps)
	{
		b = nextOp();
		if (Value.isTrue(equality(ecc, a, text, b, opText())))
		{
			nextOps += nextOp().data.integer + 1;
			*ops = nextOps;
			break;
		}
		else
			++*ops;
	}
	
	return nextOp();
}

// MARK: Iteration

#define stepIteration(value, nextOps) \
	if (( value = nextOp() ).type == Value(breaker) && --value.data.integer) \
	{ \
		if (--value.data.integer) \
			return value; /* return breaker */\
		else \
		{ \
			*ops = endOps; \
			return nextOp(); \
		} \
	} \
	else \
		*ops = nextOps;

struct Value iterate (const struct Op ** const ops, struct Ecc * const ecc)
{
	const struct Op *startOps = *ops;
	const struct Op *endOps = startOps;
	const struct Op *nextOps = startOps + 1;
	struct Value value;
	
	*ops += opValue().data.integer;
	
	while (Value.isTrue(nextOp()))
		stepIteration(value, nextOps);
	
	*ops = endOps;
	return nextOp();
}

static struct Value iterateIntegerRef (
	const struct Op ** const ops,
	struct Ecc * const ecc,
	int (*compareInteger) (int32_t, int32_t),
	__typeof__(integerWontOverflowPositive) wontOverflow,
	__typeof__(compare) compareValue,
	__typeof__(addition) valueStep)
{
	const struct Op *endOps = (*ops) + opValue().data.integer;
	const struct Text *stepText = opText();
	struct Value stepValue = nextOp();
	const struct Text *indexText = opText();
	struct Value *indexRef = nextOp().data.reference;
	const struct Text *countText = opText();
	struct Value *countRef = nextOp().data.reference;
	const struct Op *nextOps = *ops;
	struct Value value;
	
	if (indexRef->type == Value(integer) && countRef->type == Value(integer))
	{
		int32_t step = valueStep == addition? stepValue.data.integer: -stepValue.data.integer;
		for (; compareInteger(indexRef->data.integer, countRef->data.integer); indexRef->data.integer += step)
		{
			stepIteration(value, nextOps);
			
			if (indexRef->type == Value(integer) && countRef->type == Value(integer) && wontOverflow(indexRef->data.integer, step))
				continue;
			else
			{
				*indexRef = valueStep(ecc, *indexRef, indexText, *countRef, countText);
				goto deoptimize;
			}
		}
		goto done;
	}
	
deoptimize:
	for (; Value.isTrue(compareValue(ecc, *indexRef, indexText, *countRef, countText))
		 ; *indexRef = valueStep(ecc, *indexRef, indexText, stepValue, stepText)
		 )
		stepIteration(value, nextOps);
	
done:
	*ops = endOps;
	return nextOp();
}

struct Value iterateLessRef (const struct Op ** const ops, struct Ecc * const ecc)
{
	return iterateIntegerRef(ops, ecc, integerLess, integerWontOverflowPositive, valueLess, addition);
}

struct Value iterateLessOrEqualRef (const struct Op ** const ops, struct Ecc * const ecc)
{
	return iterateIntegerRef(ops, ecc, integerLessOrEqual, integerWontOverflowPositive, valueLessOrEqual, addition);
}

struct Value iterateMoreRef (const struct Op ** const ops, struct Ecc * const ecc)
{
	return iterateIntegerRef(ops, ecc, integerMore, integerWontOverflowNegative, valueMore, subtraction);
}

struct Value iterateMoreOrEqualRef (const struct Op ** const ops, struct Ecc * const ecc)
{
	return iterateIntegerRef(ops, ecc, integerMoreOrEqual, integerWontOverflowNegative, valueMoreOrEqual, subtraction);
}

struct Value iterateInRef (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value *ref = nextOp().data.reference;
	struct Value object = nextOp();
	struct Value value = nextOp();
	const struct Op *startOps = *ops;
	const struct Op *endOps = startOps + value.data.integer;
	
	if (Value.isObject(object))
	{
		uint32_t index;
		
		for (index = 0; index < object.data.object->elementCount; ++index)
		{
			if (!(object.data.object->element[index].data.flags & Object(isValue)))
				continue;
			
			*ref = Value.chars(Chars.create("%d", index));
			
			stepIteration(value, startOps);
		}
		
		for (index = 2; index <= object.data.object->hashmapCount; ++index)
		{
			if (!(object.data.object->hashmap[index].data.flags & Object(isValue)))
				continue;
			
			*ref = Value.key(object.data.object->hashmap[index].data.key);
			
			stepIteration(value, startOps);
		}
	}
	
	*ops = endOps;
	return nextOp();
}
