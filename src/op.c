//
//  op.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "op.h"

// MARK: - Private

#define nextOp() (++context->ops)->native(context)
#define opValue() (context->ops)->value
#define opText(O) &(context->ops + O)->text

//

static struct Value retain(struct Value value)
{
	if (value.type == Value(charsType))
		++value.data.chars->referenceCount;
	if (value.type >= Value(objectType))
		++value.data.object->referenceCount;
	
	return value;
}

static struct Value release(struct Value value)
{
	if (value.type == Value(charsType))
		--value.data.chars->referenceCount;
	if (value.type >= Value(objectType))
		--value.data.object->referenceCount;
	
	return value;
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

static void multiplyBinary (double *a, double b)
{
	*a *= b;
}

static void divideBinary (double *a, double b)
{
	*a /= b;
}

static void moduloBinary (double *a, double b)
{
	*a = fmod(*a, b);
}

static void addBinary (double *a, double b)
{
	*a += b;
}

static void minusBinary (double *a, double b)
{
	*a -= b;
}

static void leftShiftInteger (int32_t *a, int32_t b)
{
	*a <<= (uint32_t)b;
}

static void rightShiftInteger (int32_t *a, int32_t b)
{
	*a >>= (uint32_t)b;
}

static void unsignedRightShiftInteger (int32_t *a, int32_t b)
{
	*a = (uint32_t)*a >> (uint32_t)b;
}

static void bitAndInteger (int32_t *a, int32_t b)
{
	*a &= b;
}

static void bitXorInteger (int32_t *a, int32_t b)
{
	*a ^= b;
}

static void bitOrInteger (int32_t *a, int32_t b)
{
	*a |= b;
}

static double incrementBinary (double *a)
{
	return ++(*a);
}

static double decrementBinary (double *a)
{
	return --(*a);
}

static double postIncrementBinary (double *a)
{
	return (*a)++;
}

static double postDecrementBinary (double *a)
{
	return (*a)--;
}

// MARK: - Static Members

// MARK: - Methods

struct Op make (const Native(Function) native, struct Value value, struct Text text)
{
	return (struct Op){ native, value, text };
}

const char * toChars (const Native(Function) native)
{
	#define _(X) { #X, X, },
	struct {
		const char *name;
		const Native(Function) native;
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

// MARK: call

static inline struct Value callOps (struct Native(Context) * const context, struct Object *environment)
{
	if (context->depth >= context->ecc->maximumCallDepth)
		Ecc.jmpEnv(context->ecc, Value.error(Error.rangeError(context->parent->ops->text, "maximum depth exceeded")));
	
	context->environment = environment;
	return context->ops->native(context);
}

static inline struct Value callOpsRelease (struct Native(Context) * const context, struct Object *environment)
{
	struct Value result;
	uint16_t index, count;
	
	result = callOps(context, environment);
	
	for (index = 2, count = environment->hashmapCount; index < count; ++index)
		release(environment->hashmap[index].data.value);
	
	return result;
}

static inline void populateEnvironmentWithArguments (struct Object *environment, struct Object *arguments, int parameterCount)
{
	uint32_t index = 0;
	int argumentCount = arguments->elementCount;
	
	environment->hashmap[2].data.value = Value.object(arguments);
	
	if (argumentCount <= parameterCount)
		for (; index < argumentCount; ++index)
			environment->hashmap[index + 3].data.value = retain(arguments->element[index].data.value);
	else
	{
		for (; index < parameterCount; ++index)
			environment->hashmap[index + 3].data.value = retain(arguments->element[index].data.value);
	}
}

static inline void populateEnvironmentWithArgumentsVA (struct Object *environment, int parameterCount, int argumentCount, va_list ap)
{
	uint32_t index = 0;
	
	struct Object *arguments = Arguments.createSized(argumentCount);
	environment->hashmap[2].data.value = Value.object(arguments);
	
	if (argumentCount <= parameterCount)
		for (; index < argumentCount; ++index)
			environment->hashmap[index + 3].data.value = arguments->element[index].data.value = retain(va_arg(ap, struct Value));
	else
	{
		for (; index < parameterCount; ++index)
			environment->hashmap[index + 3].data.value = arguments->element[index].data.value = retain(va_arg(ap, struct Value));
		
		for (; index < argumentCount; ++index)
			arguments->element[index].data.value = va_arg(ap, struct Value);
	}
}

static inline void populateEnvironmentWithArgumentsOps (struct Native(Context) * const context, struct Object *environment, struct Object *arguments, int parameterCount, int argumentCount)
{
	uint32_t index = 0;
	
	environment->hashmap[2].data.value = Value.object(arguments);
	
	if (argumentCount <= parameterCount)
		for (; index < argumentCount; ++index)
			environment->hashmap[index + 3].data.value = arguments->element[index].data.value = retain(nextOp());
	else
	{
		for (; index < parameterCount; ++index)
			environment->hashmap[index + 3].data.value = arguments->element[index].data.value = retain(nextOp());
		
		for (; index < argumentCount; ++index)
			arguments->element[index].data.value = nextOp();
	}
}

static inline void populateEnvironmentVA (struct Object *environment, int parameterCount, int argumentCount, va_list ap)
{
	uint32_t index = 0;
	if (argumentCount <= parameterCount)
		for (; index < argumentCount; ++index)
			environment->hashmap[index + 3].data.value = retain(va_arg(ap, struct Value));
	else
	{
		for (; index < parameterCount; ++index)
			environment->hashmap[index + 3].data.value = retain(va_arg(ap, struct Value));
		
		for (; index < argumentCount; ++index)
			va_arg(ap, struct Value);
	}
}

static inline void populateEnvironment (struct Native(Context) * const context, struct Object *environment, int parameterCount, int argumentCount)
{
	uint32_t index = 0;
	if (argumentCount <= parameterCount)
		for (; index < argumentCount; ++index)
			environment->hashmap[index + 3].data.value = retain(nextOp());
	else
	{
		for (; index < parameterCount; ++index)
			environment->hashmap[index + 3].data.value = retain(nextOp());
		
		for (; index < argumentCount; ++index)
			nextOp();
	}
}

struct Value callFunctionArguments (struct Native(Context) * const context, int argumentOffset, struct Function *function, struct Value this, struct Object *arguments)
{
	struct Native(Context) subContext = {
		.ops = function->oplist->ops,
		.parent = context,
		.this = this,
		.argumentOffset = argumentOffset,
		.ecc = context->ecc,
		.depth = context->depth + 1,
	};
	
	if (function->flags & Function(needHeap))
	{
		struct Object *environment = Object.copy(&function->environment);
		
		if (function->flags & Function(needArguments))
		{
			struct Object *copy = Arguments.createSized(arguments->elementCount);
			memcpy(copy->element, arguments->element, sizeof(*copy->element) * copy->elementCount);
			arguments = copy;
		}
		populateEnvironmentWithArguments(environment, arguments, function->parameterCount);
		
		return callOps(&subContext, environment);
	}
	else
	{
		struct Object environment = function->environment;
		typeof(*function->environment.hashmap) hashmap[function->environment.hashmapCapacity];
		
		memcpy(hashmap, function->environment.hashmap, sizeof(hashmap));
		environment.hashmap = hashmap;
		populateEnvironmentWithArguments(&environment, arguments, function->parameterCount);
		
		return callOpsRelease(&subContext, &environment);
	}
}

struct Value callFunctionVA (struct Native(Context) * const context, int argumentOffset, struct Function *function, struct Value this, int argumentCount, ... )
{
	struct Native(Context) subContext = {
		.ops = function->oplist->ops,
		.parent = context,
		.this = this,
		.argumentOffset = argumentOffset,
		.ecc = context->ecc,
		.depth = context->depth + 1,
	};
	va_list ap;
	struct Value result;
	
	va_start(ap, argumentCount);
	
	if (function->flags & Function(needHeap))
	{
		struct Object *environment = Object.copy(&function->environment);
		
		if (function->flags & Function(needArguments))
			populateEnvironmentWithArgumentsVA(environment, function->parameterCount, argumentCount, ap);
		else
			populateEnvironmentVA(environment, function->parameterCount, argumentCount, ap);
		
		result = callOps(&subContext, environment);
	}
	else
	{
		struct Object environment = function->environment;
		typeof(*function->environment.hashmap) hashmap[function->environment.hashmapCapacity];
		
		memcpy(hashmap, function->environment.hashmap, sizeof(hashmap));
		environment.hashmap = hashmap;
		
		populateEnvironmentVA(&environment, function->parameterCount, argumentCount, ap);
		
		result = callOpsRelease(&subContext, &environment);
	}
	
	va_end(ap);
	
	return result;
}

static inline struct Value callFunction (struct Native(Context) * const context, struct Function * const function, struct Value this, int32_t argumentCount, int construct)
{
	struct Native(Context) subContext = {
		.ops = function->oplist->ops,
		.parent = context,
		.this = this,
		.construct = construct,
		.ecc = context->ecc,
		.depth = context->depth + 1,
	};
	
	if (function->flags & Function(needHeap))
	{
		struct Object *environment = Object.copy(&function->environment);
		
		if (function->flags & Function(needArguments))
			populateEnvironmentWithArgumentsOps(context, environment, Arguments.createSized(argumentCount), function->parameterCount, argumentCount);
		else
			populateEnvironment(context, environment, function->parameterCount, argumentCount);
		
		return callOps(&subContext, environment);
	}
	else if (function->flags & Function(needArguments))
	{
		struct Object environment = function->environment;
		struct Object arguments = Object.identity;
		typeof(*environment.hashmap) hashmap[function->environment.hashmapCapacity];
		typeof(*arguments.element) element[argumentCount];
		
		memcpy(hashmap, function->environment.hashmap, sizeof(hashmap));
		environment.hashmap = hashmap;
		arguments.element = element;
		arguments.elementCount = argumentCount;
		populateEnvironmentWithArgumentsOps(context, &environment, &arguments, function->parameterCount, argumentCount);
		
		return callOpsRelease(&subContext, &environment);
	}
	else
	{
		struct Object environment = function->environment;
		typeof(*environment.hashmap) hashmap[function->environment.hashmapCapacity];
		
		memcpy(hashmap, function->environment.hashmap, sizeof(hashmap));
		environment.hashmap = hashmap;
		populateEnvironment(context, &environment, function->parameterCount, argumentCount);
		
		return callOpsRelease(&subContext, &environment);
	}
}

static inline struct Value callValue (struct Native(Context) * const context, struct Value value, struct Value this, int32_t argumentCount, int construct, const struct Text *text)
{
	if (value.type != Value(functionType))
		Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(*text, "%.*s not a function", text->length, text->bytes)));
	
	return callFunction(context, value.data.function, this, argumentCount, construct);
}

struct Value construct (struct Native(Context) * const context)
{
	const struct Text *text = opText(1);
	int32_t argumentCount = opValue().data.integer;
	struct Value value, *prototype, object, function = nextOp();
	
	if (function.type != Value(functionType))
		goto error;
	
	prototype = Object.member(&function.data.function->object, Key(prototype), NULL);
	if (!prototype)
		goto error;
	
	if (Value.isObject(*prototype))
	{
		++prototype->data.object->referenceCount;
		object = Value.object(Object.create(prototype->data.object));
	}
	else
		object = Value.object(Object(prototype));
	
	value = callValue(context, function, object, argumentCount, 1, text);
	
	if (Value.isObject(value))
		return value;
	else
		return object;
	
error:
	Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(*text, "%.*s is not a constructor", text->length, text->bytes)));
}

struct Value call (struct Native(Context) * const context)
{
	const struct Text *text = opText(1);
	int32_t argumentCount = opValue().data.integer;
	struct Value value = nextOp();
	return callValue(context, value, Value(undefined), argumentCount, 0, text);
}

struct Value eval (struct Native(Context) * const context)
{
	struct Value value;
	struct Input *input;
	int32_t argumentCount = opValue().data.integer;
	struct Native(Context) subContext = {
		.parent = context,
		.this = context->this,
		.environment = context->environment,
		.ecc = context->ecc,
	};
	
	if (!argumentCount)
		return Value(undefined);
	
	value = Value.toString(context, nextOp());
	while (--argumentCount)
		nextOp();
	
	input = Input.createFromBytes(Value.stringBytes(value), Value.stringLength(value), "(eval)");
	Ecc.evalInputWithContext(context->ecc, input, &subContext);
	
	return context->ecc->result;
}


// Expression

struct Value noop (struct Native(Context) * const context)
{
	return Value(undefined);
}

struct Value value (struct Native(Context) * const context)
{
	return opValue();
}

struct Value valueConstRef (struct Native(Context) * const context)
{
	return Value.reference((struct Value *)&opValue());
}

struct Value text (struct Native(Context) * const context)
{
	return Value.text(opText(0));
}

struct Value function (struct Native(Context) * const context)
{
	struct Value value = opValue(), result;
	struct Function *function = Function.copy(value.data.function);
	function->environment.prototype = context->environment;
	++context->environment->referenceCount;
	result = Value.function(function);
	result.flags = value.flags;
	return result;
}

struct Value object (struct Native(Context) * const context)
{
	struct Object *object = Object.create(Object(prototype));
	struct Value value;
	uint32_t count;
	
	for (count = opValue().data.integer; count--;)
	{
		value = nextOp();
		
		if (value.type == Value(keyType))
			Object.addMember(object, value.data.key, retain(nextOp()), value.flags);
		else if (value.type == Value(integerType))
			Object.addElement(object, value.data.integer, retain(nextOp()), value.flags);
	}
	return Value.object(object);
}

struct Value array (struct Native(Context) * const context)
{
	uint32_t length = opValue().data.integer;
	struct Object *object = Array.createSized(length);
	struct Value value;
	uint32_t index;
	
	for (index = 0; index < length; ++index)
	{
		value = nextOp();
		if (value.check == 1)
		{
			object->element[index].data.value = retain(value);
			object->element[index].data.value.flags = 0;
		}
	}
	return Value.object(object);
}

struct Value this (struct Native(Context) * const context)
{
	return context->this;
}

struct Value getLocalRef (struct Native(Context) * const context)
{
	struct Key key = opValue().data.key;
	struct Value *ref = Object.member(context->environment, key, NULL);
	if (!ref)
		Ecc.jmpEnv(context->ecc, Value.error(Error.referenceError(context->ops->text, "%.*s is not defined", Key.textOf(key)->length, Key.textOf(key)->bytes)));
	
	return Value.reference(ref);
}

struct Value getLocal (struct Native(Context) * const context)
{
	return *getLocalRef(context).data.reference;
}

struct Value setLocal (struct Native(Context) * const context)
{
	struct Value *ref = getLocalRef(context).data.reference;
	struct Value value = nextOp();
	value.flags = 0;
	release(*ref);
	return *ref = value;
}

struct Value getLocalSlotRef (struct Native(Context) * const context)
{
	return Value.reference(&context->environment->hashmap[opValue().data.integer].data.value);
}

struct Value getLocalSlot (struct Native(Context) * const context)
{
	return context->environment->hashmap[opValue().data.integer].data.value;
}

struct Value setLocalSlot (struct Native(Context) * const context)
{
	int32_t slot = opValue().data.integer;
	struct Value value = nextOp();
	value.flags = 0;
	release(context->environment->hashmap[slot].data.value);
	return context->environment->hashmap[slot].data.value = retain(value);
}

struct Value getParentSlotRef (struct Native(Context) * const context)
{
	int32_t slot = opValue().data.integer & 0xffff;
	int32_t count = opValue().data.integer >> 16;
	struct Object *object = context->environment;
	while (count--)
		object = object->prototype;
	
	return Value.reference(&object->hashmap[slot].data.value);
}

struct Value getParentSlot (struct Native(Context) * const context)
{
	struct Value *ref = getParentSlotRef(context).data.reference;
	return *ref;
}

struct Value setParentSlot (struct Native(Context) * const context)
{
	struct Value *ref = getParentSlotRef(context).data.reference;
	struct Value value = nextOp();
	release(*ref);
	return *ref = retain(value);
}

struct Value getMemberRef (struct Native(Context) * const context)
{
	const struct Text *text = opText(0);
	struct Key key = opValue().data.key;
	struct Value object = nextOp();
	struct Value *ref;
	int own = 0;
	
	if (!Value.isObject(object))
		object = Value.toObject(context, object, Native(noIndex));
	
	context->refObject = object.data.object;
	ref = Object.member(object.data.object, key, &own);
	
	if (!ref || !own)
	{
		if (object.data.object->flags & Object(sealed))
			Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(*text, "object is not extensible")));
		
		ref = Object.addMember(object.data.object, key, Value(undefined), 0);
	}
	
	return Value.reference(ref);
}

struct Value getMember (struct Native(Context) * const context)
{
	struct Key key = opValue().data.key;
	struct Value object = nextOp();
	
	if (!Value.isObject(object))
		object = Value.toObject(context, object, Native(noIndex));
	
	return Object.getMember(object.data.object, key, context);
}

struct Value setMember (struct Native(Context) * const context)
{
	const struct Text *text = opText(0);
	struct Key key = opValue().data.key;
	struct Value object = nextOp();
	struct Value value = retain(nextOp());
	
	if (!Value.isObject(object))
		object = Value.toObject(context, object, Native(noIndex));
	
	Object.putMember(object.data.object, key, context, value, text);
	
	return value;
}

struct Value callMember (struct Native(Context) * const context)
{
	int32_t argumentCount = opValue().data.integer;
	const struct Text *text = &(++context->ops)->text;
	struct Key key = opValue().data.key;
	struct Value object = nextOp();
	
	if (!Value.isObject(object))
		object = Value.toObject(context, object, Native(noIndex));
	
	return callValue(context, Object.getMember(object.data.object, key, context), object, argumentCount, 0, text);
}

struct Value deleteMember (struct Native(Context) * const context)
{
	const struct Text *text = opText(0);
	struct Key key = opValue().data.key;
	struct Value object = Value.toObject(context, nextOp(), Native(noIndex));
	int result = Object.deleteMember(object.data.object, key);
	if (!result)
		Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(*text, "property '%.*s' is non-configurable and can't be deleted", Key.textOf(key)->length, Key.textOf(key)->bytes)));
	
	return Value.truth(result);
}

struct Value getPropertyRef (struct Native(Context) * const context)
{
	const struct Text *text = opText(1);
	struct Value object = nextOp();
	struct Value property = nextOp();
	struct Value *ref;
	int own = 0;
	
	if (Value.isObject(property))
		property = Value.toPrimitive(context, property, opText(0), Value(hintAuto));
	
	if (Value.isPrimitive(object))
		object = Value.toObject(context, object, Native(noIndex));
	
	context->refObject = object.data.object;
	ref = Object.property(object.data.object, property, &own);
	
	if (!ref || !own)
	{
		if (object.data.object->flags & Object(sealed))
			Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(*text, "object is not extensible")));
		
		ref = Object.addProperty(object.data.object, property, Value(undefined), 0);
	}
	
	return Value.reference(ref);
}

struct Value getProperty (struct Native(Context) * const context)
{
	struct Value object = nextOp();
	struct Value property = nextOp();
	
	if (Value.isObject(property))
		property = Value.toPrimitive(context, property, opText(0), Value(hintAuto));
	
	if (Value.isPrimitive(object))
		object = Value.toObject(context, object, Native(noIndex));
	
	return Object.getProperty(object.data.object, property, context);
}

struct Value setProperty (struct Native(Context) * const context)
{
	const struct Text *text = opText(0);
	struct Value object = nextOp();
	struct Value property = nextOp();
	struct Value value = retain(nextOp());
	
	if (Value.isObject(property))
		property = Value.toPrimitive(context, property, opText(0), Value(hintAuto));
	
	if (Value.isPrimitive(object))
		object = Value.toObject(context, object, Native(noIndex));
	
	Object.putProperty(object.data.object, property, context, value, text);
	
	return value;
}

struct Value callProperty (struct Native(Context) * const context)
{
	int32_t argumentCount = opValue().data.integer;
	const struct Text *text = &(++context->ops)->text;
	struct Value object = nextOp();
	struct Value property = nextOp();
	
	if (Value.isObject(property))
		property = Value.toPrimitive(context, property, opText(0), Value(hintAuto));
	
	if (Value.isPrimitive(object))
		object = Value.toObject(context, object, Native(noIndex));
	
	return callValue(context, Object.getProperty(object.data.object, property, context), object, argumentCount, 0, text);

}

struct Value deleteProperty (struct Native(Context) * const context)
{
	const struct Text *text = opText(0);
	struct Value object = nextOp();
	struct Value property =  nextOp();
	
	if (Value.isObject(property))
		property = Value.toPrimitive(context, property, opText(0), Value(hintAuto));
	
	if (Value.isPrimitive(object))
		object = Value.toObject(context, object, Native(noIndex));
	
	int result = Object.deleteProperty(object.data.object, property);
	if (!result)
	{
		struct Value string = Value.toString(NULL, property);
		Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(*text, "property '%.*s' can't be deleted", Value.stringLength(string), Value.stringBytes(string))));
	}
	return Value.truth(result);
}

struct Value result (struct Native(Context) * const context)
{
	struct Value result = nextOp();
	context->breaker = -1;
	return result;
}

struct Value resultValue (struct Native(Context) * const context)
{
	struct Value result = opValue();
	context->breaker = -1;
	return result;
}

struct Value pushEnvironment (struct Native(Context) * const context)
{
	context->environment = Object.create(context->environment);
	return opValue();
}

struct Value popEnvironment (struct Native(Context) * const context)
{
	context->environment = context->environment->prototype;
	return opValue();
}

struct Value exchange (struct Native(Context) * const context)
{
	struct Value value = opValue();
	nextOp();
	return value;
}

struct Value typeOf (struct Native(Context) * const context)
{
	struct Value value = nextOp();
	return Value.toType(value);
}

struct Value equal (struct Native(Context) * const context)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	return Value.equals(context, a, b, text, opText(0));
}

struct Value notEqual (struct Native(Context) * const context)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	return Value.truth(!Value.isTrue(Value.equals(context, a, b, text, opText(0))));
}

struct Value identical (struct Native(Context) * const context)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	return Value.same(context, a, b, text, opText(0));
}

struct Value notIdentical (struct Native(Context) * const context)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	return Value.truth(!Value.isTrue(Value.same(context, a, b, text, opText(0))));
}

struct Value less (struct Native(Context) * const context)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	if (a.type == Value(binaryType) && b.type == Value(binaryType))
		return Value.truth(a.data.binary < b.data.binary);
	else
		return Value.less(context, a, b, text, opText(0));
}

struct Value lessOrEqual (struct Native(Context) * const context)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	if (a.type == Value(binaryType) && b.type == Value(binaryType))
		return Value.truth(a.data.binary <= b.data.binary);
	else
		return Value.lessOrEqual(context, a, b, text, opText(0));
}

struct Value more (struct Native(Context) * const context)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	if (a.type == Value(binaryType) && b.type == Value(binaryType))
		return Value.truth(a.data.binary > b.data.binary);
	else
		return Value.more(context, a, b, text, opText(0));
}

struct Value moreOrEqual (struct Native(Context) * const context)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	if (a.type == Value(binaryType) && b.type == Value(binaryType))
		return Value.truth(a.data.binary >= b.data.binary);
	else
		return Value.moreOrEqual(context, a, b, text, opText(0));
}

struct Value instanceOf (struct Native(Context) * const context)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	struct Object *object;
	
	if (!Value.isObject(a))
		return Value(false);
	
	if (b.type != Value(functionType))
		Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(*text, "%.*s not a function", text->length, text->bytes)));
	
	b = Object.getMember(b.data.object, Key(prototype), context);
	if (!Value.isObject(b))
		Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(*text, "%.*s.prototype not an object", text->length, text->bytes)));
	
	object = a.data.object;
	while (( object = object->prototype ))
		if (object == b.data.object)
			return Value(true);
	
	return Value(false);
}

struct Value in (struct Native(Context) * const context)
{
	struct Value property = nextOp();
	struct Value object = nextOp();
	struct Value *ref;
	
	if (!Value.isObject(object))
		Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(context->ops->text, "invalid 'in' operand %.*s", context->ops->text.length, context->ops->text.bytes)));
	
	ref = Object.property(object.data.object, property, NULL);
	
	return Value.truth(ref != NULL);
}

struct Value multiply (struct Native(Context) * const context)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	if (a.type == Value(binaryType) && b.type == Value(binaryType))
	{
		a.data.binary *= b.data.binary;
		return a;
	}
	else
		return Value.binary(Value.toBinary(a).data.binary * Value.toBinary(b).data.binary);
}

struct Value divide (struct Native(Context) * const context)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	if (a.type == Value(binaryType) && b.type == Value(binaryType))
	{
		a.data.binary /= b.data.binary;
		return a;
	}
	else
		return Value.binary(Value.toBinary(a).data.binary / Value.toBinary(b).data.binary);
}

struct Value modulo (struct Native(Context) * const context)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	if (a.type == Value(binaryType) && b.type == Value(binaryType))
		return Value.binary(fmod(a.data.binary, b.data.binary));
	else
		return Value.binary(fmod(Value.toBinary(a).data.binary, Value.toBinary(b).data.binary));
}

struct Value add (struct Native(Context) * const context)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	if (a.type == Value(binaryType) && b.type == Value(binaryType))
	{
		a.data.binary += b.data.binary;
		return a;
	}
	else
		return Value.add(context, a, b, text, opText(0));
}

struct Value minus (struct Native(Context) * const context)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	if (a.type == Value(binaryType) && b.type == Value(binaryType))
	{
		a.data.binary -= b.data.binary;
		return a;
	}
	else
		return Value.binary(Value.toBinary(a).data.binary - Value.toBinary(b).data.binary);
}

struct Value leftShift (struct Native(Context) * const context)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer(Value.toInteger(a).data.integer << (uint32_t)Value.toInteger(b).data.integer);
}

struct Value rightShift (struct Native(Context) * const context)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer(Value.toInteger(a).data.integer >> (uint32_t)Value.toInteger(b).data.integer);
}

struct Value unsignedRightShift (struct Native(Context) * const context)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer((uint32_t)Value.toInteger(a).data.integer >> (uint32_t)Value.toInteger(b).data.integer);
}

struct Value bitwiseAnd (struct Native(Context) * const context)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer(Value.toInteger(a).data.integer & Value.toInteger(b).data.integer);
}

struct Value bitwiseXor (struct Native(Context) * const context)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer(Value.toInteger(a).data.integer ^ Value.toInteger(b).data.integer);
}

struct Value bitwiseOr (struct Native(Context) * const context)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer(Value.toInteger(a).data.integer | Value.toInteger(b).data.integer);
}

struct Value logicalAnd (struct Native(Context) * const context)
{
	const int32_t opCount = opValue().data.integer;
	if (!Value.isTrue(nextOp()))
	{
		context->ops += opCount;
		return Value(false);
	}
	else
		return Value.truth(Value.isTrue(nextOp()));
}

struct Value logicalOr (struct Native(Context) * const context)
{
	const int32_t opCount = opValue().data.integer;
	if (Value.isTrue(nextOp()))
	{
		context->ops += opCount;
		return Value(true);
	}
	else
		return Value.truth(Value.isTrue(nextOp()));
}

struct Value positive (struct Native(Context) * const context)
{
	struct Value a = nextOp();
	if (a.type == Value(binaryType))
		return a;
	else
		return Value.toBinary(a);
}

struct Value negative (struct Native(Context) * const context)
{
	struct Value a = nextOp();
	if (a.type == Value(binaryType))
		return Value.binary(-a.data.binary);
	else
		return Value.binary(-Value.toBinary(a).data.binary);
}

struct Value invert (struct Native(Context) * const context)
{
	struct Value a = nextOp();
	return Value.integer(~Value.toInteger(a).data.integer);
}

struct Value not (struct Native(Context) * const context)
{
	struct Value a = nextOp();
	return Value.truth(!Value.isTrue(a));
}

// MARK: assignement

static struct Value changeBinaryRef (struct Native(Context) * const context, double (*operationBinary)(double *))
{
	const struct Text *text = opText(1);
	struct Value *ref = nextOp().data.reference;
	
	if (ref->type != Value(binaryType) || ref->flags & Value(readonly))
	{
		struct Value value = Value.toBinary(Object.getValue(context->refObject, ref, context));
		double result = operationBinary(&value.data.binary);
		Object.putValue(context->refObject, ref, context, value, text);
		return Value.binary(result);
	}
	return Value.binary(operationBinary(&ref->data.binary));
}

struct Value incrementRef (struct Native(Context) * const context)
{
	return changeBinaryRef(context, incrementBinary);
}

struct Value decrementRef (struct Native(Context) * const context)
{
	return changeBinaryRef(context, decrementBinary);
}

struct Value postIncrementRef (struct Native(Context) * const context)
{
	return changeBinaryRef(context, postIncrementBinary);
}

struct Value postDecrementRef (struct Native(Context) * const context)
{
	return changeBinaryRef(context, postDecrementBinary);
}

//

static struct Value operationAny (struct Native(Context) * const context, void (*operationBinary)(double *, double), struct Value *a, struct Value b, const struct Text *text)
{
	b = Value.toBinary(b);
	
	struct Value value = Value.toBinary(Object.getValue(context->refObject, a, context));
	operationBinary(&value.data.binary, b.data.binary);
	return *Object.putValue(context->refObject, a, context, value, text);
}

static struct Value addAny (struct Native(Context) * const context, void (*operationBinary)(double *, double), struct Value *a, struct Value b, const struct Text *text)
{
	struct Value value = Value.add(context, Object.getValue(context->refObject, a, context), b, text, opText(0));
	return *Object.putValue(context->refObject, a, context, retain(value), text);
}

static struct Value assignBinaryRef (struct Native(Context) * const context, void (*operationBinary)(double *, double), typeof (operationAny) operationAny)
{
	const struct Text *text = opText(1);
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	
	if (ref->type != Value(binaryType) || b.type != Value(binaryType) || ref->flags & Value(readonly))
		return operationAny(context, operationBinary, ref, b, text);
	
	operationBinary(&ref->data.binary, b.data.binary);
	return *ref;
}

struct Value multiplyAssignRef (struct Native(Context) * const context)
{
	return assignBinaryRef(context, multiplyBinary, operationAny);
}

struct Value divideAssignRef (struct Native(Context) * const context)
{
	return assignBinaryRef(context, divideBinary, operationAny);
}

struct Value moduloAssignRef (struct Native(Context) * const context)
{
	return assignBinaryRef(context, moduloBinary, operationAny);
}

struct Value addAssignRef (struct Native(Context) * const context)
{
	return assignBinaryRef(context, addBinary, addAny);
}

struct Value minusAssignRef (struct Native(Context) * const context)
{
	return assignBinaryRef(context, minusBinary, operationAny);
}

//

static struct Value assignIntegerRef (struct Native(Context) * const context, void (*operationInteger)(int32_t *, int32_t))
{
	const struct Text *text = opText(1);
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	
	if (b.type != Value(integerType))
		b = Value.toInteger(b);
	
	if (ref->type != Value(integerType) || ref->flags & Value(readonly))
	{
		struct Value value = Value.toInteger(Object.getValue(context->refObject, ref, context));
		operationInteger(&value.data.integer, b.data.integer);
		return *Object.putValue(context->refObject, ref, context, value, text);
	}
	operationInteger(&ref->data.integer, b.data.integer);
	return *ref;
}

struct Value leftShiftAssignRef (struct Native(Context) * const context)
{
	return assignIntegerRef(context, leftShiftInteger);
}

struct Value rightShiftAssignRef (struct Native(Context) * const context)
{
	return assignIntegerRef(context, rightShiftInteger);
}

struct Value unsignedRightShiftAssignRef (struct Native(Context) * const context)
{
	return assignIntegerRef(context, unsignedRightShiftInteger);
}

struct Value bitAndAssignRef (struct Native(Context) * const context)
{
	return assignIntegerRef(context, bitAndInteger);
}

struct Value bitXorAssignRef (struct Native(Context) * const context)
{
	return assignIntegerRef(context, bitXorInteger);
}

struct Value bitOrAssignRef (struct Native(Context) * const context)
{
	return assignIntegerRef(context, bitOrInteger);
}


// MARK: Statement

struct Value try (struct Native(Context) * const context)
{
	const struct Op *end = context->ops + opValue().data.integer;
	struct Key key;
	
	const struct Op * volatile rethrowOps = NULL;
	volatile int rethrow = 0, breaker = 0;
	volatile struct Value value = Value(undefined);
	struct Value finallyValue;
	
	if (!setjmp(*Ecc.pushEnv(context->ecc))) // try
		value = nextOp();
	else
	{
		value = context->ecc->result;
		rethrowOps = context->ops;
		
		if (!rethrow) // catch
		{
			rethrow = 1;
			context->ops = end + 1; // bypass catch jump
			key = nextOp().data.key;
			
			if (!Key.isEqual(key, Key(none)))
			{
				Object.addMember(context->environment, key, value, 0);
				value = nextOp(); // execute until noop
				rethrow = 0;
			}
		}
	}
	
	Ecc.popEnv(context->ecc);
	
	breaker = context->breaker;
	context->breaker = 0;
	context->ops = end; // op[end] = Op.jump, to after catch
	finallyValue = nextOp(); // jump to after catch, and execute until noop
	
	if (context->breaker) /* return breaker */
		return finallyValue;
	else if (rethrow)
	{
		context->ops = rethrowOps;
		Ecc.jmpEnv(context->ecc, retain(value));
	}
	else if (breaker)
	{
		context->breaker = breaker;
		return value;
	}
	else
		return nextOp();
}

noreturn
struct Value throw (struct Native(Context) * const context)
{
	context->ecc->text = *opText(1);
	Ecc.jmpEnv(context->ecc, retain(nextOp()));
}

struct Value debug (struct Native(Context) * const context)
{
	__asm__("int3");
	return nextOp();
}

struct Value next (struct Native(Context) * const context)
{
	return nextOp();
}

struct Value nextIf (struct Native(Context) * const context)
{
	struct Value value = opValue();
	if (!Value.isTrue(nextOp()))
		return value;
	
	return nextOp();
}

struct Value expression (struct Native(Context) * const context)
{
	release(context->ecc->result);
	context->ecc->result = retain(nextOp());
	return nextOp();
}

struct Value discard (struct Native(Context) * const context)
{
	nextOp();
	return nextOp();
}

struct Value discardN (struct Native(Context) * const context)
{
	switch (opValue().data.integer)
	{
		default:
			assert(0);
			abort();
		
		case 16:
			nextOp();
		case 15:
			nextOp();
		case 14:
			nextOp();
		case 13:
			nextOp();
		case 12:
			nextOp();
		case 11:
			nextOp();
		case 10:
			nextOp();
		case 9:
			nextOp();
		case 8:
			nextOp();
		case 7:
			nextOp();
		case 6:
			nextOp();
		case 5:
			nextOp();
		case 4:
			nextOp();
		case 3:
			nextOp();
		case 2:
			nextOp();
		case 1:
			nextOp();
	}
	return nextOp();
}

struct Value jump (struct Native(Context) * const context)
{
	int32_t offset = opValue().data.integer;
	context->ops += offset;
	return nextOp();
}

struct Value jumpIf (struct Native(Context) * const context)
{
	int32_t offset = opValue().data.integer;
	struct Value value = nextOp();
	if (Value.isTrue(value))
		context->ops += offset;
	
	return nextOp();
}

struct Value jumpIfNot (struct Native(Context) * const context)
{
	int32_t offset = opValue().data.integer;
	struct Value value = nextOp();
	if (!Value.isTrue(value))
		context->ops += offset;
	
	return nextOp();
}

struct Value switchOp (struct Native(Context) * const context)
{
	int32_t offset = opValue().data.integer;
	const struct Op *nextOps = context->ops + offset;
	struct Value a = nextOp(), b;
	const struct Text *text = opText(0);
	
	while (context->ops < nextOps)
	{
		b = nextOp();
		if (Value.isTrue(Value.equals(context, a, b, text, opText(0))))
		{
			nextOps += nextOp().data.integer + 1;
			context->ops = nextOps;
			break;
		}
		else
			++context->ops;
	}
	
	return nextOp();
}

// MARK: Iteration

#define stepIteration(value, nextOps) \
	{ \
		value = nextOp(); \
		if (context->breaker && --context->breaker) \
		{ \
			if (--context->breaker) \
				return value; /* return breaker */\
			else \
				break; \
		} \
		else \
			context->ops = nextOps; \
		 \
		Pool.collectUnreferencedFromIndices(counts); \
	}

struct Value breaker (struct Native(Context) * const context)
{
	context->breaker = opValue().data.integer;
	return Value(undefined);
}

struct Value iterate (struct Native(Context) * const context)
{
	const struct Op *startOps = context->ops;
	const struct Op *endOps = startOps;
	const struct Op *nextOps = startOps + 1;
	struct Value value;
	uint32_t counts[3];
	
	context->ops += opValue().data.integer;
	
	Pool.getCounts(counts);
	
	while (Value.isTrue(nextOp()))
		stepIteration(value, nextOps);
	
	context->ops = endOps;
	return nextOp();
}

static struct Value iterateIntegerRef (
	struct Native(Context) * const context,
	int (*compareInteger) (int32_t, int32_t),
	typeof(integerWontOverflowPositive) wontOverflow,
	typeof(Value.compare) compareValue,
	typeof(Value.add) valueStep)
{
	const struct Op *endOps = context->ops + opValue().data.integer;
	const struct Text *stepText = opText(0);
	struct Value stepValue = nextOp();
	const struct Text *indexText = opText(0);
	struct Value *indexRef = nextOp().data.reference;
	const struct Text *countText = opText(0);
	struct Value *countRef = nextOp().data.reference;
	const struct Op *nextOps = context->ops;
	struct Value value;
	uint32_t counts[3];
	
	Pool.getCounts(counts);
	
	if (indexRef->type == Value(binaryType) && indexRef->data.binary >= INT32_MIN && indexRef->data.binary <= INT32_MAX)
	{
		struct Value integerValue = Value.toInteger(*indexRef);
		if (indexRef->data.binary == integerValue.data.integer)
			*indexRef = integerValue;
	}
	
	if (countRef->type == Value(binaryType) && countRef->data.binary >= INT32_MIN && countRef->data.binary <= INT32_MAX)
	{
		struct Value integerValue = Value.toInteger(*countRef);
		if (countRef->data.binary == integerValue.data.integer)
			*countRef = integerValue;
	}
	
	if (indexRef->type == Value(integerType) && countRef->type == Value(integerType))
	{
		int32_t step = valueStep == Value.add? stepValue.data.integer: -stepValue.data.integer;
		if (!wontOverflow(countRef->data.integer, step - 1))
			goto deoptimize;
		
		for (; compareInteger(indexRef->data.integer, countRef->data.integer); indexRef->data.integer += step)
			stepIteration(value, nextOps);
		
		goto done;
	}
	
deoptimize:
	for (; Value.isTrue(compareValue(context, *indexRef, *countRef, indexText, countText))
		 ; *indexRef = valueStep(context, *indexRef, stepValue, indexText, stepText)
		 )
		stepIteration(value, nextOps);
	
done:
	context->ops = endOps;
	return nextOp();
}

struct Value iterateLessRef (struct Native(Context) * const context)
{
	return iterateIntegerRef(context, integerLess, integerWontOverflowPositive, Value.less, Value.add);
}

struct Value iterateLessOrEqualRef (struct Native(Context) * const context)
{
	return iterateIntegerRef(context, integerLessOrEqual, integerWontOverflowPositive, Value.lessOrEqual, Value.add);
}

struct Value iterateMoreRef (struct Native(Context) * const context)
{
	return iterateIntegerRef(context, integerMore, integerWontOverflowNegative, Value.more, Value.subtract);
}

struct Value iterateMoreOrEqualRef (struct Native(Context) * const context)
{
	return iterateIntegerRef(context, integerMoreOrEqual, integerWontOverflowNegative, Value.moreOrEqual, Value.subtract);
}

struct Value iterateInRef (struct Native(Context) * const context)
{
	struct Value *ref = nextOp().data.reference;
	enum Value(Flags) flags = ref->flags;
	struct Value object = nextOp();
	struct Value value = nextOp();
	const struct Op *startOps = context->ops;
	const struct Op *endOps = startOps + value.data.integer;
	uint32_t counts[3];
	
	Pool.getCounts(counts);
	
	if (Value.isObject(object))
	{
		uint32_t index;
		
		for (index = 0; index < object.data.object->elementCount; ++index)
		{
			if (!(object.data.object->element[index].data.value.check == 1))
				continue;
			
			*ref = Value.chars(Chars.create("%d", index));
			ref->flags = flags;
			
			stepIteration(value, startOps);
		}
		
		for (index = 2; index < object.data.object->hashmapCount; ++index)
		{
			if (!(object.data.object->hashmap[index].data.value.check == 1))
				continue;
			
			*ref = Value.key(object.data.object->hashmap[index].data.key);
			ref->flags = flags;
			
			stepIteration(value, startOps);
		}
	}
	
	context->ops = endOps;
	return nextOp();
}
