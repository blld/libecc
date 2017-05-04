//
//  op.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#define Implementation
#include "op.h"

#include "ecc.h"
#include "oplist.h"
#include "pool.h"

// MARK: - Private

#define nextOp() (++context->ops)->native(context)
#define opValue() (context->ops)->value
#define opText(O) &(context->ops + O)->text

#if DEBUG

	#if _MSC_VER
		#define trap() __debugbreak()
	#elif __GNUC__
		#if __i386__ || __x86_64__
			#define trap() __asm__ ("int $3")
		#elif __APPLE__
			#include <sys/syscall.h>
			#if __arm64__
				#define trap() __asm__ __volatile__ ("mov w0,%w0\n mov w1,%w1\n mov w16,%w2\n svc #128":: "r"(getpid()), "r"(SIGTRAP), "r"(SYS_kill): "x0", "x1", "x16", "cc")
			#elif __arm__
				#define trap() __asm__ __volatile__ ("mov r0, %0\n mov r1, %1\n mov r12, %2\n swi #128":: "r"(getpid()), "r"(SIGTRAP), "r"(SYS_kill): "r0", "r1", "r12", "cc")
			#endif
		#endif
	#endif

	#ifndef trap
		#if defined(SIGTRAP)
			#define trap() raise(SIGTRAP)
		#else
			#define trap() raise(SIGINT)
		#endif
	#endif

static int debug = 0;

void usage(void)
{
	Env.printColor(0, Env(bold), "\n\t-- libecc: basic gdb/lldb commands --\n");
	Env.printColor(Env(green), Env(bold), "\tstep-in\n");
	fprintf(stderr, "\t  c\n");
	Env.printColor(Env(green), Env(bold), "\tcontinue\n");
	fprintf(stderr, "\t  p debug=0\n");
	fprintf(stderr, "\t  c\n\n");
}

static void printBacktrace(struct Context * const context)
{
	int depth = context->depth, count;
	struct Context frame;
	
	while (depth)
	{
		count = depth--;
		frame = *context;
		while (count--)
			frame = *frame.parent;
		
		if (frame.text)
			Ecc.printTextInput(frame.ecc, *frame.text, 0);
	}
}

static struct Value trapOp(struct Context *context, int offset)
{
	const struct Text *text = opText(offset);
	
	context->text = text;
	if (debug && text->bytes && text->length) {
		Env.newline();
		printBacktrace(context);
		Ecc.printTextInput(context->ecc, *context->text, 1);
		
		trap();
	}
/* gdb/lldb infos: p usage() */    return nextOp();
}


#else
	#define trapOp(context, offset) nextOp()
#endif

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

static inline struct Value callOps (struct Context * const context, struct Object *environment)
{
	if (context->depth >= context->ecc->maximumCallDepth)
		Context.rangeError(context, Chars.create("maximum depth exceeded"));
	
	context->environment = environment;
	return context->ops->native(context);
}

static inline struct Value callOpsRelease (struct Context * const context, struct Object *environment)
{
	struct Value result;
	uint16_t index, count;
	
	result = callOps(context, environment);
	
	for (index = 2, count = environment->hashmapCount; index < count; ++index)
		release(environment->hashmap[index].value);
	
	return result;
}

static inline void populateEnvironmentWithArguments (struct Object *environment, struct Object *arguments, int32_t parameterCount)
{
	int32_t index = 0;
	int argumentCount = arguments->elementCount;
	
	environment->hashmap[2].value = retain(Value.object(arguments));
	
	if (argumentCount <= parameterCount)
		for (; index < argumentCount; ++index)
			environment->hashmap[index + 3].value = retain(arguments->element[index].value);
	else
	{
		for (; index < parameterCount; ++index)
			environment->hashmap[index + 3].value = retain(arguments->element[index].value);
	}
}

static inline void populateEnvironmentWithArgumentsVA (struct Object *environment, int32_t parameterCount, int32_t argumentCount, va_list ap)
{
	int32_t index = 0;
	
	struct Object *arguments = Arguments.createSized(argumentCount);
	environment->hashmap[2].value = retain(Value.object(arguments));
	
	if (argumentCount <= parameterCount)
		for (; index < argumentCount; ++index)
			environment->hashmap[index + 3].value = arguments->element[index].value = retain(va_arg(ap, struct Value));
	else
	{
		for (; index < parameterCount; ++index)
			environment->hashmap[index + 3].value = arguments->element[index].value = retain(va_arg(ap, struct Value));
		
		for (; index < argumentCount; ++index)
			arguments->element[index].value = va_arg(ap, struct Value);
	}
}

static inline void populateEnvironmentWithArgumentsOps (struct Context * const context, struct Object *environment, struct Object *arguments, int32_t parameterCount, int32_t argumentCount)
{
	int32_t index = 0;
	
	environment->hashmap[2].value = retain(Value.object(arguments));
	
	if (argumentCount <= parameterCount)
		for (; index < argumentCount; ++index)
			environment->hashmap[index + 3].value = arguments->element[index].value = retain(nextOp());
	else
	{
		for (; index < parameterCount; ++index)
			environment->hashmap[index + 3].value = arguments->element[index].value = retain(nextOp());
		
		for (; index < argumentCount; ++index)
			arguments->element[index].value = nextOp();
	}
}

static inline void populateEnvironmentVA (struct Object *environment, int32_t parameterCount, int32_t argumentCount, va_list ap)
{
	int32_t index = 0;
	if (argumentCount <= parameterCount)
		for (; index < argumentCount; ++index)
			environment->hashmap[index + 3].value = retain(va_arg(ap, struct Value));
	else
		for (; index < parameterCount; ++index)
			environment->hashmap[index + 3].value = retain(va_arg(ap, struct Value));
}

static inline void populateEnvironment (struct Context * const context, struct Object *environment, int32_t parameterCount, int32_t argumentCount)
{
	int32_t index = 0;
	if (argumentCount <= parameterCount)
		for (; index < argumentCount; ++index)
			environment->hashmap[index + 3].value = retain(nextOp());
	else
	{
		for (; index < parameterCount; ++index)
			environment->hashmap[index + 3].value = retain(nextOp());
		
		for (; index < argumentCount; ++index)
			nextOp();
	}
}

struct Value callFunctionArguments (struct Context * const context, int argumentOffset, struct Function *function, struct Value this, struct Object *arguments)
{
	struct Context subContext = {
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
		union Object(Hashmap) hashmap[function->environment.hashmapCapacity];
		
		memcpy(hashmap, function->environment.hashmap, sizeof(hashmap));
		environment.hashmap = hashmap;
		populateEnvironmentWithArguments(&environment, arguments, function->parameterCount);
		
		return callOpsRelease(&subContext, &environment);
	}
}

struct Value callFunctionVA (struct Context * const context, int argumentOffset, struct Function *function, struct Value this, int argumentCount, va_list ap)
{
	struct Context subContext = {
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
			populateEnvironmentWithArgumentsVA(environment, function->parameterCount, argumentCount, ap);
		else
			populateEnvironmentVA(environment, function->parameterCount, argumentCount, ap);
		
		return callOps(&subContext, environment);
	}
	else
	{
		struct Object environment = function->environment;
		union Object(Hashmap) hashmap[function->environment.hashmapCapacity];
		
		memcpy(hashmap, function->environment.hashmap, sizeof(hashmap));
		environment.hashmap = hashmap;
		
		populateEnvironmentVA(&environment, function->parameterCount, argumentCount, ap);
		
		return callOpsRelease(&subContext, &environment);
	}
}

static inline struct Value callFunction (struct Context * const context, struct Function * const function, struct Value this, int32_t argumentCount, int construct)
{
	struct Context subContext = {
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
		union Object(Hashmap) hashmap[function->environment.hashmapCapacity];
		union Object(Element) element[argumentCount];
		
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
		union Object(Hashmap) hashmap[function->environment.hashmapCapacity];
		
		memcpy(hashmap, function->environment.hashmap, sizeof(hashmap));
		environment.hashmap = hashmap;
		populateEnvironment(context, &environment, function->parameterCount, argumentCount);
		
		return callOpsRelease(&subContext, &environment);
	}
}

static inline struct Value callValue (struct Context * const context, struct Value value, struct Value this, int32_t argumentCount, int construct, const struct Text *text)
{
	if (value.type != Value(functionType))
		Context.typeError(context, Chars.create("'%.*s' is not a function", text->length, text->bytes));
	
	if (value.data.function->flags & Function(useBoundThis))
		return callFunction(context, value.data.function, value.data.function->boundThis, argumentCount, construct);
	else
		return callFunction(context, value.data.function, this, argumentCount, construct);
}

struct Value construct (struct Context * const context)
{
	const struct Text *text = opText(1);
	int32_t argumentCount = opValue().data.integer;
	struct Value value, *prototype, object, function = nextOp();
	
	if (function.type != Value(functionType))
		goto error;
	
	prototype = Object.member(&function.data.function->object, Key(prototype));
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
	Context.setTextIndex(context, Context(funcIndex));
	Context.typeError(context, Chars.create("'%.*s' is not a constructor", text->length, text->bytes));
}

struct Value call (struct Context * const context)
{
	const struct Text *text = opText(1);
	int32_t argumentCount = opValue().data.integer;
	struct Value value = nextOp();
	return callValue(context, value, Value(undefined), argumentCount, 0, text);
}

struct Value eval (struct Context * const context)
{
	struct Value value;
	struct Input *input;
	int32_t argumentCount = opValue().data.integer;
	struct Context subContext = {
		.parent = context,
		.this = context->this,
		.environment = context->environment,
		.ecc = context->ecc,
		.depth = context->depth + 1,
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

struct Value noop (struct Context * const context)
{
	return Value(undefined);
}

struct Value value (struct Context * const context)
{
	return opValue();
}

struct Value valueConstRef (struct Context * const context)
{
	return Value.reference((struct Value *)&opValue());
}

struct Value text (struct Context * const context)
{
	return Value.text(opText(0));
}

struct Value function (struct Context * const context)
{
	struct Value value = opValue(), result;
	struct Function *function = Function.copy(value.data.function);
	function->environment.prototype = context->environment;
	++context->environment->referenceCount;
	result = Value.function(function);
	result.flags = value.flags;
	return result;
}

struct Value object (struct Context * const context)
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

struct Value array (struct Context * const context)
{
	uint32_t length = opValue().data.integer;
	struct Object *object = Array.createSized(length);
	struct Value value;
	uint32_t index;
	
	for (index = 0; index < length; ++index)
	{
		value = nextOp();
		if (value.check == 1)
			object->element[index].value = retain(value);
	}
	return Value.object(object);
}

struct Value this (struct Context * const context)
{
	return context->this;
}

struct Value getLocalRef (struct Context * const context)
{
	struct Key key = opValue().data.key;
	struct Value *ref = Object.member(context->environment, key);
	if (!ref)
		Context.referenceError(context, Chars.create("'%.*s' is not defined", Key.textOf(key)->length, Key.textOf(key)->bytes));
	
	return Value.reference(ref);
}

struct Value getLocal (struct Context * const context)
{
	return *getLocalRef(context).data.reference;
}

struct Value setLocal (struct Context * const context)
{
	struct Value *ref = getLocalRef(context).data.reference;
	struct Value value = nextOp();
	if (ref->flags & Value(sealed))
		return value;
	
	retain(value);
	release(*ref);
	ref->data = value.data;
	ref->type = value.type;
	return value;
}

struct Value getLocalSlotRef (struct Context * const context)
{
	return Value.reference(&context->environment->hashmap[opValue().data.integer].value);
}

struct Value getLocalSlot (struct Context * const context)
{
	return context->environment->hashmap[opValue().data.integer].value;
}

struct Value setLocalSlot (struct Context * const context)
{
	int32_t slot = opValue().data.integer;
	struct Value value = nextOp();
	struct Value *ref = &context->environment->hashmap[slot].value;
	if (ref->flags & Value(sealed))
		return value;
	
	retain(value);
	release(*ref);
	ref->data = value.data;
	ref->type = value.type;
	return value;
}

struct Value getParentSlotRef (struct Context * const context)
{
	int32_t slot = opValue().data.integer & 0xffff;
	int32_t count = opValue().data.integer >> 16;
	struct Object *object = context->environment;
	while (count--)
		object = object->prototype;
	
	return Value.reference(&object->hashmap[slot].value);
}

struct Value getParentSlot (struct Context * const context)
{
	struct Value *ref = getParentSlotRef(context).data.reference;
	return *ref;
}

struct Value setParentSlot (struct Context * const context)
{
	struct Value *ref = getParentSlotRef(context).data.reference;
	struct Value value = nextOp();
	if (ref->flags & Value(sealed))
		return value;
	
	retain(value);
	release(*ref);
	ref->data = value.data;
	ref->type = value.type;
	return value;
}

static void prepareObject (struct Context * const context, struct Value *object)
{
	const struct Text *textObject = opText(1);
	*object = nextOp();
	
	if (Value.isPrimitive(*object))
	{
		Context.setText(context, textObject);
		*object = Value.toObject(context, *object);
	}
}

struct Value getMemberRef (struct Context * const context)
{
	const struct Text *text = opText(0);
	struct Key key = opValue().data.key;
	struct Value object, *ref;
	
	prepareObject(context, &object);
	
	context->refObject = object.data.object;
	ref = Object.memberOwn(object.data.object, key);
	
	if (!ref)
	{
		if (object.data.object->flags & Object(sealed))
		{
			Context.setText(context, text);
			Context.typeError(context, Chars.create("object is not extensible"));
		}
		ref = Object.addMember(object.data.object, key, Value(undefined), 0);
	}
	
	return Value.reference(ref);
}

struct Value getMember (struct Context * const context)
{
	struct Key key = opValue().data.key;
	struct Value object;
	
	prepareObject(context, &object);
	
	return Object.getMember(object.data.object, key, context);
}

struct Value setMember (struct Context * const context)
{
	const struct Text *text = opText(0);
	struct Key key = opValue().data.key;
	struct Value object, value;
	
	prepareObject(context, &object);
	value = retain(nextOp());
	
	Context.setText(context, text);
	Object.putMember(object.data.object, key, context, value);
	
	return value;
}

struct Value callMember (struct Context * const context)
{
	int32_t argumentCount = opValue().data.integer;
	const struct Text *text = &(++context->ops)->text;
	struct Key key = opValue().data.key;
	struct Value object;
	
	prepareObject(context, &object);
	
	return callValue(context, Object.getMember(object.data.object, key, context), object, argumentCount, 0, text);
}

struct Value deleteMember (struct Context * const context)
{
	const struct Text *text = opText(0);
	struct Key key = opValue().data.key;
	struct Value object;
	int result;
	
	prepareObject(context, &object);
	
	result = Object.deleteMember(object.data.object, key);
	if (!result)
	{
		Context.setText(context, text);
		Context.typeError(context, Chars.create("property '%.*s' is non-configurable and can't be deleted", Key.textOf(key)->length, Key.textOf(key)->bytes));
	}
	
	return Value.truth(result);
}

static void prepareObjectProperty (struct Context * const context, struct Value *object, struct Value *property)
{
	const struct Text *textProperty;
	
	prepareObject(context, object);
	
	textProperty = opText(1);
	*property = nextOp();
	
	if (Value.isObject(*property))
	{
		Context.setText(context, textProperty);
		*property = Value.toPrimitive(context, *property, Value(hintAuto));
	}
}

struct Value getPropertyRef (struct Context * const context)
{
	const struct Text *text = opText(1);
	struct Value object, property;
	struct Value *ref;
	
	prepareObjectProperty(context, &object, &property);
	
	context->refObject = object.data.object;
	ref = Object.propertyOwn(object.data.object, property);
	
	if (!ref)
	{
		if (object.data.object->flags & Object(sealed))
		{
			Context.setText(context, text);
			Context.typeError(context, Chars.create("object is not extensible"));
		}
		ref = Object.addProperty(object.data.object, property, Value(undefined), 0);
	}
	
	return Value.reference(ref);
}

struct Value getProperty (struct Context * const context)
{
	struct Value object, property;
	
	prepareObjectProperty(context, &object, &property);
	
	return Object.getProperty(object.data.object, property, context);
}

struct Value setProperty (struct Context * const context)
{
	const struct Text *text = opText(0);
	struct Value object, property, value;
	
	prepareObjectProperty(context, &object, &property);
	
	value = retain(nextOp());
	
	Context.setText(context, text);
	Object.putProperty(object.data.object, property, context, value);
	
	return value;
}

struct Value callProperty (struct Context * const context)
{
	int32_t argumentCount = opValue().data.integer;
	const struct Text *text = &(++context->ops)->text;
	struct Value object, property;
	
	prepareObjectProperty(context, &object, &property);
	
	return callValue(context, Object.getProperty(object.data.object, property, context), object, argumentCount, 0, text);
}

struct Value deleteProperty (struct Context * const context)
{
	const struct Text *text = opText(0);
	struct Value object, property;
	int result;
	
	prepareObjectProperty(context, &object, &property);
	
	result = Object.deleteProperty(object.data.object, property);
	if (!result)
	{
		struct Value string = Value.toString(NULL, property);
		Context.setText(context, text);
		Context.typeError(context, Chars.create("property '%.*s' can't be deleted", Value.stringLength(string), Value.stringBytes(string)));
	}
	return Value.truth(result);
}

struct Value pushEnvironment (struct Context * const context)
{
	context->environment = Object.create(context->environment);
	return opValue();
}

struct Value popEnvironment (struct Context * const context)
{
	struct Object *environment = context->environment;
	context->environment = context->environment->prototype;
	environment->prototype = NULL;
	return opValue();
}

struct Value exchange (struct Context * const context)
{
	struct Value value = opValue();
	nextOp();
	return value;
}

struct Value typeOf (struct Context * const context)
{
	struct Value value = nextOp();
	return Value.toType(value);
}

struct Value equal (struct Context * const context)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	
	Context.setText(context, text);
	return Value.equals(context, a, b);
}

struct Value notEqual (struct Context * const context)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	
	Context.setText(context, text);
	return Value.truth(!Value.isTrue(Value.equals(context, a, b)));
}

struct Value identical (struct Context * const context)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	
	Context.setText(context, text);
	return Value.same(context, a, b);
}

struct Value notIdentical (struct Context * const context)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	
	Context.setText(context, text);
	return Value.truth(!Value.isTrue(Value.same(context, a, b)));
}

struct Value less (struct Context * const context)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	
	Context.setText(context, text);
	if (a.type == Value(binaryType) && b.type == Value(binaryType))
		return Value.truth(a.data.binary < b.data.binary);
	else
		return Value.less(context, a, b);
}

struct Value lessOrEqual (struct Context * const context)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	
	Context.setText(context, text);
	if (a.type == Value(binaryType) && b.type == Value(binaryType))
		return Value.truth(a.data.binary <= b.data.binary);
	else
		return Value.lessOrEqual(context, a, b);
}

struct Value more (struct Context * const context)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	
	Context.setText(context, text);
	if (a.type == Value(binaryType) && b.type == Value(binaryType))
		return Value.truth(a.data.binary > b.data.binary);
	else
		return Value.more(context, a, b);
}

struct Value moreOrEqual (struct Context * const context)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	
	Context.setText(context, text);
	if (a.type == Value(binaryType) && b.type == Value(binaryType))
		return Value.truth(a.data.binary >= b.data.binary);
	else
		return Value.moreOrEqual(context, a, b);
}

struct Value instanceOf (struct Context * const context)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	struct Object *object;
	
	if (!Value.isObject(a))
		return Value(false);
	
	if (b.type != Value(functionType))
		Context.typeError(context, Chars.create("'%.*s' not a function", text->length, text->bytes));
	
	b = Object.getMember(b.data.object, Key(prototype), context);
	if (!Value.isObject(b))
		Context.typeError(context, Chars.create("'%.*s'.prototype not an object", text->length, text->bytes));
	
	object = a.data.object;
	while (( object = object->prototype ))
		if (object == b.data.object)
			return Value(true);
	
	return Value(false);
}

struct Value in (struct Context * const context)
{
	struct Value property = nextOp();
	struct Value object = nextOp();
	struct Value *ref;
	
	if (!Value.isObject(object))
		Context.typeError(context, Chars.create("'%.*s' not an object", context->ops->text.length, context->ops->text.bytes));
	
	ref = Object.property(object.data.object, property);
	
	return Value.truth(ref != NULL);
}

struct Value add (struct Context * const context)
{
	struct Value a = nextOp();
	const struct Text *text = opText(0);
	struct Value b = nextOp();
	
	Context.setText(context, text);
	if (a.type == Value(binaryType) && b.type == Value(binaryType))
	{
		a.data.binary += b.data.binary;
		return a;
	}
	else
		return Value.add(context, a, b);
}

struct Value minus (struct Context * const context)
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

struct Value multiply (struct Context * const context)
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

struct Value divide (struct Context * const context)
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

struct Value modulo (struct Context * const context)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	if (a.type == Value(binaryType) && b.type == Value(binaryType))
		return Value.binary(fmod(a.data.binary, b.data.binary));
	else
		return Value.binary(fmod(Value.toBinary(a).data.binary, Value.toBinary(b).data.binary));
}

struct Value leftShift (struct Context * const context)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer(Value.toInteger(a).data.integer << (uint32_t)Value.toInteger(b).data.integer);
}

struct Value rightShift (struct Context * const context)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer(Value.toInteger(a).data.integer >> (uint32_t)Value.toInteger(b).data.integer);
}

struct Value unsignedRightShift (struct Context * const context)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer((uint32_t)Value.toInteger(a).data.integer >> (uint32_t)Value.toInteger(b).data.integer);
}

struct Value bitwiseAnd (struct Context * const context)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer(Value.toInteger(a).data.integer & Value.toInteger(b).data.integer);
}

struct Value bitwiseXor (struct Context * const context)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer(Value.toInteger(a).data.integer ^ Value.toInteger(b).data.integer);
}

struct Value bitwiseOr (struct Context * const context)
{
	struct Value a = nextOp();
	struct Value b = nextOp();
	return Value.integer(Value.toInteger(a).data.integer | Value.toInteger(b).data.integer);
}

struct Value logicalAnd (struct Context * const context)
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

struct Value logicalOr (struct Context * const context)
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

struct Value positive (struct Context * const context)
{
	struct Value a = nextOp();
	if (a.type == Value(binaryType))
		return a;
	else
		return Value.toBinary(a);
}

struct Value negative (struct Context * const context)
{
	struct Value a = nextOp();
	if (a.type == Value(binaryType))
		return Value.binary(-a.data.binary);
	else
		return Value.binary(-Value.toBinary(a).data.binary);
}

struct Value invert (struct Context * const context)
{
	struct Value a = nextOp();
	return Value.integer(~Value.toInteger(a).data.integer);
}

struct Value not (struct Context * const context)
{
	struct Value a = nextOp();
	return Value.truth(!Value.isTrue(a));
}

// MARK: assignement

#define unaryBinaryOpRef(OP) \
	const struct Text *text = opText(0); \
	struct Value *ref = nextOp().data.reference; \
	struct Value a; \
	double result; \
	 \
	a = *ref; \
	if (a.flags & (Value(readonly) | Value(accessor))) \
	{ \
		Context.setText(context, text); \
		a = Value.toBinary(release(Object.getValue(context->refObject, ref, context))); \
		result = OP; \
		Object.putValue(context->refObject, ref, context, a); \
		return Value.binary(result); \
	} \
	else if (a.type != Value(binaryType)) \
		a = Value.toBinary(release(a)); \
	 \
	result = OP; \
	*ref = a; \
	return Value.binary(result); \

struct Value incrementRef (struct Context * const context)
{
	unaryBinaryOpRef(++a.data.binary)
}

struct Value decrementRef (struct Context * const context)
{
	unaryBinaryOpRef(--a.data.binary)
}

struct Value postIncrementRef (struct Context * const context)
{
	unaryBinaryOpRef(a.data.binary++)
}

struct Value postDecrementRef (struct Context * const context)
{
	unaryBinaryOpRef(a.data.binary--)
}

#define assignOpRef(OP, TYPE, CONV) \
	const struct Text *text = opText(0); \
	struct Value *ref = nextOp().data.reference; \
	struct Value a, b = nextOp(); \
	 \
	if (b.type != TYPE) \
		b = CONV(b); \
	 \
	a = *ref; \
	if (a.flags & (Value(readonly) | Value(accessor))) \
	{ \
		Context.setText(context, text); \
		a = CONV(release(Object.getValue(context->refObject, ref, context))); \
		OP; \
		return *Object.putValue(context->refObject, ref, context, a); \
	} \
	else if (a.type == TYPE) \
	{ \
		OP; \
		return *ref = a; \
	} \
	else \
	{ \
		a = CONV(release(a)); \
		OP; \
		return *ref = a; \
	} \

#define assignBinaryOpRef(OP) assignOpRef(OP, Value(binaryType), Value.toBinary)
#define assignIntegerOpRef(OP) assignOpRef(OP, Value(integerType), Value.toInteger)

struct Value addAssignRef (struct Context * const context)
{
	const struct Text *text = opText(0);
	struct Value *ref = nextOp().data.reference;
	struct Value a, b = nextOp();
	
	a = *ref;
	if (a.type == Value(binaryType) && b.type == Value(binaryType))
	{
		a.data.binary += b.data.binary;
		return *ref = a;
	}
	else
	{
		Context.setText(context, text);
		if (a.flags & (Value(readonly) | Value(accessor)))
		{
			a = release(Object.getValue(context->refObject, ref, context));
			a = retain(Value.add(context, a, b));
			return *Object.putValue(context->refObject, ref, context, a);
		}
		else
		{
			a = retain(Value.add(context, release(a), b));
			return *ref = a;
		}
	}
}

struct Value minusAssignRef (struct Context * const context)
{
	assignBinaryOpRef(a.data.binary -= b.data.binary);
}

struct Value multiplyAssignRef (struct Context * const context)
{
	assignBinaryOpRef(a.data.binary *= b.data.binary);
}

struct Value divideAssignRef (struct Context * const context)
{
	assignBinaryOpRef(a.data.binary /= b.data.binary);
}

struct Value moduloAssignRef (struct Context * const context)
{
	assignBinaryOpRef(a.data.binary = fmod(a.data.binary, b.data.binary));
}

struct Value leftShiftAssignRef (struct Context * const context)
{
	assignIntegerOpRef(a.data.integer <<= (uint32_t)b.data.integer);
}

struct Value rightShiftAssignRef (struct Context * const context)
{
	assignIntegerOpRef(a.data.integer >>= (uint32_t)b.data.integer);
}

struct Value unsignedRightShiftAssignRef (struct Context * const context)
{
	assignIntegerOpRef(a.data.integer = (uint32_t)a.data.integer >> (uint32_t)b.data.integer);
}

struct Value bitAndAssignRef (struct Context * const context)
{
	assignIntegerOpRef(a.data.integer &= (uint32_t)b.data.integer);
}

struct Value bitXorAssignRef (struct Context * const context)
{
	assignIntegerOpRef(a.data.integer ^= (uint32_t)b.data.integer);
}

struct Value bitOrAssignRef (struct Context * const context)
{
	assignIntegerOpRef(a.data.integer |= (uint32_t)b.data.integer);
}


// MARK: Statement

struct Value debugger (struct Context * const context)
{
	#if DEBUG
	debug = 1;
	#endif
	return trapOp(context, 0);
}

useframe
struct Value try (struct Context * const context)
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
				if (value.type == Value(functionType))
				{
					value.data.function->flags |= Function(useBoundThis);
					value.data.function->boundThis = Value.object(context->environment);
				}
				Object.addMember(context->environment, key, value, 0);
				value = nextOp(); // execute until noop
				rethrow = 0;
				if (context->breaker)
					popEnvironment(context);
			}
		}
		else // rethrow
			popEnvironment(context);
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
struct Value throw (struct Context * const context)
{
	context->ecc->text = *opText(1);
	Ecc.jmpEnv(context->ecc, retain(trapOp(context, 0)));
}

struct Value next (struct Context * const context)
{
	return nextOp();
}

struct Value nextIf (struct Context * const context)
{
	struct Value value = opValue();
	
	if (!Value.isTrue(trapOp(context, 1)))
		return value;
	
	return nextOp();
}

struct Value autoreleaseExpression (struct Context * const context)
{
	uint32_t counts[3];
	
	Pool.getCounts(counts);
	release(context->ecc->result);
	context->ecc->result = retain(trapOp(context, 1));
	Pool.collectUnreferencedFromIndices(counts);
	return nextOp();
}

struct Value autoreleaseDiscard (struct Context * const context)
{
	uint32_t counts[3];
	
	Pool.getCounts(counts);
	trapOp(context, 1);
	Pool.collectUnreferencedFromIndices(counts);
	return nextOp();
}

struct Value expression (struct Context * const context)
{
	release(context->ecc->result);
	context->ecc->result = retain(trapOp(context, 1));
	return nextOp();
}

struct Value discard (struct Context * const context)
{
	trapOp(context, 1);
	return nextOp();
}

struct Value discardN (struct Context * const context)
{
	switch (opValue().data.integer)
	{
		default:
			Ecc.fatal("Invalid discardN : %d", opValue().data.integer);
		
		case 16:
			trapOp(context, 1);
		case 15:
			trapOp(context, 1);
		case 14:
			trapOp(context, 1);
		case 13:
			trapOp(context, 1);
		case 12:
			trapOp(context, 1);
		case 11:
			trapOp(context, 1);
		case 10:
			trapOp(context, 1);
		case 9:
			trapOp(context, 1);
		case 8:
			trapOp(context, 1);
		case 7:
			trapOp(context, 1);
		case 6:
			trapOp(context, 1);
		case 5:
			trapOp(context, 1);
		case 4:
			trapOp(context, 1);
		case 3:
			trapOp(context, 1);
		case 2:
			trapOp(context, 1);
		case 1:
			trapOp(context, 1);
	}
	return nextOp();
}

struct Value jump (struct Context * const context)
{
	int32_t offset = opValue().data.integer;
	context->ops += offset;
	return nextOp();
}

struct Value jumpIf (struct Context * const context)
{
	int32_t offset = opValue().data.integer;
	struct Value value;
	
	value = trapOp(context, 1);
	if (Value.isTrue(value))
		context->ops += offset;
	
	return nextOp();
}

struct Value jumpIfNot (struct Context * const context)
{
	int32_t offset = opValue().data.integer;
	struct Value value;
	
	value = trapOp(context, 1);
	if (!Value.isTrue(value))
		context->ops += offset;
	
	return nextOp();
}

struct Value result (struct Context * const context)
{
	struct Value result = trapOp(context, 0);
	context->breaker = -1;
	return result;
}

struct Value resultVoid (struct Context * const context)
{
	struct Value result = Value(undefined);
	context->breaker = -1;
	return result;
}

struct Value switchOp (struct Context * const context)
{
	int32_t offset = opValue().data.integer;
	const struct Op *nextOps = context->ops + offset;
	struct Value a, b;
	const struct Text *text = opText(1);
	
	a = trapOp(context, 1);
	
	while (context->ops < nextOps)
	{
		b = nextOp();
		
		Context.setText(context, text);
		if (Value.isTrue(Value.equals(context, a, b)))
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
	}

struct Value breaker (struct Context * const context)
{
	context->breaker = opValue().data.integer;
	return Value(undefined);
}

struct Value iterate (struct Context * const context)
{
	const struct Op *startOps = context->ops;
	const struct Op *endOps = startOps;
	const struct Op *nextOps = startOps + 1;
	struct Value value;
	
	context->ops += opValue().data.integer;
	
	for (; Value.isTrue(nextOp()); nextOp())
		stepIteration(value, nextOps);
	
	context->ops = endOps;
	return nextOp();
}

static struct Value iterateIntegerRef (
	struct Context * const context,
	int (*compareInteger) (int32_t, int32_t),
	int (*wontOverflow) (int32_t, int32_t),
	struct Value io_libecc_interface_Unwrap((*compareValue)) (struct Context * const, struct Value, struct Value),
	struct Value io_libecc_interface_Unwrap((*valueStep)) (struct Context * const, struct Value, struct Value))
{
	const struct Op *endOps = context->ops + opValue().data.integer;
	struct Value stepValue = nextOp();
	struct Value *indexRef = nextOp().data.reference;
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
		int32_t step = valueStep == Value.add? stepValue.data.integer: -stepValue.data.integer;
		if (!wontOverflow(countRef->data.integer, step - 1))
			goto deoptimize;
		
		for (; compareInteger(indexRef->data.integer, countRef->data.integer); indexRef->data.integer += step)
			stepIteration(value, nextOps);
		
		goto done;
	}
	
deoptimize:
	for (; Value.isTrue(compareValue(context, *indexRef, *countRef))
		 ; *indexRef = valueStep(context, *indexRef, stepValue)
		 )
		stepIteration(value, nextOps);
	
done:
	context->ops = endOps;
	return nextOp();
}

struct Value iterateLessRef (struct Context * const context)
{
	return iterateIntegerRef(context, integerLess, integerWontOverflowPositive, Value.less, Value.add);
}

struct Value iterateLessOrEqualRef (struct Context * const context)
{
	return iterateIntegerRef(context, integerLessOrEqual, integerWontOverflowPositive, Value.lessOrEqual, Value.add);
}

struct Value iterateMoreRef (struct Context * const context)
{
	return iterateIntegerRef(context, integerMore, integerWontOverflowNegative, Value.more, Value.subtract);
}

struct Value iterateMoreOrEqualRef (struct Context * const context)
{
	return iterateIntegerRef(context, integerMoreOrEqual, integerWontOverflowNegative, Value.moreOrEqual, Value.subtract);
}

struct Value iterateInRef (struct Context * const context)
{
	struct Value *ref = nextOp().data.reference;
	struct Value object = nextOp();
	struct Value value = nextOp(), key;
	const struct Op *startOps = context->ops;
	const struct Op *endOps = startOps + value.data.integer;
	
	if (Value.isObject(object))
	{
		uint32_t index;
		
		for (index = 0; index < object.data.object->elementCount; ++index)
		{
			if (!(object.data.object->element[index].value.check == 1))
				continue;
			
			key = Value.chars(Chars.create("%d", index));
			ref->data = key.data;
			ref->type = key.type;
			
			stepIteration(value, startOps);
		}
		
		for (index = 2; index < object.data.object->hashmapCount; ++index)
		{
			if (!(object.data.object->hashmap[index].value.check == 1))
				continue;
			
			key = Value.key(object.data.object->hashmap[index].value.key);
			ref->data = key.data;
			ref->type = key.type;
			
			stepIteration(value, startOps);
		}
	}
	
	context->ops = endOps;
	return nextOp();
}
