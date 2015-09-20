//
//  op.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "op.h"

// MARK: - Private

#define nextOp() (++*ops)->function(ops, ecc)
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

// MARK: - Static Members

// MARK: - Methods

struct Op make (const Function function, struct Value value, struct Text text)
{
	return (struct Op){ function, value, text };
}

const char * toChars (const Function function)
{
	#define _(X) { #X, X, },
	struct {
		const char *name;
		const Function function;
	} static const functionList[] = {
		io_libecc_op_List
	};
	#undef _
	
	for (int index = 0; index < sizeof(functionList); ++index)
		if (functionList[index].function == function)
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

static inline void populateContextWithArgumentsVA (struct Object *context, int parameterCount, int argumentCount, va_list ap)
{
	struct Object *arguments = Object.create(NULL);
	Object.resizeElement(arguments, argumentCount);
	context->hashmap[2].data.value = Value.object(arguments);
	
	uint_fast32_t index = 0;
	if (argumentCount <= parameterCount)
		for (; index < argumentCount; ++index)
			context->hashmap[index + 3].data.value = arguments->element[index].data.value = va_arg(ap, struct Value);
	else
	{
		for (; index < parameterCount; ++index)
			context->hashmap[index + 3].data.value = arguments->element[index].data.value = va_arg(ap, struct Value);
		
		for (; index < argumentCount; ++index)
			arguments->element[index].data.value = va_arg(ap, struct Value);
	}
}

static inline void populateContextWithArguments (const Instance * const ops, struct Ecc * const ecc, struct Object *context, int parameterCount, int argumentCount)
{
	struct Object *arguments = Object.create(NULL);
	Object.resizeElement(arguments, argumentCount);
	context->hashmap[2].data.value = Value.object(arguments);
	
	uint_fast32_t index = 0;
	if (argumentCount <= parameterCount)
		for (; index < argumentCount; ++index)
			context->hashmap[index + 3].data.value = arguments->element[index].data.value = nextOp();
	else
	{
		for (; index < parameterCount; ++index)
			context->hashmap[index + 3].data.value = arguments->element[index].data.value = nextOp();
		
		for (; index < argumentCount; ++index)
			arguments->element[index].data.value = nextOp();
	}
}

static inline void populateContextVA (struct Object *context, int parameterCount, int argumentCount, va_list ap)
{
	uint_fast32_t index = 0;
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

static inline void populateContext (const Instance * const ops, struct Ecc * const ecc, struct Object *context, int parameterCount, int argumentCount)
{
	uint_fast32_t index = 0;
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

static inline struct Value callOps (const Instance ops, struct Ecc * const ecc, struct Object *context, struct Value this, int construct)
{
	struct Value callerThis = ecc->this;
	struct Object *callerContext = ecc->context;
	int callerConstruct = ecc->construct;
	const Instance callOps = ops;
	struct Value value;
	
	context->prototype = callerContext;
	
	ecc->this = this;
	ecc->context = context;
	ecc->construct = construct;
	
	ops->function(&callOps, ecc);
	value = ecc->result;
	
	ecc->this = callerThis;
	ecc->context = callerContext;
	ecc->construct = callerConstruct;
	
	return value;
}

struct Value callClosureVA (struct Closure *closure, struct Ecc * const ecc, struct Value this, int argumentCount, ... )
{
	if (closure->needHeap)
	{
		struct Object *context = Object.copy(&closure->context);
		
		va_list ap;
		va_start(ap, argumentCount);
		if (closure->needArguments)
			populateContextWithArgumentsVA(context, closure->parameterCount, argumentCount, ap);
		else
			populateContextVA(context, closure->parameterCount, argumentCount, ap);
		
		va_end(ap);
		
		return callOps(closure->oplist->ops, ecc, context, this, 0);
	}
	else
	{
		__typeof__(*closure->context.hashmap) hashmap[closure->context.hashmapCapacity];
		memcpy(hashmap, closure->context.hashmap, sizeof(hashmap));
		
		struct Object stackContext = closure->context;
		stackContext.hashmap = hashmap;
		
		va_list ap;
		va_start(ap, argumentCount);
		populateContextVA(&stackContext, closure->parameterCount, argumentCount, ap);
		va_end(ap);
		
		return callOps(closure->oplist->ops, ecc, &stackContext, this, 0);
	}
}

static inline struct Value callClosure (const Instance * const ops, struct Ecc * const ecc, struct Closure * const closure, int32_t argumentCount, int construct)
{
	if (closure->needHeap)
	{
		struct Object *context = Object.copy(&closure->context);
		
		if (closure->needArguments)
			populateContextWithArguments(ops, ecc, context, closure->parameterCount, argumentCount);
		else
			populateContext(ops, ecc, context, closure->parameterCount, argumentCount);
		
		return callOps(closure->oplist->ops, ecc, context, ecc->refObject, construct);
	}
	else
	{
		__typeof__(*closure->context.hashmap) hashmap[closure->context.hashmapCapacity];
		memcpy(hashmap, closure->context.hashmap, sizeof(hashmap));
		
		struct Object stackContext = closure->context;
		stackContext.hashmap = hashmap;
		
		populateContext(ops, ecc, &stackContext, closure->parameterCount, argumentCount);
		
		return callOps(closure->oplist->ops, ecc, &stackContext, ecc->refObject, construct);
	}
}

struct Value construct (const Instance * const ops, struct Ecc * const ecc)
{
	struct Text text = (*ops)->text;
	int32_t argumentCount = opValue().data.integer;
	struct Value value = nextOp();
	if (value.type != Value(closure))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(text, "%.*s is not a constructor", (*ops)->text.length, (*ops)->text.location)));
	
	return callClosure(ops, ecc, value.data.closure, argumentCount, 1);
}

struct Value call (const Instance * const ops, struct Ecc * const ecc)
{
	struct Text text = (*ops)->text;
	int32_t argumentCount = opValue().data.integer;
	struct Value value = nextOp();
	if (value.type != Value(closure))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(text, "%.*s is not a function", (*ops)->text.length, (*ops)->text.location)));
	
	return callClosure(ops, ecc, value.data.closure, argumentCount, 0);
}


// Expression

struct Value noop (const Instance * const ops, struct Ecc * const ecc)
{
	return Value.undefined();
}

struct Value value (const Instance * const ops, struct Ecc * const ecc)
{
	return opValue();
}

struct Value valueConstRef (const Instance * const ops, struct Ecc * const ecc)
{
	return Value.reference((struct Value *)&opValue());
}

struct Value text (const Instance * const ops, struct Ecc * const ecc)
{
	return Value.text(opText());
}

struct Value closure (const Instance * const ops, struct Ecc * const ecc)
{
	struct Closure *closure = Closure.copy(opValue().data.closure);
	closure->context.prototype = ecc->context;
	return Value.closure(closure);
}

struct Value object (const Instance * const ops, struct Ecc * const ecc)
{
	struct Object *object = Object.create(Object.prototype());
	struct Value value;
	for (uint_fast32_t count = opValue().data.integer; count--;)
	{
		value = nextOp();
		if (value.type == Value(identifier))
			Object.add(object, value.data.identifier, nextOp(), Object(writable) | Object(enumerable) | Object(configurable));
		else if (value.type == Value(integer))
			Object.addElementAtIndex(object, value.data.integer, nextOp());
	}
	return Value.object(object);
}

struct Value array (const Instance * const ops, struct Ecc * const ecc)
{
	uint32_t length = opValue().data.integer;
	struct Object *object = Object.create(Array.prototype());
	Object.resizeElement(object, length);
	for (uint_fast32_t index = 0; index < length; ++index)
	{
		object->element[index].data.value = nextOp();
		object->element[index].data.flags |= Object(isValue);
	}
	
	return Value.object(object);
}

struct Value this (const Instance * const ops, struct Ecc * const ecc)
{
	return ecc->this;
}

struct Value getLocal (const Instance * const ops, struct Ecc * const ecc)
{
	struct Identifier identifier = opValue().data.identifier;
	struct Value *ref = Object.ref(ecc->context, identifier, 0);
	if (!ref)
		Ecc.jmpEnv(ecc, Value.error(Error.referenceError((*ops)->text, "%.*s is not defined", (*ops)->text.length, (*ops)->text.location)));
	
	return *ref;
}

struct Value getLocalRef (const Instance * const ops, struct Ecc * const ecc)
{
	struct Identifier identifier = opValue().data.identifier;
	struct Value *ref = Object.ref(ecc->context, identifier, 0);
	if (!ref)
		Ecc.jmpEnv(ecc, Value.error(Error.referenceError((*ops)->text, "%.*s is not defined", (*ops)->text.length, (*ops)->text.location)));
	
	return Value.reference(ref);
}

struct Value setLocal (const Instance * const ops, struct Ecc * const ecc)
{
	struct Identifier identifier = opValue().data.identifier;
	struct Value *ref = Object.ref(ecc->context, identifier, 0);
	if (!ref)
		Ecc.jmpEnv(ecc, Value.error(Error.referenceError((*ops)->text, "%.*s is not defined", (*ops)->text.length, (*ops)->text.location)));
	
	struct Value value = nextOp();
	
//	fprintf(stderr, "set ");
//	Identifier.dumpTo(identifier, stderr);
//	fprintf(stderr, " = ");
//	Value.dumpTo(&value, stderr);
//	fprintf(stderr, "\n");
	
//	if (ecc->context->traceCount && Value.isDynamic(value))
//		Value.retain(value, ecc->context->traceCount);
	
	return *ref = value;
}

struct Value deleteLocal (const Instance * const ops, struct Ecc * const ecc)
{
	struct Identifier identifier = opValue().data.identifier;
	struct Value result = Object.delete(ecc->context, identifier);
	if (!Value.isTrue(result))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError((*ops)->text, "property '%.*s' is non-configurable and can't be deleted", (*ops)->text.length, (*ops)->text.location)));
	
	return result;
}

struct Value getLocalSlot (const Instance * const ops, struct Ecc * const ecc)
{
	int32_t slot = opValue().data.integer;
	return ecc->context->hashmap[slot].data.value;
}

struct Value getLocalSlotRef (const Instance * const ops, struct Ecc * const ecc)
{
	int32_t slot = opValue().data.integer;
	return Value.reference(&ecc->context->hashmap[slot].data.value);
}

struct Value setLocalSlot (const Instance * const ops, struct Ecc * const ecc)
{
	int32_t slot = opValue().data.integer;
	struct Value value = nextOp();
	
//	if (ecc->context->traceCount && Value.isDynamic(value))
//		Value.retain(value, ecc->context->traceCount);
	
	return ecc->context->hashmap[slot].data.value = value;
}

struct Value getMember (const Instance * const ops, struct Ecc * const ecc)
{
	struct Identifier identifier = opValue().data.identifier;
	struct Value object = Value.toObject(nextOp(), ecc, opText());
	ecc->refObject = object;
	return Object.get(object.data.object, identifier);
}

struct Value getMemberRef (const Instance * const ops, struct Ecc * const ecc)
{
	struct Identifier identifier = opValue().data.identifier;
	struct Value object = Value.toObject(nextOp(), ecc, opText());
	ecc->refObject = object;
	struct Value *ref = Object.ref(object.data.object, identifier, 1);
	return Value.reference(ref);
}

struct Value setMember (const Instance * const ops, struct Ecc * const ecc)
{
	struct Identifier identifier = opValue().data.identifier;
	struct Value object = Value.toObject(nextOp(), ecc, opText());
	struct Value value = nextOp();
	Object.set(object.data.object, identifier, value);
	return value;
}

struct Value deleteMember (const Instance * const ops, struct Ecc * const ecc)
{
	const struct Text *text = opText();
	struct Identifier identifier = opValue().data.identifier;
	struct Value object = Value.toObject(nextOp(), ecc, opText());
	struct Value result = Object.delete(object.data.object, identifier);
	if (!Value.isTrue(result))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(*text, "property '%.*s' is non-configurable and can't be deleted", Identifier.textOf(identifier)->length, Identifier.textOf(identifier)->location)));
	
	return result;
}

struct Value getProperty (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value object = Value.toObject(nextOp(), ecc, opText());
	ecc->refObject = object;
	struct Value property = nextOp();
	
	if (Value.isString(property))
	{
		int digitsOnly = 1;
		const char *chars = Value.stringChars(property);
		uint16_t length = Value.stringLength(property);
		
		for (uint16_t index = 0; index < length; ++index)
			if (!isdigit(chars[index]))
			{
				digitsOnly = 0;
				break;
			}
		
		if (digitsOnly)
		{
			struct Value index = Value.toBinary(property);
			if (index.data.binary >= 0 && index.data.binary < INT32_MAX)
				return object.data.object->element[(int32_t)index.data.binary].data.value;
		}
		
		property = Value.identifier(Identifier.makeWithText(Text.make(chars, length), 1));
	}
	
	return Object.get(object.data.object, property.data.identifier);
}

struct Value getPropertyRef (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value object = Value.toObject(nextOp(), ecc, opText());
	ecc->refObject = object;
	struct Value property = nextOp();
	struct Value *ref = Object.ref(object.data.object, property.data.identifier, 1);
	return Value.reference(ref);
}

struct Value setProperty (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value object = Value.toObject(nextOp(), ecc, opText());
	
	struct Value index = nextOp();
//	if (index.type == Value(binary))
//	{
//		Ecc.printTextInput(ecc, (*ops)->text);
//		Env.printWarning("Using floating-point as property name polute identifier's pool; using as an int. (use '%lf' alternatively)", index.data.binary);
//		index = Value.integer(index.data.binary);
//	}
//	else if (Value.isString(&index))
//	{
//		const char *chars = Value.stringChars(&index);
//		for (uint_fast16_t index = 0, count = Value.stringLength(&index); index < count; ++index)
//			if ()
//	}
//	
//	if (index.type == Value(integer))
//	{
//		if (index.data.binary >= 0 && index.data.binary < object.data.object->elementCount)
//			return object.data.object->element[index.data.integer];
//		else
//			return Value.undefined();
//	}

//	if (Value.isString(&identifier))
//	{
//		int isElement = 1;
//		const char *chars = Value.stringChars(&identifier);
//		for (uint16_t index = 0, length = Value.stringLength(&identifier); index < length; ++index)
//		{
//			
//		}
//	}
	
	if (index.type == Value(binary))
	{
		if (index.data.binary == (int32_t)index.data.binary)
		{
		}
		else
		{
			struct Value string = Value.toString(index);
			index = Value.identifier(Identifier.makeWithText(Text.make(Value.stringChars(string), Value.stringLength(string)), 1));
		}
	}
	
	struct Value value = nextOp();
	Object.set(object.data.object, index.data.identifier, value);
	return value;
}

struct Value deleteProperty (const Instance * const ops, struct Ecc * const ecc)
{
	const struct Text *text = opText();
	struct Value object = Value.toObject(nextOp(), ecc, opText());
	struct Identifier identifier = nextOp().data.identifier;
	struct Value result = Object.delete(object.data.object, identifier);
	if (!Value.isTrue(result))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(*text, "property '%.*s' is non-configurable and can't be deleted", Identifier.textOf(identifier)->length, Identifier.textOf(identifier)->location)));
	
	return result;
}

struct Value result (const Instance * const ops, struct Ecc * const ecc)
{
	ecc->result = nextOp();
	return Value.breaker(0);
}

struct Value exchange (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value value = opValue();
	nextOp();
	return value;
}

struct Value typeOf (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value value = nextOp();
	return Value.toType(value);
}

struct Value equal (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	const struct Text *text = opText();
	struct Value b = nextOp();
	return equality(ecc, a, text, b, opText());
}

struct Value notEqual (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	const struct Text *text = opText();
	struct Value b = nextOp();
	return Value.boolean(!Value.isTrue(equality(ecc, a, text, b, opText())));
}

struct Value identical (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	const struct Text *text = opText();
	struct Value b = nextOp();
	return identicality(ecc, a, text, b, opText());
}

struct Value notIdentical (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	const struct Text *text = opText();
	struct Value b = nextOp();
	return Value.boolean(!Value.isTrue(identicality(ecc, a, text, b, opText())));
}

struct Value less (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	const struct Text *text = opText();
	struct Value b = nextOp();
	return valueLess(ecc, a, text, b, opText());
}

struct Value lessOrEqual (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	const struct Text *text = opText();
	struct Value b = nextOp();
	return valueLessOrEqual(ecc, a, text, b, opText());
}

struct Value more (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	const struct Text *text = opText();
	struct Value b = nextOp();
	return valueMore(ecc, a, text, b, opText());
}

struct Value moreOrEqual (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	const struct Text *text = opText();
	struct Value b = nextOp();
	return valueMoreOrEqual(ecc, a, text, b, opText());
}

struct Value instanceOf (const Instance * const ops, struct Ecc * const ecc)
{
//	struct Value a = nextOp();
//	struct Value b = nextOp();
//	return Value.boolean(a.data.integer == b.data.integer);
	assert(0);
	abort();
}

struct Value in (const Instance * const ops, struct Ecc * const ecc)
{
//	struct Value a = nextOp();
//	struct Value b = nextOp();
//	return Value.boolean(a.data.integer == b.data.integer);
	assert(0);
	abort();
}

struct Value multiply (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.binary(Value.toBinary(a).data.binary * Value.toBinary(b).data.binary);
}

struct Value divide (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.binary(Value.toBinary(a).data.binary / Value.toBinary(b).data.binary);
}

struct Value modulo (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.binary(fmod(Value.toBinary(a).data.binary, Value.toBinary(b).data.binary));
}

struct Value add (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	const struct Text *text = opText();
	struct Value b = nextOp();
	return addition(ecc, a, text, b, opText());
}

struct Value minus (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.binary(Value.toBinary(a).data.binary - Value.toBinary(b).data.binary);
}

struct Value leftShift (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer(Value.toInteger(a).data.integer << (uint32_t)Value.toInteger(b).data.integer);
}

struct Value rightShift (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer(Value.toInteger(a).data.integer >> (uint32_t)Value.toInteger(b).data.integer);
}

struct Value unsignedRightShift (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer((uint32_t)Value.toInteger(a).data.integer >> (uint32_t)Value.toInteger(b).data.integer);
}

struct Value bitwiseAnd (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer(Value.toInteger(a).data.integer & Value.toInteger(b).data.integer);
}

struct Value bitwiseXor (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer(Value.toInteger(a).data.integer ^ Value.toInteger(b).data.integer);
}

struct Value bitwiseOr (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer(Value.toInteger(a).data.integer | Value.toInteger(b).data.integer);
}

struct Value logicalAnd (const Instance * const ops, struct Ecc * const ecc)
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

struct Value logicalOr (const Instance * const ops, struct Ecc * const ecc)
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

struct Value positive (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	return Value.toBinary(a);
}

struct Value negative (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	return Value.binary(-Value.toBinary(a).data.binary);
}

struct Value invert (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	return Value.integer(~Value.toInteger(a).data.integer);
}

struct Value not (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	return Value.boolean(!Value.isTrue(a));
}

// MARK: assignement

struct Value incrementRef (const Instance * const ops, struct Ecc * const ecc)
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

struct Value decrementRef (const Instance * const ops, struct Ecc * const ecc)
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

struct Value postIncrementRef (const Instance * const ops, struct Ecc * const ecc)
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

struct Value postDecrementRef (const Instance * const ops, struct Ecc * const ecc)
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

struct Value multiplyAssignRef (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	*ref = Value.toBinary(*ref);
	ref->data.binary *= Value.toBinary(b).data.binary;
	return *ref;
}

struct Value divideAssignRef (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	*ref = Value.toBinary(*ref);
	ref->data.binary /= Value.toBinary(b).data.binary;
	return *ref;
}

struct Value moduloAssignRef (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	*ref = Value.toBinary(*ref);
	ref->data.binary = fmod(ref->data.binary, Value.toBinary(b).data.binary);
	return *ref;
}

struct Value addAssignRef (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value *a = nextOp().data.reference;
	const struct Text *text = opText();
	struct Value b = nextOp();
	*a = addition(ecc, *a, text, b, opText());
	return *a;
}

struct Value minusAssignRef (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	*ref = Value.toBinary(*ref);
	ref->data.binary -= Value.toBinary(b).data.binary;
	return *ref;
}

struct Value leftShiftAssignRef (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	*ref = Value.toInteger(*ref);
	ref->data.integer <<= (uint32_t)Value.toInteger(b).data.integer;
	return *ref;
}

struct Value rightShiftAssignRef (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	*ref = Value.toInteger(*ref);
	ref->data.integer >>= (uint32_t)Value.toInteger(b).data.integer;
	return *ref;
}

struct Value unsignedRightShiftAssignRef (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	*ref = Value.toInteger(*ref);
	uint32_t uint = ref->data.integer;
	uint >>= (uint32_t)Value.toInteger(b).data.integer;
	ref->data.integer = uint;
	return *ref;
}

struct Value bitAndAssignRef (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	*ref = Value.toInteger(*ref);
	ref->data.integer &= Value.toInteger(b).data.integer;
	return *ref;
}

struct Value bitXorAssignRef (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	*ref = Value.toInteger(*ref);
	ref->data.integer ^= Value.toInteger(b).data.integer;
	return *ref;
}

struct Value bitOrAssignRef (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	*ref = Value.toInteger(*ref);
	ref->data.integer |= Value.toInteger(b).data.integer;
	return *ref;
}


// MARK: Statement

struct Value try(const Instance * const ops, struct Ecc * const ecc)
{
	const struct Op *end = *ops + opValue().data.integer;
	struct Value value = Value.undefined();
	struct Object *context = Object.create(ecc->context);
	struct Identifier identifier = Identifier.none();
	
	const struct Op *rethrowOps = NULL;
	int rethrow = 0;
	
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
			identifier = nextOp().data.identifier;
			
			if (!Identifier.isEqual(identifier, Identifier.none()))
			{
				Object.add(context, identifier, ecc->result, 0);
				ecc->result = Value.undefined();
				value = nextOp(); // execute until noop
				rethrow = 0;
			}
		}
	}
	
	Ecc.popEnv(ecc);
	
	ecc->context = context->prototype;
	
	*ops = end; // op[end] = Op.jump, to after catch
	struct Value finallyValue = nextOp(); // jump to after catch, and execute until noop
	
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

struct Value throw (const Instance * const ops, struct Ecc * const ecc)
{
	const struct Op *throwOps = *ops;
	struct Value value = nextOp();
	*ops = throwOps + 1;
	Ecc.jmpEnv(ecc, value);
}

struct Value debug (const Instance * const ops, struct Ecc * const ecc)
{
	__asm__("int3");
	return nextOp();
}

struct Value next (const Instance * const ops, struct Ecc * const ecc)
{
	return nextOp();
}

struct Value nextIf (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value value = opValue();
	if (!Value.isTrue(nextOp()))
		return value;
	
	return nextOp();
}

struct Value expression (const Instance * const ops, struct Ecc * const ecc)
{
	ecc->result = nextOp();
	return nextOp();
}

struct Value discard (const Instance * const ops, struct Ecc * const ecc)
{
	nextOp();
	return nextOp();
}

struct Value jump (const Instance * const ops, struct Ecc * const ecc)
{
	int32_t offset = opValue().data.integer;
	*ops += offset;
	return nextOp();
}

struct Value jumpIf (const Instance * const ops, struct Ecc * const ecc)
{
	int32_t offset = opValue().data.integer;
	struct Value value = nextOp();
	if (Value.isTrue(value))
		*ops += offset;
	
	return nextOp();
}

struct Value jumpIfNot (const Instance * const ops, struct Ecc * const ecc)
{
	int32_t offset = opValue().data.integer;
	struct Value value = nextOp();
	if (!Value.isTrue(value))
		*ops += offset;
	
	return nextOp();
}

struct Value switchOp (const Instance * const ops, struct Ecc * const ecc)
{
	const struct Op *nextOps = *ops + opValue().data.integer;
	struct Value a = nextOp(), b;
	const struct Text *text = opText();
	
	while (*ops < nextOps)
	{
		b = nextOp();
		if (Value.isTrue(equality(ecc, a, text, b, opText())))
		{
			*ops = nextOps + nextOp().data.integer + 1;
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

struct Value iterate (const Instance * const ops, struct Ecc * const ecc)
{
	const Instance startOps = *ops;
	const Instance endOps = startOps;
	const Instance nextOps = startOps + 1;
	struct Value value;
	
	*ops += opValue().data.integer;
	
	while (Value.isTrue(nextOp()))
		stepIteration(value, nextOps);
	
	*ops = endOps;
	return nextOp();
}

static struct Value iterateIntegerRef (
	const Instance * const ops,
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

struct Value iterateLessRef (const Instance * const ops, struct Ecc * const ecc)
{
	return iterateIntegerRef(ops, ecc, integerLess, integerWontOverflowPositive, valueLess, addition);
}

struct Value iterateLessOrEqualRef (const Instance * const ops, struct Ecc * const ecc)
{
	return iterateIntegerRef(ops, ecc, integerLessOrEqual, integerWontOverflowPositive, valueLessOrEqual, addition);
}

struct Value iterateMoreRef (const Instance * const ops, struct Ecc * const ecc)
{
	return iterateIntegerRef(ops, ecc, integerMore, integerWontOverflowNegative, valueMore, subtraction);
}

struct Value iterateMoreOrEqualRef (const Instance * const ops, struct Ecc * const ecc)
{
	return iterateIntegerRef(ops, ecc, integerMoreOrEqual, integerWontOverflowNegative, valueMoreOrEqual, subtraction);
}

struct Value iterateInRef (const Instance * const ops, struct Ecc * const ecc)
{
	struct Value *ref = nextOp().data.reference;
	struct Value object = nextOp();
	struct Value value = nextOp();
	const Instance startOps = *ops;
	const Instance endOps = startOps + value.data.integer;
	uint_fast32_t index;
	
	for (index = 0; index < object.data.object->elementCount; ++index)
	{
		if (object.data.object->element[index].data.value.type == Value(undefined))
			continue;
		
		*ref = Value.chars(Chars.create("%d", index));
		
		stepIteration(value, startOps);
	}
	
	for (index = 2; index <= object.data.object->hashmapCount; ++index)
	{
		if (!(object.data.object->hashmap[index].data.flags & Object(isValue)) || object.data.object->hashmap[index].data.value.type == Value(undefined))
			continue;
		
		*ref = Value.identifier(object.data.object->hashmap[index].data.identifier);
		
		stepIteration(value, startOps);
	}
	
	*ops = endOps;
	return nextOp();
}
