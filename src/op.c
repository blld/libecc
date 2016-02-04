//
//  op.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "op.h"

// MARK: - Private

#define nextOp() (++context->ops)->native(context, ecc)
#define opValue() (context->ops)->value
#define opText(O) &(context->ops + O)->text

//

static struct Value addition (struct Ecc * const ecc, struct Value a, const struct Text *aText, struct Value b, const struct Text *bText)
{
	if (a.type == Value(binaryType) && b.type == Value(binaryType))
		return Value.binary(a.data.binary + b.data.binary);
	else if (a.type == Value(integerType) && b.type == Value(integerType))
		return Value.binary((double)a.data.integer + (double)b.data.integer);
	else if (Value.isNumber(a) && Value.isNumber(b))
		return Value.binary(Value.toBinary(a).data.binary + Value.toBinary(b).data.binary);
	else
	{
		a = Value.toPrimitive(NULL, ecc, a, aText, Value(hintAuto));
		b = Value.toPrimitive(NULL, ecc, b, bText, Value(hintAuto));
		
		if (Value.isString(a) || Value.isString(b))
		{
			uint16_t lengthA = Value.toBufferLength(a);
			uint16_t lengthB = Value.toBufferLength(b);
			struct Chars *chars = Chars.createSized(lengthA + lengthB);
			Value.toBuffer(a, chars->chars, chars->length + 1);
			Value.toBuffer(b, chars->chars + lengthA, chars->length - lengthA + 1);
			return Value.chars(chars);
		}
		else
			return Value.binary(Value.toBinary(a).data.binary + Value.toBinary(b).data.binary);
	}
}

static struct Value subtraction (struct Ecc * const ecc, struct Value a, const struct Text *aText, struct Value b, const struct Text *bText)
{
	if (a.type == Value(binaryType) && b.type == Value(binaryType))
		return Value.binary(a.data.binary - b.data.binary);
	else if (a.type == Value(integerType) && b.type == Value(integerType))
		return Value.binary(a.data.integer - b.data.integer);
	
	return Value.binary(Value.toBinary(a).data.binary - Value.toBinary(b).data.binary);
}

static struct Value equality (struct Ecc * const ecc, struct Value a, const struct Text *aText, struct Value b, const struct Text *bText)
{
	if (a.type == Value(binaryType) && b.type == Value(binaryType))
		return Value.truth(a.data.binary == b.data.binary);
	else if (a.type == Value(integerType) && b.type == Value(integerType))
		return Value.truth(a.data.integer == b.data.integer);
	else if (Value.isNumber(a) && Value.isNumber(b))
		return Value.truth(Value.toBinary(a).data.binary == Value.toBinary(b).data.binary);
	else if (Value.isString(a) && Value.isString(b))
	{
		uint32_t aLength = Value.stringLength(a);
		uint32_t bLength = Value.stringLength(b);
		if (aLength != bLength)
			return Value(false);
		
		return Value.truth(!memcmp(Value.stringChars(a), Value.stringChars(b), aLength));
	}
	else if (Value.isBoolean(a) && Value.isBoolean(b))
		return Value.truth(a.type == b.type);
	else if (Value.isObject(a) && Value.isObject(b))
		return Value.truth(a.data.object == b.data.object);
	else if (a.type == b.type)
		return Value(true);
	else if (a.type == Value(nullType) && b.type == Value(undefinedType))
		return Value(true);
	else if (a.type == Value(undefinedType) && b.type == Value(nullType))
		return Value(true);
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
		a = Value.toPrimitive(NULL, ecc, a, aText, Value(hintAuto));
		b = Value.toPrimitive(NULL, ecc, b, bText, Value(hintAuto));
		return equality(ecc, a, aText, b, bText);
	}
	
	return Value(false);
}

static struct Value identicality (struct Ecc * const ecc, struct Value a, const struct Text *aText, struct Value b, const struct Text *bText)
{
	if (a.type == Value(binaryType) && b.type == Value(binaryType))
		return Value.truth(a.data.binary == b.data.binary);
	else if (a.type == Value(integerType) && b.type == Value(integerType))
		return Value.truth(a.data.integer == b.data.integer);
	else if (Value.isNumber(a) && Value.isNumber(b))
		return Value.truth(Value.toBinary(a).data.binary == Value.toBinary(b).data.binary);
	else if (Value.isString(a) && Value.isString(b))
	{
		uint32_t aLength = Value.stringLength(a);
		uint32_t bLength = Value.stringLength(b);
		if (aLength != bLength)
			return Value(false);
		
		return Value.truth(!memcmp(Value.stringChars(a), Value.stringChars(b), aLength));
	}
	else if (Value.isBoolean(a) && Value.isBoolean(b))
		return Value.truth(a.type == b.type);
	else if (Value.isObject(a) && Value.isObject(b))
		return Value.truth(a.data.object == b.data.object);
	else if (a.type == b.type)
		return Value(true);
	
	return Value(false);
}

static struct Value compare (struct Ecc * const ecc, struct Value a, const struct Text *aText, struct Value b, const struct Text *bText)
{
	a = Value.toPrimitive(NULL, ecc, a, aText, Value(hintNumber));
	b = Value.toPrimitive(NULL, ecc, b, bText, Value(hintNumber));
	
	if (Value.isString(a) && Value.isString(b))
	{
		uint32_t aLength = Value.stringLength(a);
		uint32_t bLength = Value.stringLength(b);
		if (aLength == bLength)
			return Value.truth(memcmp(Value.stringChars(a), Value.stringChars(b), aLength) < 0);
		else
			return Value.truth(aLength < bLength);
	}
	else
	{
		a = Value.toBinary(a);
		b = Value.toBinary(b);
		
		if (isnan(a.data.binary) || isnan(b.data.binary))
			return Value(undefined);
		
		return Value.truth(a.data.binary < b.data.binary);
	}
}

static struct Value valueLess (struct Ecc * const ecc, struct Value a, const struct Text *aText, struct Value b, const struct Text *bText)
{
	a = compare(ecc, a, aText, b, bText);
	if (a.type == Value(undefinedType))
		return Value(false);
	else
		return a;
}

static struct Value valueMore (struct Ecc * const ecc, struct Value a, const struct Text *aText, struct Value b, const struct Text *bText)
{
	a = compare(ecc, b, bText, a, aText);
	if (a.type == Value(undefinedType))
		return Value(false);
	else
		return a;
}

static struct Value valueLessOrEqual (struct Ecc * const ecc, struct Value a, const struct Text *aText, struct Value b, const struct Text *bText)
{
	a = compare(ecc, b, bText, a, aText);
	if (a.type == Value(undefinedType) || a.type == Value(trueType))
		return Value(false);
	else
		return Value(true);
}

static struct Value valueMoreOrEqual (struct Ecc * const ecc, struct Value a, const struct Text *aText, struct Value b, const struct Text *bText)
{
	a = compare(ecc, a, aText, b, bText);
	if (a.type == Value(undefinedType) || a.type == Value(trueType))
		return Value(false);
	else
		return Value(true);
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

static inline struct Value getRefValue(struct Native(Context) * const context, struct Ecc * const ecc, struct Value *ref, struct Value this)
{
	if (!ref)
		return Value(undefined);
	
	if (ref->type == Value(functionType) && ref->data.function->flags & Function(isAccessor))
	{
		if (ref->data.function->flags & Function(isGetter))
			return callFunctionVA(context, ecc, 0, ref->data.function, this, 0);
		else if (ref->data.function->pair)
			return callFunctionVA(context, ecc, 0, ref->data.function->pair, this, 0);
	}
	
	return *ref;
}

static inline struct Value setRefValue(struct Native(Context) * const context, struct Ecc * const ecc, struct Value *ref, struct Value this, struct Value value, const struct Text *text)
{
	if (ref->type == Value(functionType) && ref->data.function->flags & Function(isAccessor))
	{
		if (ref->data.function->flags & Function(isSetter))
			callFunctionVA(context, ecc, 0, ref->data.function, this, 1, value);
		else if (ref->data.function->pair)
			callFunctionVA(context, ecc, 0, ref->data.function->pair, this, 1, value);
		else
			Ecc.jmpEnv(ecc, Value.error(Error.typeError(*text, "%.*s is read-only accessor", text->length, text->location)));
		
		return value;
	}
	
	if (ref->check == 1)
	{
		if (ref->flags & Value(readonly))
			Ecc.jmpEnv(ecc, Value.error(Error.typeError(*text, "%.*s is read-only property", text->length, text->location)));
		
		value.flags = ref->flags;
	}
	else if (this.data.object->flags & Object(sealed))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(*text, "%.*s is not extensible", text->length, text->location)));
	else
		value.flags = 0;
	
	return *ref = value;
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

static inline void populateEnvironmentWithArguments (struct Object *environment, struct Object *arguments, int parameterCount)
{
	uint32_t index = 0;
	int argumentCount = arguments->elementCount;
	
	environment->hashmap[2].data.value = Value.object(arguments);
	
	if (argumentCount <= parameterCount)
		for (; index < argumentCount; ++index)
			environment->hashmap[index + 3].data.value = arguments->element[index].data.value;
	else
	{
		for (; index < parameterCount; ++index)
			environment->hashmap[index + 3].data.value = arguments->element[index].data.value;
	}
}

static inline void populateEnvironmentWithArgumentsVA (struct Object *environment, int parameterCount, int argumentCount, va_list ap)
{
	uint32_t index = 0;
	
	struct Object *arguments = Arguments.createSized(argumentCount);
	environment->hashmap[2].data.value = Value.object(arguments);
	
	if (argumentCount <= parameterCount)
		for (; index < argumentCount; ++index)
			environment->hashmap[index + 3].data.value = arguments->element[index].data.value = va_arg(ap, struct Value);
	else
	{
		for (; index < parameterCount; ++index)
			environment->hashmap[index + 3].data.value = arguments->element[index].data.value = va_arg(ap, struct Value);
		
		for (; index < argumentCount; ++index)
			arguments->element[index].data.value = va_arg(ap, struct Value);
	}
}

static inline void populateEnvironmentWithArgumentsOps (struct Native(Context) * const context, struct Ecc * const ecc, struct Object *environment, struct Object *arguments, int parameterCount, int argumentCount)
{
	uint32_t index = 0;
	
	environment->hashmap[2].data.value = Value.object(arguments);
	
	if (argumentCount <= parameterCount)
		for (; index < argumentCount; ++index)
			environment->hashmap[index + 3].data.value = arguments->element[index].data.value = nextOp();
	else
	{
		for (; index < parameterCount; ++index)
			environment->hashmap[index + 3].data.value = arguments->element[index].data.value = nextOp();
		
		for (; index < argumentCount; ++index)
			arguments->element[index].data.value = nextOp();
	}
}

static inline void populateEnvironmentVA (struct Object *environment, int parameterCount, int argumentCount, va_list ap)
{
	uint32_t index = 0;
	if (argumentCount <= parameterCount)
		for (; index < argumentCount; ++index)
			environment->hashmap[index + 3].data.value = va_arg(ap, struct Value);
	else
	{
		for (; index < parameterCount; ++index)
			environment->hashmap[index + 3].data.value = va_arg(ap, struct Value);
		
		for (; index < argumentCount; ++index)
			va_arg(ap, struct Value);
	}
}

static inline void populateEnvironment (struct Native(Context) * const context, struct Ecc * const ecc, struct Object *environment, int parameterCount, int argumentCount)
{
	uint32_t index = 0;
	if (argumentCount <= parameterCount)
		for (; index < argumentCount; ++index)
			environment->hashmap[index + 3].data.value = nextOp();
	else
	{
		for (; index < parameterCount; ++index)
			environment->hashmap[index + 3].data.value = nextOp();
		
		for (; index < argumentCount; ++index)
			nextOp();
	}
}

static inline struct Value callOps (struct Native(Context) * const context, struct Ecc * const ecc, struct Object *environment, int construct)
{
	struct Object *callerEnvironement = ecc->environment;
	
	ecc->environment = environment;
	ecc->construct = construct;
	
	context->ops->native(context, ecc);
	
	ecc->environment = callerEnvironement;
	
	return ecc->result;
}

struct Value callFunctionArguments (struct Native(Context) * const parent, struct Ecc * const ecc, int argumentOffset, struct Function *function, struct Value this, struct Object *arguments)
{
	struct Native(Context) context = { function->oplist->ops, parent, this, argumentOffset };
	
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
		
		return callOps(&context, ecc, environment, 0);
	}
	else
	{
		struct Object environment = function->environment;
		__typeof__(*function->environment.hashmap) hashmap[function->environment.hashmapCapacity];
		
		memcpy(hashmap, function->environment.hashmap, sizeof(hashmap));
		environment.hashmap = hashmap;
		
		populateEnvironmentWithArguments(&environment, arguments, function->parameterCount);
		
		return callOps(&context, ecc, &environment, 0);
	}
}

struct Value callFunctionVA (struct Native(Context) * const parent, struct Ecc * const ecc, int argumentOffset, struct Function *function, struct Value this, int argumentCount, ... )
{
	struct Native(Context) context = { function->oplist->ops, parent, this, argumentOffset };
	
	if (function->flags & Function(needHeap))
	{
		struct Object *environment = Object.copy(&function->environment);
		va_list ap;
		
		va_start(ap, argumentCount);
		
		if (function->flags & Function(needArguments))
			populateEnvironmentWithArgumentsVA(environment, function->parameterCount, argumentCount, ap);
		else
			populateEnvironmentVA(environment, function->parameterCount, argumentCount, ap);
		
		va_end(ap);
		
		return callOps(&context, ecc, environment, 0);
	}
	else
	{
		struct Object environment = function->environment;
		__typeof__(*function->environment.hashmap) hashmap[function->environment.hashmapCapacity];
		va_list ap;
		
		memcpy(hashmap, function->environment.hashmap, sizeof(hashmap));
		environment.hashmap = hashmap;
		
		va_start(ap, argumentCount);
		populateEnvironmentVA(&environment, function->parameterCount, argumentCount, ap);
		va_end(ap);
		
		return callOps(&context, ecc, &environment, 0);
	}
}

static inline struct Value callFunction (struct Native(Context) * const parent, struct Ecc * const ecc, struct Function * const function, int32_t argumentCount, int construct)
{
	struct Native(Context) context = { function->oplist->ops, parent, ecc->refObject };
	
	if (function->flags & Function(needHeap))
	{
		struct Object *environment = Object.copy(&function->environment);
		
		if (function->flags & Function(needArguments))
			populateEnvironmentWithArgumentsOps(parent, ecc, environment, Arguments.createSized(argumentCount), function->parameterCount, argumentCount);
		else
			populateEnvironment(parent, ecc, environment, function->parameterCount, argumentCount);
		
		return callOps(&context, ecc, environment, construct);
	}
	else if (function->flags & Function(needArguments))
	{
		struct Object environment = function->environment;
		struct Object arguments = Object.identity;
		__typeof__(*environment.hashmap) hashmap[function->environment.hashmapCapacity];
		__typeof__(*arguments.element) element[argumentCount];
		
		memcpy(hashmap, function->environment.hashmap, sizeof(hashmap));
		environment.hashmap = hashmap;
		arguments.element = element;
		arguments.elementCount = argumentCount;
		
		populateEnvironmentWithArgumentsOps(parent, ecc, &environment, &arguments, function->parameterCount, argumentCount);
		
		return callOps(&context, ecc, &environment, construct);
	}
	else
	{
		struct Object environment = function->environment;
		__typeof__(*environment.hashmap) hashmap[function->environment.hashmapCapacity];
		
		memcpy(hashmap, function->environment.hashmap, sizeof(hashmap));
		environment.hashmap = hashmap;
		populateEnvironment(parent, ecc, &environment, function->parameterCount, argumentCount);
		
		return callOps(&context, ecc, &environment, construct);
	}
}

struct Value construct (struct Native(Context) * const context, struct Ecc * const ecc)
{
	const struct Text *text = opText(1);
	int32_t argumentCount = opValue().data.integer;
	struct Value object;
	struct Value value = nextOp();
	if (value.type != Value(functionType))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(*text, "%.*s is not a constructor", text->length, text->location)));
	
	object = ecc->refObject = Value.object(Object.create(Object.get(&value.data.function->object, Key(prototype)).data.object));
	value = callFunction(context, ecc, value.data.function, argumentCount, 1);
	
	if (Value.isObject(value))
		return value;
	else
		return object;
}

struct Value call (struct Native(Context) * const context, struct Ecc * const ecc)
{
	const struct Text *text = opText(1);
	int32_t argumentCount = opValue().data.integer;
	struct Value value = nextOp();
	if (value.type != Value(functionType))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(*text, "%.*s not a function", text->length, text->location)));
	
	return callFunction(context, ecc, value.data.function, argumentCount, 0);
}

struct Value eval (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value;
	struct Input *input;
	int32_t argumentCount = opValue().data.integer;
	struct Native(Context) subContext = { NULL, context, context->this };
	
	if (!argumentCount)
		return nextOp();
	
	value = nextOp();
	while (--argumentCount)
		nextOp();
	
	input = Input.createFromBytes(Value.stringChars(value), Value.stringLength(value), "(eval)");
	Ecc.evalInputWithContext(ecc, input, &subContext);
	
	return ecc->result;
}


// Expression

struct Value noop (struct Native(Context) * const context, struct Ecc * const ecc)
{
	return Value(undefined);
}

struct Value value (struct Native(Context) * const context, struct Ecc * const ecc)
{
	return opValue();
}

struct Value valueConstRef (struct Native(Context) * const context, struct Ecc * const ecc)
{
	return Value.reference((struct Value *)&opValue());
}

struct Value text (struct Native(Context) * const context, struct Ecc * const ecc)
{
	return Value.text(opText(0));
}

struct Value function (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Function *function = Function.copy(opValue().data.function);
	function->environment.prototype = ecc->environment;
	return Value.function(function);
}

struct Value object (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Object *object = Object.create(Object(prototype));
	struct Value value;
	uint32_t count;
	
	for (count = opValue().data.integer; count--;)
	{
		value = nextOp();
		
		if (value.type == Value(keyType))
			Object.add(object, value.data.key, nextOp(), 0);
		else if (value.type == Value(integerType))
			Object.addElementAtIndex(object, value.data.integer, nextOp(), 0);
	}
	return Value.object(object);
}

struct Value array (struct Native(Context) * const context, struct Ecc * const ecc)
{
	uint32_t length = opValue().data.integer;
	struct Object *object = Array.createSized(length);
	struct Value value;
	uint32_t index;
	
	for (index = 0; index < length; ++index)
	{
		value = nextOp();
		if (value.type != Value(breakerType))
		{
			object->element[index].data.value = value;
			object->element[index].data.value.flags = 0;
		}
	}
	return Value.object(object);
}

struct Value this (struct Native(Context) * const context, struct Ecc * const ecc)
{
	return context->this;
}

struct Value getLocal (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Key key = opValue().data.key;
	struct Value *ref = Object.getMember(ecc->environment, key);
	if (!ref)
		Ecc.jmpEnv(ecc, Value.error(Error.referenceError(context->ops->text, "%.*s is not defined", context->ops->text.length, context->ops->text.location)));
	
	ecc->refObject = Value(undefined);
	return *ref;
}

struct Value getLocalRef (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Key key = opValue().data.key;
	struct Value *ref = Object.getMember(ecc->environment, key);
	if (!ref)
		Ecc.jmpEnv(ecc, Value.error(Error.referenceError(context->ops->text, "%.*s is not defined", context->ops->text.length, context->ops->text.location)));
	
	return Value.reference(ref);
}

struct Value setLocal (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Key key = opValue().data.key;
	struct Value *ref = Object.getMember(ecc->environment, key);
	if (!ref)
		Ecc.jmpEnv(ecc, Value.error(Error.referenceError(context->ops->text, "%.*s is not defined", context->ops->text.length, context->ops->text.location)));
	
	return *ref = nextOp();
}

struct Value getLocalSlot (struct Native(Context) * const context, struct Ecc * const ecc)
{
	int32_t slot = opValue().data.integer;
	ecc->refObject = Value(undefined);
	return ecc->environment->hashmap[slot].data.value;
}

struct Value getLocalSlotRef (struct Native(Context) * const context, struct Ecc * const ecc)
{
	int32_t slot = opValue().data.integer;
	return Value.reference(&ecc->environment->hashmap[slot].data.value);
}

struct Value setLocalSlot (struct Native(Context) * const context, struct Ecc * const ecc)
{
	int32_t slot = opValue().data.integer;
	return ecc->environment->hashmap[slot].data.value = nextOp();
}

struct Value getParentSlot (struct Native(Context) * const context, struct Ecc * const ecc)
{
	int32_t slot = opValue().data.integer & 0xffff;
	int32_t count = opValue().data.integer >> 16;
	struct Object *object = ecc->environment;
	while (count--)
		object = object->prototype;
	
	ecc->refObject = Value(undefined);
	return object->hashmap[slot].data.value;
}

struct Value getParentSlotRef (struct Native(Context) * const context, struct Ecc * const ecc)
{
	int32_t slot = opValue().data.integer & 0xffff;
	int32_t count = opValue().data.integer >> 16;
	struct Object *object = ecc->environment;
	while (count--)
		object = object->prototype;
	
	return Value.reference(&object->hashmap[slot].data.value);
}

struct Value setParentSlot (struct Native(Context) * const context, struct Ecc * const ecc)
{
	int32_t slot = opValue().data.integer & 0xffff;
	int32_t count = opValue().data.integer >> 16;
	struct Object *object = ecc->environment;
	while (count--)
		object = object->prototype;
	
	return object->hashmap[slot].data.value = nextOp();
}

struct Value getMember (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Key key = opValue().data.key;
	struct Value object = nextOp();
	struct Value *ref;
	
	if (!Value.isObject(object))
		object = Value.toObject(context, ecc, object, Native(noIndex));
	
	ecc->refObject = object;
	ref = Object.getMember(object.data.object, key);
	
	return getRefValue(context, ecc, ref, object);
}

struct Value getMemberRef (struct Native(Context) * const context, struct Ecc * const ecc)
{
	const struct Text *text = opText(0);
	const struct Text *textObject = opText(1);
	struct Key key = opValue().data.key;
	struct Value object = nextOp();
	struct Value *ref;
	
	if (!Value.isObject(object))
		object = Value.toObject(context, ecc, object, Native(noIndex));
	
	ecc->refObject = object;
	ref = Object.getMember(object.data.object, key);
	
	if (!ref)
	{
		if (object.data.object->flags & Object(sealed))
			Ecc.jmpEnv(ecc, Value.error(Error.typeError(*text, "%.*s is not extensible", textObject->length, textObject->location)));
		
		ref = Object.add(object.data.object, key, Value(undefined), 0);
	}
	
	return Value.reference(ref);
}

struct Value setMember (struct Native(Context) * const context, struct Ecc * const ecc)
{
	const struct Text *text = opText(0);
	const struct Text *textObject = opText(1);
	struct Value key = opValue();
	struct Value object = nextOp();
	struct Value value = nextOp();
	struct Value *ref;
	
	if (!Value.isObject(object))
		object = Value.toObject(context, ecc, object, Native(noIndex));
	
	ref = Object.getOwnMember(object.data.object, key.data.key);
	
	if (ref)
		return setRefValue(context, ecc, ref, object, value, text);
	else if (object.data.object->flags & Object(sealed))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(*text, "%.*s is not extensible", textObject->length, textObject->location)));
	else
		Object.add(object.data.object, key.data.key, value, 0);
	
	return value;
}

struct Value deleteMember (struct Native(Context) * const context, struct Ecc * const ecc)
{
	const struct Text *text = opText(0);
	struct Key key = opValue().data.key;
	struct Value object = Value.toObject(context, ecc, nextOp(), Native(noIndex));
	int result = Object.delete(object.data.object, key);
	if (!result)
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(*text, "property '%.*s' is non-configurable and can't be deleted", Key.textOf(key)->length, Key.textOf(key)->location)));
	
	return Value.truth(result);
}

struct Value getProperty (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value object = nextOp();
	struct Value property = nextOp();
	struct Value *ref;
	
	if (!Value.isObject(object))
		object = Value.toObject(context, ecc, object, Native(noIndex));
	
	ecc->refObject = object;
	ref = Object.getProperty(object.data.object, property);
	
	return getRefValue(context, ecc, ref, object);
}

struct Value getPropertyRef (struct Native(Context) * const context, struct Ecc * const ecc)
{
	const struct Text *text = opText(1);
	struct Value object = nextOp();
	struct Value property = nextOp();
	struct Value *ref;
	
	if (!Value.isObject(object))
		object = Value.toObject(context, ecc, object, Native(noIndex));
	
	ecc->refObject = object;
	ref = Object.getProperty(object.data.object, property);
	
	if (!ref)
	{
		Object.setProperty(object.data.object, property, Value(undefined));
		ref = Object.getProperty(object.data.object, property);
		if (!ref)
			Ecc.jmpEnv(ecc, Value.error(Error.referenceError(*text, "%.*s is not extensible", text->length, text->location)));
	}
	
	return Value.reference(ref);
}

struct Value setProperty (struct Native(Context) * const context, struct Ecc * const ecc)
{
	const struct Text *text = opText(0);
	const struct Text *textObject = opText(1);
	struct Value object = nextOp();
	struct Value property = nextOp();
	struct Value value = nextOp();
	struct Value *ref;
	
	if (!Value.isObject(object))
		object = Value.toObject(context, ecc, object, Native(noIndex));
	
	ref = Object.getOwnProperty(object.data.object, property);
	
	if (ref)
		return setRefValue(context, ecc, ref, object, value, text);
	else if (object.data.object->flags & Object(sealed))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(*text, "%.*s is not extensible", textObject->length, textObject->location)));
	else
		Object.setProperty(object.data.object, property, value);
	
	return value;
}

struct Value deleteProperty (struct Native(Context) * const context, struct Ecc * const ecc)
{
	const struct Text *text = opText(0);
	struct Value object = Value.toObject(context, ecc, nextOp(), Native(noIndex));
	struct Value property = nextOp();
	int result = Object.deleteProperty(object.data.object, property);
	if (!result)
	{
		struct Value string = Value.toString(property);
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(*text, "property '%.*s' is non-configurable and can't be deleted", Value.stringLength(string), Value.stringChars(string))));
	}
	return Value.truth(result);
}

struct Value result (struct Native(Context) * const context, struct Ecc * const ecc)
{
	ecc->result = nextOp();
	return Value.breaker(0);
}

struct Value resultValue (struct Native(Context) * const context, struct Ecc * const ecc)
{
	ecc->result = opValue();
	return Value.breaker(0);
}

struct Value pushEnvironment (struct Native(Context) * const context, struct Ecc * const ecc)
{
	ecc->environment = Object.create(ecc->environment);
	return opValue();
}

struct Value popEnvironment (struct Native(Context) * const context, struct Ecc * const ecc)
{
	ecc->environment = ecc->environment->prototype;
	return opValue();
}

struct Value exchange (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value = opValue();
	nextOp();
	return value;
}

struct Value typeOf (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value = nextOp();
	return Value.toType(value);
}

struct Value equal (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	return equality(ecc, a, text, b, opText(0));
}

struct Value notEqual (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	return Value.truth(!Value.isTrue(equality(ecc, a, text, b, opText(0))));
}

struct Value identical (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	return identicality(ecc, a, text, b, opText(0));
}

struct Value notIdentical (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	return Value.truth(!Value.isTrue(identicality(ecc, a, text, b, opText(0))));
}

struct Value less (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	if (a.type == Value(binaryType) && b.type == Value(binaryType))
		return Value.truth(a.data.binary < b.data.binary);
	else
		return valueLess(ecc, a, text, b, opText(0));
}

struct Value lessOrEqual (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	if (a.type == Value(binaryType) && b.type == Value(binaryType))
		return Value.truth(a.data.binary <= b.data.binary);
	else
		return valueLessOrEqual(ecc, a, text, b, opText(0));
}

struct Value more (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	if (a.type == Value(binaryType) && b.type == Value(binaryType))
		return Value.truth(a.data.binary > b.data.binary);
	else
		return valueMore(ecc, a, text, b, opText(0));
}

struct Value moreOrEqual (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	if (a.type == Value(binaryType) && b.type == Value(binaryType))
		return Value.truth(a.data.binary >= b.data.binary);
	else
		return valueMoreOrEqual(ecc, a, text, b, opText(0));
}

struct Value instanceOf (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	struct Object *object;
	
	if (!Value.isObject(a))
		return Value(false);
	
	if (b.type != Value(functionType))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(*text, "%.*s not a function", text->length, text->location)));
	
	b = Object.get(b.data.object, Key(prototype));
	if (!Value.isObject(b))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(*text, "%.*s.prototype not an object", text->length, text->location)));
	
	object = a.data.object;
	while (( object = object->prototype ))
		if (object == b.data.object)
			return Value(true);
	
	return Value(false);
}

struct Value in (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value property = nextOp();
	struct Value object = nextOp();
	struct Value *ref;
	
	if (!Value.isObject(object))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(context->ops->text, "invalid 'in' operand %.*s", context->ops->text.length, context->ops->text.location)));
	
	ref = Object.getProperty(object.data.object, property);
	return Value.truth(ref != NULL);
}

struct Value multiply (struct Native(Context) * const context, struct Ecc * const ecc)
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

struct Value divide (struct Native(Context) * const context, struct Ecc * const ecc)
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

struct Value modulo (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	if (a.type == Value(binaryType) && b.type == Value(binaryType))
		return Value.binary(fmod(a.data.binary, b.data.binary));
	else
		return Value.binary(fmod(Value.toBinary(a).data.binary, Value.toBinary(b).data.binary));
}

struct Value add (struct Native(Context) * const context, struct Ecc * const ecc)
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
		return addition(ecc, a, text, b, opText(0));
}

struct Value minus (struct Native(Context) * const context, struct Ecc * const ecc)
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

struct Value leftShift (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer(Value.toInteger(a).data.integer << (uint32_t)Value.toInteger(b).data.integer);
}

struct Value rightShift (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer(Value.toInteger(a).data.integer >> (uint32_t)Value.toInteger(b).data.integer);
}

struct Value unsignedRightShift (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer((uint32_t)Value.toInteger(a).data.integer >> (uint32_t)Value.toInteger(b).data.integer);
}

struct Value bitwiseAnd (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer(Value.toInteger(a).data.integer & Value.toInteger(b).data.integer);
}

struct Value bitwiseXor (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer(Value.toInteger(a).data.integer ^ Value.toInteger(b).data.integer);
}

struct Value bitwiseOr (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer(Value.toInteger(a).data.integer | Value.toInteger(b).data.integer);
}

struct Value logicalAnd (struct Native(Context) * const context, struct Ecc * const ecc)
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

struct Value logicalOr (struct Native(Context) * const context, struct Ecc * const ecc)
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

struct Value positive (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	if (a.type == Value(binaryType))
		return a;
	else
		return Value.toBinary(a);
}

struct Value negative (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	if (a.type == Value(binaryType))
		return Value.binary(-a.data.binary);
	else
		return Value.binary(-Value.toBinary(a).data.binary);
}

struct Value invert (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	return Value.integer(~Value.toInteger(a).data.integer);
}

struct Value not (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value a = nextOp();
	return Value.truth(!Value.isTrue(a));
}

// MARK: assignement

struct Value incrementRef (struct Native(Context) * const context, struct Ecc * const ecc)
{
	const struct Text *text = opText(1);
	struct Value *ref = nextOp().data.reference;
	if (ref->type != Value(binaryType))
	{
		struct Value value = Value.toBinary(getRefValue(context, ecc, ref, ecc->refObject));
		++value.data.binary;
		return setRefValue(context, ecc, ref, ecc->refObject, value, text);
	}
	++ref->data.binary;
	return *ref;
}

struct Value decrementRef (struct Native(Context) * const context, struct Ecc * const ecc)
{
	const struct Text *text = opText(1);
	struct Value *ref = nextOp().data.reference;
	if (ref->type != Value(binaryType))
	{
		struct Value value = Value.toBinary(getRefValue(context, ecc, ref, ecc->refObject));
		--value.data.binary;
		return setRefValue(context, ecc, ref, ecc->refObject, value, text);
	}
	--ref->data.binary;
	return *ref;
}

struct Value postIncrementRef (struct Native(Context) * const context, struct Ecc * const ecc)
{
	const struct Text *text = opText(1);
	struct Value *ref = nextOp().data.reference;
	struct Value value = *ref;
	if (ref->type != Value(binaryType))
	{
		struct Value newValue = Value.toBinary(getRefValue(context, ecc, ref, ecc->refObject));
		++newValue.data.binary;
		return setRefValue(context, ecc, ref, ecc->refObject, newValue, text);
	}
	++ref->data.binary;
	return value;
}

struct Value postDecrementRef (struct Native(Context) * const context, struct Ecc * const ecc)
{
	const struct Text *text = opText(1);
	struct Value *ref = nextOp().data.reference;
	struct Value value = *ref;
	if (ref->type != Value(binaryType))
	{
		struct Value newValue = Value.toBinary(getRefValue(context, ecc, ref, ecc->refObject));
		--newValue.data.binary;
		return setRefValue(context, ecc, ref, ecc->refObject, newValue, text);
	}
	--ref->data.binary;
	return value;
}

struct Value multiplyAssignRef (struct Native(Context) * const context, struct Ecc * const ecc)
{
	const struct Text *text = opText(1);
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	if (b.type != Value(binaryType))
		b = Value.toBinary(b);
	
	if (ref->type != Value(binaryType))
	{
		struct Value value = Value.toBinary(getRefValue(context, ecc, ref, ecc->refObject));
		value.data.binary *= b.data.binary;
		return setRefValue(context, ecc, ref, ecc->refObject, value, text);
	}
	ref->data.binary *= b.data.binary;
	return *ref;
}

struct Value divideAssignRef (struct Native(Context) * const context, struct Ecc * const ecc)
{
	const struct Text *text = opText(1);
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	if (b.type != Value(binaryType))
		b = Value.toBinary(b);
	
	if (ref->type != Value(binaryType))
	{
		struct Value value = Value.toBinary(getRefValue(context, ecc, ref, ecc->refObject));
		value.data.binary /= b.data.binary;
		return setRefValue(context, ecc, ref, ecc->refObject, value, text);
	}
	ref->data.binary /= b.data.binary;
	return *ref;
}

struct Value moduloAssignRef (struct Native(Context) * const context, struct Ecc * const ecc)
{
	const struct Text *text = opText(1);
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	if (b.type != Value(binaryType))
		b = Value.toBinary(b);
	
	if (ref->type != Value(binaryType))
	{
		struct Value value = Value.toBinary(getRefValue(context, ecc, ref, ecc->refObject));
		value.data.binary = fmod(ref->data.binary, b.data.binary);
		return setRefValue(context, ecc, ref, ecc->refObject, value, text);
	}
	ref->data.binary = fmod(ref->data.binary, b.data.binary);
	return *ref;
}

struct Value addAssignRef (struct Native(Context) * const context, struct Ecc * const ecc)
{
	const struct Text *text = opText(1);
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	if (ref->type != Value(binaryType) || b.type != Value(binaryType))
	{
		struct Value value = addition(ecc, getRefValue(context, ecc, ref, ecc->refObject), text, b, opText(0));
		return setRefValue(context, ecc, ref, ecc->refObject, value, text);
	}
	ref->data.binary += b.data.binary;
	return *ref;
}

struct Value minusAssignRef (struct Native(Context) * const context, struct Ecc * const ecc)
{
	const struct Text *text = opText(1);
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	if (b.type != Value(binaryType))
		b = Value.toBinary(b);
	
	if (ref->type != Value(binaryType))
	{
		struct Value value = Value.toBinary(getRefValue(context, ecc, ref, ecc->refObject));
		value.data.binary -= b.data.binary;
		return setRefValue(context, ecc, ref, ecc->refObject, value, text);
	}
	ref->data.binary -= b.data.binary;
	return *ref;
}

struct Value leftShiftAssignRef (struct Native(Context) * const context, struct Ecc * const ecc)
{
	const struct Text *text = opText(1);
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	if (b.type != Value(integerType))
		b = Value.toInteger(b);
	
	if (ref->type != Value(integerType))
	{
		struct Value value = Value.toInteger(getRefValue(context, ecc, ref, ecc->refObject));
		value.data.integer <<= (uint32_t)b.data.integer;
		return setRefValue(context, ecc, ref, ecc->refObject, value, text);
	}
	ref->data.integer <<= (uint32_t)b.data.integer;
	return *ref;
}

struct Value rightShiftAssignRef (struct Native(Context) * const context, struct Ecc * const ecc)
{
	const struct Text *text = opText(1);
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	if (b.type != Value(integerType))
		b = Value.toInteger(b);
	
	if (ref->type != Value(integerType))
	{
		struct Value value = Value.toInteger(getRefValue(context, ecc, ref, ecc->refObject));
		value.data.integer >>= (uint32_t)b.data.integer;
		return setRefValue(context, ecc, ref, ecc->refObject, value, text);
	}
	ref->data.integer >>= (uint32_t)b.data.integer;
	return *ref;
}

struct Value unsignedRightShiftAssignRef (struct Native(Context) * const context, struct Ecc * const ecc)
{
	const struct Text *text = opText(1);
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	if (b.type != Value(integerType))
		b = Value.toInteger(b);
	
	if (ref->type != Value(integerType))
	{
		struct Value value = Value.toInteger(getRefValue(context, ecc, ref, ecc->refObject));
		value.data.integer = (uint32_t)ref->data.integer >> (uint32_t)b.data.integer;
		return setRefValue(context, ecc, ref, ecc->refObject, value, text);
	}
	ref->data.integer = (uint32_t)ref->data.integer >> (uint32_t)b.data.integer;
	return *ref;
}

struct Value bitAndAssignRef (struct Native(Context) * const context, struct Ecc * const ecc)
{
	const struct Text *text = opText(1);
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	if (b.type != Value(integerType))
		b = Value.toInteger(b);
	
	if (ref->type != Value(integerType))
	{
		struct Value value = Value.toInteger(getRefValue(context, ecc, ref, ecc->refObject));
		value.data.integer &= b.data.integer;
		return setRefValue(context, ecc, ref, ecc->refObject, value, text);
	}
	ref->data.integer &= b.data.integer;
	return *ref;
}

struct Value bitXorAssignRef (struct Native(Context) * const context, struct Ecc * const ecc)
{
	const struct Text *text = opText(1);
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	if (b.type != Value(integerType))
		b = Value.toInteger(b);
	
	if (ref->type != Value(integerType))
	{
		struct Value value = Value.toInteger(getRefValue(context, ecc, ref, ecc->refObject));
		value.data.integer ^= b.data.integer;
		return setRefValue(context, ecc, ref, ecc->refObject, value, text);
	}
	ref->data.integer ^= b.data.integer;
	return *ref;
}

struct Value bitOrAssignRef (struct Native(Context) * const context, struct Ecc * const ecc)
{
	const struct Text *text = opText(1);
	struct Value *ref = nextOp().data.reference;
	struct Value b = nextOp();
	if (b.type != Value(integerType))
		b = Value.toInteger(b);
	
	if (ref->type != Value(integerType))
	{
		struct Value value = Value.toInteger(getRefValue(context, ecc, ref, ecc->refObject));
		value.data.integer |= b.data.integer;
		return setRefValue(context, ecc, ref, ecc->refObject, value, text);
	}
	ref->data.integer |= b.data.integer;
	return *ref;
}


// MARK: Statement

struct Value try (struct Native(Context) * const context, struct Ecc * const ecc)
{
	const struct Op *end = context->ops + opValue().data.integer;
	struct Key key;
	
	const struct Op * volatile rethrowOps = NULL;
	volatile int rethrow = 0;
	volatile struct Value value = Value(undefined);
	struct Value finallyValue;
	
	if (!setjmp(*Ecc.pushEnv(ecc))) // try
		value = nextOp();
	else
	{
		value = ecc->result;
		rethrowOps = context->ops;
		
		if (!rethrow) // catch
		{
			rethrow = 1;
			context->ops = end + 1; // bypass catch jump
			key = nextOp().data.key;
			
			if (!Key.isEqual(key, Key(none)))
			{
				Object.add(ecc->environment, key, ecc->result, 0);
				ecc->result = Value(undefined);
				value = nextOp(); // execute until noop
				rethrow = 0;
			}
		}
	}
	
	Ecc.popEnv(ecc);
	
	context->ops = end; // op[end] = Op.jump, to after catch
	finallyValue = nextOp(); // jump to after catch, and execute until noop
	
	if (finallyValue.type != Value(undefinedType)) /* return breaker */
		return finallyValue;
	else if (rethrow)
	{
		context->ops = rethrowOps;
		Ecc.jmpEnv(ecc, value);
	}
	else if (value.type != Value(undefinedType)) /* return breaker */
		return value;
	else
		return nextOp();
}

dead struct Value throw (struct Native(Context) * const context, struct Ecc * const ecc)
{
	ecc->text = *opText(1);
	Ecc.jmpEnv(ecc, nextOp());
}

struct Value debug (struct Native(Context) * const context, struct Ecc * const ecc)
{
	__asm__("int3");
	return nextOp();
}

struct Value next (struct Native(Context) * const context, struct Ecc * const ecc)
{
	return nextOp();
}

struct Value nextIf (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value = opValue();
	if (!Value.isTrue(nextOp()))
		return value;
	
	return nextOp();
}

struct Value expression (struct Native(Context) * const context, struct Ecc * const ecc)
{
	ecc->result = nextOp();
	return nextOp();
}

struct Value discard (struct Native(Context) * const context, struct Ecc * const ecc)
{
	nextOp();
	return nextOp();
}

struct Value discard2 (struct Native(Context) * const context, struct Ecc * const ecc)
{
	nextOp(), nextOp();
	return nextOp();
}

struct Value discard3 (struct Native(Context) * const context, struct Ecc * const ecc)
{
	nextOp(), nextOp(), nextOp();
	return nextOp();
}

struct Value discard4 (struct Native(Context) * const context, struct Ecc * const ecc)
{
	nextOp(), nextOp(), nextOp(), nextOp();
	return nextOp();
}

struct Value discard5 (struct Native(Context) * const context, struct Ecc * const ecc)
{
	nextOp(), nextOp(), nextOp(), nextOp(), nextOp();
	return nextOp();
}

struct Value discard6 (struct Native(Context) * const context, struct Ecc * const ecc)
{
	nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp();
	return nextOp();
}

struct Value discard7 (struct Native(Context) * const context, struct Ecc * const ecc)
{
	nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp();
	return nextOp();
}

struct Value discard8 (struct Native(Context) * const context, struct Ecc * const ecc)
{
	nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp();
	return nextOp();
}

struct Value discard9 (struct Native(Context) * const context, struct Ecc * const ecc)
{
	nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp();
	return nextOp();
}

struct Value discard10 (struct Native(Context) * const context, struct Ecc * const ecc)
{
	nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp();
	return nextOp();
}

struct Value discard11 (struct Native(Context) * const context, struct Ecc * const ecc)
{
	nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp();
	return nextOp();
}

struct Value discard12 (struct Native(Context) * const context, struct Ecc * const ecc)
{
	nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp();
	return nextOp();
}

struct Value discard13 (struct Native(Context) * const context, struct Ecc * const ecc)
{
	nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp();
	return nextOp();
}

struct Value discard14 (struct Native(Context) * const context, struct Ecc * const ecc)
{
	nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp();
	return nextOp();
}

struct Value discard15 (struct Native(Context) * const context, struct Ecc * const ecc)
{
	nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp();
	return nextOp();
}

struct Value discard16 (struct Native(Context) * const context, struct Ecc * const ecc)
{
	nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp(), nextOp();
	return nextOp();
}

struct Value jump (struct Native(Context) * const context, struct Ecc * const ecc)
{
	int32_t offset = opValue().data.integer;
	context->ops += offset;
	return nextOp();
}

struct Value jumpIf (struct Native(Context) * const context, struct Ecc * const ecc)
{
	int32_t offset = opValue().data.integer;
	struct Value value = nextOp();
	if (Value.isTrue(value))
		context->ops += offset;
	
	return nextOp();
}

struct Value jumpIfNot (struct Native(Context) * const context, struct Ecc * const ecc)
{
	int32_t offset = opValue().data.integer;
	struct Value value = nextOp();
	if (!Value.isTrue(value))
		context->ops += offset;
	
	return nextOp();
}

struct Value switchOp (struct Native(Context) * const context, struct Ecc * const ecc)
{
	int32_t offset = opValue().data.integer;
	const struct Op *nextOps = context->ops + offset;
	struct Value a = nextOp(), b;
	const struct Text *text = opText(0);
	
	while (context->ops < nextOps)
	{
		b = nextOp();
		if (Value.isTrue(equality(ecc, a, text, b, opText(0))))
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
	if (( value = nextOp() ).type == Value(breakerType) && --value.data.integer) \
	{ \
		if (--value.data.integer) \
			return value; /* return breaker */\
		else \
			break; \
	} \
	else \
		context->ops = nextOps;

struct Value iterate (struct Native(Context) * const context, struct Ecc * const ecc)
{
	const struct Op *startOps = context->ops;
	const struct Op *endOps = startOps;
	const struct Op *nextOps = startOps + 1;
	struct Value value;
	
	context->ops += opValue().data.integer;
	
	while (Value.isTrue(nextOp()))
		stepIteration(value, nextOps);
	
	context->ops = endOps;
	return nextOp();
}

static struct Value iterateIntegerRef (
	struct Native(Context) * const context,
	struct Ecc * const ecc,
	int (*compareInteger) (int32_t, int32_t),
	__typeof__(integerWontOverflowPositive) wontOverflow,
	__typeof__(compare) compareValue,
	__typeof__(addition) valueStep)
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
		int32_t step = valueStep == addition? stepValue.data.integer: -stepValue.data.integer;
		if (!wontOverflow(countRef->data.integer, step - 1))
			goto deoptimize;
		
		for (; compareInteger(indexRef->data.integer, countRef->data.integer); indexRef->data.integer += step)
			stepIteration(value, nextOps);
		
		goto done;
	}
	
deoptimize:
	for (; Value.isTrue(compareValue(ecc, *indexRef, indexText, *countRef, countText))
		 ; *indexRef = valueStep(ecc, *indexRef, indexText, stepValue, stepText)
		 )
		stepIteration(value, nextOps);
	
done:
	context->ops = endOps;
	return nextOp();
}

struct Value iterateLessRef (struct Native(Context) * const context, struct Ecc * const ecc)
{
	return iterateIntegerRef(context, ecc, integerLess, integerWontOverflowPositive, valueLess, addition);
}

struct Value iterateLessOrEqualRef (struct Native(Context) * const context, struct Ecc * const ecc)
{
	return iterateIntegerRef(context, ecc, integerLessOrEqual, integerWontOverflowPositive, valueLessOrEqual, addition);
}

struct Value iterateMoreRef (struct Native(Context) * const context, struct Ecc * const ecc)
{
	return iterateIntegerRef(context, ecc, integerMore, integerWontOverflowNegative, valueMore, subtraction);
}

struct Value iterateMoreOrEqualRef (struct Native(Context) * const context, struct Ecc * const ecc)
{
	return iterateIntegerRef(context, ecc, integerMoreOrEqual, integerWontOverflowNegative, valueMoreOrEqual, subtraction);
}

struct Value iterateInRef (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value *ref = nextOp().data.reference;
	enum Value(Flags) flags = ref->flags;
	struct Value object = nextOp();
	struct Value value = nextOp();
	const struct Op *startOps = context->ops;
	const struct Op *endOps = startOps + value.data.integer;
	
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
